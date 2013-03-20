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

#include "mm_pool.h"

MM_POOL *mm_pool_new() {
	MM_POOL *pool;

	if ((pool = calloc(1, sizeof(MM_POOL))) == NULL)
		return NULL;

	return pool;
}

void mm_pool_delete(MM_POOL *pool) {
	MM_POOL_NODE *node;

	while (pool->nodes) {
		node = pool->nodes->next;
		mm_free0((void*) &(pool->nodes));
		pool->nodes = node;
	}

	mm_free0((void*) &(pool));
}

void *mm_pool_alloc0(MM_POOL *pool, uint32_t sz) {
	MM_POOL_NODE *node;

	if ((node = calloc(1, sizeof(MM_POOL_NODE) + sz)) == NULL)
		return NULL;

	if (!pool->nodes) {
		pool->nodes = node;
		pool->last = pool->nodes;
	} else {
		pool->last->next = node;
		pool->last = pool->last->next;
	}
	node->alloc = ((char *) node) + sizeof(MM_POOL_NODE *);

	return node->alloc;
}
