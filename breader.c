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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "breader.h"
#include "crust/common.h"
#include "config.h"


#ifndef WORDS_BIGENDIAN  // Little endian
#define HTOLE64(x) (x)
#define HTOBE64(x) bswap64(x)
#else  // Big endian
#define HTOLE64(x) bswap64(x)
#define HTOBE64(x) (x)
#endif

void breader_init(void *addr, size_t len, enum actf_byte_order bo, struct breader *br)
{
    br->bo = bo;
    br->start_ptr = addr;
    br->read_ptr = addr;
    br->end_ptr = br->read_ptr + len;
    br->lookahead = 0;
    br->lookahead_bit_cnt = 0;
    br->tot_bit_cnt = 0;

    br->debug.ds_addr = addr;
    br->debug.ds_len = len;
}

void breader_set_bo(struct breader *br, enum actf_byte_order bo)
{
    if (br->bo == bo) {
	return;
    }
    br->lookahead = bswap64(br->lookahead);
    br->bo = bo;
}

uint64_t breader_peek_le(struct breader *br, size_t cnt)
{
    assert(cnt <= br->lookahead_bit_cnt);
    return br->lookahead & ((UINT64_C(1) << cnt) - 1);
}

uint64_t breader_peek_be(struct breader *br, size_t cnt)
{
    assert(cnt <= br->lookahead_bit_cnt);
    return br->lookahead >> (64 - cnt);
}

uint64_t breader_peek(struct breader *br, size_t cnt)
{
    assert(cnt <= br->lookahead_bit_cnt);
    if (br->bo == ACTF_LIL_ENDIAN) {
	return breader_peek_le(br, cnt);
    } else {
	return breader_peek_be(br, cnt);
    }
}

#define breader_consumem(br, cnt, shft)		\
    do {					\
	assert(cnt <= br->lookahead_bit_cnt);	\
	br->lookahead = br->lookahead shft cnt;	\
	br->lookahead_bit_cnt -= cnt;		\
	br->tot_bit_cnt += cnt;			\
    } while (0)

void breader_consume_le(struct breader *br, size_t cnt)
{
    breader_consumem(br, cnt, >>);
}

void breader_consume_be(struct breader *br, size_t cnt)
{
    breader_consumem(br, cnt, <<);
}

void breader_consume(struct breader *br, size_t cnt)
{
    assert(cnt <= br->lookahead_bit_cnt);
    if (br->bo == ACTF_LIL_ENDIAN) {
	breader_consume_le(br, cnt);
    } else {
	breader_consume_be(br, cnt);
    }
}

#define breader_consume_checkedm(br, cnt, BO)				\
    do {								\
	if (cnt <= br->lookahead_bit_cnt) {				\
	    breader_consume_##BO(br, cnt);				\
	    return;							\
	}								\
	size_t to_consume = cnt - br->lookahead_bit_cnt;		\
	breader_consume_##BO(br, br->lookahead_bit_cnt);		\
	/* Empty lookahead since we will be skipping bytes. It can hold a */ \
	/* spare byte which is kept when ORing in refill. */		\
	br->lookahead = 0;						\
	/* Should be 8-bit aligned now since we fetch full bytes only. */ \
	size_t bytes_to_consume = to_consume >> 3;			\
	size_t avail_bytes_to_consume = br->end_ptr - br->read_ptr;	\
	if (likely(avail_bytes_to_consume <= bytes_to_consume)) {	\
	    br->read_ptr += avail_bytes_to_consume;			\
	    br->tot_bit_cnt += avail_bytes_to_consume * 8;		\
	} else {							\
	    br->read_ptr = br->read_ptr + bytes_to_consume;		\
	    br->tot_bit_cnt += bytes_to_consume * 8;			\
									\
	    size_t bits_to_consume = to_consume & 0x7;			\
	    size_t __maybe_unused avail_bits = breader_refill_##BO(br);	\
	    /* Should be at least a byte available so safe to consume */ \
	    /* without checking. */					\
	    assert(avail_bits >= bits_to_consume);			\
	    breader_consume(br, bits_to_consume);			\
	}								\
    } while (0)

void breader_consume_checked_le(struct breader *br, size_t cnt)
{
    breader_consume_checkedm(br, cnt, le);
}

void breader_consume_checked_be(struct breader *br, size_t cnt)
{
    breader_consume_checkedm(br, cnt, be);
}

void breader_consume_checked(struct breader *br, size_t cnt)
{
    if (br->bo == ACTF_LIL_ENDIAN) {
	breader_consume_checked_le(br, cnt);
    } else {
	breader_consume_checked_be(br, cnt);
    }
}

static uint64_t read_unalign_64(uint8_t *p)
{
    uint64_t res;
    memcpy(&res, p, sizeof(res));
    return res;
}

#define breader_refillm(br, shft, htobo64)				\
    do {								\
	if (likely((br->end_ptr - br->read_ptr) >= 8)) {		\
	    /* read 64-bit */						\
	    uint64_t next = read_unalign_64(br->read_ptr);		\
	    br->lookahead |= htobo64(next) shft br->lookahead_bit_cnt;	\
	    br->read_ptr += (63 - br->lookahead_bit_cnt) >> 3;		\
	    br->lookahead_bit_cnt |= MAX_READ_BITS;			\
	} else {							\
	    /* more carefully read remainder */				\
	    uint64_t next = 0;						\
	    size_t n_bytes = br->end_ptr - br->read_ptr;		\
	    memcpy(&next, br->read_ptr, n_bytes);			\
	    br->lookahead |= htobo64(next) shft br->lookahead_bit_cnt;	\
									\
	    size_t n_used_bytes = (63 - br->lookahead_bit_cnt) >> 3;	\
	    /* Semi-illegal branch. Maybe can be smarter. */		\
	    n_used_bytes = n_bytes < n_used_bytes ? n_bytes : n_used_bytes; \
	    br->read_ptr += n_used_bytes;				\
	    br->lookahead_bit_cnt += n_used_bytes * 8;			\
	}								\
    } while (0)

size_t breader_refill_le(struct breader *br)
{
    breader_refillm(br, <<, HTOLE64);
    return br->lookahead_bit_cnt;
}

size_t breader_refill_be(struct breader *br)
{
    breader_refillm(br, >>, HTOBE64);
    return br->lookahead_bit_cnt;
}

/* It looks like better code is generated when this function is split
 * into breader_refill_le/breader_refill_be. Benchmark it. The BO
 * dispatching can still be done from within breader in
 * breader_refill.
 *
 * Could also duplicate all functions and let the user call the
 * correct set of be/le functions to push out the bo check even more
 * e.g. consume_le/consume_be, peek_le/peek_be, refill_le, refill_be.
 */
size_t breader_refill(struct breader *br)
{
    if (br->bo == ACTF_LIL_ENDIAN) {
	return breader_refill_le(br);
    } else {
	return breader_refill_be(br);
    }
}

bool breader_byte_aligned(struct breader *br)
{
    return br->lookahead_bit_cnt % 8 == 0;
}

#define breader_alignm(br, align, BO)					\
    do {								\
	uint64_t new_tot_bit_cnt = ((br->tot_bit_cnt + align - 1) & -align); \
	assert(new_tot_bit_cnt >= br->tot_bit_cnt);			\
	size_t to_be_consumed = new_tot_bit_cnt - br->tot_bit_cnt;	\
	breader_consume_checked_##BO(br, to_be_consumed);		\
    } while (0)

void breader_align_le(struct breader *br, uint64_t align)
{
    breader_alignm(br, align, le);
}

void breader_align_be(struct breader *br, uint64_t align)
{
    breader_alignm(br, align, be);
}

void breader_align(struct breader *br, uint64_t align)
{
    if (br->bo == ACTF_LIL_ENDIAN) {
	breader_align_le(br, align);
    } else {
	breader_align_be(br, align);
    }
}

size_t breader_bits_remaining(struct breader *br)
{
    return (br->end_ptr - br->read_ptr) * 8 + br->lookahead_bit_cnt;
}

bool breader_has_bits_remaining(struct breader *br)
{
    return br->lookahead_bit_cnt || br->read_ptr < br->end_ptr;
}

size_t breader_bytes_remaining(struct breader *br)
{
    return (br->end_ptr - br->read_ptr) + (br->lookahead_bit_cnt >> 3);
}

uint8_t *breader_peek_bytes(struct breader *br)
{
    return br->read_ptr - (br->lookahead_bit_cnt >> 3);
}

uint64_t breader_read_bits(struct breader *br, size_t cnt)
{
    assert(cnt <= MAX_READ_BITS);
    if (br->lookahead_bit_cnt < cnt) {
	size_t len = breader_refill(br);
	if (len < cnt) {
	    return UINT64_MAX;
	}
    }
    uint64_t res = breader_peek(br, cnt);
    breader_consume(br, cnt);
    return res;
}

uint8_t breader_read_bit(struct breader *br)
{
    return breader_read_bits(br, 1);
}

uint8_t *breader_read_bytes(struct breader *br, size_t cnt)
{
    assert(breader_byte_aligned(br));
    size_t avail_bytes = breader_bytes_remaining(br);
    if (avail_bytes < cnt) {
	return NULL;
    }
    uint8_t *ptr = br->read_ptr - (br->lookahead_bit_cnt >> 3);
    breader_consume_checked(br, cnt * 8);
    return ptr;
}

void breader_seek(struct breader *br, size_t off, enum breader_seek_op whence)
{
    switch (whence) {
    case BREADER_SEEK_SET:
	if (off < (size_t)(br->end_ptr - br->start_ptr)) {
	    br->read_ptr = br->start_ptr + off;
	} else {
	    br->read_ptr = br->end_ptr;
	}
	break;
    case BREADER_SEEK_CUR:
	if (off < (size_t)(br->end_ptr - br->read_ptr)) {
	    br->read_ptr += off;
	} else {
	    br->read_ptr = br->end_ptr;
	}
	break;
    case BREADER_SEEK_END:
	br->read_ptr = br->end_ptr;
	break;
    default:
	return;
    }
    br->tot_bit_cnt = (br->read_ptr - br->start_ptr) * 8;
    br->lookahead = 0;
    br->lookahead_bit_cnt = 0;
}

void breader_free(struct breader *br)
{
    if (! br) {
	return;
    }
}
