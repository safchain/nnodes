/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <types.h>

#include <mm.h>
#include <hl.h>
#include <misc.h>
#include <xml.h>
#include <log.h>

#include "rdb.h"
#include "options.h"

static char *curr_addr = NULL;
static uint16_t curr_port = 53;

static int _add_addr (LIST *addrs) {
	struct rdb_addr_s *addr;

	if ((addr = (struct rdb_addr_s *) malloc (sizeof(struct rdb_addr_s))) == NULL)
		return -1;

	addr->addr = curr_addr;
	addr->port = curr_port;

	if (hl_list_push(addrs, &addr, sizeof (struct rdb_addr_s *)) == -1)
		return -1;

	return 0;
}

static int _add_all_addr (LIST *addrs) {
	char ip[INET_ADDRSTRLEN];
	struct ifconf ifconf;
	struct ifreq ifr[50];
	struct sockaddr_in *s_in;
	int ifs, sock, i;

	ifconf.ifc_buf = (char *) ifr;
	ifconf.ifc_len = sizeof ifr;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	if (ioctl(sock, SIOCGIFCONF, &ifconf) == -1)
		return -1;

	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	for (i = 0; i < ifs; i++) {
		s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;	
		
		if (!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip))) {
			LOG_ERR("unable to get address of %s", ifr[i].ifr_name);
			return -1;
		}

		curr_addr = strdup(ip);
		if (_add_addr (addrs) == -1) {
			LOG_ERR("unable to add address of %s to listener interfaces", ifr[i].ifr_name);
			return -1;
		}
	}

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	struct rdb_options_s *options = ((RDB *) config->backptr)->options;

	if (strcasecmp(config->xpath, "/nnodes/options/listen") == 0) {
		if (strcasecmp(attrname, "port") == 0 && attrvalue) {
			curr_port = atoi(attrvalue);
		}
		else if (strcasecmp(attrname, "addr") == 0 && attrvalue) {
			curr_addr = strdup(attrvalue);	
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/user") == 0) {
		if (strcasecmp(attrname, "uid") == 0 && attrvalue) {
			options->uid = atoi(attrvalue);
		} else if (strcasecmp(attrname, "gid") == 0 && attrvalue) {
			options->gid = atoi(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/pid") == 0) {
		if (strcasecmp(attrname, "file") == 0 && attrvalue) {
			options->pid = strdup(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/threads") == 0) {
		if (strcasecmp(attrname, "value") == 0 && attrvalue) {
			options->nthreads = atoi(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/log") == 0) {
		if (strcasecmp(attrname, "file") == 0 && attrvalue) {
			options->log_file = strdup(attrvalue);
		}
		else if (strcasecmp(attrname, "level") == 0 && attrvalue) {
			options->log_level = atoi(attrvalue);
		}
		else if (strcasecmp(attrname, "maxfile") == 0 && attrvalue) {
			options->log_maxfile = atoi(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/websrv") == 0) {
		if (strcasecmp(attrname, "xslt") == 0 && attrvalue) {
			options->websrv_xslt = strdup(attrvalue);
		}
		else if (strcasecmp(attrname, "port") == 0 && attrvalue) {
			options->websrv_port = atoi(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/options/geoip") == 0) {
#ifdef HAVE_GEOIP
		if (strcasecmp(attrname, "database") == 0 && attrvalue) {
			if ((options->gi = GeoIP_open(attrvalue, GEOIP_MEMORY_CACHE)) == NULL) {
				LOG_ERR("error opening database %s", attrvalue);
				return -1;
			}
		}
#else
		LOG_WARN ("%s\n", "not compiled with geoip");
#endif

	}

	return 0;
}

static int _tag_open(char *tagname, XML_CONFIG *config) {
	RDB *rdb = (RDB *) config->backptr;

	if (strcasecmp(config->xpath, "/nnodes/options") == 0) {
		if ((rdb->options = calloc(1, sizeof (struct rdb_options_s))) == NULL)
			return -1;
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	RDB *rdb = (RDB *) config->backptr;

	if (strcasecmp(config->xpath, "/nnodes/options/listen") == 0) {
		if (curr_addr) {
			if (! rdb->options->addrs)
				if((rdb->options->addrs = hl_list_alloc ()) == NULL)
					return -1;
			if (strcmp(curr_addr, "0.0.0.0") == 0)
				_add_all_addr(rdb->options->addrs);
			else
				_add_addr (rdb->options->addrs);
		}
		curr_addr = NULL;
		curr_port = 53;
	}

	return 0;
}

void rdb_options_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_tag_attr = _tag_attr;
	xc.on_tag_open = _tag_open;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/options", &xc, sizeof(XML_CALLBACKS));
}
