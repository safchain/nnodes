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
#include <log.h>
#include <dns.h>

#include "zones.h"
#include "zone.h"
#include "cname.h"
#include "a.h"
#include "soa.h"
#include "txt.h"
#include "mx.h"
#include "ptr.h"
#include "ns.h"
#include "rdb.h"

struct rdb_zone_s rdb_zone;

static int _read_zone_src(char *filename, XML_CONFIG *config) {
	XML_CONFIG *xml_zone_config;

	if ((xml_zone_config = xml_alloc_config()) == NULL)
		goto clean;

	xml_zone_config->backptr = config->backptr;

	rdb_zone_add_xc(xml_zone_config->callbacks);
	rdb_soa_add_xc(xml_zone_config->callbacks);
	rdb_a_add_xc(xml_zone_config->callbacks);
	rdb_cname_add_xc(xml_zone_config->callbacks);
	rdb_ns_add_xc(xml_zone_config->callbacks);
	rdb_txt_add_xc(xml_zone_config->callbacks);
	rdb_mx_add_xc(xml_zone_config->callbacks);
	rdb_ptr_add_xc(xml_zone_config->callbacks);

	if (xml_parse_file(filename, xml_zone_config) == -1)
		goto clean;

	if (strlen(xml_zone_config->error) != 0) {
		LOG_ERR("%s", xml_zone_config->error);
		goto clean;
	}

	if (xml_zone_config->callbacks)
		hl_hash_free(xml_zone_config->callbacks);

	xml_free_config(xml_zone_config);

	return 0;

clean:
	if (xml_zone_config->callbacks)
		hl_hash_free(xml_zone_config->callbacks);

	xml_free_config(xml_zone_config);

	return -1;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	int rtv;

	if (strcasecmp(config->xpath, "/nnodes/zones/zone") == 0) {
		if (strcasecmp(attrname, "name") == 0) {
			if (rdb_zone.name)
				mm_free0((void*) &rdb_zone.name);
			rdb_zone.name = strdup(attrvalue);
		} else if (strcasecmp(attrname, "class") == 0) {
			if ((rtv = dns_class_to_int(attrvalue)) == -1) {
				LOG_ERR("invalid class, line #%d", config->line);

				return -1;
			}
			rdb_zone.class = rtv;
		} else if (strcasecmp(attrname, "ttl") == 0)
			rdb_zone.ttl = atoi(attrvalue);
		else if (strcasecmp(attrname, "src") == 0)
			if (_read_zone_src(attrvalue, config) == -1)
				return -1;
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/zones/zone") == 0)
		if (rdb_zone.name)
			mm_free0((void*) &rdb_zone.name);

	return 0;
}

void rdb_zone_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/zones/zone", &xc, sizeof(XML_CALLBACKS));
}
