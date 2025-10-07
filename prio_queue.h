/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2024  Adam Wendelin <adwe live se>
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

#ifndef PRIO_QUEUE_H
#define PRIO_QUEUE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"


struct node {
	int64_t key;
	size_t value;
};

/* Priority queue implemented with a binary heap. */
struct prio_queue {
	// children at indices 2i+1 and 2i+2
	// parent at index (i-1)/2
	struct node *nodes;
	size_t len;
	size_t cap;
};


static inline int prio_queue_init(struct prio_queue *pq, size_t sz)
{
	struct node *nodes = malloc(sz * sizeof(*nodes));
	if (!nodes) {
		return ACTF_OOM;
	}
	*pq = (struct prio_queue) {
		.nodes = nodes,
		.len = 0,
		.cap = sz,
	};
	return ACTF_OK;
}

static inline void prio_queue_clear(struct prio_queue *pq)
{
	pq->len = 0;
}

static inline void prio_queue_bottomup_fix_heap(struct prio_queue *pq, size_t i)
{
	size_t cur_i = i;
	while (cur_i) {
		size_t par_i = (cur_i - 1) / 2;
		if (pq->nodes[par_i].key <= pq->nodes[cur_i].key) {
			break;
		}
		/* the parent is smaller than cur, swap and go up. */
		struct node tmp = pq->nodes[cur_i];
		pq->nodes[cur_i] = pq->nodes[par_i];
		pq->nodes[par_i] = tmp;
		cur_i = par_i;
	}
}

/* The user must make sure to not push more elements than they
 * initialized the queue with (pq->len < pq->cap). */
static inline void prio_queue_push_unchecked(struct prio_queue *pq, struct node n)
{
	assert(pq->len < pq->cap);
	pq->nodes[pq->len] = n;
	pq->len++;
	prio_queue_bottomup_fix_heap(pq, pq->len - 1);
}

static inline void prio_queue_topdown_fix_heap(struct prio_queue *pq, size_t i)
{
	size_t cur_i = i;
	while (true) {
		size_t left_i = 2 * cur_i + 1;
		size_t right_i = 2 * cur_i + 2;
		size_t smallest_i = cur_i;

		if (left_i < pq->len && pq->nodes[left_i].key < pq->nodes[smallest_i].key) {
			smallest_i = left_i;
		}
		if (right_i < pq->len && pq->nodes[right_i].key < pq->nodes[smallest_i].key) {
			smallest_i = right_i;
		}
		if (smallest_i == cur_i) {
			break;
		}
		// the child is smaller than cur, swap and go down that
		// branch.
		struct node tmp = pq->nodes[cur_i];
		pq->nodes[cur_i] = pq->nodes[smallest_i];
		pq->nodes[smallest_i] = tmp;
		cur_i = smallest_i;
	}
}

/* The user must make sure that pq has any elements to pop (pq->len >
 * 0). */
static inline struct node prio_queue_pop_unchecked(struct prio_queue *pq)
{
	assert(pq->len > 0);
	struct node min = pq->nodes[0];
	pq->nodes[0] = pq->nodes[pq->len - 1];
	pq->len--;
	prio_queue_topdown_fix_heap(pq, 0);
	return min;
}

/* The user must make sure that pq has any elements to peek (pq->len >
 * 0). */
static inline struct node prio_queue_peek_unchecked(struct prio_queue *pq)
{
	assert(pq->len > 0);
	return pq->nodes[0];
}

static inline void prio_queue_free(struct prio_queue *pq)
{
	if (!pq) {
		return;
	}
	free(pq->nodes);
}

#endif /* PRIO_QUEUE_H */
