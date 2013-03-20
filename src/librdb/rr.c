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
#include <log.h>
#include <ast.h>

#include "rr.h"

extern AST_CONSTS_LIST acls_consts;

int rdb_add_rr(HCODE *zones, char *key, struct dns_rr_s *new) {
	AST_CONST *conzt;
	LIST **list;
	LIST *nlist;
	char *str;
	int len;

	if ((list = hl_hash_get(zones, key)) == NULL) {
		nlist = hl_list_alloc();

		if (dns_strtoraw(new->name, new->raw, sizeof(new->raw)) == -1)
			return -1;

		if (hl_list_push(nlist, &new, sizeof(struct dns_rr_s *)) == -1)
			return -1;

		/* acl add */
		len = snprintf(NULL, 0, "%s %s %s", new->name,
				dns_int_to_class(new->class),
				dns_int_to_type(new->type));
		len++;
		if ((str = malloc(len)) == NULL)
			return -1;
		snprintf(str, len, "%s %s %s", new->name,
				dns_int_to_class(new->class),
				dns_int_to_type(new->type));

		if ((conzt = ast_add_const(&acls_consts, str)) != NULL) {
			conzt->type = AST_TYPE_POINTER;
			conzt->cnst.pointer = nlist;
		}
		free(str);

		str = dns_get_data_str(new);
		LOG_INFO("add record [%s %s %s %s] in database",
				new->name,
				dns_int_to_class(new->class),
				dns_int_to_type(new->type),
				str);
		free(str);

		return hl_hash_put(zones, key, &nlist, sizeof(struct List *));
	}

	if (dns_strtoraw(new->name, new->raw, sizeof(new->raw)) == -1)
		return -1;

	if (hl_list_push(*list, &new, sizeof(struct dns_rr_s *)) == -1)
		return -1;

	str = dns_get_data_str(new);
	LOG_INFO("add record [%s %s %s %s] in database",
			new->name,
			dns_int_to_class(new->class),
			dns_int_to_type(new->type),
			str);
	free(str);

	return 0;
}
