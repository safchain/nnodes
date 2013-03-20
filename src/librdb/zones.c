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

#include <dns.h>

#include "zones.h"
#include "rdb.h"

struct rdb_zones_s rdb_zones;

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
        if (strcasecmp(config->xpath, "/nnodes/zones") == 0) {
                if (strcasecmp(attrname, "ttl") == 0 && attrvalue)
                        rdb_zones.ttl = atoi(attrvalue);
        }

        return 0;
}

void rdb_zones_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof (xc));

	xc.on_tag_attr = _tag_attr;
        
	hl_hash_put(xch, "/nnodes/zones", &xc, sizeof (XML_CALLBACKS));
}
