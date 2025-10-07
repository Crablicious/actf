/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2025  Adam Wendelin <adwe live se>
 *
 * ACTF is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ACTF is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ACTF. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef SLIST_H
#define SLIST_H

#include <stddef.h>
#include <stdbool.h>

/* Simple single linked list. Only functions which actually are decent
 * for it is implemented. If you need more, implement a better data
 * structure. */

struct slist_node {
	struct slist_node *next;
};

struct slist {
	struct slist_node *head;
	size_t len;
};

static inline void slist_push(struct slist *l, struct slist_node *n)
{
	n->next = l->head;
	l->head = n;
	l->len++;
}

static inline struct slist_node *slist_pop(struct slist *l)
{
	if (!l->head) {
		return NULL;
	}
	struct slist_node *head = l->head;
	l->head = head->next;
	head->next = NULL;
	l->len--;
	return head;
}

static inline struct slist_node *slist_peek(struct slist *l)
{
	return l->head;
}

static inline bool slist_isempty(struct slist *l)
{
	return l->head == NULL;
}

static inline size_t slist_len(struct slist *l)
{
	return l->len;
}

/* slist_foreach loops over all entries of l. The current node is
 * assigned to the `struct slist_node *` in n. */
#define SLIST_FOREACH(l, n) for ((n) = (l)->head; (n); (n) = (n)->next)


#endif /* SLIST_H */
