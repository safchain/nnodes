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

#include <types.h>

#include <mm.h>
#include <hl.h>
#include <misc.h>
#include <xml.h>
#include <mon.h>
#include <log.h>
#include <dns.h>

#include "rr.h"
#include "zones.h"
#include "zone.h"
#include "rdb.h"

struct rdb_soa_s {
	char *record;
	char *master;
	char *mail;
	uint32_t serial;
	uint32_t refresh;
	uint32_t retry;
	uint32_t expire;
	uint32_t minimum;
	uint32_t ttl;
};
static struct rdb_soa_s rdb_soa;

static int _valid_data(XML_CONFIG *config) {
	if (!rdb_zone.name) {
		LOG_ERR("no zone name at line #%d", config->line);
		return -1;
	}

	if (!dns_is_name_closed(rdb_zone.name)) {
		LOG_ERR("zone name not closed at line #%d", config->line);
		return -1;
	}

	if (!rdb_soa.record) {
		LOG_ERR("invalid record at line #%d, no record attribute", config->line);
		return -1;
	}
	if (!rdb_soa.master) {
		LOG_ERR("invalid record at line #%d, missing master entry in SOA record", config->line);
		return -1;
	}
	if (!rdb_soa.mail) {
		LOG_ERR("invalid record at line #%d, missing mail entry in SOA record", config->line);
		return -1;
	}
	if (!dns_is_name_closed(rdb_soa.master)) {
		LOG_ERR("invalid record at line #%d, SOA master entry not close: %s", config->line, rdb_soa.master);
		return -1;
	}
	if (!dns_is_name_closed(rdb_soa.mail)) {
		LOG_ERR("invalid record at line #%d, SOA mail entry not close: %s", config->line, rdb_soa.mail);
		return -1;
	}

	return 0;
}

static int _make_rr(XML_CONFIG *config) {
	struct dns_rr_s *rr = NULL;
	char *key = NULL;
	int rtv;

	if (_valid_data(config) == -1)
		return -1;

	if ((rr = calloc(1, sizeof(struct dns_rr_s))) == NULL) {
		LOG_ERR("memory allocation error at line #%d", config->line);
		return -1;
	}

	if (strcasecmp(rdb_soa.record, "@") == 0)
		snprintf(rr->name, sizeof(rr->name), "%s", rdb_zone.name);
	else {
		if (dns_is_name_closed(rdb_soa.record))
			snprintf(rr->name, sizeof(rr->name), "%s", rdb_soa.record);
		else
			snprintf(rr->name, sizeof(rr->name), "%s.%s", rdb_soa.record, rdb_zone.name);
	}

	if (rdb_zone.ttl)
		rr->ttl = rdb_zone.ttl;
	if (rdb_soa.minimum)
		rr->ttl = rdb_soa.minimum;
	if (rdb_soa.ttl)
		rr->ttl = rdb_soa.ttl;

	rr->class = rdb_zone.class;
	rr->type = SOA_TYPE;

	rtv = dns_mk_soa_data(rdb_soa.master, rdb_soa.mail, rdb_soa.serial,
			rdb_soa.refresh, rdb_soa.retry, rdb_soa.expire, rdb_soa.minimum,
			rr);

	if (rtv == -1) {
		LOG_ERR("record not valid at line #%d", config->line);
		goto clean;
	}

	if ((key = dns_compute_rr_key(rr->name, rr->class, rr->type)) == NULL) {
		LOG_ERR("record not valid at line #%d", config->line);
		goto clean;
	}

	if (rdb_add_rr((HCODE *) ((RDB *) config->backptr)->zones, key, rr) == -1) {
		LOG_ERR("error during record insertion at line #%d", config->line);
		goto clean;
	}

	free(key);

	return 0;

clean:
	if (rr)
		free(rr);
	if (key)
		free(key);

	return -1;
}

static int _text(char *text, XML_CONFIG *config) {
	int len;

	if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/master") == 0) {
		if (rdb_soa.master)
			mm_free0((void*) &rdb_soa.master);

		if (dns_is_name_closed(text)) {
                        rdb_soa.master = strdup(text);
		}
                else {
                        len = snprintf(NULL, 0, "%s.%s", text, rdb_zone.name);
			len++;
                        if ((rdb_soa.master = malloc(len)) == NULL)
                                return -1;
                        snprintf(rdb_soa.master, len, "%s.%s", text, rdb_zone.name);
                }
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/mail") == 0) {
		if (rdb_soa.mail)
			mm_free0((void*) &rdb_soa.mail);

		if (dns_is_name_closed(text))
			rdb_soa.mail = strdup(text);
		else {
			len = snprintf(NULL, 0, "%s.%s", text, rdb_zone.name);
			len++;
			if ((rdb_soa.mail = malloc(len)) == NULL)
                                return -1;
			snprintf(rdb_soa.mail, len, "%s.%s", text, rdb_zone.name);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/serial") == 0)
		rdb_soa.serial = atoi(text);
	else if (strcasecmp(config->xpath,
			"/nnodes/zones/zone/soa/soa:data/refresh") == 0)
		rdb_soa.refresh = atoi(text);
	else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/retry") == 0)
		rdb_soa.retry = atoi(text);
	else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/expire") == 0)
		rdb_soa.expire = atoi(text);
	else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data/minimum") == 0)
		rdb_soa.minimum = atoi(text);

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa") == 0) {
		if (strcasecmp(attrname, "record") == 0 && attrvalue) {
			if (rdb_soa.record)
				mm_free0((void*) &rdb_soa.record);
			rdb_soa.record = strdup(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data")	== 0) {
		if (strcasecmp(attrname, "ttl") == 0 && attrvalue)
			rdb_soa.ttl = atoi(attrvalue);
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/soa/soa:data") == 0) {
		_make_rr(config);

		if (rdb_soa.record)
			mm_free0((void*) &rdb_soa.record);
		if (rdb_soa.master)
			mm_free0((void*) &rdb_soa.master);
		if (rdb_soa.mail)
			mm_free0((void*) &rdb_soa.mail);
	}

	return 0;
}

void rdb_soa_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/zones/zone/soa", &xc, sizeof(XML_CALLBACKS));
}
