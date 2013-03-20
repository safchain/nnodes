/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <types.h>
#include <mm.h>
#include "hash.h"
#include "list.h"

#define HSIZE 255

static char error[255];

char *hl_hash_geterror(void) {
	return error;
}

HCODE *hl_hash_alloc(uint32_t size) {
	HCODE *hcode;

	if (! size)
		size = HSIZE;

	if ((hcode = malloc(sizeof(HCODE))) == NULL) {
		snprintf(error, sizeof(error), "memory allocation error");
		return NULL;
	}

	if ((hcode->nodes = calloc(size + 1, sizeof(HNODE))) == NULL) {
		snprintf(error, sizeof(error), "memory allocation error");
		free(hcode);
		return NULL;
	}
	hcode->hsize = size;

	return hcode;
}

static inline int _hl_hash_get_key(HCODE *hcode, char *key) {
	uint32_t index = 0;

	while (*key != '\0')
		index = index * 33 + *key++;

	return index & hcode->hsize;
}

HNODE *hl_hash_put(HCODE *hcode, char *key, void *value, uint32_t size) {
	HNODE *hnode_ptr = hcode->nodes;
	uint32_t index = 0;

	index = _hl_hash_get_key(hcode, key);

	hnode_ptr = (HNODE *) (hnode_ptr + index);
	do {
		/* new key / value */
		if (!hnode_ptr->key) {
			if ((hnode_ptr->key = strdup(key)) == NULL) {
				snprintf(error, sizeof(error), "memory allocation error");
				return NULL;
			}

			if ((hnode_ptr->value = malloc(size)) == NULL) {
				snprintf(error, sizeof(error), "memory allocation error");
				goto error;
			}
			memcpy(hnode_ptr->value, value, size);

			break;
		}

		/* old key / new value ? */
		if (strcmp(key, hnode_ptr->key) == 0) {
			free(hnode_ptr->value);
			if ((hnode_ptr->value = malloc(size)) == NULL) {
				snprintf(error, sizeof(error), "memory allocation error");
				goto error;
			}
			memcpy(hnode_ptr->value, value, size);

			break;
		}

		/* new node ? */
		if (!hnode_ptr->next) {
			if ((hnode_ptr->next = calloc(1, sizeof(HNODE))) == NULL) {
				snprintf(error, sizeof(error), "memory allocation error");
				goto error;
			}
		}

		hnode_ptr = hnode_ptr->next;
	} while (hnode_ptr);

	return hnode_ptr;

error:
	free(hnode_ptr->key);

	return hnode_ptr;
}

void *hl_hash_get(HCODE *hcode, char *key) {
	HNODE *hnode_ptr = hcode->nodes;
	uint32_t index = 0;

	index = _hl_hash_get_key(hcode, key);

	hnode_ptr = (HNODE *) (hnode_ptr + index);
	do {
		if (hnode_ptr->key)
			if (strcmp(key, hnode_ptr->key) == 0)
				return hnode_ptr->value;

		hnode_ptr = hnode_ptr->next;
	} while (hnode_ptr);

	return NULL;
}

LIST *hl_hash_keys(HCODE *hcode) {
	HNODE *hnode_ptr = hcode->nodes;
	LIST *list;
	uint32_t i;

	if ((list = hl_list_alloc()) == NULL)
		return NULL;

	for (i = 0; i != hcode->hsize; i++) {
		hnode_ptr = (HNODE *) (hcode->nodes + i);

		do {
			if (hnode_ptr->key) {
				hl_list_push(list, hnode_ptr->key, strlen(hnode_ptr->key) + 1);
			}
			hnode_ptr = hnode_ptr->next;
		} while (hnode_ptr);
	}

	return list;
}

LIST *hl_hash_values(HCODE *hcode) {
	HNODE *hnode_ptr = hcode->nodes;
	LIST *list;
	uint32_t i;

	if ((list = hl_list_alloc()) == NULL)
		return NULL;

	for (i = 0; i != hcode->hsize; i++) {
		hnode_ptr = (HNODE *) (hcode->nodes + i);

		do {
			if (hnode_ptr->key)
				hl_list_push(list, hnode_ptr->value, sizeof(void *));
			hnode_ptr = hnode_ptr->next;
		} while (hnode_ptr);
	}

	return list;
}

void hl_hash_reset(HCODE *hcode) {
	HNODE *hnodes_ptr;
	HNODE *hnode_ptr;
	HNODE *next_ptr;
	uint32_t size = hcode->hsize;
	uint32_t i;

	hnodes_ptr = hcode->nodes;

	for (i = 0; i != size; i++) {
		hnode_ptr = ((HNODE *) hnodes_ptr) + i;

		if (!hnode_ptr->key)
			continue;

		mm_free0((void*) &hnode_ptr->key);
		free(hnode_ptr->value);

		hnode_ptr = hnode_ptr->next;
		while (hnode_ptr) {
			mm_free0((void*) &hnode_ptr->key);
			free(hnode_ptr->value);

			next_ptr = hnode_ptr->next;
			mm_free0((void*) &hnode_ptr);

			hnode_ptr = next_ptr;
		}
	}
}

void hl_hash_init_iterator(HCODE *hcode, HCODE_ITERATOR *iterator) {
	iterator->hcode = hcode;
	iterator->index = 0;
	iterator->current = NULL;
}

inline HNODE *hl_hash_iterate(HCODE_ITERATOR *iterator) {
	HNODE *hnodes_ptr;
	HNODE *hnode_ptr;
	uint32_t size;

	size = iterator->hcode->hsize;
	hnodes_ptr = iterator->hcode->nodes;
	while (iterator->index != size) {
		if (iterator->current) {
			hnode_ptr = iterator->current;
			iterator->current = NULL;
		} else
			hnode_ptr = (HNODE *) (hnodes_ptr + iterator->index++);

		if (hnode_ptr->key) {
			iterator->current = hnode_ptr->next;
			return hnode_ptr;
		}
	}

	return NULL;
}

int hl_hash_del(char *key) {
	return 0;
}

void hl_hash_free_node(HNODE * hnode) {
	mm_free0((void*) &hnode->key);
	free(hnode->value);
}

void hl_hash_free(HCODE * hcode) {
	hl_hash_reset(hcode);
	free(hcode->nodes);
	free(hcode);
}
