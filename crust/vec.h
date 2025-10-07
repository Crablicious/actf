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

/* "Generic" typed vector in C using macros.

   The functions return zero on success and negative errno on error.

   http://arnold.uthar.net/index.php?n=Work.TemplatesC

   Create a vector for a new type with the following defines and
   include:

   #define TYPE int
   #define TYPED_NAME(x) int_##x
   #include "vec.h"
*/

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct TYPED_NAME (vec) TYPED_NAME(vec);

// could do container_of shenanigans to have private data.
struct TYPED_NAME (vec) {
	TYPE *data;
	size_t len;

	size_t cap;
};

/* Initializes a vector able to hold `cap` amount of elements. A
 * zeroed vec struct is equal to a zero-capacity vector.
 */
static inline struct TYPED_NAME (vec) TYPED_NAME(vec_init) (size_t cap) {
	struct TYPED_NAME (vec) v;
	v.data = malloc(cap * sizeof(TYPE));
	v.len = 0;
	v.cap = cap;
	return v;
}

static inline TYPE *TYPED_NAME(vec_idx) (const struct TYPED_NAME(vec) *v, size_t i) {
	return &v->data[i];
}

static inline int TYPED_NAME(vec_resize) (struct TYPED_NAME(vec) *v) {
	const size_t INITIAL_VEC_SIZE = 4;
	size_t new_cap = v->cap ? v->cap * 2 : INITIAL_VEC_SIZE;
	TYPE *new_data = realloc(v->data, sizeof(TYPE) * new_cap);
	if (!new_data) {
		return -ENOMEM;
	}
	v->cap = new_cap;
	v->data = new_data;
	return 0;
}

static inline int TYPED_NAME(vec_push) (struct TYPED_NAME(vec) *v, TYPE val) {
	if (v->len >= v->cap) {
		int rc = TYPED_NAME(vec_resize) (v);
		if (rc < 0) {
			return rc;
		}
	}
	v->data[v->len++] = val;
	return 0;
}

// | x | x | x | i | a | b | c |
//   0           3
// len = 7
// pop_idx(v, 3) should yield:
// | x | x | x | a | b | c |
static inline TYPE TYPED_NAME(vec_pop_idx) (struct TYPED_NAME(vec) *v, size_t i) {
	TYPE val = *TYPED_NAME(vec_idx) (v, i);
	memmove(v->data + i, v->data + (i + 1), (v->len - i - 1) * sizeof(TYPE));
	v->len--;
	return val;
}

static inline TYPE TYPED_NAME(vec_pop) (struct TYPED_NAME(vec) *v) {
	return TYPED_NAME(vec_pop_idx)(v, v->len - 1);
}

static inline void TYPED_NAME(vec_clear) (struct TYPED_NAME(vec) *v) {
	v->len = 0;
}

static inline bool TYPED_NAME(vec_is_empty) (struct TYPED_NAME(vec) *v) {
	return v->len == 0;
}

static inline void TYPED_NAME(vec_free) (struct TYPED_NAME(vec) *v) {
	if (!v) {
		return;
	}
	free(v->data);
}

#undef TYPE
#undef TYPED_NAME
