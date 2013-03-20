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
#include <sys/types.h>

#include <types.h>

#include <mm.h>
#include <hl.h>
#include <misc.h>
#include <xml.h>
#include <mon.h>
#include <log.h>

#include "rdb.h"
#include "monitors.h"

static char *id;
static MON mon;

static int _text(char *text, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/monitors/pipe") == 0)
		mon.mon.pipe.cmd = strdup(text);

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/monitors/pipe") == 0) {
		if (strcasecmp(attrname, "id") == 0 && attrvalue) {
			if (id)
				mm_free0((void *) &id);

			if ((id = strdup(attrvalue)) == NULL) {
				LOG_ERR("memory allocation error at line #%d", config->line);
				return -1;
			}
		} else if (strcasecmp(attrname, "refresh") == 0 && attrvalue) {
			mon.refresh = atoi(attrvalue);
		} else if (strcasecmp(attrname, "protection") == 0 &&
			strcasecmp(attrvalue, "true") == 0) {
			mon.protection = 1;
		} else if (strcasecmp(attrname, "try") == 0 && attrvalue) {
			mon.try = atoi(attrvalue);
		}
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/monitors/pipe") == 0 && id) {
		mon.type = MON_TYPE_PIPE;

		if ((mon.rrs = hl_list_alloc()) == NULL)
			return -1;

		if ((mon.name = strdup(id)) == NULL) {
			LOG_ERR("memory allocation error at line #%d", config->line);
			return -1;
		}

		if (! mon.mon.pipe.cmd) {
			LOG_ERR("no command line for monitor type pipe %s at line #%d", id, config->line);
			return -1;
		}

		if (hl_hash_put((HCODE *) ((RDB *) config->backptr)->monitors, id, &mon, sizeof(MON)) == NULL) {
			free(mon.name);
			return -1;
		}
	}

	return 0;
}

void rdb_monitors_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/monitors", &xc, sizeof(XML_CALLBACKS));
}
