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

#ifndef MAP_UTIL_H
#define MAP_UTIL_H


/* The fastest hash is no hash */
static inline size_t hash_uint64(uint64_t val)
{
    return (size_t)val;
}

/* MurmurHash3 */
static inline size_t hash_murmur64(uint64_t val)
{
    val ^= val >> 33;
    val *= UINT64_C(0xff51afd7ed558ccd);
    val ^= val >> 33;
    val *= UINT64_C(0xc4ceb9fe1a85ec53);
    val ^= val >> 33;
    return (size_t)val;
}

/* fnv-1a for null-terminated strings */
static inline size_t hash_fnv(const char *s)
{
    size_t prime, offset_basis;
    if (sizeof(prime) == 4) {
	prime = UINT32_C(16777619);
	offset_basis = UINT32_C(2166136261);
    } else { // assume 64-bit
	prime = UINT64_C(1099511628211);
	offset_basis = UINT64_C(14695981039346656037);
    }
    size_t hash = offset_basis;
    while (*s) {
	hash ^= *s;
	hash *= prime;
	s++;
    }
    return hash;
}

static inline int uint64cmp(uint64_t v1, uint64_t v2)
{
    return v2 - v1;
}

/* Based on public domain snippet (2024-09-01):
   https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2 */
static inline uint32_t map_next_pow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    v += (v == 0);
    return v;
}

static inline bool map_is_pow2(size_t v)
{
    return v ? (v & (v - 1)) == 0 : false;
}

#endif /* MAP_UTIL_H */
