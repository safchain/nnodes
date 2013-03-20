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

#ifndef __MM_POOL_H
#define __MM_POOL_H

#include "types.h"

typedef struct mm_pool_s MM_POOL;
typedef struct mm_pool_node_s MM_POOL_NODE;

struct mm_pool_s {
	MM_POOL_NODE *nodes;
	MM_POOL_NODE *last;
};

struct mm_pool_node_s {
	MM_POOL_NODE *next;
	void *alloc;
};


/* prototypes */
MM_POOL *mm_pool_new();
void mm_pool_delete(MM_POOL *);
void *mm_pool_alloc0(MM_POOL *, uint32_t);

#endif
