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

struct rdb_ns_s {
	char *record;
	char *data;
	uint32_t ttl;
};
static struct rdb_ns_s rdb_ns;

static int _valid_data(XML_CONFIG *config) {
	if (!rdb_zone.name) {
		LOG_ERR("no zone name at line #%d", config->line);
		return -1;
	}

	if (!dns_is_name_closed(rdb_zone.name)) {
		LOG_ERR("zone name not closed at line #%d", config->line);
		return -1;
	}

	if (!rdb_ns.record) {
		LOG_ERR("invalid record at line #%d, no record attribute", config->line);
		return -1;
	}

	if (!rdb_ns.data) {
		LOG_ERR("invalid record at line #%d, missing data for NS record", config->line);
		return -1;
	}

	return 0;
}

static int _make_rr(XML_CONFIG *config) {
	struct dns_rr_s *rr = NULL;
	char *ns, *key = NULL;
	int len, rtv;

	if (_valid_data(config) == -1)
		return -1;

	if ((rr = calloc(1, sizeof(struct dns_rr_s))) == NULL) {
		LOG_ERR("memory allocation error at line #%d", config->line);
		return -1;
	}

	if (strcasecmp(rdb_ns.record, "@") == 0)
		snprintf(rr->name, sizeof(rr->name), "%s", rdb_zone.name);
	else {
		if (dns_is_name_closed(rdb_ns.record))
			snprintf(rr->name, sizeof(rr->name), "%s", rdb_ns.record);
		else
			snprintf(rr->name, sizeof(rr->name), "%s.%s", rdb_ns.record, rdb_zone.name);
	}

	if (strcasecmp(rdb_ns.data, "@") == 0) {
		mm_free0((void*) &rdb_ns.data);
		rdb_ns.data = strdup(rdb_zone.name);
	} else {
		if (!dns_is_name_closed(rdb_ns.data)) {
			len = strlen(rdb_ns.data) + 1 + strlen(rdb_zone.name) + 1;

			if ((ns = (char *) calloc(len, sizeof(char))) == NULL) {
				LOG_ERR("memory allocation error at line #%d", config->line);
				goto clean;
			}
			snprintf(ns, len, "%s.%s", rdb_ns.data, rdb_zone.name);

			mm_free0((void*) &rdb_ns.data);
			rdb_ns.data = ns;
		}
	}

	if (rdb_zone.ttl)
		rr->ttl = rdb_zone.ttl;
	if (rdb_ns.ttl)
		rr->ttl = rdb_ns.ttl;

	rr->class = rdb_zone.class;
	rr->type = NS_TYPE;

	rtv = dns_mk_ns_data(rdb_ns.data, rr);
	if (rtv == -1) {
		LOG_ERR("record not valid at line #%d", config->line);
		goto clean;
	}

	if ((key = dns_compute_rr_key(rr->name, rr->class, rr->type)) == NULL) {
		LOG_ERR("record not valid at line #%d", config->line);
		goto clean;
	}

	if (rdb_add_rr((HCODE *) ((RDB *) config->backptr)->zones, key, rr) == -1) {
		LOG_ERR("error during record insertion at line #%d\n", config->line);
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
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/ns/ns:data") == 0)
		rdb_ns.data = strdup(text);

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue,
		XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/ns") == 0) {
		if (strcasecmp(attrname, "record") == 0 && attrvalue) {
			if (rdb_ns.record)
				mm_free0((void*) &rdb_ns.record);
			rdb_ns.record = strdup(attrvalue);
		}
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/ns/ns:data") == 0
			&& attrvalue) {
		if (strcasecmp(attrname, "ttl") == 0 && attrvalue)
			rdb_ns.ttl = atoi(attrvalue);
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/ns/ns:data") == 0) {
		_make_rr(config);

		if (rdb_ns.data)
			mm_free0((void*) &rdb_ns.data);
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/ns") == 0) {
		if (rdb_ns.record)
			mm_free0((void*) &rdb_ns.record);
	}

	return 0;
}

void rdb_ns_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/zones/zone/ns", &xc, sizeof(XML_CALLBACKS));
}
