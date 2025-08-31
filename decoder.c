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
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "breader.h"
#include "event_int.h"
#include "decoder.h"
#include "crust/common.h"
#include "crust/arena.h"
#include "event_generator.h"
#include "fld.h"
#include "fld_cls_int.h"
#include "metadata_int.h"
#include "pkt_int.h"
#include "error.h"
#include "pkt_state.h"


enum dec_ctx {
    DEC_CTX_PKT_HEADER,
    DEC_CTX_PKT_CTX,
    DEC_CTX_EVENT_HEADER,
    DEC_CTX_EVENT_COMMON_CTX,
    DEC_CTX_EVENT_SPECIFIC_CTX,
    DEC_CTX_EVENT_PAYLOAD,
};

struct dec_state {
    struct arena pkt_arena;
    struct arena ev_arena;
    struct pkt_state pkt_s;
    struct actf_pkt pkt;
    struct actf_event *ev;
    enum dec_ctx ctx;
};

enum decoding_state {
    DECODING_STATE_OK = 0,
    DECODING_STATE_RESUME_PKT = 1 << 0,
    DECODING_STATE_RESUME_SEEK = 1 << 1,
    DECODING_STATE_ERROR = 1 << 2,
};

struct actf_decoder {
    // the state is multi-faceted to allow for a successful seek to
    // exist alongside a dormant error. MULTI-LEVEL ERROR DRIFTING.
    enum decoding_state state;
    // stored error code if state & DECODING_STATE_ERROR since we
    // want to return all valid events before returning an error.
    int err_rc;
    // the idx of evs to return if state & DECODING_STATE_RESUME_SEEK
    // it is the first event matching the seek query.
    size_t seek_evs_off;
    size_t seek_evs_len;
    const struct actf_metadata *metadata;
    struct breader br;
    struct actf_event **evs;
    size_t evs_cap;
    struct dec_state dec_s;
    struct error err;
};


static int fld_cls_decode(struct actf_decoder *dec, const struct actf_fld_cls *cls,
			  struct actf_fld *val);


static struct actf_fld *fld_loc_locate(const struct actf_fld_loc loc, struct actf_decoder *dec,
				       struct actf_fld *cur);


static struct actf_fld *get_struct_member(struct actf_fld *val, const char *name)
{
    assert(val->type == ACTF_FLD_TYPE_STRUCT);
    for (size_t i = 0; i < val->cls->cls.struct_.n_members &&
	     val->cls->cls.struct_.member_clses[i].name; i++) {
	if (strcmp(name, val->cls->cls.struct_.member_clses[i].name) == 0) {
	    return &val->d.struct_.vals[i];
	}
    }
    return NULL;
}

static int get_arr_cls_len(struct actf_fld *val, struct actf_decoder *dec, size_t *len)
{
    struct error *e = &dec->err;
    switch (val->cls->type) {
    case ACTF_FLD_CLS_STATIC_LEN_ARR:
	*len = val->cls->cls.static_len_arr.len;
	return ACTF_OK;
    case ACTF_FLD_CLS_DYN_LEN_ARR: {
	struct actf_fld *len_val = fld_loc_locate(val->cls->cls.dyn_len_arr.len_fld_loc,
						  dec, val);
	if (! len_val) {
	    eprependf(e, "no dynamic-length-array length");
	    return ACTF_MISSING_FLD_LOC;
	}
	if (len_val->type != ACTF_FLD_TYPE_UINT) {
	    eprintf(e, "dynamic-length-array field has a length indicator that is not an unsigned integer");
	    return ACTF_WRONG_FLD_TYPE;
	}
	*len = len_val->d.uint.val;
	return ACTF_OK;
    }
    default:
	eprintf(e, "trying to retrieve length of a non-array field class");
	return ACTF_INTERNAL;
    }
}

/* Not finding a field is considered an error. */
static struct actf_fld *fld_loc_locate(const struct actf_fld_loc loc, struct actf_decoder *dec,
				       struct actf_fld *cur)
{
    struct actf_pkt *pkt = &dec->dec_s.pkt;
    struct actf_event *ev = dec->dec_s.ev;
    struct error *e = &dec->err;

    // 6.3.2 Field location procedure
    struct actf_fld *fld;
    switch (loc.origin) {
    case ACTF_FLD_LOC_ORIGIN_NONE:
	fld = cur->parent;
	break;
    case ACTF_FLD_LOC_ORIGIN_PKT_HEADER:
	fld = &pkt->props[ACTF_PKT_PROP_HEADER];
	break;
    case ACTF_FLD_LOC_ORIGIN_PKT_CTX:
	fld = &pkt->props[ACTF_PKT_PROP_CTX];
	break;
    case ACTF_FLD_LOC_ORIGIN_EVENT_HEADER:
	fld = ev ? &ev->props[ACTF_EVENT_PROP_HEADER] : NULL;
	break;
    case ACTF_FLD_LOC_ORIGIN_EVENT_COMMON_CTX:
	fld = ev ? &ev->props[ACTF_EVENT_PROP_COMMON_CTX] : NULL;
	break;
    case ACTF_FLD_LOC_ORIGIN_EVENT_SPECIFIC_CTX:
	fld = ev ? &ev->props[ACTF_EVENT_PROP_SPECIFIC_CTX] : NULL;
	break;
    case ACTF_FLD_LOC_ORIGIN_EVENT_PAYLOAD:
	fld = ev ? &ev->props[ACTF_EVENT_PROP_PAYLOAD] : NULL;
	break;
    default:
	fld = NULL;
	break;
    }
    if (! fld || fld->type != ACTF_FLD_TYPE_STRUCT) {
	eprintf(e, "unable to locate field with origin: %s",
		actf_fld_loc_origin_name(loc.origin) ? actf_fld_loc_origin_name(loc.origin) :
		"relative");
	return NULL;
    }
    for (size_t i = 0; i < loc.path_len; i++) {
	const char *next_path = loc.path[i];
	if (next_path) {
	    fld = get_struct_member(fld, next_path);
	    if (! fld) {
		eprintf(e, "field location struct has no member named %s", next_path);
		return NULL;
	    }
	    if (fld->type == ACTF_FLD_TYPE_NIL) {
		eprintf(e, "field location points to a field which is not yet decoded");
		return NULL;
	    }
	} else {
	    if (! fld->parent) {
		eprintf(e, "field location points to a field's containing struct, but the field has no encompassing struct");
		return NULL;
	    }
	    fld = fld->parent;
	}

	switch (fld->type) {
	case ACTF_FLD_TYPE_BOOL:
	case ACTF_FLD_TYPE_SINT:
	case ACTF_FLD_TYPE_UINT:
	    if (i != (loc.path_len - 1)) {
		eprintf(e, "field location points to an integer-based field-value"
			" but there are remaining elements in the field location path");
		return NULL;
	    }
	    return fld;
	case ACTF_FLD_TYPE_STRUCT:
	    break;
	case ACTF_FLD_TYPE_ARR: {
	    /* While V is an array:
	       - If V isn’t currently being decoded, then report an error and abort the data stream decoding process.
	       - Set V to the element of V currently being decoded.
	    */
	    do {
		size_t len;
		if (get_arr_cls_len(fld, dec, &len) < 0) {
		    return NULL;
		}
		if (fld->d.arr.n_vals == len) {
		    eprintf(e, "trying to lookup a field location in an already decoded array");
		    return NULL;
		}
		fld = &fld->d.arr.vals[fld->d.arr.n_vals];
	    } while (fld->type == ACTF_FLD_TYPE_ARR);
	    break;
	}
	default:
	    eprintf(e, "field location points to a non-supported field-class");
	    return NULL;
	}
    }
    eprintf(e, "unable to find field location");
    return NULL;
}

/* Returns the number of content bits left in the provided packet. */
static uint64_t pkt_bits_remaining(struct pkt_state *pkt_s, struct breader *br)
{
    if ((br->tot_bit_cnt - pkt_s->bit_off) < pkt_s->content_len) {
	return pkt_s->content_len - (br->tot_bit_cnt - pkt_s->bit_off);
    } else {
	return 0;
    }
}

/* CTF2-SPEC-2.0 says:
 * "If X ≥ PKT_CONTENT_LEN, then report an error and abort the data stream decoding process."
 * X is "Current decoding offset/position (bits) from the beginning of P[acket]."
 * PKT_CONTENT_LEN is "Current content length (bits) of P[acket]."
 *
 * In code below, X is represented by `br->tot_bit_cnt - pkt_s->bit_off`
 * and PKT_CONTENT_LEN is represented by `pkt_s->content_len`.
 *
 * Aligning to the absolute end of the packet's content length
 * should be permitted since we have not read out-of-bounds with
 * regards to the content length, so X >= PKT_CONTENT_LEN in the
 * specification should probably be X > PKT_CONTENT_LEN. A
 * field-class can have zero size so aligning to the end of the
 * content can still yield a successful decoding.
 */
#define do_alignm(br, align, pkt_s, e, bo)				\
    do {								\
	breader_align_##bo(br, align);					\
	if (unlikely((br->tot_bit_cnt - pkt_s->bit_off) > pkt_s->content_len)) { \
	    eprintf(e, "trying to read more bits than content length of packet"); \
	    return ACTF_NOT_ENOUGH_BITS;				\
	}								\
	return ACTF_OK;							\
    } while (0)

static int do_align_le(struct breader *br, size_t align, struct pkt_state *pkt_s,
		       struct error *e)
{
    do_alignm(br, align, pkt_s, e, le);
}

static int do_align_be(struct breader *br, size_t align, struct pkt_state *pkt_s,
		       struct error *e)
{
    do_alignm(br, align, pkt_s, e, be);
}

static int do_align(struct breader *br, size_t align, struct pkt_state *pkt_s,
		    struct error *e)
{
    if (br->bo == ACTF_LIL_ENDIAN) {
	return do_align_le(br, align, pkt_s, e);
    } else {
	return do_align_be(br, align, pkt_s, e);
    }
}

/* Reverses the bits of v. Based on
 * https://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
 * Public Domain code 2024-04-07 */
static uint64_t reverse_bits(uint64_t v, size_t len)
{
    uint64_t r = v; // r will be reversed bits of v; first get LSB of v
    int s = sizeof(v) * 8 - 1; // extra shift needed at end

    for (v >>= 1; v; v >>= 1)
    {
	r <<= 1;
	r |= v & 1;
	s--;
    }
    r <<= s; // shift when v's highest bits are zero
    r >>= (sizeof(v) * 8 - len);
    return r;
}

static inline bool needs_reverse_bits_le(const struct fxd_len_bit_arr_fld_cls *cls)
{
    return cls->bito == ACTF_LAST_TO_FIRST;
}

static inline bool needs_reverse_bits_be(const struct fxd_len_bit_arr_fld_cls *cls)
{
    return cls->bito == ACTF_FIRST_TO_LAST;
}

static inline uint64_t read_upper_bits_le(struct breader *br, uint64_t result,
					  size_t remain_bits, size_t first_bits)
{
    return result | (breader_peek_le(br, remain_bits) << first_bits);
}

static inline uint64_t read_upper_bits_be(struct breader *br, uint64_t result,
					  size_t remain_bits, size_t first_bits)
{
    (void)first_bits;
    return (result << remain_bits) | breader_peek_be(br, remain_bits);
}

#define fxd_len_bit_arr_decodem(dec, cls, val, BO)			\
    do {								\
	assert(cls->len <= 64);						\
	int rc;								\
	struct pkt_state *pkt_s = &dec->dec_s.pkt_s;			\
	struct breader *br = &dec->br;					\
	struct error *e = &dec->err;					\
									\
	breader_set_bo(br, cls->bo);					\
	if (unlikely((rc = do_align_##BO(br, cls->align, pkt_s, e)) < 0)) { \
	    return rc;							\
	}								\
	if (unlikely(pkt_bits_remaining(pkt_s, br) < cls->len)) {	\
	    eprintf(e, "not enough bits to read in packet");		\
	    return ACTF_NOT_ENOUGH_BITS;				\
	}								\
	if (unlikely((pkt_s->opt_flags & PKT_LAST_BO) &&		\
		     pkt_s->last_bo != cls->bo &&			\
		     ! breader_byte_aligned(br))) {			\
	    eprintf(e, "changing byte-order in the middle of a byte");	\
	    return ACTF_MID_BYTE_ENDIAN_SWAP;				\
	}								\
	/* refill is run unconditionally here, it could be more efficient \
	 * to decode lots of small ints by only refilling if cls->len <	\
	 * br->lookahead_bit_cnt.					\
	 */								\
	size_t avail_bits = breader_refill_##BO(br);			\
	if (unlikely(! avail_bits)) {					\
	    eprintf(e, "not enough bits to read in bit stream");	\
	    return ACTF_NOT_ENOUGH_BITS;				\
	}								\
	/* The sanity check below should not be needed since an equivalent \
	 * check will be done for `remain_bits below`. Not checking for	\
	 * bits here just means that we will consume the bits before	\
	 * realizing they won't be enough.				\
	 */								\
	/* if (cls->len < MAX_READ_BITS && avail_bits < cls->len) { */	\
	/* 	eprintf(e, "Not enough bits to read"); */		\
	/* 	return ACTF_NOT_ENOUGH_BITS; */				\
	/* } */								\
									\
	/* Read up to MAX_READ_BITS */					\
	size_t first_bits = MIN(avail_bits, cls->len);			\
	uint64_t result = breader_peek_##BO(br, first_bits);		\
	breader_consume_##BO(br, first_bits);				\
									\
	size_t remain_bits = cls->len - first_bits;			\
	if (remain_bits) {						\
	    /* Read the last cls->len - MAX_READ_BITS bits */		\
	    avail_bits = breader_refill_##BO(br);			\
	    if (unlikely(avail_bits < remain_bits)) {			\
		eprintf(e, "not enough bits to read in bit stream");	\
		return ACTF_NOT_ENOUGH_BITS;				\
	    }								\
	    result = read_upper_bits_##BO(br, result, remain_bits, first_bits);	\
	    breader_consume_##BO(br, remain_bits);			\
	}								\
	/* Reverse the bits if necessary. */				\
	if (needs_reverse_bits_##BO(cls)) {				\
	    *val = reverse_bits(result, cls->len);			\
	} else {							\
	    *val = result;						\
	}								\
									\
	pkt_s->last_bo = cls->bo;					\
	pkt_s->opt_flags |= PKT_LAST_BO;				\
	return ACTF_OK;							\
    } while (0)								\

static int fxd_len_bit_arr_decode(struct actf_decoder *dec,
				  const struct fxd_len_bit_arr_fld_cls *cls,
				  uint64_t *val)
{
    if (cls->bo == ACTF_LIL_ENDIAN) {
	fxd_len_bit_arr_decodem(dec, cls, val, le);
    } else {
	fxd_len_bit_arr_decodem(dec, cls, val, be);
    }
}

static int fld_cls_fxd_len_bit_arr_decode(struct actf_decoder *dec,
					  const struct actf_fld_cls *gen_cls,
					  struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_BIT_ARR);
    int rc;
    const struct fxd_len_bit_arr_fld_cls *cls = &gen_cls->cls.fxd_len_bit_arr;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, cls, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }
    val->type = ACTF_FLD_TYPE_UINT;
    val->d.uint.val = bit_arr;
    return ACTF_OK;
}

static int fld_cls_fxd_len_bit_map_decode(struct actf_decoder *dec,
					  const struct actf_fld_cls *gen_cls,
					  struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_BIT_MAP);
    int rc;
    const struct fxd_len_bit_map_fld_cls *cls = &gen_cls->cls.fxd_len_bit_map;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, &cls->bit_arr, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }
    val->type = ACTF_FLD_TYPE_BIT_MAP;
    val->d.bit_map.val = bit_arr;
    return ACTF_OK;
}

static uint64_t sext(uint64_t val, uint64_t n_bits)
{
    uint64_t sign_bit = val >> (n_bits - 1);
    if (n_bits < 64 && sign_bit) {
	uint64_t sign_mask = ((uint64_t)1 << (64 - n_bits)) - 1;
	val |= (sign_mask << n_bits);
    }
    return val;
}

static int fld_cls_fxd_len_sint_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_SINT);
    int rc;
    const struct fxd_len_int_fld_cls *cls = &gen_cls->cls.fxd_len_int;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, &cls->bit_arr, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_SINT;
    val->d.int_.val = sext(bit_arr, cls->bit_arr.len);
    return ACTF_OK;
}

static int fld_cls_fxd_len_uint_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_UINT);
    int rc;
    const struct fxd_len_int_fld_cls *cls = &gen_cls->cls.fxd_len_int;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, &cls->bit_arr, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_UINT;
    val->d.uint.val = bit_arr;
    return ACTF_OK;
}

static int fld_cls_fxd_len_bool_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_BOOL);
    int rc;
    const struct fxd_len_bool_fld_cls *cls = &gen_cls->cls.fxd_len_bool;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, &cls->bit_arr, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_BOOL;
    val->d.bool_.val = !! bit_arr;
    return ACTF_OK;
}

static int fld_cls_fxd_len_float_decode(struct actf_decoder *dec,
					const struct actf_fld_cls *gen_cls,
					struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_FXD_LEN_FLOAT);
    int rc;
    const struct fxd_len_float_fld_cls *cls = &gen_cls->cls.fxd_len_float;

    uint64_t bit_arr;
    if ((rc = fxd_len_bit_arr_decode(dec, &cls->bit_arr, &bit_arr)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_REAL;
    switch (cls->bit_arr.len) {
    case 32: {
	union {
	    uint32_t u;
	    float f;
	} conv = {.u = bit_arr};
	val->d.real.f32 = conv.f;
	break;
    }
    case 64: {
	union {
	    uint64_t u;
	    double f;
	} conv = {.u = bit_arr};
	val->d.real.f64 = conv.f;
	break;
    }
    default:
	eprintf(&dec->err, "unsupported float of length %" PRIu64, cls->bit_arr.len);
	return ACTF_UNSUPPORTED_LENGTH;
    }
    return ACTF_OK;
}

static int fld_cls_var_len_int_decode(struct actf_decoder *dec,
				      const struct actf_fld_cls *gen_cls,
				      uint64_t *val,
				      uint64_t *n_bits)
{
    int rc;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    breader_set_bo(br, ACTF_LIL_ENDIAN);  // not req if I read bytes.
    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint64_t result = 0;
    unsigned shift = 0;
    bool fin = false;
    while (! fin) {
	size_t avail_bits = breader_refill(br);  // will be divisable by 8 due to align
	if (! avail_bits) {
	    eprintf(e, "not enough bits to decode variable length integer");
	    return ACTF_NOT_ENOUGH_BITS;
	}
	while (! fin && avail_bits) {
	    result |= (breader_peek(br, 7) << shift);
	    breader_consume(br, 7);
	    fin = ! breader_peek(br, 1);
	    breader_consume(br, 1);

	    avail_bits -= 8;
	    shift += 7;
	}
	if ((! fin && pkt_bits_remaining(pkt_s, br) < 8) ||
	    (fin && (br->tot_bit_cnt - pkt_s->bit_off) > pkt_s->content_len)) {
	    eprintf(e, "not enough bits to read in packet");
	    return ACTF_NOT_ENOUGH_BITS;
	}
    }

    *val = result;
    *n_bits = MIN(shift, 64);  // Result is always truncated to 64-bits
    return ACTF_OK;
}

static int fld_cls_var_len_uint_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_VAR_LEN_UINT);
    int rc;
    uint64_t uval, n_bits;
    if ((rc = fld_cls_var_len_int_decode(dec, gen_cls, &uval, &n_bits)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_UINT;
    val->d.uint.val = uval;
    val->d.uint.len = n_bits;
    return ACTF_OK;
}

static int fld_cls_var_len_sint_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_VAR_LEN_SINT);
    int rc;
    uint64_t uval, n_bits;
    if ((rc = fld_cls_var_len_int_decode(dec, gen_cls, &uval, &n_bits)) < 0) {
	eprependf(&dec->err, actf_fld_cls_type_name(gen_cls->type));
	return rc;
    }

    val->type = ACTF_FLD_TYPE_SINT;
    val->d.int_.val = sext(uval, n_bits);
    return ACTF_OK;
}

/* find_null_term returns a pointer to the last byte of the null
 * terminator if it exists. */
static uint8_t *find_null_term(uint8_t *ptr, size_t n_bytes, enum actf_encoding enc)
{
    uint8_t *term = NULL; // Will point to the last byte of the null terminator
    size_t codepoint_sz = actf_encoding_to_codepoint_size(enc);
    switch (codepoint_sz) {
    case 1:
	term = memchr(ptr, 0, n_bytes);
	break;
    default:
	for (size_t i = 0; i < n_bytes; i += codepoint_sz) {
	    size_t j;
	    for (j = 0; j < codepoint_sz; j++) {
		if (ptr[i+j]) {
		    break;
		}
	    }
	    if (j == codepoint_sz) {
		term = ptr + i + j - 1;
		break;
	    }
	}
	break;
    }
    return term;
}

static int fld_cls_null_term_str_decode(struct actf_decoder *dec,
					const struct actf_fld_cls *gen_cls,
					struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_NULL_TERM_STR);
    int rc;
    const struct null_term_str_fld_cls *cls = &gen_cls->cls.null_term_str;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < actf_encoding_to_codepoint_size(cls->base.enc) * 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *ptr = breader_peek_bytes(br);
    size_t n_bytes = MIN(breader_bytes_remaining(br), pkt_bits_remaining(pkt_s, br) / 8);
    uint8_t *term = find_null_term(ptr, n_bytes, cls->base.enc);
    if (! term) {
	eprintf(e, "not enough bytes to decode null terminated string");
	return ACTF_NOT_ENOUGH_BITS;
    }
    size_t str_len = term - ptr;
    str_len++;

    if (! breader_read_bytes(br, str_len)) {
	// We have already verified with bytes_remaining that it is
	// fine, but we check anyway in case brain was unlucky.
	eprintf(e, "not enough bytes but it should have been ok");
	return ACTF_INTERNAL;
    }

    val->type = ACTF_FLD_TYPE_STR;
    val->d.str.ptr = ptr;
    val->d.str.len = str_len;
    return ACTF_OK;
}

/* is_valid_str_sz checks if str_sz is a valid amount of bytes for the
 * specified encoding. For example, 5 bytes is NOT ok for an utf-32
 * encoding, but would be fine for a utf-8 encoding. */
static bool is_valid_str_sz(size_t str_sz, enum actf_encoding enc)
{
    size_t codepoint_sz = actf_encoding_to_codepoint_size(enc);
    return !(str_sz % codepoint_sz);
}

// NOTE: Could pull fld_cls_static_len_str_decode and
// fld_cls_dyn_len_str_decode together into the same function if I use
// fld_cls helpers to get encoding and len from them.
static int fld_cls_static_len_str_decode(struct actf_decoder *dec,
					 const struct actf_fld_cls *gen_cls,
					 struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_STATIC_LEN_STR);
    int rc;
    const struct static_len_str_fld_cls *cls = &gen_cls->cls.static_len_str;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < cls->len * 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *ptr = breader_read_bytes(br, cls->len);
    if (! ptr) {
	eprintf(e, "not enough bytes to decode static-length-string");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *term = find_null_term(ptr, cls->len, cls->base.enc);
    size_t str_len;
    if (term) {
	str_len = term - ptr;
	str_len++;  // +1 to include null-terminator in the len.
    } else {
	/* No requirement that the string is actually null-terminated.
	 * Let it be non-null-terminated.
	 */
	if (! is_valid_str_sz(cls->len, cls->base.enc)) {
	    eprintf(e, "invalid amount of bytes in \"%s\" string", actf_encoding_to_name(cls->base.enc));
	    return ACTF_INVALID_STR_LEN;
	}
	str_len = cls->len;
    }

    val->type = ACTF_FLD_TYPE_STR;
    val->d.str.ptr = ptr;
    val->d.str.len = str_len;
    return ACTF_OK;
}

static int fld_cls_dyn_len_str_decode(struct actf_decoder *dec,
				      const struct actf_fld_cls *gen_cls,
				      struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_DYN_LEN_STR);
    int rc;
    const struct dyn_len_str_fld_cls *cls = &gen_cls->cls.dyn_len_str;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    /* The value of the field that indicates the length */
    struct actf_fld *len_val = fld_loc_locate(cls->len_fld_loc, dec, val);
    if (! len_val) {
	eprependf(e, "no dynamic-length-string length");
	return ACTF_MISSING_FLD_LOC;
    }
    if (len_val->type != ACTF_FLD_TYPE_UINT) {
	eprintf(e, "dynamic-length-string field has a length indicator that is not an unsigned integer");
	return ACTF_WRONG_FLD_TYPE;
    }
    size_t len = len_val->d.uint.val;

    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < len * 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *ptr = breader_read_bytes(br, len);
    if (! ptr) {
	eprintf(e, "not enough bytes to decode dynamic-length-string");
	return ACTF_NOT_ENOUGH_BITS;
    }
    uint8_t *term = find_null_term(ptr, len, cls->base.enc);
    size_t str_len;
    if (term) {
	str_len = term - ptr;
	str_len++;  // +1 to include null-terminator in the len.
    } else {
	/* No requirement that the string is actually null-terminated.
	 * Let it be non-null-terminated.
	 */
	if (! is_valid_str_sz(len, cls->base.enc)) {
	    eprintf(e, "invalid amount of bytes in \"%s\" string", actf_encoding_to_name(cls->base.enc));
	    return ACTF_INVALID_STR_LEN;
	}
	str_len = len;
    }

    val->type = ACTF_FLD_TYPE_STR;
    val->d.str.ptr = ptr;
    val->d.str.len = str_len;
    return ACTF_OK;
}

static int fld_cls_static_len_blob_decode(struct actf_decoder *dec,
					  const struct actf_fld_cls *gen_cls,
					  struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_STATIC_LEN_BLOB);
    int rc;
    const struct static_len_blob_fld_cls *cls = &gen_cls->cls.static_len_blob;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < cls->len * 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *ptr = breader_read_bytes(br, cls->len);
    if (! ptr) {
	eprintf(e, "not enough bytes to decode static-length-blob");
	return ACTF_NOT_ENOUGH_BITS;
    }

    val->type = ACTF_FLD_TYPE_BLOB;
    val->d.blob.ptr = ptr;
    val->d.blob.len = cls->len;
    return ACTF_OK;
}

/* Very similar to dyn_len_str, could maybe commonalize. */
static int fld_cls_dyn_len_blob_decode(struct actf_decoder *dec,
				       const struct actf_fld_cls *gen_cls,
				       struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_DYN_LEN_BLOB);
    int rc;
    const struct dyn_len_blob_fld_cls *cls = &gen_cls->cls.dyn_len_blob;
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct breader *br = &dec->br;
    struct error *e = &dec->err;

    /* The value of the field that indicates the length */
    struct actf_fld *len_val = fld_loc_locate(cls->len_fld_loc, dec, val);
    if (! len_val) {
	eprependf(e, "no dynamic-length-blob length");
	return ACTF_MISSING_FLD_LOC;
    }
    if (len_val->type != ACTF_FLD_TYPE_UINT) {
	eprintf(e, "dynamic-length-blob field has a length indicator that is not an unsigned integer");
	return ACTF_WRONG_FLD_TYPE;
    }
    size_t len = len_val->d.uint.val;

    if ((rc = do_align(br, actf_fld_cls_get_align_req(gen_cls), pkt_s, e)) < 0) {
	return rc;
    }
    if (pkt_bits_remaining(pkt_s, br) < len * 8) {
	eprintf(e, "not enough bits to read in packet");
	return ACTF_NOT_ENOUGH_BITS;
    }

    uint8_t *ptr = breader_read_bytes(br, len);
    if (! ptr) {
	eprintf(e, "not enough bytes to decode dynamic-length-blob");
	return ACTF_NOT_ENOUGH_BITS;
    }

    val->type = ACTF_FLD_TYPE_BLOB;
    val->d.blob.ptr = ptr;
    val->d.blob.len = len;
    return ACTF_OK;
}

uint64_t calc_new_def_clk_val(const struct actf_fld_cls *cls, struct actf_fld *val, uint64_t def_clk_val)
{
    // 6.2.1. Clock value update
    uint64_t v = actf_fld_uint64(val);
    uint64_t l;
    switch (cls->type) {
    case ACTF_FLD_CLS_FXD_LEN_UINT:
	l = cls->cls.fxd_len_int.bit_arr.len;
	break;
    case ACTF_FLD_CLS_VAR_LEN_UINT:
	l = val->d.uint.len / 7 + !!(val->d.uint.len % 7);
	break;
    default:
	// Ignore updating for a non-supported type.
	return def_clk_val;
    }
    uint64_t mask = ((uint64_t) 2 << (l - 1)) - 1;
    uint64_t h = def_clk_val & (~mask);
    uint64_t cur = def_clk_val & mask;
    if (v >= cur) {
	return h + v;
    } else {
	return h + mask + 1 + v;
    }
}

/* ev_s can be NULL when decoding the packet related properties. */
static int handle_fld_roles(struct actf_decoder *dec, enum actf_role roles,
			    const struct actf_fld_cls *cls, struct actf_fld *val)
{
    struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
    struct event_state *ev_s = dec->dec_s.ev ? &dec->dec_s.ev->ev_s : NULL;
    const struct actf_preamble *preamble = &dec->metadata->preamble;
    struct error *e = &dec->err;

    switch (dec->dec_s.ctx) {
    case DEC_CTX_PKT_HEADER:
	if (roles & ACTF_ROLE_DSTREAM_CLS_ID) {
	    uint64_t dsc_id = actf_fld_uint64(val);
	    pkt_s->dsc.id = dsc_id;
	}
	if (roles & ACTF_ROLE_DSTREAM_ID) {
	    uint64_t ds_id = actf_fld_uint64(val);
	    pkt_s->ds_id = ds_id;
	    pkt_s->opt_flags |= PKT_DSTREAM_ID;
	}
	if (roles & ACTF_ROLE_PKT_MAGIC_NUM) {
	    uint64_t magic = actf_fld_uint64(val);
	    if (magic != PACKET_MAGIC_NUMBER) {
		/* TODO: Maybe add a flag or something to not exit out */
		eprintf(e, "packet magic number 0x%" PRIx64 " is incorrect, should be 0x%" PRIx64,
			magic, PACKET_MAGIC_NUMBER);
		return ACTF_MAGIC_MISMATCH;
	    }
	}
	if (roles & ACTF_ROLE_METADATA_STREAM_UUID) {
	    assert(val->type == ACTF_FLD_TYPE_BLOB);
	    if (memcmp(preamble->uuid.d, val->d.blob.ptr, ACTF_UUID_N_BYTES) != 0) {
		eprintf(e, "UUID in data stream does not match UUID in metadata");
		return ACTF_UUID_MISMATCH;
	    }
	}
	break;
    case DEC_CTX_PKT_CTX:
	if (roles & ACTF_ROLE_DEF_CLK_TSTAMP) {
	    pkt_s->def_clk_val = calc_new_def_clk_val(cls, val, pkt_s->def_clk_val);
	    pkt_s->begin_def_clk_val = pkt_s->def_clk_val;
	}
	if (roles & ACTF_ROLE_DISC_EVENT_CNT_SNAPSHOT) {
	    pkt_s->disc_er_snap = actf_fld_uint64(val);
	    pkt_s->opt_flags |= PKT_DISC_ER_SNAP;
	}
	if (roles & ACTF_ROLE_PKT_CONTENT_LEN) {
	    pkt_s->content_len = actf_fld_uint64(val);
	}
	if (roles & ACTF_ROLE_PKT_END_DEF_CLK_TSTAMP) {
	    pkt_s->end_def_clk_val = actf_fld_uint64(val);
	    pkt_s->opt_flags |= PKT_END_DEF_CLK_VAL;
	}
	if (roles & ACTF_ROLE_PKT_SEQ_NUM) {
	    pkt_s->seq_num = actf_fld_uint64(val);
	    pkt_s->opt_flags |= PKT_SEQ_NUM;
	}
	if (roles & ACTF_ROLE_PKT_TOT_LEN) {
	    pkt_s->tot_len = actf_fld_uint64(val);
	}
	break;
    case DEC_CTX_EVENT_HEADER:
	if (roles & ACTF_ROLE_EVENT_CLS_ID) {
	    ev_s->id = actf_fld_uint64(val);
	}
	if (roles & ACTF_ROLE_DEF_CLK_TSTAMP) {
	    pkt_s->def_clk_val = calc_new_def_clk_val(cls, val, pkt_s->def_clk_val);
	    ev_s->def_clk_val = pkt_s->def_clk_val;
	}
	break;
    default:
	break;
    }
    return ACTF_OK;
}

static int fld_cls_optional_decode(struct actf_decoder *dec,
				   const struct actf_fld_cls *gen_cls,
				   struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_OPTIONAL);
    int rc;
    const struct optional_fld_cls *cls = &gen_cls->cls.optional;
    struct error *e = &dec->err;

    /* The value of the field that indicates if the optional field is
     * enabled or not.
     */
    struct actf_fld *sel_val = fld_loc_locate(cls->sel_fld_loc, dec, val);
    if (! sel_val) {
	eprependf(e, "no optional selector field");
	return ACTF_MISSING_FLD_LOC;
    }

    struct actf_fld_cls *opt_fld_cls = NULL;
    if (sel_val->type == ACTF_FLD_TYPE_BOOL) {
	if (sel_val->d.bool_.val) {
	    opt_fld_cls = cls->fld_cls;
	}
    } else if (! actf_rng_set_len(&cls->sel_fld_rng_set)) {
	eprintf(e, "selector field of optional field is not a boolean, but there are no selector-field-ranges specified");
	return ACTF_NO_SELECTOR_FLD;
    } else if (sel_val->type == ACTF_FLD_TYPE_SINT) {
	if (actf_rng_set_intersect_sint(&cls->sel_fld_rng_set, sel_val->d.int_.val)) {
	    opt_fld_cls = cls->fld_cls;
	}
    } else if (sel_val->type == ACTF_FLD_TYPE_UINT) {
	if (actf_rng_set_intersect_uint(&cls->sel_fld_rng_set, sel_val->d.uint.val)) {
	    opt_fld_cls = cls->fld_cls;
	}
    } else {
	eprintf(e, "selector field of optional is not an integer field");
	return ACTF_WRONG_FLD_TYPE;
    }

    /* The optional field-class is disabled */
    if (! opt_fld_cls) {
	val->type = ACTF_FLD_TYPE_NIL;
	return ACTF_OK;
    }
    /* The optional field-class is enabled */
    if ((rc = fld_cls_decode(dec, opt_fld_cls, val)) < 0) {
	eprependf(e, "optional field-class");
	return rc;
    }
    enum actf_role roles = actf_fld_cls_roles(opt_fld_cls);
    if (roles && (rc = handle_fld_roles(dec, roles, opt_fld_cls, val)) < 0) {
	return rc;
    }
    return ACTF_OK;
}

static int fld_cls_variant_decode(struct actf_decoder *dec,
				  const struct actf_fld_cls *gen_cls,
				  struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_VARIANT);
    int rc;
    const struct variant_fld_cls *cls = &gen_cls->cls.variant;
    struct error *e = &dec->err;

    /* The value of the field that indicates what variant option to
     * select.
     */
    struct actf_fld *sel_val = fld_loc_locate(cls->sel_fld_loc, dec, val);
    if (! sel_val) {
	eprependf(e, "no variant selector field");
	return ACTF_MISSING_FLD_LOC;
    }

    struct actf_fld_cls *opt_fld_cls = NULL;
    if (sel_val->type == ACTF_FLD_TYPE_SINT) {
	for (size_t i = 0; i < cls->n_opts; i++) {
	    struct variant_fld_cls_opt *opt = &cls->opts[i];
	    if (actf_rng_set_intersect_sint(&opt->sel_fld_rng_set, sel_val->d.int_.val)) {
		opt_fld_cls = &opt->fc;
	    }
	}
    } else if (sel_val->type == ACTF_FLD_TYPE_UINT) {
	for (size_t i = 0; i < cls->n_opts; i++) {
	    struct variant_fld_cls_opt *opt = &cls->opts[i];
	    if (actf_rng_set_intersect_uint(&opt->sel_fld_rng_set, sel_val->d.uint.val)) {
		opt_fld_cls = &opt->fc;
	    }
	}
    } else {
	eprintf(e, "selector field of variant is not an integer field");
	return ACTF_WRONG_FLD_TYPE;
    }
    if (! opt_fld_cls) {
	eprintf(e, "selector field of variant does not match any option");
	return ACTF_NO_SELECTOR_FLD;
    }

    /* Decode the selected variant option field-class */
    if ((rc = fld_cls_decode(dec, opt_fld_cls, val)) < 0) {
	eprependf(e, "variant field-class");
	return rc;
    }
    enum actf_role roles = actf_fld_cls_roles(opt_fld_cls);
    if (roles && (rc = handle_fld_roles(dec, roles, opt_fld_cls, val)) < 0) {
	return rc;
    }
    return ACTF_OK;
}

static struct arena *dec_ctx_arena(struct dec_state *dec_s)
{
    switch (dec_s->ctx) {
    case DEC_CTX_PKT_HEADER:
    case DEC_CTX_PKT_CTX:
	return &dec_s->pkt_arena;
    case DEC_CTX_EVENT_HEADER:
    case DEC_CTX_EVENT_COMMON_CTX:
    case DEC_CTX_EVENT_SPECIFIC_CTX:
    case DEC_CTX_EVENT_PAYLOAD:
	return &dec_s->ev_arena;
    default:
	assert(! "trying to map unknown decoding context to arena!");
	return NULL;
    }
}

static int fld_cls_struct_decode(struct actf_decoder *dec,
				 const struct actf_fld_cls *gen_cls,
				 struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_STRUCT);
    int rc;
    const struct struct_fld_cls cls = gen_cls->cls.struct_;
    struct error *e = &dec->err;

    if ((rc = do_align(&dec->br, actf_fld_cls_get_align_req(gen_cls), &dec->dec_s.pkt_s, e)) < 0) {
	return rc;
    }

    struct arena *arena = dec_ctx_arena(&dec->dec_s);
    struct actf_fld *vals = arena_allocn(arena, struct actf_fld, cls.n_members);
    if (! vals) {
	eprintf(e, "arena_allocn: %s", strerror(errno));
	return ACTF_OOM;
    }
    memset(vals, 0, cls.n_members * sizeof(*vals));

    /* Must initialize the struct before decoding its members since
     * they can have field locations pointing to earlier members held
     * by this struct.
     */
    val->type = ACTF_FLD_TYPE_STRUCT;
    val->d.struct_.vals = vals;

    size_t i;
    for (i = 0; i < cls.n_members; i++) {
	const struct struct_fld_member_cls *member = &cls.member_clses[i];
	/* Set the current struct as the parent of the to-be decoded
	 * value.
	 */
	vals[i].parent = val;
	if ((rc = fld_cls_decode(dec, &member->cls, &vals[i])) < 0) {
	    eprependf(e, "structure member %s", member->name);
	    return rc;
	}
	enum actf_role roles = actf_fld_cls_roles(&member->cls);
	if (roles && (rc = handle_fld_roles(dec, roles, &member->cls, &vals[i])) < 0) {
	    i++;
	    return rc;
	}
    }
    return ACTF_OK;
}

static int fld_cls_arr_decode(struct actf_decoder *dec, size_t align,
			      const struct arr_fld_cls *arr_fld_cls,
			      size_t arr_len, struct actf_fld *val)
{
    int rc;
    struct error *e = &dec->err;
    if ((rc = do_align(&dec->br, align, &dec->dec_s.pkt_s, e)) < 0) {
	return rc;
    }

    struct arena *arena = dec_ctx_arena(&dec->dec_s);
    struct actf_fld *vals = arena_allocn(arena, struct actf_fld, arr_len);
    if (! vals) {
	eprintf(e, "arena_allocn: %s", strerror(errno));
	return ACTF_OOM;
    }
    memset(vals, 0, arr_len * sizeof(*vals));

    val->type = ACTF_FLD_TYPE_ARR;
    val->d.arr.vals = vals;

    size_t i;
    for (i = 0; i < arr_len; i++) {
	vals[i].parent = val->parent;
	if ((rc = fld_cls_decode(dec, arr_fld_cls->ele_fld_cls, &vals[i])) < 0) {
	    return rc;
	}
	val->d.arr.n_vals++;
    }
    return ACTF_OK;
}

static int fld_cls_static_len_arr_decode(struct actf_decoder *dec,
					 const struct actf_fld_cls *gen_cls,
					 struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_STATIC_LEN_ARR);
    const struct static_len_arr_fld_cls *cls = &gen_cls->cls.static_len_arr;
    int rc;
    struct error *e = &dec->err;
    size_t align = actf_fld_cls_get_align_req(gen_cls);
    size_t len;
    if ((rc = get_arr_cls_len(val, dec, &len)) < 0) {
	eprependf(e, "static-length-array");
	return rc;
    }
    if ((rc = fld_cls_arr_decode(dec, align, &cls->base, cls->len, val)) < 0) {
	eprependf(e, "static-length-array members");
	return rc;
    }
    return ACTF_OK;
}

static int fld_cls_dyn_len_arr_decode(struct actf_decoder *dec,
				      const struct actf_fld_cls *gen_cls,
				      struct actf_fld *val)
{
    assert(gen_cls->type == ACTF_FLD_CLS_DYN_LEN_ARR);
    const struct dyn_len_arr_fld_cls *cls = &gen_cls->cls.dyn_len_arr;
    int rc;
    struct error *e = &dec->err;
    size_t align = actf_fld_cls_get_align_req(gen_cls);
    size_t len;
    if ((rc = get_arr_cls_len(val, dec, &len)) < 0) {
	eprependf(e, "dynamic-length-array");
	return rc;
    }
    if ((rc = fld_cls_arr_decode(dec, align, &cls->base, len, val)) < 0) {
	eprependf(e, "dynamic-length-array members");
	return rc;
    }
    return ACTF_OK;
}

/* This could be in fld_cls.h but then that header becomes dependent
 * on pkt_state and breader, which seems lame especially since
 * decoding will modify pkt_state and breader.
 *
 * It would be nice since all switch-cases are confined to fld_cls.c,
 * but I don't think it's worth for now to create a dependency-mess.
 * Not sure how encoding will work either.
 */
static int fld_cls_decode(struct actf_decoder *dec, const struct actf_fld_cls *cls,
			  struct actf_fld *val)
{
    val->cls = cls;
    switch (cls->type) {
    case ACTF_FLD_CLS_NIL:
	return ACTF_INTERNAL;
    case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
	return fld_cls_fxd_len_bit_arr_decode(dec, cls, val);
    case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
	return fld_cls_fxd_len_bit_map_decode(dec, cls, val);
    case ACTF_FLD_CLS_FXD_LEN_UINT:
	return fld_cls_fxd_len_uint_decode(dec, cls, val);
    case ACTF_FLD_CLS_FXD_LEN_SINT:
	return fld_cls_fxd_len_sint_decode(dec, cls, val);
    case ACTF_FLD_CLS_FXD_LEN_BOOL:
	return fld_cls_fxd_len_bool_decode(dec, cls, val);
    case ACTF_FLD_CLS_FXD_LEN_FLOAT:
	return fld_cls_fxd_len_float_decode(dec, cls, val);
    case ACTF_FLD_CLS_VAR_LEN_UINT:
	return fld_cls_var_len_uint_decode(dec, cls, val);
    case ACTF_FLD_CLS_VAR_LEN_SINT:
	return fld_cls_var_len_sint_decode(dec, cls, val);
    case ACTF_FLD_CLS_NULL_TERM_STR:
	return fld_cls_null_term_str_decode(dec, cls, val);
    case ACTF_FLD_CLS_STATIC_LEN_STR:
	return fld_cls_static_len_str_decode(dec, cls, val);
    case ACTF_FLD_CLS_DYN_LEN_STR:
	return fld_cls_dyn_len_str_decode(dec, cls, val);
    case ACTF_FLD_CLS_STATIC_LEN_BLOB:
	return fld_cls_static_len_blob_decode(dec, cls, val);
    case ACTF_FLD_CLS_DYN_LEN_BLOB:
	return fld_cls_dyn_len_blob_decode(dec, cls, val);
    case ACTF_FLD_CLS_STRUCT:
	return fld_cls_struct_decode(dec, cls, val);
    case ACTF_FLD_CLS_STATIC_LEN_ARR:
	return fld_cls_static_len_arr_decode(dec, cls, val);
    case ACTF_FLD_CLS_DYN_LEN_ARR:
	return fld_cls_dyn_len_arr_decode(dec, cls, val);
    case ACTF_FLD_CLS_OPTIONAL:
	return fld_cls_optional_decode(dec, cls, val);
    case ACTF_FLD_CLS_VARIANT:
	return fld_cls_variant_decode(dec, cls, val);
    case ACTF_N_FLD_CLSES:
	return ACTF_INTERNAL;
    }
    return ACTF_INTERNAL;
}

static void dec_state_init(struct dec_state *dec_s, uint64_t pkt_bit_off)
{
    arena_clear(&dec_s->pkt_arena);
    arena_clear(&dec_s->ev_arena);
    pkt_state_init(&dec_s->pkt_s);
    dec_s->pkt_s.bit_off = pkt_bit_off;
    actf_pkt_init(&dec_s->pkt, &dec_s->pkt_s);
    dec_s->ev = NULL;
}

static int actf_decoder_ev_decode(struct actf_decoder *dec, struct actf_event *ev)
{
    int rc;
    struct dec_state *dec_s = &dec->dec_s;
    struct error *e = &dec->err;

    actf_event_init(ev, &dec_s->pkt);
    struct event_state *ev_s = &ev->ev_s;
    dec_s->ev = ev;

    /* Decode event-record-header-field-class */
    if (dec_s->pkt_s.dsc.cls->event_hdr.type != ACTF_FLD_CLS_NIL) {
	dec_s->ctx = DEC_CTX_EVENT_HEADER;
	if ((rc = fld_cls_decode(dec, &dec_s->pkt_s.dsc.cls->event_hdr,
				 &ev->props[ACTF_EVENT_PROP_HEADER])) < 0) {
	    eprependf(e, "event-record-header-field-class");
	    return rc;
	}
    }
    /* Set ERC based on ERC_ID */
    struct actf_event_cls **evcp = u64toevc_find(&dec_s->pkt_s.dsc.cls->idtoevc, ev_s->id);
    if (evcp) {
	ev_s->cls = *evcp;
    } else  {
	eprintf(e, "no event record class with id %" PRIu64 " in data stream %" PRIu64,
		ev_s->id, dec_s->pkt_s.dsc.id);
	return ACTF_NO_SUCH_ID;
    }
    /* Decode event-record-common-context-field-class of dsc */
    if (dec_s->pkt_s.dsc.cls->event_common_ctx.type != ACTF_FLD_CLS_NIL) {
	dec_s->ctx = DEC_CTX_EVENT_COMMON_CTX;
	if ((rc = fld_cls_decode(dec, &dec_s->pkt_s.dsc.cls->event_common_ctx,
				 &ev->props[ACTF_EVENT_PROP_COMMON_CTX])) < 0) {
	    eprependf(e, "event-record-common-context-field-class");
	    return rc;
	}
    }
    /* Decode specific-context-field-class of erc */
    if (ev_s->cls->spec_ctx.type != ACTF_FLD_CLS_NIL) {
	dec_s->ctx = DEC_CTX_EVENT_SPECIFIC_CTX;
	if ((rc = fld_cls_decode(dec, &ev_s->cls->spec_ctx,
				 &ev->props[ACTF_EVENT_PROP_SPECIFIC_CTX])) < 0) {
	    eprependf(e, "specific-context-field-class");
	    return rc;
	}
    }
    /* Decode payload-field-class of erc */
    if (ev_s->cls->payload.type != ACTF_FLD_CLS_NIL) {
	dec_s->ctx = DEC_CTX_EVENT_PAYLOAD;
	if ((rc = fld_cls_decode(dec, &ev_s->cls->payload,
				 &ev->props[ACTF_EVENT_PROP_PAYLOAD])) < 0) {
	    eprependf(e, "payload-field-class");
	    return rc;
	}
    }
    return ACTF_OK;
}

/* actf_decoder_pkt_hdrctx_decode resets the decoding state and
 * decodes a packet-header and packet-context at the current bit
 * offset. */
static int actf_decoder_pkt_hdrctx_decode(struct actf_decoder *dec)
{
    int rc;
    const struct actf_metadata *metadata = dec->metadata;
    struct breader *br = &dec->br;
    struct dec_state *dec_s = &dec->dec_s;
    struct error *e = &dec->err;

    dec_state_init(dec_s, br->tot_bit_cnt);

    /* Decode packet-header-field-class */
    if (metadata->trace_cls.pkt_hdr.type != ACTF_FLD_CLS_NIL) {
	/* To follow the spec, the role behavior should be done after
	 * each field has been decoded and not after the whole header
	 * has been decoded. Thus roles are handled inline during the
	 * fld_cls_decode in `handle_fld_roles`.
	 */
	dec_s->ctx = DEC_CTX_PKT_HEADER;
	if ((rc = fld_cls_decode(dec, &metadata->trace_cls.pkt_hdr,
				 &dec_s->pkt.props[ACTF_PKT_PROP_HEADER])) < 0) {
	    eprependf(e, "packet-header-field-class");
	    return rc;
	}
    }
    /* Set DSC based on DSC_ID */
    struct actf_dstream_cls **dscp = u64todsc_find(&metadata->idtodsc, dec_s->pkt_s.dsc.id);
    if (dscp) {
	dec_s->pkt_s.dsc.cls = *dscp;
	dec_s->pkt_s.opt_flags |= PKT_DSTREAM_CLS;
    } else {
	eprintf(e, "no data stream class with id %" PRIu64 " found",
		dec_s->pkt_s.dsc.id);
	return ACTF_NO_SUCH_ID;
    }
    /* Decode packet-context-field-class */
    if (dec_s->pkt_s.dsc.cls->pkt_ctx.type != ACTF_FLD_CLS_NIL) {
	/* See role comment by packet-header decoding */
	dec_s->ctx = DEC_CTX_PKT_CTX;
	if ((rc = fld_cls_decode(dec, &dec_s->pkt_s.dsc.cls->pkt_ctx,
				 &dec_s->pkt.props[ACTF_PKT_PROP_CTX])) < 0) {
	    eprependf(e, "packet-context-field-class");
	    return rc;
	}
	if (dec_s->pkt_s.opt_flags & PKT_END_DEF_CLK_VAL &&
	    dec_s->pkt_s.def_clk_val > dec_s->pkt_s.end_def_clk_val) {
	    eprintf(e, "packet beginning timestamp (%" PRIu64 ") is greater than packet end timestamp (%" PRIu64 ")",
		    dec_s->pkt_s.def_clk_val, dec_s->pkt_s.end_def_clk_val);
	    // SHOULD report an error
	}
	if (dec_s->pkt_s.tot_len == UINT64_MAX && dec_s->pkt_s.content_len != UINT64_MAX) {
	    dec_s->pkt_s.tot_len = dec_s->pkt_s.content_len;
	}
	if (dec_s->pkt_s.content_len == UINT64_MAX && dec_s->pkt_s.tot_len != UINT64_MAX) {
	    dec_s->pkt_s.content_len = dec_s->pkt_s.tot_len;
	}
	if (dec_s->pkt_s.content_len > dec_s->pkt_s.tot_len) {
	    eprintf(e, "packet content length (%" PRIu64 ") larger than total length of packet (%" PRIu64 ")",
		    dec_s->pkt_s.content_len, dec_s->pkt_s.tot_len);
	    return ACTF_INVALID_CONTENT_LEN;
	}
    }

    return ACTF_OK;
}

static int actf_decoder_pkt_decode(struct actf_decoder *dec, size_t *evs_len)
{
    int rc;
    struct breader *br = &dec->br;
    struct dec_state *dec_s = &dec->dec_s;

    if (dec->state & DECODING_STATE_RESUME_PKT) {
	dec->state &= ~DECODING_STATE_RESUME_PKT;
	arena_clear(&dec->dec_s.ev_arena);
    } else if ((rc = actf_decoder_pkt_hdrctx_decode(dec)) < 0) {
	return rc;
    }

    while (breader_has_bits_remaining(&dec->br) && pkt_bits_remaining(&dec_s->pkt_s, br)) {
	if (*evs_len >= dec->evs_cap) {
	    /* Output buffer is full; we return it to the caller and
	     * keep going here on next call. */
	    dec->state |= DECODING_STATE_RESUME_PKT;
	    return ACTF_OK;
	}
	if ((rc = actf_decoder_ev_decode(dec, dec->evs[*evs_len])) < 0) {
	    return rc;
	}
	*evs_len += 1;
    }

    if (dec_s->pkt_s.tot_len != UINT64_MAX && dec_s->pkt_s.content_len != UINT64_MAX) {
	/* Set X to PKT_TOTAL_LEN, aka skip pointer forward to pkt_off
	 * + pkt_s.tot_len.
	 */
	size_t skip_cnt = (dec_s->pkt_s.bit_off + dec_s->pkt_s.tot_len) - br->tot_bit_cnt;
	breader_consume_checked(br, skip_cnt);
    }
    return ACTF_OK;
}

struct actf_decoder *actf_decoder_init(void *data, size_t data_len, size_t evs_cap,
				       const struct actf_metadata *metadata)
{
    struct actf_decoder *dec = malloc(sizeof(*dec));
    if (! dec) {
        return NULL;
    }
    dec->err = ERROR_EMPTY;
    if (evs_cap == 0) {
	evs_cap = ACTF_DEFAULT_EVS_CAP;
    }
    struct actf_event **evs = actf_event_arr_alloc(evs_cap);
    if (! evs) {
	error_free(&dec->err);
	free(dec);
	return NULL;
    }

    dec->state = DECODING_STATE_OK;
    dec->metadata = metadata;
    breader_init(data, data_len, ACTF_LIL_ENDIAN, &dec->br);
    dec->evs = evs;
    dec->evs_cap = evs_cap;
    // The packet arena holds a single packet header/context at a time.
    dec->dec_s.pkt_arena = (struct arena){.default_cap = 16 * sizeof(struct actf_fld)};
    // The event arena holds evs_cap event header/context/payload at a time.
    dec->dec_s.ev_arena = (struct arena){.default_cap = 16 * sizeof(struct actf_fld) * evs_cap};
    return dec;
}

int actf_decoder_decode(actf_decoder *dec, struct actf_event ***evs, size_t *evs_len)
{
    int rc;
    if (dec->state & DECODING_STATE_RESUME_SEEK) {
	dec->state &= ~DECODING_STATE_RESUME_SEEK;
	*evs = dec->evs + dec->seek_evs_off;
	*evs_len = dec->seek_evs_len;
	return ACTF_OK;
    } else if (dec->state & DECODING_STATE_ERROR) {
	*evs_len = 0;
	*evs = NULL;
	return dec->err_rc;
    }
    *evs_len = 0;
    *evs = dec->evs;
    if (breader_has_bits_remaining(&dec->br)) {
	// Run actf_decoder_pkt_decode until:
	// 1. A packet is fully decoded
	// 2. The output buffer is full
	// 3. An error has occurred.
	if ((rc = actf_decoder_pkt_decode(dec, evs_len)) < 0) {
	    dec->state |= DECODING_STATE_ERROR;
	    dec->err_rc = rc;
	    if (*evs_len == 0) {
		return rc;
	    }
	}
    }
    return ACTF_OK;
}

static int decoder_decode(void *self, struct actf_event ***evs, size_t *evs_len)
{
    actf_decoder *dec = self;
    return actf_decoder_decode(dec, evs, evs_len);
}

int actf_decoder_seek_ns_from_origin(actf_decoder *dec, int64_t tstamp)
{
    /* seek means we want to setup the decoder so that the next call
     * to decode will return an event with a tstamp greater or equal
     * to the sought tstamp. */
    dec->state = DECODING_STATE_OK;
    /* TODO: make this seek smart based on index (if avail) */
    breader_seek(&dec->br, 0, BREADER_SEEK_SET);

    /* search packet by packet for the correct seek location. If the
     * packet ends before tstamp, skip it! */
    int rc = ACTF_OK;
    while (breader_has_bits_remaining(&dec->br)) {
	if ((rc = actf_decoder_pkt_hdrctx_decode(dec)) < 0) {
	    dec->state |= DECODING_STATE_ERROR;
	    dec->err_rc = rc;
	    return rc;
	}
	struct pkt_state *pkt_s = &dec->dec_s.pkt_s;
	const actf_clk_cls *clkc = actf_dstream_cls_clk_cls(pkt_s->dsc.cls);
	if (clkc && (pkt_s->opt_flags & PKT_END_DEF_CLK_VAL &&
		     actf_clk_cls_cc_to_ns_from_origin(clkc, pkt_s->end_def_clk_val) < tstamp)) {
	    uint64_t off = sataddu64(pkt_s->bit_off, pkt_s->tot_len);
	    breader_seek(&dec->br, (size_t)(off / 8), BREADER_SEEK_SET);
	    continue;
	}
	dec->state |= DECODING_STATE_RESUME_PKT;

	size_t evs_len = 0;
	struct actf_event **evs = NULL;
	while ((rc = actf_decoder_decode(dec, &evs, &evs_len)) == ACTF_OK && evs_len) {
	    for (size_t i = 0; i < evs_len; i++) {
		if (actf_event_tstamp_ns_from_origin(evs[i]) >= tstamp) {
		    /* if the decoder faces an error, all valid events
		     * will be returned and it will set the state to
		     * error, so we add the resume_seek state here. First
		     * call to decode will return sought events, second
		     * call will return error. */
		    dec->state |= DECODING_STATE_RESUME_SEEK;
		    dec->seek_evs_off = i;
		    dec->seek_evs_len = evs_len - i;
		    return ACTF_OK;
		}
	    }
	    if (pkt_bits_remaining(pkt_s, &dec->br) == 0) {
		// out of events in the packet
		break;
	    }
	}
    }

    return rc;
}

static int decoder_seek_ns_from_origin(void *self, int64_t tstamp)
{
    actf_decoder *dec = self;
    return actf_decoder_seek_ns_from_origin(dec, tstamp);
}

const char *actf_decoder_last_error(actf_decoder *dec)
{
    if (! dec || dec->err.buf[0] == '\0') {
	return NULL;
    }
    return dec->err.buf;
}

static const char *decoder_last_error(void *self)
{
    actf_decoder *dec = self;
    return actf_decoder_last_error(dec);
}

void actf_decoder_free(struct actf_decoder *dec)
{
    if (! dec) {
	return;
    }
    actf_event_arr_free(dec->evs);
    arena_free(&dec->dec_s.pkt_arena);
    arena_free(&dec->dec_s.ev_arena);
    error_free(&dec->err);
    free(dec);
}

struct actf_event_generator actf_decoder_to_generator(actf_decoder *dec)
{
    return (struct actf_event_generator) {
	.generate = decoder_decode,
	.seek_ns_from_origin = decoder_seek_ns_from_origin,
	.last_error = decoder_last_error,
	.self = dec,
    };
}
