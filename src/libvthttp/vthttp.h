/*
*
* Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VTHTTP_H_
#define VTHTTP_H_

#define TIMEOUT 			30
#define MAX_HEADER_SIZE 	64 * 1024

typedef struct {
	char method[32];
	char uri[4096];
	char host[256];
	uint16_t port;
} vthttp_query_t;

typedef struct {
	int32_t sock;
	vthttp_query_t query;
	char opts[MAX_HEADER_SIZE - sizeof(vthttp_query_t)];
} vthttp_req_t;

typedef struct {
	int32_t sock;
	int (*on_request)(void *srv, vthttp_req_t *req);

	int status;
} vthttp_srv_t;


/* prototypes */
int vthttp_run(vthttp_srv_t *);
void vthttp_stop(vthttp_srv_t *);
vthttp_srv_t *vthttp_init(uint16_t);
int vthttp_send(vthttp_req_t *, char *, int);
int vthttp_send_str(vthttp_req_t *, char *);

#endif
