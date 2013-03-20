/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mm.h>
#include "list.h"

LIST *hl_list_alloc() {
	LIST *list;

	if ((list = calloc(1, sizeof(LIST))) == NULL) {
		snprintf(list->errmsg, sizeof(list->errmsg), "not enough memory");
		return NULL;
	}

	return list;
}

int hl_list_push(LIST *list, void *value, uint32_t size) {
	LIST_NODE *node_ptr;

	if (!list->nodes) {
		if ((list->nodes = malloc(sizeof(LIST_NODE) + size)) == NULL) {
			snprintf(list->errmsg, sizeof(list->errmsg), "not enough memory");
			return -1;
		}
		node_ptr = list->nodes;
	} else {
		node_ptr = list->last;
		if ((node_ptr->next = malloc(sizeof(LIST_NODE) + size)) == NULL) {
			snprintf(list->errmsg, sizeof(list->errmsg), "not enough memory");
			return -1;
		}
		node_ptr = node_ptr->next;
	}
	node_ptr->next = NULL;
	memcpy((char *) node_ptr + sizeof(LIST_NODE), value, size);

	list->last = node_ptr;

	return 0;
}

void hl_list_pop(LIST *list) {
	LIST_NODE *node_ptr = list->last;

	if (!list->nodes->next) {
		free(list->nodes);
		list->last = NULL;
	} else {
		while (node_ptr->next->next)
			node_ptr = node_ptr->next;

		mm_free0((void *) &node_ptr->next);
		list->last = node_ptr;
	}
}

void *hl_list_get(LIST *list, uint32_t index) {
	LIST_NODE *node_ptr = list->nodes;
	uint32_t i = 0;

	while (node_ptr) {
		if (i++ == index)
			return ((char *) node_ptr + sizeof(LIST_NODE));
		node_ptr = node_ptr->next;
	}

	return NULL;
}

void hl_list_round_robin(LIST *list) {
	LIST_NODE *node_ptr = list->nodes;

	if (node_ptr->next) {
		list->nodes = node_ptr->next;
		list->last->next = node_ptr;
		node_ptr->next =  NULL;

		list->last = node_ptr;
	}
}

void hl_list_init_iterator(LIST *list, LIST_ITERATOR *iterator) {
	iterator->current = list->nodes;
}

inline void *hl_list_iterate(LIST_ITERATOR *iterator) {
	void *value;

	if (!iterator || !iterator->current)
		return NULL;

	value = ((char *) iterator->current + sizeof(LIST_NODE));
	iterator->current = iterator->current->next;

	return value;
}

inline uint32_t hl_list_count(LIST *list) {
	LIST_NODE *node_ptr = list->nodes;
	uint32_t i = 0;

	while (node_ptr) {
		node_ptr = node_ptr->next;
		i++;
	}

	return i;
}

inline void hl_list_reset(LIST *list) {
	LIST_NODE *next_ptr;

	while (list->nodes) {
		next_ptr = list->nodes->next;
		free(list->nodes);
		list->nodes = next_ptr;
	}
	list->last = list->nodes;
}

void hl_list_free(LIST *list) {
	hl_list_reset(list);
	free(list);
}
