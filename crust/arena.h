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

#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stdlib.h>

#include "common.h"


struct region;

struct arena {
	struct region *head;
	struct region *tail;
	struct region *free;
	size_t default_cap;
};

#define REGION_DEFAULT_SZ 8192
#define ARENA_EMPTY (struct arena) {.head = NULL, .tail = NULL, .default_cap = REGION_DEFAULT_SZ}

struct region {
	struct {
		struct region *next;
		// We keep the capacity here so that we're able to
		// allocate larger than default regions if
		// necessary.
		size_t cap;
		size_t len;
	} hdr;
	uint8_t reg[];
};

static inline struct region *region_init(size_t sz)
{
	struct region *reg = malloc(sizeof(*reg) + sz);
	if (!reg) {
		return NULL;
	}
	reg->hdr.next = NULL;
	reg->hdr.cap = sz;
	reg->hdr.len = 0;
	return reg;
}

static inline void region_free(struct region *reg)
{
	free(reg);
}

static inline uint8_t *region_alloc_unchk(struct region *reg, size_t sz)
{
	uint8_t *chunk = reg->reg + reg->hdr.len;
	reg->hdr.len += sz;
	return chunk;
}

static inline uint8_t *region_alloc(struct region *reg, size_t sz, size_t align)
{
	size_t padded_len = (reg->hdr.len + align - 1) & ~(align - 1);
	if (padded_len + sz > reg->hdr.cap) {
		return NULL;
	}
	reg->hdr.len = padded_len + sz;
	return reg->reg + padded_len;
}

static inline bool region_fits(struct region *reg, size_t sz)
{
	return reg->hdr.len + sz <= reg->hdr.cap;
}

/* pops the next region from the free list that fits sz */
static inline struct region *arena_pop_free_reg(struct arena *a, size_t sz)
{
	struct region **parp = &a->free;
	struct region *cur = a->free;
	while (cur) {
		if (region_fits(cur, sz)) {
			*parp = cur->hdr.next;
			cur->hdr.next = NULL;
			return cur;
		}
		parp = &cur->hdr.next;
		cur = cur->hdr.next;
	}
	return NULL;
}

#define arena_allocn(arena, t, n) arena_alloc_align(arena, (n) * sizeof(t), _Alignof(t))
#define arena_alloc1(arena, t) arena_alloc_align(arena, sizeof(t), _Alignof(t))
#define arena_alloc(arena, sz) arena_alloc_align(arena, sz, _Alignof(max_align_t))

static inline void *arena_alloc_align(struct arena *a, size_t sz, size_t align)
{
	/* Alloc from the existing tail if possible */
	uint8_t *p = NULL;
	if (a->tail && (p = region_alloc(a->tail, sz, align))) {
		return p;
	}

	/* Alloc from free list if possible */
	struct region *fits;
	if (!(fits = arena_pop_free_reg(a, sz))) {
		/* Allocate a new region that fits sz. */
		if (!(fits = region_init(MAX(sz, a->default_cap)))) {
			return NULL;
		}
	}

	if (a->tail) {
		a->tail->hdr.next = fits;
	} else {
		a->head = fits;
	}
	a->tail = fits;
	return region_alloc_unchk(fits, sz);
}

static inline void arena_clear(struct arena *a)
{
	if (!a->head) {
		return;
	}
	struct region *par = NULL;
	struct region *cur = a->head;
	while (cur) {
		cur->hdr.len = 0;
		par = cur;
		cur = cur->hdr.next;
	}
	par->hdr.next = a->free;
	a->free = a->head;
	a->head = NULL;
	a->tail = NULL;
}

static inline void region_list_free(struct region *list)
{
	struct region *cur = list;
	while (cur) {
		struct region *next = cur->hdr.next;
		region_free(cur);
		cur = next;
	}
}

static inline void arena_free(struct arena *a)
{
	if (!a) {
		return;
	}
	region_list_free(a->head);
	region_list_free(a->free);
}

#endif /* ARENA_H */
