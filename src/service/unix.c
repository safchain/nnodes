/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <types.h>

#include <mm.h>
#include <misc.h>
#include <rdb.h>
#include <net.h>
#include <ast.h>
#include <log.h>
#include <vthttp.h>
#include <nnodes.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_STRING 	"nnodes"
#define VERSION 		"0.6"
#endif

#define NTHREADS	8

static char *config = NULL;
static RDB *rdb = NULL;

LOG *logg = NULL;

static uint32_t _get_rdb_nthreads(RDB *rdb) {
	if (! rdb->options->nthreads)
		return NTHREADS;
	else
		return rdb->options->nthreads;
}

static int _get_rdb_uid(RDB *rdb) {
	return rdb->options->uid;
}

static int _get_rdb_gid(RDB *rdb) {
	return rdb->options->gid;
}

static int _set_guid(void) {
	int uid = 0, gid = 0;
	int rtv = 0;

	if ((gid = _get_rdb_gid(rdb)) != -1) {
		if (setgid((uid_t) gid) == -1)
			rtv = -1;
	}
	if ((uid = _get_rdb_uid(rdb)) != -1) {
		if (setuid((uid_t) uid) == -1)
			rtv = -1;
	}

	if (rtv == -1)
		LOG_ERR("Unable to set uid %d or/and gid %d", uid, gid);

	return rtv;
}

static void _sig_reload_rdb(int sig) {
	RDB *newrdb = NULL;

	if ((newrdb = rdb_read(config)) == NULL)
		return;

	if (_get_rdb_gid(rdb) != _get_rdb_gid(newrdb))
		LOG_WARN("unable to change gid during a reload do a full restart");
	if (_get_rdb_uid(rdb) != _get_rdb_uid(newrdb))
		LOG_WARN("unable to change uid during a reload do a full restart\n");

	rdb_free(rdb);
	rdb = newrdb;

	LOG_INFO("configuration reloaded\n");
}

static int _set_sig_hup(void) {
	signal(SIGHUP, _sig_reload_rdb);

	return 0;
}

static void _prepare_monitors(HCODE *monitors, int efd) {
	HCODE_ITERATOR iterator;
	HNODE *hnode_ptr;
	MON *mon;

	hl_hash_init_iterator(monitors, &iterator);
	while ((hnode_ptr = hl_hash_iterate(&iterator))) {
		mon = (MON *) hnode_ptr->value;
		mon_prepare(mon, efd);
	}
}

static void _execute_monitors(HCODE *monitors) {
	HCODE_ITERATOR iterator;
	HNODE *hnode_ptr;
	MON *mon;

	hl_hash_init_iterator(monitors, &iterator);
	while ((hnode_ptr = hl_hash_iterate(&iterator))) {
		mon = (MON *) hnode_ptr->value;
		mon_execute(mon);
	}
}

static int _mon_loop() {
	struct epoll_event events[20];
	int efd, nfds, i;

	efd = epoll_create(sizeof(events));
	if (!efd) {
		LOG_ERR("epoll creation error %d", getpid());
		return -1;
	}

	/* prepare monitors, set efd */
	_prepare_monitors(rdb->monitors, efd);

	while (1) {
		nfds = epoll_wait(efd, events, 20, 1000);
		if (nfds > 0)
			for (i = 0; i != nfds; i++)
				mon_fetch_result((MON *) events[i].data.ptr);

		/* execute monitors */
		_execute_monitors(rdb->monitors);
	}

	return -1;
}

static int  _vhttp_send_header(vthttp_srv_t *srv, vthttp_req_t *req) {
	vthttp_send_str(req, "HTTP/1.1 200 OK\r\n");
	vthttp_send_str(req, "User-Agent: ");
	vthttp_send_str(req, PACKAGE_STRING);
	vthttp_send_str(req, "\r\n");

	return 0;
}

static int _vhttp_send_error(vthttp_srv_t *srv, vthttp_req_t *req) {
	vthttp_send_str(req, "HTTP/1.1 500 Internal Server Error\r\n");
	vthttp_send_str(req, "User-Agent: ");
	vthttp_send_str(req, PACKAGE_STRING);
	vthttp_send_str(req, "\r\n");

	return -1;
}

static int _vhttp_send_notfound(vthttp_srv_t *srv, vthttp_req_t *req) {
	vthttp_send_str(req, "HTTP/1.1 404 Not Found\r\n");
	vthttp_send_str(req, "User-Agent: ");
	vthttp_send_str(req, PACKAGE_STRING);
	vthttp_send_str(req, "\r\n");

	return -1;
}

static int _vthttp_status_callback(vthttp_srv_t *srv, vthttp_req_t *req) {
	HCODE_ITERATOR hIterator;
	LIST_ITERATOR lIterator;
	struct dns_rr_s **rr;
	char *str, *msg;
	HNODE *node;
	LIST **list;
	int len;

	/* send headers */
	_vhttp_send_header(srv, req);

	vthttp_send_str(req, "Content-type: text/xml\r\n");
	vthttp_send_str(req, "\r\n");
	vthttp_send_str(req, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	if (rdb->options->websrv_xslt)
		vthttp_send_str(req, "<?xml-stylesheet href=\"/xslt\" type=\"text/xsl\"?>\n");

	vthttp_send_str(req, "<nnodes version=\"");
	vthttp_send_str(req, VERSION);
	vthttp_send_str(req, "\">\n");

	vthttp_send_str(req, "<records>\n");

	hl_hash_init_iterator(rdb->zones, &hIterator);
	while ((node = hl_hash_iterate(&hIterator)) != NULL) {
		list = node->value;

		hl_list_init_iterator(*list, &lIterator);
		while ((rr = hl_list_iterate(&lIterator)) != NULL) {
			if ((str = dns_get_data_str(*rr)) == NULL)
				continue;

			pthread_rwlock_rdlock(&((*rr)->extra.rwlock_unavailable));

			len = snprintf(NULL, 0, "\t<record name=\"%s\" class=\"%s\" type=\"%s\" unavailable=\"%d\" errors=\"%d\"><![CDATA[%s]]></record>\n",
					(*rr)->name,
					dns_int_to_class((*rr)->class),
					dns_int_to_type((*rr)->type),
					(*rr)->extra.unavailable,
					(*rr)->extra.errors,
					str);
			len++;
			if ((msg = malloc(len)) == NULL) {
				pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));

				free(str);
				continue;
			}

			snprintf(msg, len, "\t<record name=\"%s\" class=\"%s\" type=\"%s\" unavailable=\"%d\" errors=\"%d\"><![CDATA[%s]]></record>\n",
					(*rr)->name,
					dns_int_to_class((*rr)->class),
					dns_int_to_type((*rr)->type),
					(*rr)->extra.unavailable,
					(*rr)->extra.errors,
					str);

			pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));

			vthttp_send_str(req, msg);

			free(str);
			free(msg);
		}
	}

	vthttp_send_str(req, "</records>\n</nnodes>\n");

	return 0;
}

static int _vthttp_xslt_callback (vthttp_srv_t *srv, vthttp_req_t *req) {
	FILE *fp;
	char buff[BUFSIZ];
	int r;

	if (! rdb->options->websrv_xslt) {
		LOG_ERR("no xslt file path in config file\n");
		return _vhttp_send_notfound(srv, req);
	}
	if ((fp = fopen (rdb->options->websrv_xslt, "r")) == NULL) {
		LOG_ERR("unable to open xslt file: %s\n", rdb->options->websrv_xslt);
		return _vhttp_send_notfound(srv, req);
	}

	/* send headers */
	_vhttp_send_header(srv, req);

	vthttp_send_str(req, "Content-type: text/xsl\r\n");
	vthttp_send_str(req, "\r\n");

	while ((r = fread(buff, 1, BUFSIZ, fp)) > 0)
		vthttp_send(req, buff, r);

	return 0;
}

static int _vthttp_req_callback (vthttp_srv_t *srv, vthttp_req_t *req) {
	if (strcmp (req->query.uri, "/status") == 0)
		return _vthttp_status_callback(srv, req);
	else if (strcmp (req->query.uri, "/stats") == 0)
		return _vthttp_status_callback(srv, req);
	else if (strcmp (req->query.uri, "/xslt") == 0)
		return _vthttp_xslt_callback(srv, req);
	return -1;
}

static void *_vthttp_loop(void *attr) {
	vthttp_srv_t *srv;

	if ((srv = vthttp_init(rdb->options->websrv_port)) == NULL) {
		LOG_ERR("unable to start webserver port: %d\n", rdb->options->websrv_port);

		return NULL;
	}

	srv->on_request = _vthttp_req_callback;
	vthttp_run(srv);

	return NULL;
}

static void *_lookup_loop(void *attr) {
	char in[BUFSIZ], out[BUFSIZ];
	int read = BUFSIZ, to_send;
	struct sockaddr client;
	unsigned int sockaddr_len;
	LOOKUP *lookup;
	SOCKUTP *sockutp = (SOCKUTP *) attr;

	/* initialize lookup environnement */
	if((lookup = nnodes_lookup_init(rdb)) == NULL)
		return NULL;

	while(1) {
		sockaddr_len = sizeof(struct sockaddr);

		read = recvfrom(sockutp->udp_sock, in, BUFSIZ, 0, &client, &sockaddr_len);
		if (read) {
			to_send = nnodes_lookup(lookup, &client, in, out, BUFSIZ);
			if (to_send > 0)
				sendto(sockutp->udp_sock, out, to_send, 0, &client, sockaddr_len);
		}
		memset(in, 0, read);
	}

	return NULL;
}

static int _usage(char *bin) {
	fprintf(stderr, "Usage: %s --cf config [--df] [--ct]\n", bin);

	return -1;
}

static int _write_pid(void) {
	char *path = NULL;
	FILE *fp = NULL;
	int rtv = 0;

	if (rdb->options == NULL || (path = rdb->options->pid) == NULL) {
		LOG_ERR("unable to find pid entry in configuration file");
		return -1;
	}

	if ((fp = fopen(path, "wb")) == NULL) {
		LOG_ERR("unable to open pid file %s, %s", path, strerror(errno));
		rtv = -1;
	}

	if (fp && fprintf(fp, "%d", getpid()) <= 0) {
		LOG_ERR("Unable to write pid file %s, %s", path, strerror(errno));
		rtv = -1;
	};

	if (fp)
		fclose(fp);

	return rtv;
}

int main(int argc, char **argv) {
	pthread_t *lookup_threads;
	pthread_t *vthttp_thread;
	uint32_t nthreads;
	int32_t idx, df = 0, ct = 0, i, j;
	char c;
	pid_t pid;
	void *rtv;
	LIST_ITERATOR iterator;
	struct rdb_addr_s **addr;
	int interfaces = 0;
	SOCKUTP *sockutp;
	struct option long_options[] = {
			{ "cf", 1, 0, 0 }, { "df", 0, 0, 0 }, { "ct", 0, 0, 0 }, { 0, 0, 0,
					0 } };

	while (1) {
		c = getopt_long(argc, argv, "", long_options, &idx);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (strcmp(long_options[idx].name, "cf") == 0)
				config = strdup(optarg);
			else if (strcmp(long_options[idx].name, "df") == 0)
				df = 1;
			else if (strcmp(long_options[idx].name, "ct") == 0)
				ct = 1;
			break;
		default:
			return _usage(argv[0]);
		}
	}

	if (!config)
		return _usage(argv[0]);

	if ((rdb = rdb_read(config)) == NULL)
		return -1;

	if (ct) /* config test */
		return 0;

	_set_sig_hup();

	if (!df)
		if (fork())
			return 0;

	if (_write_pid() == -1)
		return -1;

	if (! rdb->options->addrs) {
		LOG_ERR("no listening interfaces available");
		return -1;
	}

	interfaces = hl_list_count(rdb->options->addrs);
	if (! interfaces) {
		LOG_ERR("no listening interfaces available");
		return -1;
	}

	nthreads = _get_rdb_nthreads(rdb);
	if ((lookup_threads = (pthread_t *) malloc(sizeof(pthread_t) * nthreads * interfaces)) == NULL)
		return -1;

	i = 0;
	hl_list_init_iterator(rdb->options->addrs, &iterator);
	while ((addr = hl_list_iterate(&iterator)) != NULL) {
		LOG_INFO("add listening addr %s:%d\n", (*addr)->addr, (*addr)->port);

		if ((sockutp = net_bind((*addr)->addr, (*addr)->port)) == NULL)
			return NULL;

		for (j = 0; j != nthreads; j++, i++)
			pthread_create(&lookup_threads[i], NULL, _lookup_loop, sockutp);
	}

	if (rdb->options->websrv_port) {
		if ((vthttp_thread = (pthread_t *) malloc(sizeof(pthread_t))) == NULL)
			return -1;
		pthread_create(&vthttp_thread, NULL, _vthttp_loop, NULL);
	}
	/* change uid/gid */
	if (_set_guid() == -1)
		return -1;

	/* monitoring loop */
	_mon_loop();

	return 0;
}
