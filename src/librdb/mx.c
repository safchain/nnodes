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

struct rdb_mx_s {
	struct rdb_common_s common;
	uint32_t preference;
	char *exchange;
};
static struct rdb_mx_s rdb_mx;

static int _valid_data(XML_CONFIG *config) {
	if (!rdb_zone.name) {
		LOG_ERR("no zone name at line #%d", config->line);
		return -1;
	}

	if (!dns_is_name_closed(rdb_zone.name)) {
		LOG_ERR("zone name not closed at line #%d", config->line);
		return -1;
	}

	if (!rdb_mx.common.record) {
		LOG_ERR("invalid record at line #%d, no record attribute", config->line);
		return -1;
	}
	if (!rdb_mx.exchange) {
		LOG_ERR("invalid record at line #%d, missing exchange entry in MX record", config->line);
		return -1;
	}
	if (!rdb_mx.preference) {
		LOG_ERR("invalid record at line #%d, missing preference entry in MX record", config->line);
		return -1;
	}

	return 0;
}

static int _make_rr(XML_CONFIG *config) {
	struct dns_rr_s *rr = NULL;
	char *exchange, *key = NULL;
	int len, rtv;

	if (_valid_data(config) == -1)
		return -1;

	if ((rr = rdb_prepare_rr(&(rdb_mx.common), config)) == NULL) {
		LOG_ERR("memory allocation error at line #%d", config->line);
		return -1;
	}

	rr->type = MX_TYPE;

	if (strcasecmp(rdb_mx.exchange, "@") == 0) {
		mm_free0((void*) &rdb_mx.exchange);
		rdb_mx.exchange = strdup(rdb_zone.name);
	} else {
		if (!dns_is_name_closed(rdb_mx.exchange)) {
			len = strlen(rdb_mx.exchange) + 1 + strlen(rdb_zone.name) + 1;

			if ((exchange = (char *) calloc(len, sizeof(char))) == NULL) {
				LOG_ERR("memory allocation error at line #%d", config->line);
				goto clean;
			}
			snprintf(exchange, len, "%s.%s", rdb_mx.exchange, rdb_zone.name);

			mm_free0((void*) &rdb_mx.exchange);
			rdb_mx.exchange = exchange;
		}
	}

	rtv = dns_mk_mx_data(rdb_mx.preference, rdb_mx.exchange, rr);
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
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx/mx:data/exchange") == 0) {
		if (rdb_mx.exchange)
			mm_free0((void*) &rdb_mx.exchange);
		rdb_mx.exchange = strdup(text);
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx/mx:data/preference") == 0)
		rdb_mx.preference = atoi(text);

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx") == 0)
		return rdb_tag_attr(&(rdb_mx.common), attrname, attrvalue, config);
	else if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx/mx:data") == 0)
		return rdb_data_tag_attr(&(rdb_mx.common), attrname, attrvalue, config);

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx/mx:data") == 0) {
		_make_rr(config);

		if (rdb_mx.exchange)
			mm_free0((void*) &rdb_mx.exchange);

		rdb_mx.preference = 0;
		rdb_data_tag_close(&(rdb_mx.common));
	} else if (strcasecmp(config->xpath, "/nnodes/zones/zone/mx") == 0)
		rdb_tag_close(&(rdb_mx.common));

	return 0;
}

void rdb_mx_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/zones/zone/mx", &xc, sizeof(XML_CALLBACKS));
}
