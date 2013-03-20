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
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>

#include <types.h>
#include <time.h>

#include <mm.h>
#include <hl.h>
#include <xml.h>
#include <dns.h>
#include <log.h>
#include <ast.h>
#include <nnodes.h>

#include "rdb.h"

#include "cname.h"
#include "a.h"
#include "soa.h"
#include "txt.h"
#include "mx.h"
#include "ptr.h"
#include "ns.h"
#include "zone.h"
#include "zones.h"
#include "options.h"
#include "monitors.h"
#include "acls.h"

#ifdef HAVE_GEOIP
#include <GeoIP.h>
#endif

AST_CONSTS_LIST acls_consts 	= { .consts = NULL, .last = NULL };
AST_VARS_LIST 	acls_vars 		= { .vars = NULL, .last = NULL };
AST_OPS_LIST 	acls_ops 		= { .ops = NULL, .last = NULL };

uint32_t rdb_rrs_available(LIST *rrs) {
	struct dns_rr_s **rr;
	LIST_ITERATOR iterator;

	hl_list_init_iterator(rrs, &iterator);

	while ((rr = hl_list_iterate(&iterator)) != NULL) {
		pthread_rwlock_rdlock(&((*rr)->extra.rwlock_unavailable));
		if (! (*rr)->extra.unavailable) {
			pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
			return 1;
		}
		pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
	}

	return 0;
}
_AST_OP_P(acl_available, return rdb_rrs_available((LIST *) val0); )

uint32_t _acl_ipaddr(char *ip) {
	return inet_addr(ip);
}
_AST_OP_S(acl_ipaddr, return _acl_ipaddr((char *) val0); )

#ifdef HAVE_GEOIP
uint32_t _acl_geoip_region(char *region, LOOKUP *lookup) {
	GeoIPRegion *gir;
	char *code;

	if (! lookup->rdb->options || ! lookup->rdb->options->gi)
		return 1;

	code = GeoIP_country_code_by_ipnum (lookup->rdb->options->gi, htonl(((struct sockaddr_in *)lookup->client)->sin_addr.s_addr));
	if (code != NULL && strcasecmp(region, code) == 0) {
		return 1;
	}
	return 0;
}
_AST_OP_S(acl_geoip_region, return _acl_geoip_region((char *) val0, (LOOKUP *) context); )
#endif

/* ast vars */
uint32_t acl_var_timestamp (void *context) {
	return (uint32_t) time(NULL);
}

static int _free_zones(HCODE *zones) {
	struct dns_rr_s **rr;
	LIST_ITERATOR keys_iterator, list_iterator;
	LIST *keys, **list;
	char *key;

	if (zones == NULL)
		return 0;

	if ((keys = hl_hash_keys(zones)) == NULL)
		return -1;

	hl_list_init_iterator(keys, &keys_iterator);

	while ((key = hl_list_iterate(&keys_iterator)) != NULL) {
		if ((list = hl_hash_get(zones, key)) != NULL) {
			hl_list_init_iterator(*list, &list_iterator);

			while ((rr = hl_list_iterate(&list_iterator)) != NULL) {
				if ((*rr)->data)
					free((*rr)->data);
				free(*rr);
			}

			hl_list_free(*list);
		}
	}
	hl_list_free(keys);
	hl_hash_free(zones);

	return 0;
}

static int _free_options(struct rdb_options_s *options) {

	if (options->pid)
		free (options->pid);
	if (options->log_file)
		free (options->log_file);

	free(options);

	return 0;
}

static int _free_monitors(HCODE *monitors) {
	HCODE_ITERATOR iterator;
	HNODE *hnode_ptr;
	MON *mon;

	hl_hash_init_iterator(monitors, &iterator);
	while ((hnode_ptr = hl_hash_iterate(&iterator))) {
		mon = (MON *) hnode_ptr->value;
		mon_terminate(mon);

		free(mon->name);
	}

	hl_hash_free(monitors);

	return 0;
}

static int _free_acls(HCODE *acls) {
	HCODE_ITERATOR iterator;
	HNODE *node;
	AST_TREE **tree;

	hl_hash_init_iterator(acls, &iterator);

	while ((node = hl_hash_iterate(&iterator))) {
		tree = (AST_TREE **) node->value;

		free((*tree)->expr);

		ast_free(*tree);
	}

	hl_hash_free(acls);

	return 0;
}

static int _init_monitors(HCODE *monitors) {
	HCODE_ITERATOR iterator;
	HNODE *hnode_ptr;
	MON *mon;

	hl_hash_init_iterator(monitors, &iterator);
	while ((hnode_ptr = hl_hash_iterate(&iterator))) {
		mon = (MON *) hnode_ptr->value;
		if (mon_initialize(mon) == -1)
			return -1;
	}

	return 0;
}

static int _init_acls(HCODE *acls) {
	HCODE_ITERATOR iterator;
	HNODE *node;
	AST_TREE **tree;
	AST_VAR *var;
	AST_OP *op;

	/* add vars */
	if ((var = ast_add_var(&acls_vars, "timestamp")) != NULL) {
		var->type = AST_TYPE_INTEGER;
		var->func.integer = acl_var_timestamp;
	}

	/* add ops */
	if ((op = ast_add_op(&acls_ops, "available")) != NULL) {
		op->type = AST_OP_TYPE_P;
		op->func = acl_available;
	}
#ifdef HAVE_GEOIP
	if ((op = ast_add_op(&acls_ops, "region")) != NULL) {
		op->type = AST_OP_TYPE_S;
		op->func = acl_geoip_region;
	}
#endif

	hl_hash_init_iterator(acls, &iterator);

	while ((node = hl_hash_iterate(&iterator))) {
		tree = (AST_TREE **) node->value;

		if (ast_compil(*tree, NULL, &acls_vars, &acls_consts, &acls_ops) == -1) {
			LOG_ERR("acl %s compilation error: %s", node->key, (*tree)->error);

			return -1;
		}

		(*tree)->name = node->key;
	}

	return 0;
}

int rdb_free(RDB *rdb) {
	if (rdb && rdb->zones && _free_zones(rdb->zones) == -1)
		return -1;

	if (rdb && rdb->options && _free_options(rdb->options) == -1)
		return -1;

	if (rdb && rdb->monitors && _free_monitors(rdb->monitors) == -1)
		return -1;

	if (rdb && rdb->acls && _free_acls(rdb->acls) == -1)
		return -1;

	free(rdb);

	return 0;
}

static int _rdb_read_first_pass(char *filename, RDB *rdb) {
	XML_CONFIG *xml_config;
	int rtv = 0;

	if ((xml_config = xml_alloc_config()) == NULL) {
		rtv = -1;
		goto clean;
	}

	xml_config->backptr = rdb;

	rdb_options_add_xc(xml_config->callbacks);
	rdb_monitors_add_xc(xml_config->callbacks);

	if ((rdb->monitors = hl_hash_alloc(0)) == NULL) {
		rtv = -1;
		goto clean;
	}

	rdb_acls_add_xc(xml_config->callbacks);

	if ((rdb->acls = hl_hash_alloc(0)) == NULL) {
		rtv = -1;
		goto clean;
	}

	if (xml_parse_file(filename, xml_config) == -1) {
		rtv = -1;
		goto clean;
	}

	if (strlen(xml_config->error) != 0) {
		LOG_ERR("configuration error: %s", xml_config->error);
		rtv = -1;
		goto clean;
	}

clean:
	if (xml_config->callbacks)
		hl_hash_free(xml_config->callbacks);

	xml_free_config(xml_config);

	return rtv;
}

RDB *rdb_read(char *filename) {
	XML_CONFIG *xml_config;
	RDB *rdb = NULL;

	if ((xml_config = xml_alloc_config()) == NULL)
		goto clean;

	if ((rdb = calloc(1, sizeof(RDB))) == NULL)
		goto clean;

	/* first pass read, options monitors and others things used by zones */
	if (_rdb_read_first_pass(filename, rdb) == -1)
		goto clean;

	/* open log file */
	if (rdb->options && rdb->options->log_file)
		logg = log_open(rdb->options->log_file, rdb->options->log_level, rdb->options->log_maxfile);

	if ((rdb->zones = hl_hash_alloc(0)) == NULL)
		goto clean;

	xml_config->backptr = rdb;

	rdb_zones_add_xc(xml_config->callbacks);
	rdb_zone_add_xc(xml_config->callbacks);
	rdb_soa_add_xc(xml_config->callbacks);
	rdb_a_add_xc(xml_config->callbacks);
	rdb_cname_add_xc(xml_config->callbacks);
	rdb_ns_add_xc(xml_config->callbacks);
	rdb_txt_add_xc(xml_config->callbacks);
	rdb_mx_add_xc(xml_config->callbacks);
	rdb_ptr_add_xc(xml_config->callbacks);

	if (xml_parse_file(filename, xml_config) == -1)
		goto clean;

	if (strlen(xml_config->error) != 0)
		goto clean;

	if (xml_config->callbacks)
		hl_hash_free(xml_config->callbacks);

	xml_free_config(xml_config);

	/* initialize acls, records needs for constant */
	if (_init_acls(rdb->acls) == -1) {
		goto clean;
	}

	/* initialize monitors, records needs */
	if (_init_monitors(rdb->monitors) == -1) {
		goto clean;
	}

	return rdb;

clean:
	rdb_free(rdb);

	if (strlen(xml_config->error) != 0)
		LOG_ERR("%s", xml_config->error);

	if (xml_config->callbacks)
		hl_hash_free(xml_config->callbacks);

	xml_free_config(xml_config);

	return NULL;
}
