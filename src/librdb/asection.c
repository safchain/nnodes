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

#include "asection.h"

int rdb_add_asection(HCODE *zones, char *key, struct dns_asection_s *new) {
        struct dns_asection_s **last;
        LIST **list;
        LIST *nlist;
        uint32_t weight = 0, count;

        if ((list = hl_hash_get(zones, key)) == NULL) {
                nlist = hl_list_alloc();

                if (dns_strtoraw(new->name, new->raw, sizeof (new->raw)) == -1)
                        return -1;

                new->extra.weight_min = 0;
                new->extra.weight_max = new->extra.weight;

                if (hl_list_push(nlist, &new, sizeof (struct dns_asection_s *)) == -1)
                        return -1;

                return hl_hash_put(zones, key, &nlist, sizeof (struct List *));
        }

        if ((count = hl_list_count(*list)) > 0) {
                if ((last = hl_list_get(*list, count - 1)) != NULL) {
                        weight = (*last)->extra.weight_max;
                }
        }

        if (dns_strtoraw(new->name, new->raw, sizeof (new->raw)) == -1)
                return -1;

	fprintf (stderr, "%s => %d\n", new->name, new->extra.weight);

	
        new->extra.weight_min = weight;
        new->extra.weight_max = weight + new->extra.weight;

        if (hl_list_push(*list, &new, sizeof (struct dns_asection_s *)) == -1)
                return -1;

        return 0;
}
