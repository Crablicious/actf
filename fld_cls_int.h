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

#ifndef ACTF_FLD_CLS_INT_H
#define ACTF_FLD_CLS_INT_H

#include <stdbool.h>
#include <stdint.h>
#include <json-c/json_object.h>

#include "fld_cls.h"
#include "flags_int.h"
#include "crust/common.h"
#include "mappings.h"
#include "metadata.h"
#include "fld_loc_int.h"
#include "rng_int.h"
#include "error.h"

/* Forward-declare since ctfjson depends on struct actf_fld_cls */
struct ctfjson;

extern const char *DEFAULT_MEDIA_TYPE;
extern const enum actf_encoding DEFAULT_ENCODING;
extern const enum actf_base DEFAULT_DISPLAY_BASE;
extern const uint64_t DEFAULT_ALIGNMENT;

/* actf_encoding_to_name translates an encoding to its codepoint size. */
size_t actf_encoding_to_codepoint_size(enum actf_encoding enc);

/* actf_encoding_to_name translates an encoding to its name. */
const char *actf_encoding_to_name(enum actf_encoding enc);

/* actf_fld_cls_type_name translates a field class type to its string
 * representation. */
const char *actf_fld_cls_type_name(enum actf_fld_cls_type type);
/* Returns the effective alignment requirement of the field class. */
size_t actf_fld_cls_get_align_req(const struct actf_fld_cls *fc);

int actf_fld_cls_parse(struct json_object *fld_cls_jobj, const struct actf_metadata *metadata,
		       struct actf_fld_cls *fld_cls, struct error *e);
void actf_fld_cls_free(struct actf_fld_cls *fc);

struct fxd_len_bit_arr_fld_cls {
	uint64_t len; // length in bits
	enum actf_byte_order bo; // byte-order
	enum actf_bit_order bito; // bit-order
	uint64_t align;
};

struct fxd_len_bit_map_fld_cls {
	struct fxd_len_bit_arr_fld_cls bit_arr;
	struct actf_flags flags;
};

/* "Abstract" base for ints */
struct int_fld_cls {
	enum actf_base pref_display_base;
	struct actf_mappings maps;
};

/* Could consider splitting int/uint since they have some differences:
   - roles are only applicable to uint
   - sign extension when decoding only applies to int
   - decoding yields different field values
*/
struct fxd_len_int_fld_cls {
	struct int_fld_cls base;
	struct fxd_len_bit_arr_fld_cls bit_arr;
	enum actf_role roles; // Really only needs to be specified for fixed-length-unsigned-integer.
};

struct fxd_len_bool_fld_cls {
	struct fxd_len_bit_arr_fld_cls bit_arr;
};

struct fxd_len_float_fld_cls {
	struct fxd_len_bit_arr_fld_cls bit_arr;
};

struct var_len_int_fld_cls {
	struct int_fld_cls base;
	enum actf_role roles;
};

/* "Abstract" base for strings */
struct str_fld_cls {
	enum actf_encoding enc;
};

struct null_term_str_fld_cls {
	struct str_fld_cls base;
};

struct static_len_str_fld_cls {
	struct str_fld_cls base;
	uint64_t len; // can be zero
};

struct static_len_blob_fld_cls {
	uint64_t len; // can be zero, whatever that entails?
	char *media_type;
	enum actf_role roles;
};

struct dyn_len_str_fld_cls {
	struct str_fld_cls base;
	struct actf_fld_loc len_fld_loc;
};

struct dyn_len_blob_fld_cls {
	struct actf_fld_loc len_fld_loc;
	char *media_type;
};

/* struct_fld_member_cls requires fld_cls so it must be defined after
 * it.
 */
struct struct_fld_member_cls;

struct struct_fld_cls {
	// Could represent the member clses more data oriented by having
	// one array for the fld-clses and one for the names. Maybe means
	// faster decoding/encoding since all field classes will be
	// tightly packed. No improvement after trying it out.
	struct struct_fld_member_cls *member_clses;
	size_t n_members;
	// MUST be a positive power of two
	uint64_t min_align;
	/* effective align derived from member classes. */
	uint64_t align;
};

/* "Abstract" base for static and dynamic arrays. */
struct arr_fld_cls {
	struct actf_fld_cls *ele_fld_cls;
	uint64_t min_align;
};

struct static_len_arr_fld_cls {
	struct arr_fld_cls base;
	size_t len;
};

struct dyn_len_arr_fld_cls {
	struct arr_fld_cls base;
	struct actf_fld_loc len_fld_loc;
};

struct optional_fld_cls {
	struct actf_fld_cls *fld_cls;
	struct actf_fld_loc sel_fld_loc;
	struct actf_rng_set sel_fld_rng_set; // This can have zero ranges if the
	// selector-field is a boolean.
};

struct variant_fld_cls {
	struct variant_fld_cls_opt *opts;
	size_t n_opts;
	struct actf_fld_loc sel_fld_loc;
};

struct actf_fld_cls {
	enum actf_fld_cls_type type;
	union {
		struct fxd_len_bit_arr_fld_cls fxd_len_bit_arr;
		struct fxd_len_bit_map_fld_cls fxd_len_bit_map;
		struct fxd_len_int_fld_cls fxd_len_int;
		struct fxd_len_bool_fld_cls fxd_len_bool;
		struct fxd_len_float_fld_cls fxd_len_float;
		struct var_len_int_fld_cls var_len_int;
		struct null_term_str_fld_cls null_term_str;
		struct static_len_str_fld_cls static_len_str;
		struct dyn_len_str_fld_cls dyn_len_str;
		struct static_len_blob_fld_cls static_len_blob;
		struct dyn_len_blob_fld_cls dyn_len_blob;
		struct struct_fld_cls struct_;
		struct static_len_arr_fld_cls static_len_arr;
		struct dyn_len_arr_fld_cls dyn_len_arr;
		struct optional_fld_cls optional;
		struct variant_fld_cls variant;
	} cls;
	char *alias; // NULL if actf_fld is not an alias. Only non-aliases
	// owns the cls structure and should free it.
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

struct variant_fld_cls_opt {
	struct actf_fld_cls fc;
	struct actf_rng_set sel_fld_rng_set;
	char *name;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

struct struct_fld_member_cls {
	char *name;
	struct actf_fld_cls cls;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

#endif /* ACTF_FLD_CLS_INT_H */
