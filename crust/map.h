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

/* Map for arbitrary types with properties:
   - Open addressing
   - Linear probing
   - Power of 2 sized for cheap wrap calcs
   - Delete with backwards-shift
   - Resize on 50% load factor, but if allocation fails you can keep
     inserting until 100% load

   The functions return zero on success and negative errno on error.

   It has the following prerequisite defines:
   MAP_NAME :: The name of the map
   MAP_KEY_TYPE :: The type of the key
   MAP_KEY_CMP :: Key comparison function `int (*cmp)(MAP_KEY_TYPE)`,
                  must return zero if equal.
   MAP_KEY_FREE :: Optional key free function
   MAP_VAL_TYPE :: The type of the value
   MAP_VAL_FREE :: Optional value free function
   MAP_HASH :: Hash function `size_t (*hash)(MAP_KEY_TYPE)`
*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "map_util.h"

/* Set optionals */
#ifndef MAP_KEY_FREE
#  define MAP_KEY_FREE(v)
#endif
#ifndef MAP_VAL_FREE
#  define MAP_VAL_FREE(v)
#endif

typedef struct MAP_NAME MAP_NAME;

/* Sentinel hash to flag an empty slot. The max value is used rather
 * than zero to avoid (basically) guaranteed collisions if we have
 * integers as key with the identity function as hash. */
#define MAP_NOENT SIZE_MAX

struct CRUST_PREFIX(MAP_NAME, _slot) {
    /* Cached MAP_HASH(key). The value MAP_NOENT means the slot is
     * unoccupied. */
    size_t hash;
    MAP_KEY_TYPE key;
    MAP_VAL_TYPE val;
};

struct MAP_NAME {
    struct CRUST_PREFIX(MAP_NAME, _slot) *slots;
    size_t len;
    size_t cap;
    size_t grow_limit;
};

struct CRUST_PREFIX(MAP_NAME, _it) {
    size_t i;
};

static inline size_t CRUST_PREFIX(MAP_NAME, _hash)(MAP_KEY_TYPE key)
{
    size_t hash = MAP_HASH(key);
    if (hash == MAP_NOENT) {
	hash--;
    }
    return hash;
}

static inline int CRUST_PREFIX(MAP_NAME, _init_cap)(struct MAP_NAME *m,
						    size_t cap)
{
    if (! map_is_pow2(cap)) {
	return -EINVAL;
    }
    m->slots = malloc(cap * sizeof(*m->slots));
    if (! m->slots) {
	return -errno;
    }
    m->len = 0;
    m->cap = cap;
    m->grow_limit = cap >> 1;
    for (size_t i = 0; i < m->cap; i++) {
	m->slots[i].hash = MAP_NOENT;
    }
    return 0;
}

static inline int CRUST_PREFIX(MAP_NAME, _init)(struct MAP_NAME *m)
{
    return CRUST_PREFIX(MAP_NAME, _init_cap)(m, 8);
}

static inline void CRUST_PREFIX(MAP_NAME, _free_int)(struct MAP_NAME *m)
{
    free(m->slots);
}

static inline void CRUST_PREFIX(MAP_NAME, _free)(struct MAP_NAME *m)
{
    if (! m) {
	return;
    }
    for (size_t i = 0; i < m->cap; i++) {
	if (m->slots[i].hash != MAP_NOENT) {
	    MAP_KEY_FREE(m->slots[i].key);
	    MAP_VAL_FREE(m->slots[i].val);
	}
    }
    CRUST_PREFIX(MAP_NAME, _free_int)(m);
}

static inline int CRUST_PREFIX(MAP_NAME, _insert_hash_unchk)(struct MAP_NAME *m,
							     size_t hash,
							     MAP_KEY_TYPE key,
							     MAP_VAL_TYPE val);

/* map_grow doubles the capacity of the map. */
static inline int CRUST_PREFIX(MAP_NAME, _grow)(struct MAP_NAME *m)
{
    int rc;
    size_t next_cap = m->cap << 1;
    if (next_cap < m->cap) {
	return -ENOMEM;
    }
    struct MAP_NAME next_m = {0};
    rc = CRUST_PREFIX(MAP_NAME, _init_cap)(&next_m, next_cap);
    if (rc < 0) {
	return rc;
    }

    for (size_t i = 0; i < m->cap; i++) {
	if (m->slots[i].hash != MAP_NOENT) {
	    CRUST_PREFIX(MAP_NAME, _insert_hash_unchk)(&next_m, m->slots[i].hash,
						     m->slots[i].key,
						     m->slots[i].val);
	}
    }

    CRUST_PREFIX(MAP_NAME, _free_int)(m);
    *m = next_m;
    return 0;
}

static inline MAP_VAL_TYPE *CRUST_PREFIX(MAP_NAME, _find)(const struct MAP_NAME *m,
							  MAP_KEY_TYPE key)
{
    size_t index = CRUST_PREFIX(MAP_NAME, _hash)(key) & (m->cap - 1);
    size_t initial_index = index;
    do {
	if (m->slots[index].hash == MAP_NOENT) {
	    return NULL;
	}
	if (MAP_KEY_CMP(m->slots[index].key, key) == 0) {
	    return &m->slots[index].val;
	}
	index = (index + 1) & (m->cap - 1);
    } while (initial_index != index);
    return NULL;
}

static inline bool CRUST_PREFIX(MAP_NAME, _contains)(const struct MAP_NAME *m,
						     MAP_KEY_TYPE key)
{
    return CRUST_PREFIX(MAP_NAME, _find)(m, key) != NULL;
}

static inline int CRUST_PREFIX(MAP_NAME, _insert_hash_unchk)(struct MAP_NAME *m,
							     size_t hash,
							     MAP_KEY_TYPE key,
							     MAP_VAL_TYPE val)
{
    size_t index = hash & (m->cap - 1);
    while (m->slots[index].hash != MAP_NOENT) {
	index = (index + 1) & (m->cap - 1);
    }
    m->slots[index].hash = hash;
    m->slots[index].key = key;
    m->slots[index].val = val;
    m->len++;
    if (m->len >= m->grow_limit) {
	CRUST_PREFIX(MAP_NAME, _grow)(m);
    }
    return 0;
}

static inline int CRUST_PREFIX(MAP_NAME, _insert)(struct MAP_NAME *m,
						  MAP_KEY_TYPE key,
						  MAP_VAL_TYPE val)
{
    if (m->len >= m->cap) {
	return -ENOMEM;
    }
    return CRUST_PREFIX(MAP_NAME, _insert_hash_unchk)(m, CRUST_PREFIX(MAP_NAME, _hash)(key),
						    key, val);
}

static inline int CRUST_PREFIX(MAP_NAME, _delete)(struct MAP_NAME *m,
						  MAP_KEY_TYPE key)
{
    size_t index = CRUST_PREFIX(MAP_NAME, _hash)(key) & (m->cap - 1);
    size_t initial_index = index;
    do {
	if (m->slots[index].hash == MAP_NOENT) {
	    return -ESRCH;
	}
	if (MAP_KEY_CMP(m->slots[index].key, key) == 0) {
	    /* Delete current index */
	    m->slots[index].hash = MAP_NOENT;
	    MAP_KEY_FREE(m->slots[index].key);
	    MAP_VAL_FREE(m->slots[index].val);
	    /* Back-shuffle any following values unless they are in their
	     * correct position. */
	    size_t prev_index = index;
	    index = (index + 1) & (m->cap - 1);
	    while (m->slots[index].hash != MAP_NOENT &&
		   (m->slots[index].hash & (m->cap - 1)) != index) {
		m->slots[prev_index] = m->slots[index];
		m->slots[index].hash = MAP_NOENT;
		prev_index = index;
		index = (index + 1) & (m->cap - 1);
	    }
	    return 0;
	}
	index = (index + 1) & (m->cap - 1);
    } while (initial_index != index);
    return -ESRCH;
}

/* Iterates over all key-value pairs of m. The iterator it must be
 * initialized to zero on the first call. */
static inline int CRUST_PREFIX(MAP_NAME, _foreach)(const struct MAP_NAME *m,
						   MAP_KEY_TYPE *k,
						   MAP_VAL_TYPE *v,
						   struct CRUST_PREFIX(MAP_NAME, _it) *it)
{
    for (; it->i < m->cap; it->i++) {
	if (m->slots[it->i].hash != MAP_NOENT) {
	    *k = m->slots[it->i].key;
	    *v = m->slots[it->i].val;
	    it->i++;
	    return 0;
	}
    }
    return -ESRCH;
}

#undef MAP_NAME
#undef MAP_KEY_TYPE
#undef MAP_KEY_CMP
#undef MAP_KEY_FREE
#undef MAP_VAL_TYPE
#undef MAP_VAL_FREE
#undef MAP_HASH
