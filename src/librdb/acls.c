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
#include <ast.h>
#include <log.h>

#include "rdb.h"
#include "acls.h"

static char *id;
static AST_TREE *tree;

static int _text(char *text, XML_CONFIG *config) {
	char *expr;
	if (strcasecmp(config->xpath, "/nnodes/acls/acl") == 0) {
		expr = strdup(text);
		if (expr == NULL) {
			LOG_ERR("memory allocation error at line #%d", config->line);
			return -1;
		}

		if ((tree = ast_init(expr)) == NULL) {
			LOG_ERR("memory allocation error at line #%d", config->line);
			return -1;
		}
		if (strlen (tree->error) != 0) {
			LOG_ERR("acl compilation error at line #%d, %s", config->line, tree->error);
			return -1;
		}
	}

	return 0;
}

static int _tag_attr(char *tagname, char *attrname, char *attrvalue, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/acls/acl") == 0) {
		if (strcasecmp(attrname, "id") == 0 && attrvalue) {
			if (id)
				mm_free0((void *) &id);

			if ((id = strdup(attrvalue)) == NULL) {
				LOG_ERR("memory allocation error at line #%d", config->line);
				return -1;
			}
		}
	}

	return 0;
}

static int _tag_close(char *tagname, XML_CONFIG *config) {
	if (strcasecmp(config->xpath, "/nnodes/acls/acl") == 0 && id) {
		if (hl_hash_put((HCODE *) ((RDB *) config->backptr)->acls, id, &tree, sizeof(AST_TREE *)) == NULL)
			return -1;
	}

	return 0;
}

void rdb_acls_add_xc(HCODE *xch) {
	XML_CALLBACKS xc;

	memset(&xc, 0, sizeof(xc));

	xc.on_text = _text;
	xc.on_tag_attr = _tag_attr;
	xc.on_tag_close = _tag_close;

	hl_hash_put(xch, "/nnodes/acls", &xc, sizeof(XML_CALLBACKS));
}
