/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __LIST_H
#define __LIST_H

#include "types.h"

typedef struct list_s LIST;
typedef struct list_node_s LIST_NODE;
typedef struct list_iterator_s LIST_ITERATOR;

struct list_s {
    LIST_NODE *nodes;
    LIST_NODE *last;
    char errmsg[255];
};

/* append value just behind the next pointer, then only one malloc/free for each node */
struct list_node_s {
    LIST_NODE *next;
};

struct list_iterator_s {
    LIST_NODE *current;
};

/* prototypes */
LIST *hl_list_alloc();
int hl_list_push(LIST *, void *, uint32_t);
void hl_list_pop(LIST *);
void *hl_list_get(LIST *, uint32_t);
void hl_list_init_iterator(LIST *, LIST_ITERATOR *);
void *hl_list_iterate(LIST_ITERATOR *);
uint32_t hl_list_count(LIST *);
void hl_list_reset(LIST *);
void hl_list_free(LIST *);
void hl_list_round_robin(LIST *);

#endif
