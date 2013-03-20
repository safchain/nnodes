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

#ifndef __RDB_COMMON_H
#define __RDB_COMMON_H

#include <types.h>
#include <hl.h>
#include <dns.h>
#include <xml.h>

struct rdb_common_s {
	char *record;
	uint16_t defaultweight;
	uint16_t weight;
	char weight_set;
	uint32_t defaultttl;
	uint32_t ttl;
	char *defaultmonitor;
	char *monitor;
	char *defaultacl;
	char *acl;
};

/* prototypes */
struct dns_rr_s *rdb_prepare_rr(struct rdb_common_s *, XML_CONFIG *);
int rdb_tag_attr(struct rdb_common_s *, char *, char *, XML_CONFIG *);
int rdb_data_tag_attr(struct rdb_common_s *, char *, char *, XML_CONFIG *);
void rdb_tag_close(struct rdb_common_s *);
void rdb_data_tag_close(struct rdb_common_s *);

#endif
