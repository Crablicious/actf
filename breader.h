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

#ifndef BREADER_H
#define BREADER_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "types.h"

/* Yes it is a bit-reader, what else? */

static const size_t MAX_READ_BITS = 56;

struct breader {
    enum actf_byte_order bo;

    uint8_t *start_ptr;
    uint8_t *read_ptr;
    uint8_t *end_ptr;  // 1 after last byte

    uint64_t lookahead;
    size_t lookahead_bit_cnt;

    uint64_t tot_bit_cnt;  // total bit offset from zero. used
			   // publically currently.

    struct {
	uint8_t *ds_addr;
	size_t ds_len; // bytes
    } debug;
};

/*
  LIL ENDIAN lookahead:
  | * | * | * | 2 | 1 | 0 |

  BIG ENDIAN lookahead:
  | 0 | 1 | 2 | * | * | * |
*/

void breader_init(void *addr, size_t len, enum actf_byte_order bo, struct breader *br);
void breader_set_bo(struct breader *br, enum actf_byte_order bo);

/* Peeking zero bits is not allowed (causes too large shifts on big endian) */
uint64_t breader_peek(struct breader *br, size_t cnt);
uint64_t breader_peek_le(struct breader *br, size_t cnt);
uint64_t breader_peek_be(struct breader *br, size_t cnt);
/* Consumes `cnt` bits. It is a requirement that at least `cnt` bits
 * are available in `breader`.
 */
void breader_consume(struct breader *br, size_t cnt);
void breader_consume_le(struct breader *br, size_t cnt);
void breader_consume_be(struct breader *br, size_t cnt);
/* Similar to `breader_consume` but with no requirement that `cnt` is
 * available in `breader`. To be used for larger hops
 * (alignment/padding).
 */
void breader_consume_checked(struct breader *br, size_t cnt);
void breader_consume_checked_le(struct breader *br, size_t cnt);
void breader_consume_checked_be(struct breader *br, size_t cnt);
size_t breader_refill(struct breader *br);
size_t breader_refill_le(struct breader *br);
size_t breader_refill_be(struct breader *br);
bool breader_byte_aligned(struct breader *br);
void breader_align(struct breader *br, uint64_t align);
void breader_align_le(struct breader *br, uint64_t align);
void breader_align_be(struct breader *br, uint64_t align);
/* breader_bits_remaining returns the number of bits which can be
 * read. This could overflow, so prefer breader_has_bits_remaining or
 * breader_bytes_remaining. */
size_t breader_bits_remaining(struct breader *br);
/* breader_has_bits_remaining returns if any bits can be read. */
bool breader_has_bits_remaining(struct breader *br);
/* Gets a read pointer to the current byte. */
uint8_t *breader_peek_bytes(struct breader *br);
size_t breader_bytes_remaining(struct breader *br);

/***** Auto refill-consume-peek API *****/
uint8_t breader_read_bit(struct breader *br);
uint64_t breader_read_bits(struct breader *br, size_t cnt);

/* Returns a pointer to a minimum of `cnt` bytes. Requires that the
 * read pointer is byte-aligned.
 */
uint8_t *breader_read_bytes(struct breader *br, size_t cnt);

enum breader_seek_op {
    BREADER_SEEK_SET,
    BREADER_SEEK_CUR,
    BREADER_SEEK_END,
};
/* breader_seek seeks off bytes whence. similar to lseek. */
void breader_seek(struct breader *br, size_t off, enum breader_seek_op whence);

void breader_free(struct breader *br);

#endif /* BREADER_H */
