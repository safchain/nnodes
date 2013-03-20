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
#include <pthread.h>
#include <types.h>

#include <mm.h>
#include <xml.h>
#include <ast.h>
#include <mon.h>
#include <log.h>

#include "rdb.h"
#include "rr.h"
#include "zone.h"
#include "common.h"

struct dns_rr_s *rdb_prepare_rr(struct rdb_common_s *common, XML_CONFIG *config) {
	struct dns_rr_s *rr = NULL;
	MON *mon = NULL;
	AST_TREE **acl = NULL;

	if ((rr = calloc(1, sizeof(struct dns_rr_s))) == NULL) {
		LOG_ERR("memory allocation error at line #%d", config->line);
		return NULL;
	}

	if (strcasecmp(common->record, "@") == 0)
		snprintf(rr->name, sizeof(rr->name), "%s", rdb_zone.name);
	else {
		if (dns_is_name_closed(common->record))
			snprintf(rr->name, sizeof(rr->name), "%s",
					common->record);
		else
			snprintf(rr->name, sizeof(rr->name), "%s.%s",
					common->record, rdb_zone.name);
	}

	if (rdb_zone.ttl)
		rr->ttl = rdb_zone.ttl;
	if (common->defaultttl)
		rr->ttl = common->defaultttl;
	if (common->ttl)
		rr->ttl = common->ttl;

	rr->class = rdb_zone.class;

	if (common->defaultweight)
		rr->extra.weight = common->defaultweight;
	if (common->weight_set)
		rr->extra.weight = common->weight;

	if (common->monitor)
		mon = hl_hash_get((HCODE *) ((RDB *) config->backptr)->monitors,
				common->monitor);
	else if (common->defaultmonitor)
		mon = hl_hash_get((HCODE *) ((RDB *) config->backptr)->monitors,
				common->defaultmonitor);

	if (mon)
		hl_list_push(mon->rrs, &rr, sizeof(struct dns_rr_s *));

	if (common->acl)
		acl = hl_hash_get((HCODE *) ((RDB *) config->backptr)->acls,
				common->acl);
	else if (common->defaultacl)
		acl = hl_hash_get((HCODE *) ((RDB *) config->backptr)->acls,
				common->defaultacl);

	if (acl)
		rr->extra.ast_acl = *acl;

	/* for threads safe */
	pthread_rwlock_init(&(rr->extra.rwlock_unavailable), NULL);

	return rr;
}

int rdb_tag_attr(struct rdb_common_s *common, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(attrname, "record") == 0 && attrvalue) {
		if (common->record)
			mm_free0((void*) &(common->record));
		common->record = strdup(attrvalue);
	} else if (strcasecmp(attrname, "weight") == 0 && attrvalue)
		common->defaultweight = atoi(attrvalue);
	else if (strcasecmp(attrname, "ttl") == 0 && attrvalue)
			common->defaultttl = atoi(attrvalue);
	else if (strcasecmp(attrname, "monitor") == 0 && attrvalue) {
		if (common->defaultmonitor)
			mm_free0((void*) &(common->defaultmonitor));
		common->defaultmonitor = strdup(attrvalue);
	} else if (strcasecmp(attrname, "acl") == 0 && attrvalue) {
		if (common->defaultacl)
			mm_free0((void*) &(common->defaultacl));
		common->defaultacl = strdup(attrvalue);
	}
	else {
		LOG_ERR("unknow attribute at line #%d", config->line);
		return -1;
	}

	return 0;
}

int rdb_data_tag_attr(struct rdb_common_s *common, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(attrname, "weight") == 0 && attrvalue) {
		common->weight = atoi(attrvalue);
		common->weight_set = 1;
	} else if (strcasecmp(attrname, "ttl") == 0 && attrvalue)
		common->ttl = atoi(attrvalue);
	else if (strcasecmp(attrname, "monitor") == 0 && attrvalue) {
		if (common->monitor)
			mm_free0((void*) &(common->monitor));
		common->monitor = strdup(attrvalue);
	} else if (strcasecmp(attrname, "acl") == 0 && attrvalue) {
		if (common->acl)
			mm_free0((void*) &(common->acl));
		common->acl = strdup(attrvalue);
	}
	else {
		LOG_ERR("unknow attribute at line #%d", config->line);
		return -1;
	}

	return 0;
}

void rdb_data_tag_close(struct rdb_common_s *common) {
	if (common->monitor)
		mm_free0((void*) &(common->monitor));
	if (common->acl)
		mm_free0((void*) &(common->acl));

	common->ttl = 0;
	common->weight = 0;
	common->weight_set = 0;
}

void rdb_tag_close(struct rdb_common_s *common) {
	if (common->record)
		mm_free0((void*) &(common->record));
	if (common->defaultmonitor)
		mm_free0((void*) &(common->defaultmonitor));
	if (common->defaultacl)
		mm_free0((void*) &(common->defaultacl));

	common->defaultttl = 0;
	common->defaultweight = 0;
}
