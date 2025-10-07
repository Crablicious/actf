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

#ifndef GVEC_H
#define GVEC_H

#include <stdlib.h>

/* Generic non-typed vector, similar to GArray. */


// could do container_of shenanigans to have private data.
struct gvec {
	void *data;
	size_t len;

	size_t cap;
	size_t ele_size;
};

// vec, type, index
#define gvec_idx(v, t, i)			\
	((t *)(v.data))[i]

// vec, value
#define gvec_push(v, val)					\
	do {							\
		if (v.len >= v.cap) {                           \
			gvec_resize(&v);			\
		}                                               \
		((__typeof__(val) *) v.data)[v.len++] = val;	\
	} while (0);

// | x | x | x | i | a | b | c |
//   0           3
// len = 7
// pop_idx(v, 3) should yield:
// | x | x | x | a | b | c |
#define gvec_pop_idx(v, i)						\
	do {								\
		size_t pop_i = i;					\
		memmove(((char *) v.data) + i * v.ele_size, ((char *) v.data) + (i + 1) * v.ele_size, (v.len - pop_i - 1) * v.ele_size); \
		v.len--;						\
	} while (0)

#define gvec_pop(v)							\
	do {								\
		gvec_pop_idx(v, v.len - 1);				\
	} while (0)

struct gvec gvec_init(size_t cap, size_t ele_size)
{
	struct gvec v;
	v.data = malloc(cap * ele_size);
	v.len = 0;
	v.cap = cap;
	v.ele_size = ele_size;
	return v;
}

void gvec_resize(struct gvec *v)
{
	v->cap *= 2;
	v->data = realloc(v->data, v->ele_size * v->cap);
}

void gvec_free(struct gvec v)
{
	free(v.data);
}

#endif /* GVEC_H */
