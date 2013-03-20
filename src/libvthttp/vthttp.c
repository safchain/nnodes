/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

#include "vthttp.h"

uint32_t _vthttp_dntoip(char *host) {
	struct hostent *hent;
	char *addr;
	unsigned long lip;

	if ((hent = gethostbyname(host)) == NULL
	)
		return INADDR_NONE;

	addr = (char *) hent->h_addr_list[0];
	memcpy(&lip, addr, 4);

	return lip;
}

int vthttp_send_str(vthttp_req_t *req, char *data) {
	int len;

	len = strlen (data);
	return vthttp_send(req, data, len);
}

int vthttp_send(vthttp_req_t *req, char *data, int sz) {
	struct timeval tv;
	fd_set wfds;
	int snd, s;

	FD_ZERO(&wfds);
	FD_SET(req->sock, &wfds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	snd = 0;
	while (snd < sz) {
		if (select(req->sock + 1, NULL, &wfds, NULL, &tv) == -1) {
			perror("select");
			return -1;
		}

		if (FD_ISSET (req->sock, &wfds)) {
			if ((s = send(req->sock, (char *) data + snd, sz - snd, 0)) == -1) {
				perror("send");
				return -1;
			}

			snd += s;
		} else {
			fprintf(stderr, "timeout on device while send\n");
			return -1;
		}
	}

	return snd;
}

void _vthttp_close(int sock) {
	shutdown(sock, 2);
	close(sock);
}

char *_vthttp_request(vthttp_srv_t *srv, vthttp_req_t *req, char *data) {
	char *method, *uri, *host, *opts;

	// end of request
	if (strstr(data, "\r\n\r\n") == NULL && strstr(data, "\n\n") == NULL)
		return data;

	opts = req->opts;

	if (strncmp(data, "GET ", 4) != 0 && strncmp(data, "POST ", 5) != 0)
		return NULL;

	// method
	method = req->query.method;
	while (*data != ' ')
		*method++ = *data++;
	data++;

	if (strncasecmp(data, "http://", 7) == 0) {
		data += 7;

		// hostname
		host = req->query.host;
		while (*data != '/') {
			if ((uint32_t)(host - req->query.host) > sizeof(req->query.host)) {
				fprintf(stderr, "hostname override\n");

				return NULL;
			}
			*host++ = *data++;
		}
	}

	// uri
	uri = req->query.uri;
	while (*data != ' ') {
		if ((uint32_t)(uri - req->query.uri) > sizeof(req->query.uri)) {
			fprintf(stderr, "uri override\n");

			return NULL;
		}
		*uri++ = *data++;
	}

	// goto end of line
	while (*data != 0 && strncmp(data, "\r\n", 2) != 0)
		data++;

	if (*data == '\0')
		return NULL;
	data += 2;

	while (*data != '\0' && strncmp(data, "\r\n", 2) != 0) {
		// host header field
		if (strncmp(data, "Host: ", 6) == 0) {
			data += 6;

			host = req->query.host;
			while (*data != '\0' && strncmp(data, "\r\n", 2) != 0) {
				if ((uint32_t)(host - req->query.host) > sizeof(req->query.host))
					return NULL;
				*host++ = *data++;
			}
			if (*data == '\0')
				return NULL;
			data += 2;
		}
		else {
			while (*data != '\0' && strncmp(data, "\r\n", 2) != 0) {
				if ((uint32_t)(opts - req->opts) > sizeof(req->opts))
					return NULL;
				*opts++ = *data++;
			}
			if (*data == '\0')
				return NULL;
			*opts++ = *data++; // \r
			*opts++ = *data++; // \n
		}
	}
	if (*data != '\0')
		data += 2;

	return data;
}

int _vthttp_bind(uint16_t port) {
	int sock;
	struct sockaddr_in sin;
	int on = 1;

	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");

		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(int)) == -1) {
		perror("setsockopt");

		return -1;
	}

	sin.sin_family = PF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);

	if (bind(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1) {
		perror("bind");

		return -1;
	}

	if (listen(sock, SOMAXCONN) == -1) {
		perror("listen");

		return -1;
	}

	return sock;
}

static int _vthttp_run(vthttp_srv_t *srv) {
	int c_sock, max_fd;
	struct sockaddr_in sin;
	fd_set rfds, fds;
	struct timeval tv;
	char c_buffer[MAX_HEADER_SIZE], *ptr;
	vthttp_req_t req;
	int r, rcv, len;

	while (1) {
		memset(&req, 0, sizeof(vthttp_req_t));

		// accept connection from a new client
		len = sizeof(struct sockaddr_in);
		if ((c_sock = accept(srv->sock, (struct sockaddr *) &sin, &len)) == -1) {
			perror("accept");

			return -1;
		}

		FD_ZERO(&rfds);
		FD_SET(c_sock, &rfds);
		max_fd = c_sock;

		memset(c_buffer, 0, sizeof(c_buffer));

		rcv = 0;
		while (1) {
			tv.tv_sec = TIMEOUT;
			tv.tv_usec = 0;

			fds = rfds;
			if (select(max_fd + 1, &fds, NULL, NULL, &tv) == -1) {
				perror("select");
				return -1;
			}

			if (FD_ISSET (c_sock, &fds)) {
				if ((r = recv(c_sock, c_buffer + rcv, sizeof(c_buffer) - rcv - 1, 0)) == -1) {
					perror("recv");

					return -1;
				}

				// client hang up, close all connection
				if (r == 0) {
					_vthttp_close(c_sock);

					break;
				}
				rcv += r;

				// request structure construction
				memset(&req, 0, sizeof(vthttp_req_t));
				ptr = _vthttp_request(srv, &req, c_buffer);
				if (ptr == c_buffer)
					continue;
				else if (ptr == NULL) {
					_vthttp_close(c_sock);

					break;
				}
				req.sock = c_sock;

				if (srv->on_request)
					srv->on_request(srv, &req);

				_vthttp_close(c_sock);

				break;
			}
			// timeout all done
			else {
				_vthttp_close(c_sock);

				break;
			}
		}
	}

	return 0;
}

vthttp_srv_t *vthttp_init(uint16_t port) {
	vthttp_srv_t *srv;

	if ((srv = (vthttp_srv_t *) calloc(sizeof(vthttp_srv_t), 1)) == NULL)
		return NULL;

	if ((srv->sock = _vthttp_bind(port)) == -1) {
		free (srv);

		return NULL;
	}

	return srv;
}

void vthttp_stop(vthttp_srv_t *srv) {
	srv->status = 0;
}

int vthttp_run(vthttp_srv_t *srv) {
	if (_vthttp_run(srv) == -1)
		return -1;

	_vthttp_close(srv->sock);

	return 0;
}

/*
int callback (vthttp_srv_t *srv, vthttp_req_t *req) {
	printf ("%s\n", req->query.uri);

	return 0;
}

void main() {
	vthttp_srv_t *srv;

	srv = vthttp_init(9090);
	srv->on_request = callback;
	vthttp_run(srv);
}
*/
