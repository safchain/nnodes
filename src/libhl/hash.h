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

#ifndef __HCODE_H
#define __HCODE_H

#include <types.h>
#include "list.h"

typedef struct hcode_s HCODE;
typedef struct hnode_s HNODE;
typedef struct hl_hash_iterator_s HCODE_ITERATOR;

struct hcode_s {
    HNODE *nodes;
    uint32_t hsize;
};

struct hnode_s {
    char *key;
    void *value;
    HNODE *next;
};

struct hl_hash_iterator_s {
    HCODE *hcode;
    uint32_t index;
    HNODE *current;
};

/* prototypes */
char *hl_hash_get_error(void);
HCODE *hl_hash_alloc();
HNODE *hl_hash_put(HCODE *, char *, void *, uint32_t);
void *hl_hash_get(HCODE *, char *);
LIST *hl_hash_keys(HCODE *);
LIST *hl_hash_values(HCODE *);
void hl_hash_reset(HCODE *);
void hl_hash_free(HCODE *);
void hl_hash_free_node(HNODE *);
HNODE *hl_hash_iterate(HCODE_ITERATOR *);
void hl_hash_init_iterator(HCODE *, HCODE_ITERATOR *);

#endif
