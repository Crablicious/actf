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

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define __maybe_unused __attribute__((unused))

#define UNUSED(a) (void)(a)
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ARRLEN(a) (sizeof(a)/sizeof(a)[0])
#define CLEAR(t) (memset(&(t), 0, sizeof(t)))

#if defined __has_builtin
#  if __has_builtin(__builtin_expect)
#    define likely(x)   __builtin_expect(x, 1)
#    define unlikely(x) __builtin_expect(x, 0)
#  endif
#endif
#ifndef likely
#  define likely(x)   (x)
#  define unlikely(x) (x)
#endif

#define CRUST_PREFIX_HELPER(x, y) x##y
#define CRUST_PREFIX(x, y) CRUST_PREFIX_HELPER(x, y)

/* C99-compliant strdup. Sets errno on failure. */
static inline char *c_strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *sdup = malloc(len);
	if (!sdup) {
		return NULL;
	}
	return memcpy(sdup, s, len);
}

/* Compiles to a bswap with clang/gcc. */
static inline uint64_t bswap64(uint64_t in)
{
	uint64_t out = 0;
	out |= (in & 0xFF00000000000000) >> 56;
	out |= (in & 0x00FF000000000000) >> 40;
	out |= (in & 0x0000FF0000000000) >> 24;
	out |= (in & 0x000000FF00000000) >> 8;
	out |= (in & 0x00000000FF000000) << 8;
	out |= (in & 0x0000000000FF0000) << 24;
	out |= (in & 0x000000000000FF00) << 40;
	out |= (in & 0x00000000000000FF) << 56;
	return out;
}

static inline uint32_t bswap32(uint32_t in)
{
	uint32_t out = 0;
	out |= (in & 0xFF000000) >> 24;
	out |= (in & 0x00FF0000) >> 8;
	out |= (in & 0x0000FF00) << 8;
	out |= (in & 0x000000FF) << 24;
	return out;
}

static inline uint16_t bswap16(uint16_t in)
{
	uint16_t out = 0;
	out |= (in & 0xFF00) >> 8;
	out |= (in & 0x00FF) << 8;
	return out;
}

static inline uint64_t sataddu64(uint64_t a, uint64_t b)
{
	uint64_t r = a + b;
	if (r < b) {
		r = UINT64_MAX;
	}
	return r;
}

#endif /* COMMON_H */
