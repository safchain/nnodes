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

#ifndef __NNODES_LOOKUP_H
#define __NNODES_LOOKUP_H

#include <hl.h>
#include <rdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LOOKUP struct lookup_s

struct lookup_s {
	RDB *rdb;

	/* queries list */
	LIST *qdsections;

	/* answers records list */
	LIST *ansections;

	/* related answers records list */
	LIST *rlsections;

	/* authorities records list */
	LIST *nssections;

	/* additional records list */
	LIST *arsections;

	/* to prevent loop */
	LIST *l_dn_done;
	HCODE *h_dn_done;

	/* to compress domains */
	HCODE *h_dn_compress;

	/* dns client */
	struct sockaddr *client;
};

/* prototypes */
LOOKUP *nnodes_lookup_init (RDB *);
int nnodes_lookup (LOOKUP *, struct sockaddr *, char *, char *, uint32_t);
void nnodes_lookup_free (LOOKUP *);

#endif
