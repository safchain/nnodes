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
#include "common.h"

struct rdb_a_s {
	struct rdb_common_s common;
	char *data;
};
static struct rdb_a_s rdb_a;

static int _valid_data(XML_CONFIG *config) {
	if (!rdb_zone.name) {
		LOG_ERR("no zone name at line #%d", config->line);
		return -1;
	}

	if (!dns_is_name_closed(rdb_zone.name)) {
		LOG_ERR("zone name not closed at line #%d", config->line);
		return -1;
	}

	if (!rdb_a.common.record) {
		LOG_ERR("invalid record at line #%d, no record attribute", config->line);
		return -1;
	}

	if (!rdb_a.data) {
		LOG_ERR("invalid record at line #%d, missing data for A record", config->line);
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

	if ((rr = rdb_prepare_rr(&(rdb_a.common), config)) == NULL) {
		LOG_ERR("memory allocation error at line #%d", config->line);
		return -1;
	}

	rr->type = A_TYPE;

	rtv = dns_mk_a_data(rdb_a.data, rr);
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
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/a/a:data") == 0)
		rdb_a.data = strdup(text);

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue,
		XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/a") == 0)
		return rdb_tag_attr(&(rdb_a.common), attrname, attrvalue, config);
	else if (strcasecmp(config->xpath, "/nnodes/zones/zone/a/a:data") == 0)
		return rdb_data_tag_attr(&(rdb_a.common), attrname, attrvalue, config);

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/a/a:data") == 0) {
		_make_rr(config);

		if (rdb_a.data)
			mm_free0((void*) &rdb_a.data);

		rdb_data_tag_close(&(rdb_a.common));
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/a") == 0)
		rdb_tag_close(&(rdb_a.common));

	return 0;
}

void rdb_a_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/zones/zone/a", &xc, sizeof(XML_CALLBACKS));
}
