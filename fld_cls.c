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
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json_object.h>

#include "crust/common.h"
#include "fld_cls.h"
#include "mappings.h"
#include "metadata_int.h"
#include "fld_loc.h"
#include "rng_int.h"
#include "json_utils.h"

const char *DEFAULT_MEDIA_TYPE = "application/octet-stream";
const enum actf_encoding DEFAULT_ENCODING = ACTF_ENCODING_UTF8;
const enum actf_base DEFAULT_DISPLAY_BASE = ACTF_BASE_DECIMAL;
const uint64_t DEFAULT_ALIGNMENT = 1;

/* Beautiful forward-declarations. */
static int fxd_len_bit_arr_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e);
static void fxd_len_bit_arr_fld_cls_free(struct fxd_len_bit_arr_fld_cls *fc);

static int fxd_len_bit_map_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e);
static void fxd_len_bit_map_fld_cls_free(struct fxd_len_bit_map_fld_cls *fc);

static int fxd_len_int_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e);
static void fxd_len_sint_fld_cls_free(struct fxd_len_int_fld_cls *fc);
static void fxd_len_uint_fld_cls_free(struct fxd_len_int_fld_cls *fc);

static int fxd_len_bool_fld_cls_parse(struct json_object *fc_jobj,
				      struct actf_fld_cls *fc, struct error *e);
static void fxd_len_bool_fld_cls_free(struct fxd_len_bool_fld_cls *fc);

static int fxd_len_float_fld_cls_parse(struct json_object *fc_jobj,
				       struct actf_fld_cls *fc, struct error *e);
static void fxd_len_float_fld_cls_free(struct fxd_len_float_fld_cls *fc);

static int var_len_int_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e);
static void var_len_sint_fld_cls_free(struct var_len_int_fld_cls *fc);
static void var_len_uint_fld_cls_free(struct var_len_int_fld_cls *fc);

static int null_term_str_fld_cls_parse(struct json_object *fc_jobj,
				       struct actf_fld_cls *fc, struct error *e);
static void null_term_str_fld_cls_free(struct null_term_str_fld_cls *fc);

static int static_len_str_fld_cls_parse(struct json_object *fc_jobj,
					struct actf_fld_cls *fc, struct error *e);
static void static_len_str_fld_cls_free(struct static_len_str_fld_cls *fc);

static int dyn_len_str_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e);
static void dyn_len_str_fld_cls_free(struct dyn_len_str_fld_cls *fc);

static int static_len_blob_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e);
static void static_len_blob_fld_cls_free(struct static_len_blob_fld_cls *fc);

static int dyn_len_blob_fld_cls_parse(struct json_object *fc_jobj,
				      struct actf_fld_cls *fc, struct error *e);
static void dyn_len_blob_fld_cls_free(struct dyn_len_blob_fld_cls *fc);

static int struct_fld_cls_parse(struct json_object *struct_fldc_jobj,
				const struct actf_metadata *metadata,
				struct actf_fld_cls *struct_fld_cls, struct error *e);
static void struct_fld_cls_free(struct struct_fld_cls *struct_fld_cls);

static int static_len_arr_fld_cls_parse(struct json_object *fc_jobj,
					const struct actf_metadata *metadata,
					struct actf_fld_cls *fc, struct error *e);
static void static_len_arr_fld_cls_free(struct static_len_arr_fld_cls *fc);

static int dyn_len_arr_fld_cls_parse(struct json_object *fc_jobj,
				     const struct actf_metadata *metadata,
				     struct actf_fld_cls *fc, struct error *e);
static void dyn_len_arr_fld_cls_free(struct dyn_len_arr_fld_cls *fc);

static int optional_fld_cls_parse(struct json_object *fc_jobj,
				  const struct actf_metadata *metadata,
				  struct actf_fld_cls *fc, struct error *e);
static void optional_fld_cls_free(struct optional_fld_cls *fc);

static int variant_fld_cls_parse(struct json_object *fc_jobj,
				 const struct actf_metadata *metadata,
				 struct actf_fld_cls *fc, struct error *e);
static void variant_fld_cls_free(struct variant_fld_cls *fc);

enum actf_fld_cls_type actf_fld_cls_type(const struct actf_fld_cls *fc)
{
	return fc->type;
}

size_t actf_fld_cls_len(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return fc->cls.fxd_len_bit_arr.len;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return fc->cls.fxd_len_bit_map.bit_arr.len;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return fc->cls.fxd_len_int.bit_arr.len;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return fc->cls.fxd_len_bool.bit_arr.len;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return fc->cls.fxd_len_float.bit_arr.len;
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
		return fc->cls.static_len_blob.len;
	case ACTF_FLD_CLS_STATIC_LEN_STR:
		return fc->cls.static_len_str.len;
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		return fc->cls.static_len_arr.len;
	default:
		return 0;
	}
}

enum actf_byte_order actf_fld_cls_byte_order(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return fc->cls.fxd_len_bit_arr.bo;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return fc->cls.fxd_len_bit_map.bit_arr.bo;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return fc->cls.fxd_len_int.bit_arr.bo;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return fc->cls.fxd_len_bool.bit_arr.bo;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return fc->cls.fxd_len_float.bit_arr.bo;
	default:
		return ACTF_LIL_ENDIAN;
	}
}

enum actf_bit_order actf_fld_cls_bit_order(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return fc->cls.fxd_len_bit_arr.bito;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return fc->cls.fxd_len_bit_map.bit_arr.bito;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return fc->cls.fxd_len_int.bit_arr.bito;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return fc->cls.fxd_len_bool.bit_arr.bito;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return fc->cls.fxd_len_float.bit_arr.bito;
	default:
		return ACTF_FIRST_TO_LAST;
	}
}

size_t actf_fld_cls_alignment(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return fc->cls.fxd_len_bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return fc->cls.fxd_len_bit_map.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return fc->cls.fxd_len_int.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return fc->cls.fxd_len_bool.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return fc->cls.fxd_len_float.bit_arr.align;
	default:
		return 0;
	}
}

const struct actf_flags *actf_fld_cls_bit_map_flags(const struct actf_fld_cls *fc)
{
	if (fc->type == ACTF_FLD_CLS_FXD_LEN_BIT_MAP) {
		return &fc->cls.fxd_len_bit_map.flags;
	} else {
		return NULL;
	}
}

enum actf_base actf_fld_cls_pref_display_base(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_SINT:
	case ACTF_FLD_CLS_FXD_LEN_UINT:
		return fc->cls.fxd_len_int.base.pref_display_base;
	case ACTF_FLD_CLS_VAR_LEN_SINT:
	case ACTF_FLD_CLS_VAR_LEN_UINT:
		return fc->cls.var_len_int.base.pref_display_base;
	default:
		return ACTF_BASE_DECIMAL;
	}
}

const struct actf_mappings *actf_fld_cls_mappings(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_SINT:
	case ACTF_FLD_CLS_FXD_LEN_UINT:
		return &fc->cls.fxd_len_int.base.maps;
	case ACTF_FLD_CLS_VAR_LEN_SINT:
	case ACTF_FLD_CLS_VAR_LEN_UINT:
		return &fc->cls.var_len_int.base.maps;
	default:
		return NULL;
	}
}

enum actf_role actf_fld_cls_roles(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_FXD_LEN_UINT:
		return fc->cls.fxd_len_int.roles;
	case ACTF_FLD_CLS_VAR_LEN_UINT:
		return fc->cls.var_len_int.roles;
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
		return fc->cls.static_len_blob.roles;
	default:
		return ACTF_ROLE_NIL;
	}
}

enum actf_encoding actf_fld_cls_encoding(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_NULL_TERM_STR:
		return fc->cls.null_term_str.base.enc;
	case ACTF_FLD_CLS_STATIC_LEN_STR:
		return fc->cls.static_len_str.base.enc;
	case ACTF_FLD_CLS_DYN_LEN_STR:
		return fc->cls.dyn_len_str.base.enc;
	default:
		return ACTF_ENCODING_UTF8;
	}
}

const char *actf_fld_cls_media_type(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
		return fc->cls.static_len_blob.media_type;
	case ACTF_FLD_CLS_DYN_LEN_BLOB:
		return fc->cls.dyn_len_blob.media_type;
	default:
		return NULL;
	}
}

const struct actf_fld_loc *actf_fld_cls_len_fld_loc(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_DYN_LEN_STR:
		return &fc->cls.dyn_len_str.len_fld_loc;
	case ACTF_FLD_CLS_DYN_LEN_BLOB:
		return &fc->cls.dyn_len_blob.len_fld_loc;
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		return &fc->cls.dyn_len_arr.len_fld_loc;
	default:
		return NULL;
	}
}

uint64_t actf_fld_cls_min_alignment(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_STRUCT:
		return fc->cls.struct_.min_align;
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		return fc->cls.static_len_arr.base.min_align;
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		return fc->cls.dyn_len_arr.base.min_align;
	default:
		return 0;
	}
}

size_t actf_fld_cls_members_len(const struct actf_fld_cls *fc)
{
	if (fc->type == ACTF_FLD_CLS_STRUCT) {
		return fc->cls.struct_.n_members;
	} else {
		return 0;
	}
}

const char *actf_fld_cls_members_name_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_STRUCT && i < fc->cls.struct_.n_members) {
		return fc->cls.struct_.member_clses[i].name;
	} else {
		return NULL;
	}
}

const struct actf_fld_cls *actf_fld_cls_members_fld_cls_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_STRUCT && i < fc->cls.struct_.n_members) {
		return &fc->cls.struct_.member_clses[i].cls;
	} else {
		return NULL;
	}
}

const struct actf_fld *actf_fld_cls_members_attributes_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_STRUCT && i < fc->cls.struct_.n_members &&
	    fc->cls.struct_.member_clses[i].attributes) {
		return &fc->cls.struct_.member_clses[i].attributes->jval.val;
	} else {
		return NULL;
	}
}

const struct actf_fld *actf_fld_cls_members_extensions_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_STRUCT && i < fc->cls.struct_.n_members &&
	    fc->cls.struct_.member_clses[i].extensions) {
		return &fc->cls.struct_.member_clses[i].extensions->jval.val;
	} else {
		return NULL;
	}
}

const struct actf_fld_cls *actf_fld_cls_element_fld_cls(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		return fc->cls.static_len_arr.base.ele_fld_cls;
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		return fc->cls.dyn_len_arr.base.ele_fld_cls;
	default:
		return NULL;
	}
}

const struct actf_fld_loc *actf_fld_cls_selector_fld_loc(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_OPTIONAL:
		return &fc->cls.optional.sel_fld_loc;
	case ACTF_FLD_CLS_VARIANT:
		return &fc->cls.variant.sel_fld_loc;
	default:
		return NULL;
	}
}

const struct actf_rng_set *actf_fld_cls_selector_rng_set(const struct actf_fld_cls *fc)
{
	if (fc->type == ACTF_FLD_CLS_OPTIONAL) {
		return &fc->cls.optional.sel_fld_rng_set;
	} else {
		return NULL;
	}
}

const struct actf_fld_cls *actf_fld_cls_optional_fld_cls(const struct actf_fld_cls *fc)
{
	if (fc->type == ACTF_FLD_CLS_OPTIONAL) {
		return fc->cls.optional.fld_cls;
	} else {
		return NULL;
	}
}

size_t actf_fld_cls_options_len(const struct actf_fld_cls *fc)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT) {
		return fc->cls.variant.n_opts;
	} else {
		return 0;
	}
}

const char *actf_fld_cls_options_name_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT && i < fc->cls.variant.n_opts) {
		return fc->cls.variant.opts[i].name;
	} else {
		return NULL;
	}
}

const struct actf_fld_cls *actf_fld_cls_options_fld_cls_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT && i < fc->cls.variant.n_opts) {
		return &fc->cls.variant.opts[i].fc;
	} else {
		return NULL;
	}
}

const struct actf_rng_set *actf_fld_cls_variant_options_selector_rng_set_idx(const struct
									     actf_fld_cls *fc,
									     size_t i)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT && i < fc->cls.variant.n_opts) {
		return &fc->cls.variant.opts[i].sel_fld_rng_set;
	} else {
		return NULL;
	}
}

const struct actf_fld *actf_fld_cls_options_attributes_idx(const struct actf_fld_cls *fc, size_t i)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT && i < fc->cls.variant.n_opts &&
	    fc->cls.variant.opts[i].attributes) {
		return &fc->cls.variant.opts[i].attributes->jval.val;
	} else {
		return NULL;
	}
}

const struct actf_fld *actf_fld_cls_variant_opt_extensions_idx(const struct actf_fld_cls *fc,
							       size_t i)
{
	if (fc->type == ACTF_FLD_CLS_VARIANT && i < fc->cls.variant.n_opts &&
	    fc->cls.variant.opts[i].extensions) {
		return &fc->cls.variant.opts[i].extensions->jval.val;
	} else {
		return NULL;
	}
}

const char *actf_fld_cls_aliased_name(const struct actf_fld_cls *fc)
{
	return fc->alias;
}

const struct actf_fld *actf_fld_cls_attributes(const struct actf_fld_cls *fc)
{
	if (fc->attributes) {
		return &fc->attributes->jval.val;
	} else {
		return NULL;
	}
}

const struct actf_fld *actf_fld_cls_extensions(const struct actf_fld_cls *fc)
{
	if (fc->extensions) {
		return &fc->extensions->jval.val;
	} else {
		return NULL;
	}
}

size_t actf_encoding_to_codepoint_size(enum actf_encoding enc)
{
	switch (enc) {
	case ACTF_ENCODING_UTF8:
		return 1;
	case ACTF_ENCODING_UTF16BE:
		return 2;
	case ACTF_ENCODING_UTF16LE:
		return 2;
	case ACTF_ENCODING_UTF32BE:
		return 4;
	case ACTF_ENCODING_UTF32LE:
		return 4;
	case ACTF_N_ENCODINGS:
		return 0;
	}
	return 0;
}

const char *actf_encoding_to_name(enum actf_encoding enc)
{
	switch (enc) {
	case ACTF_ENCODING_UTF8:
		return "utf-8";
	case ACTF_ENCODING_UTF16BE:
		return "utf-16be";
	case ACTF_ENCODING_UTF16LE:
		return "utf-16le";
	case ACTF_ENCODING_UTF32BE:
		return "utf-32be";
	case ACTF_ENCODING_UTF32LE:
		return "utf-32le";
	case ACTF_N_ENCODINGS:
		return NULL;
	}
	return NULL;
}

static const char *actf_role_name(enum actf_role role)
{
	switch (role) {
	case ACTF_ROLE_NIL:
		return NULL;
	case ACTF_ROLE_DSTREAM_CLS_ID:
		return "data-stream-class-id";
	case ACTF_ROLE_DSTREAM_ID:
		return "data-stream-id";
	case ACTF_ROLE_PKT_MAGIC_NUM:
		return "packet-magic-number";
	case ACTF_ROLE_METADATA_STREAM_UUID:
		return "metadata-stream-uuid";
	case ACTF_ROLE_DEF_CLK_TSTAMP:
		return "default-clock-timestamp";
	case ACTF_ROLE_DISC_EVENT_CNT_SNAPSHOT:
		return "discarded-event-record-counter-snapshot";
	case ACTF_ROLE_PKT_CONTENT_LEN:
		return "packet-content-length";
	case ACTF_ROLE_PKT_END_DEF_CLK_TSTAMP:
		return "packet-end-default-clock-timestamp";
	case ACTF_ROLE_PKT_SEQ_NUM:
		return "packet-sequence-number";
	case ACTF_ROLE_PKT_TOT_LEN:
		return "packet-total-length";
	case ACTF_ROLE_EVENT_CLS_ID:
		return "event-record-class-id";
	}
	return NULL;
}

const char *actf_fld_cls_type_name(enum actf_fld_cls_type type)
{
	switch (type) {
	case ACTF_FLD_CLS_NIL:
		return NULL;
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return "fixed-length-bit-array";
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return "fixed-length-bit-map";
	case ACTF_FLD_CLS_FXD_LEN_UINT:
		return "fixed-length-unsigned-integer";
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return "fixed-length-signed-integer";
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return "fixed-length-boolean";
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return "fixed-length-floating-point-number";
	case ACTF_FLD_CLS_VAR_LEN_UINT:
		return "variable-length-unsigned-integer";
	case ACTF_FLD_CLS_VAR_LEN_SINT:
		return "variable-length-signed-integer";
	case ACTF_FLD_CLS_NULL_TERM_STR:
		return "null-terminated-string";
	case ACTF_FLD_CLS_STATIC_LEN_STR:
		return "static-length-string";
	case ACTF_FLD_CLS_DYN_LEN_STR:
		return "dynamic-length-string";
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
		return "static-length-blob";
	case ACTF_FLD_CLS_DYN_LEN_BLOB:
		return "dynamic-length-blob";
	case ACTF_FLD_CLS_STRUCT:
		return "structure";
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		return "static-length-array";
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		return "dynamic-length-array";
	case ACTF_FLD_CLS_OPTIONAL:
		return "optional";
	case ACTF_FLD_CLS_VARIANT:
		return "variant";
	case ACTF_N_FLD_CLSES:
		return NULL;
	}
	return NULL;
}

static enum actf_fld_cls_type actf_fld_cls_type_from_name(const char *name)
{
	for (int i = 0; i < ACTF_N_FLD_CLSES; i++) {
		const char *type_name = actf_fld_cls_type_name(i);
		if (type_name && strcmp(name, type_name) == 0) {
			return i;
		}
	}
	return ACTF_FLD_CLS_NIL;
}

static size_t arr_fld_cls_get_align_req(const struct arr_fld_cls fc)
{
	/* The maximum value of:
	   - The value of the minimum-alignment property of F.
	   - The alignment requirement of an instance of the
	   element-field-class property of F.
	 */
	size_t min_align = fc.min_align;
	size_t ele_align = actf_fld_cls_get_align_req(fc.ele_fld_cls);
	return MAX(min_align, ele_align);
}

size_t actf_fld_cls_get_align_req(const struct actf_fld_cls *fc)
{
	switch (fc->type) {
	case ACTF_FLD_CLS_NIL:
		assert(false);
		return 1;
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		return fc->cls.fxd_len_bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		return fc->cls.fxd_len_bit_map.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		return fc->cls.fxd_len_int.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		return fc->cls.fxd_len_bool.bit_arr.align;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		return fc->cls.fxd_len_float.bit_arr.align;
	case ACTF_FLD_CLS_VAR_LEN_UINT:
	case ACTF_FLD_CLS_VAR_LEN_SINT:
	case ACTF_FLD_CLS_NULL_TERM_STR:
	case ACTF_FLD_CLS_STATIC_LEN_STR:
	case ACTF_FLD_CLS_DYN_LEN_STR:
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
	case ACTF_FLD_CLS_DYN_LEN_BLOB:
		return 8;
	case ACTF_FLD_CLS_STRUCT:{
		return fc->cls.struct_.align;
	}
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		return arr_fld_cls_get_align_req(fc->cls.static_len_arr.base);
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		return arr_fld_cls_get_align_req(fc->cls.dyn_len_arr.base);
	case ACTF_FLD_CLS_OPTIONAL:
	case ACTF_FLD_CLS_VARIANT:
		return 1;
	case ACTF_N_FLD_CLSES:
		assert(false);
		return 1;
	}
	return 1;
}

static int actf_fld_cls_resolve_alias(struct json_object *fc_jobj,
				      const struct actf_metadata *metadata,
				      struct actf_fld_cls *fc, struct error *e)
{
	assert(json_object_is_type(fc_jobj, json_type_string));
	const char *alias = json_object_get_string(fc_jobj);
	/* Attempt to resolve the alias from the metadata */
	const struct actf_fld_cls_alias *fc_alias =
	    actf_metadata_find_fld_cls_alias(metadata, alias);
	if (!fc_alias) {
		eprintf(e, "referring to alias \"%s\" which is not defined", alias);
		return ACTF_NO_SUCH_ALIAS;
	}
	/* Copy the actf_fld_cls specification in `alias` into `fc`
	 * effectively resolving fldces alias.
	 */
	memcpy(fc, &fc_alias->fld_cls, sizeof(*fc));
	fc->alias = c_strdup(alias);
	return ACTF_OK;
}

int actf_fld_cls_parse(struct json_object *fc_jobj, const struct actf_metadata *metadata,
		       struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	if (json_object_is_type(fc_jobj, json_type_object)) {
		const char *type_name;
		if (json_object_key_get_string(fc_jobj, "type", &type_name, e) < 0) {
			print_missing_key("type", "field-class", e);
			return ACTF_INVALID_FLD_CLS;
		}
		fc->type = actf_fld_cls_type_from_name(type_name);
		fc->alias = NULL;
		fc->attributes = NULL;
		if ((rc =
		     json_object_key_get_ctfjson(fc_jobj, "attributes", &fc->attributes, e)) < 0
		    && rc != ACTF_NOT_FOUND) {
			eprependf(e, "field-class %s", type_name);
			return rc;
		}
		fc->extensions = NULL;
		if ((rc =
		     json_object_key_get_ctfjson(fc_jobj, "extensions", &fc->extensions, e)) < 0
		    && rc != ACTF_NOT_FOUND) {
			eprependf(e, "field-class %s", type_name);
			return rc;
		}
		switch (fc->type) {
		case ACTF_FLD_CLS_NIL:
			return ACTF_INTERNAL;
		case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
			rc = fxd_len_bit_arr_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
			rc = fxd_len_bit_map_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_FXD_LEN_UINT:
		case ACTF_FLD_CLS_FXD_LEN_SINT:
			rc = fxd_len_int_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_FXD_LEN_BOOL:
			rc = fxd_len_bool_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_FXD_LEN_FLOAT:
			rc = fxd_len_float_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_VAR_LEN_UINT:
		case ACTF_FLD_CLS_VAR_LEN_SINT:
			rc = var_len_int_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_NULL_TERM_STR:
			rc = null_term_str_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_STATIC_LEN_STR:
			rc = static_len_str_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_DYN_LEN_STR:
			rc = dyn_len_str_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_STATIC_LEN_BLOB:
			rc = static_len_blob_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_DYN_LEN_BLOB:
			rc = dyn_len_blob_fld_cls_parse(fc_jobj, fc, e);
			break;
		case ACTF_FLD_CLS_STRUCT:
			rc = struct_fld_cls_parse(fc_jobj, metadata, fc, e);
			break;
		case ACTF_FLD_CLS_STATIC_LEN_ARR:
			rc = static_len_arr_fld_cls_parse(fc_jobj, metadata, fc, e);
			break;
		case ACTF_FLD_CLS_DYN_LEN_ARR:
			rc = dyn_len_arr_fld_cls_parse(fc_jobj, metadata, fc, e);
			break;
		case ACTF_FLD_CLS_OPTIONAL:
			rc = optional_fld_cls_parse(fc_jobj, metadata, fc, e);
			break;
		case ACTF_FLD_CLS_VARIANT:
			rc = variant_fld_cls_parse(fc_jobj, metadata, fc, e);
			break;
		case ACTF_N_FLD_CLSES:
			return ACTF_INTERNAL;
		}
		if (rc < 0) {
			eprependf(e, type_name);
		}
		return rc;
	} else if (json_object_is_type(fc_jobj, json_type_string)) {
		return actf_fld_cls_resolve_alias(fc_jobj, metadata, fc, e);
	} else {
		eprintf(e, "field-class is not a string or an object");
		return ACTF_INVALID_FLD_CLS;
	}
}

void actf_fld_cls_free(struct actf_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	if (fc->alias) {
		/* An aliased actf_fld_cls owns only its name. The actual
		 * actf_fld_cls_alias owns the actf_fld_cls.
		 */
		free(fc->alias);
		return;
	}
	ctfjson_free(fc->attributes);
	ctfjson_free(fc->extensions);
	switch (fc->type) {
	case ACTF_FLD_CLS_NIL:
		break;
	case ACTF_FLD_CLS_FXD_LEN_BIT_ARR:
		fxd_len_bit_arr_fld_cls_free(&fc->cls.fxd_len_bit_arr);
		break;
	case ACTF_FLD_CLS_FXD_LEN_BIT_MAP:
		fxd_len_bit_map_fld_cls_free(&fc->cls.fxd_len_bit_map);
		break;
	case ACTF_FLD_CLS_FXD_LEN_SINT:
		fxd_len_sint_fld_cls_free(&fc->cls.fxd_len_int);
		break;
	case ACTF_FLD_CLS_FXD_LEN_UINT:
		fxd_len_uint_fld_cls_free(&fc->cls.fxd_len_int);
		break;
	case ACTF_FLD_CLS_FXD_LEN_BOOL:
		fxd_len_bool_fld_cls_free(&fc->cls.fxd_len_bool);
		break;
	case ACTF_FLD_CLS_FXD_LEN_FLOAT:
		fxd_len_float_fld_cls_free(&fc->cls.fxd_len_float);
		break;
	case ACTF_FLD_CLS_VAR_LEN_UINT:
		var_len_uint_fld_cls_free(&fc->cls.var_len_int);
		break;
	case ACTF_FLD_CLS_VAR_LEN_SINT:
		var_len_sint_fld_cls_free(&fc->cls.var_len_int);
		break;
	case ACTF_FLD_CLS_NULL_TERM_STR:
		null_term_str_fld_cls_free(&fc->cls.null_term_str);
		break;
	case ACTF_FLD_CLS_STATIC_LEN_STR:
		static_len_str_fld_cls_free(&fc->cls.static_len_str);
		break;
	case ACTF_FLD_CLS_DYN_LEN_STR:
		dyn_len_str_fld_cls_free(&fc->cls.dyn_len_str);
		break;
	case ACTF_FLD_CLS_STATIC_LEN_BLOB:
		static_len_blob_fld_cls_free(&fc->cls.static_len_blob);
		break;
	case ACTF_FLD_CLS_DYN_LEN_BLOB:
		dyn_len_blob_fld_cls_free(&fc->cls.dyn_len_blob);
		break;
	case ACTF_FLD_CLS_STRUCT:
		struct_fld_cls_free(&fc->cls.struct_);
		break;
	case ACTF_FLD_CLS_STATIC_LEN_ARR:
		static_len_arr_fld_cls_free(&fc->cls.static_len_arr);
		break;
	case ACTF_FLD_CLS_DYN_LEN_ARR:
		dyn_len_arr_fld_cls_free(&fc->cls.dyn_len_arr);
		break;
	case ACTF_FLD_CLS_OPTIONAL:
		optional_fld_cls_free(&fc->cls.optional);
		break;
	case ACTF_FLD_CLS_VARIANT:
		variant_fld_cls_free(&fc->cls.variant);
		break;
	case ACTF_N_FLD_CLSES:
		break;
	}
}

static int int_fxd_len_bit_arr_fld_cls_parse(struct json_object *fc_jobj,
					     struct fxd_len_bit_arr_fld_cls *fxd_len_bit_arr,
					     struct error *e)
{
	int rc;
	uint64_t len = 0;
	if ((rc = json_object_key_get_gtz_uint64(fc_jobj, "length", &len, e)) < 0) {
		return rc;
	}
	if (len > 64) {
		eprintf(e, "length larger than 64 is not supported");
		return ACTF_UNSUPPORTED_LENGTH;
	}
	enum actf_byte_order bo;
	if ((rc = json_object_key_get_bo(fc_jobj, "byte-order", &bo, e)) < 0) {
		return rc;
	}
	enum actf_bit_order bito = bo == ACTF_LIL_ENDIAN ? ACTF_FIRST_TO_LAST : ACTF_LAST_TO_FIRST;
	if ((rc = json_object_key_get_bito(fc_jobj, "bit-order", &bito, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	uint64_t align = DEFAULT_ALIGNMENT;
	if ((rc = json_object_key_get_alignment(fc_jobj, "alignment", &align, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}

	fxd_len_bit_arr->len = len;
	fxd_len_bit_arr->bo = bo;
	fxd_len_bit_arr->bito = bito;
	fxd_len_bit_arr->align = align;
	return ACTF_OK;
}

static int fxd_len_bit_arr_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e)
{
	return int_fxd_len_bit_arr_fld_cls_parse(fc_jobj, &fc->cls.fxd_len_bit_arr, e);
}

static void fxd_len_bit_arr_fld_cls_free(struct fxd_len_bit_arr_fld_cls __maybe_unused *fc)
{
	return;
}

static int fxd_len_bit_map_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct fxd_len_bit_map_fld_cls *fxd_len_bit_map = &fc->cls.fxd_len_bit_map;

	struct fxd_len_bit_arr_fld_cls bit_arr;
	CLEAR(bit_arr);
	if ((rc = int_fxd_len_bit_arr_fld_cls_parse(fc_jobj, &bit_arr, e)) < 0) {
		return rc;
	}
	struct mappings maps;
	CLEAR(maps);
	if ((rc = json_object_key_get_mappings(fc_jobj, "flags", RNG_UINT, &maps, e)) < 0) {
		fxd_len_bit_arr_fld_cls_free(&bit_arr);
		return rc;
	}
	/* Bit map field class flags MUST contain one or more
	 * properties. */
	if (mappings_len(&maps) == 0) {
		eprintf(e, "flags in fixed-length-bit-map has no properties");
		mappings_free(&maps);
		fxd_len_bit_arr_fld_cls_free(&bit_arr);
		return ACTF_INVALID_FLAGS;
	}
	struct actf_flags flags;
	CLEAR(flags);
	if ((rc = actf_flags_init(&maps.d.umaps, &flags)) < 0) {
		eprintf(e, "initialize bit map of flags");
		mappings_free(&maps);
		fxd_len_bit_arr_fld_cls_free(&bit_arr);
		return rc;
	}

	fxd_len_bit_map->bit_arr = bit_arr;
	fxd_len_bit_map->flags = flags;
	return ACTF_OK;
}

static void fxd_len_bit_map_fld_cls_free(struct fxd_len_bit_map_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	fxd_len_bit_arr_fld_cls_free(&fc->bit_arr);
	actf_flags_free(&fc->flags);
}

static enum actf_role actf_role_name_get_type(const char *name)
{
	const char *role_name = NULL;
	for (int i = 0; (role_name = actf_role_name(1 << i)); i++) {
		if (strcmp(name, role_name) == 0) {
			return 1 << i;
		}
	}
	return ACTF_ROLE_NIL;
}

/* If no roles are available, `roles_out` will be set to zero. */
static int json_object_get_roles(struct json_object *jobj, enum actf_role *roles_out,
				 struct error *e)
{
	enum actf_role roles = 0;
	struct json_object *roles_jobj;
	if (json_object_object_get_ex(jobj, "roles", &roles_jobj)) {
		if (!json_object_is_type(roles_jobj, json_type_array)) {
			eprintf(e, "roles is not an array");
			return ACTF_INVALID_ROLE;
		}
		size_t len = json_object_array_length(roles_jobj);
		for (size_t i = 0; i < len; i++) {
			struct json_object *role_jobj = json_object_array_get_idx(roles_jobj, i);
			if (!json_object_is_type(role_jobj, json_type_string)) {
				eprintf(e, "role in roles is not a string");
				return ACTF_INVALID_ROLE;
			}
			const char *role_name = json_object_get_string(role_jobj);
			enum actf_role role = actf_role_name_get_type(role_name);
			// An unknown role (ACTF_ROLE_NIL) is not considered an
			// error. It could belong to a disabled extension.
			roles |= role;
		}
	}
	*roles_out = roles;
	return ACTF_OK;
}

static int json_object_key_get_base(const struct json_object *jobj, const char *key,
				    enum actf_base *base, struct error *e)
{
	int rc;
	uint64_t ubase;
	if ((rc = json_object_key_get_gtez_uint64(jobj, key, &ubase, e)) < 0) {
		return rc;
	}
	switch (ubase) {
	case ACTF_BASE_BINARY:
	case ACTF_BASE_OCTAL:
	case ACTF_BASE_DECIMAL:
	case ACTF_BASE_HEXADECIMAL:
		*base = ubase;
		return ACTF_OK;
	default:
		eprintf(e, "%s has an invalid base %" PRIu64, key, ubase);
		return ACTF_INVALID_BASE;
	}
}

static int int_fld_cls_parse(struct json_object *fc_jobj, bool is_signed,
			     struct int_fld_cls *int_fc, struct error *e)
{
	int rc;
	enum actf_base pref_display_base = DEFAULT_DISPLAY_BASE;
	if ((rc =
	     json_object_key_get_base(fc_jobj, "preferred-display-base", &pref_display_base, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	}

	struct mappings raw_maps;
	CLEAR(raw_maps);
	enum rng_type rtype = is_signed ? RNG_SINT : RNG_UINT;
	if ((rc = json_object_key_get_mappings(fc_jobj, "mappings", rtype, &raw_maps, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	struct actf_mappings maps;
	CLEAR(maps);
	rc = actf_mappings_init(&raw_maps, &maps);
	if (rc < 0) {
		eprintf(e, "initialize ctf mappings");
		mappings_free(&raw_maps);
		return rc;
	}

	int_fc->pref_display_base = pref_display_base;
	int_fc->maps = maps;
	return ACTF_OK;
}

static void int_fld_cls_free(struct int_fld_cls *int_fc)
{
	if (!int_fc) {
		return;
	}
	actf_mappings_free(&int_fc->maps);
}

static int int_fxd_len_int_fld_cls_parse(struct json_object *fc_jobj, bool is_signed,
					 struct fxd_len_int_fld_cls *fxd_len_int, struct error *e)
{
	int rc;
	struct int_fld_cls base;
	CLEAR(base);
	if ((rc = int_fld_cls_parse(fc_jobj, is_signed, &base, e)) < 0) {
		return rc;
	}
	struct fxd_len_bit_arr_fld_cls bit_arr;
	CLEAR(bit_arr);
	if ((rc = int_fxd_len_bit_arr_fld_cls_parse(fc_jobj, &bit_arr, e)) < 0) {
		int_fld_cls_free(&base);
		return rc;
	}
	enum actf_role roles = 0;
	if (!is_signed) {
		if ((rc = json_object_get_roles(fc_jobj, &roles, e)) < 0) {
			int_fld_cls_free(&base);
			fxd_len_bit_arr_fld_cls_free(&bit_arr);
			return rc;
		}
	}
	fxd_len_int->base = base;
	fxd_len_int->bit_arr = bit_arr;
	fxd_len_int->roles = roles;
	return ACTF_OK;
}

static int fxd_len_int_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e)
{
	struct fxd_len_int_fld_cls *fxd_len_int = &fc->cls.fxd_len_int;
	bool is_signed = fc->type == ACTF_FLD_CLS_FXD_LEN_SINT;
	return int_fxd_len_int_fld_cls_parse(fc_jobj, is_signed, fxd_len_int, e);
}

static void fxd_len_sint_fld_cls_free(struct fxd_len_int_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	int_fld_cls_free(&fc->base);
	fxd_len_bit_arr_fld_cls_free(&fc->bit_arr);
}

static void fxd_len_uint_fld_cls_free(struct fxd_len_int_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	int_fld_cls_free(&fc->base);
	fxd_len_bit_arr_fld_cls_free(&fc->bit_arr);
}

static int fxd_len_bool_fld_cls_parse(struct json_object *fc_jobj,
				      struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct fxd_len_bool_fld_cls *fxd_len_bool = &fc->cls.fxd_len_bool;
	if ((rc = int_fxd_len_bit_arr_fld_cls_parse(fc_jobj, &fxd_len_bool->bit_arr, e)) < 0) {
		return rc;
	}
	return ACTF_OK;
}

static void fxd_len_bool_fld_cls_free(struct fxd_len_bool_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	fxd_len_bit_arr_fld_cls_free(&fc->bit_arr);
}

static int fxd_len_float_fld_cls_parse(struct json_object *fc_jobj,
				       struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct fxd_len_float_fld_cls *fxd_len_float = &fc->cls.fxd_len_float;
	if ((rc = int_fxd_len_bit_arr_fld_cls_parse(fc_jobj, &fxd_len_float->bit_arr, e)) < 0) {
		return rc;
	}
	switch (fxd_len_float->bit_arr.len) {
	case 16:
	case 32:
	case 64:
		return ACTF_OK;
	default:
		eprintf(e, "length \"%" PRIu64 "\" is not supported", fxd_len_float->bit_arr.len);
		return ACTF_UNSUPPORTED_LENGTH;
	}
}

static void fxd_len_float_fld_cls_free(struct fxd_len_float_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	fxd_len_bit_arr_fld_cls_free(&fc->bit_arr);
}

static int int_var_len_int_fld_cls_parse(struct json_object *fc_jobj, bool is_signed,
					 struct var_len_int_fld_cls *var_len_int, struct error *e)
{
	int rc;
	struct int_fld_cls base;
	CLEAR(base);
	if ((rc = int_fld_cls_parse(fc_jobj, is_signed, &base, e)) < 0) {
		return rc;
	}
	enum actf_role roles = 0;
	if (!is_signed) {
		if ((rc = json_object_get_roles(fc_jobj, &roles, e)) < 0) {
			int_fld_cls_free(&base);
			return rc;
		}
	}
	var_len_int->base = base;
	var_len_int->roles = roles;
	return ACTF_OK;
}

static int var_len_int_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e)
{
	struct var_len_int_fld_cls *var_len_int = &fc->cls.var_len_int;
	bool is_signed = (fc->type == ACTF_FLD_CLS_VAR_LEN_SINT);
	return int_var_len_int_fld_cls_parse(fc_jobj, is_signed, var_len_int, e);
}

static void var_len_sint_fld_cls_free(struct var_len_int_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	int_fld_cls_free(&fc->base);
}

static void var_len_uint_fld_cls_free(struct var_len_int_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	int_fld_cls_free(&fc->base);
}

static int actf_encoding_name_to_type(const char *name, enum actf_encoding *type)
{
	for (int i = 0; i < ACTF_N_ENCODINGS; i++) {
		const char *enc_name = actf_encoding_to_name(i);
		if (enc_name && strcmp(name, enc_name) == 0) {
			*type = i;
			return ACTF_OK;
		}
	}
	return ACTF_NOT_FOUND;
}

static int str_fld_cls_parse(struct json_object *fc_jobj,
			     struct str_fld_cls *str_fc, struct error *e)
{
	int rc;
	enum actf_encoding enc = DEFAULT_ENCODING;
	const char *encoding_name;
	if ((rc = json_object_key_get_string(fc_jobj, "encoding", &encoding_name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_OK) {
		if (actf_encoding_name_to_type(encoding_name, &enc) < 0) {
			eprintf(e, "%s is not a valid encoding", encoding_name);
			return ACTF_INVALID_ENCODING;
		}
	}
	str_fc->enc = enc;
	return ACTF_OK;
}

static void str_fld_cls_free(struct str_fld_cls *str_fc)
{
	if (!str_fc) {
		return;
	}
}

static int null_term_str_fld_cls_parse(struct json_object *fc_jobj,
				       struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct null_term_str_fld_cls *null_term_str = &fc->cls.null_term_str;
	struct str_fld_cls base;
	if ((rc = str_fld_cls_parse(fc_jobj, &base, e)) < 0) {
		return rc;
	}
	null_term_str->base = base;
	return ACTF_OK;
}

static void null_term_str_fld_cls_free(struct null_term_str_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	str_fld_cls_free(&fc->base);
}

static int static_len_str_fld_cls_parse(struct json_object *fc_jobj,
					struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct static_len_str_fld_cls *static_len_str = &fc->cls.static_len_str;
	struct str_fld_cls base;
	if ((rc = str_fld_cls_parse(fc_jobj, &base, e)) < 0) {
		return rc;
	}
	uint64_t len;
	rc = json_object_key_get_gtez_uint64(fc_jobj, "length", &len, e);
	if ((rc = json_object_key_get_gtez_uint64(fc_jobj, "length", &len, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("length", "static-length-string field class", e);
		return ACTF_MISSING_PROPERTY;
	}
	static_len_str->base = base;
	static_len_str->len = len;
	return ACTF_OK;
}

static void static_len_str_fld_cls_free(struct static_len_str_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	str_fld_cls_free(&fc->base);
}

static int dyn_len_str_fld_cls_parse(struct json_object *fc_jobj,
				     struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct dyn_len_str_fld_cls *dyn_len_str = &fc->cls.dyn_len_str;

	struct str_fld_cls base;
	if ((rc = str_fld_cls_parse(fc_jobj, &base, e)) < 0) {
		return rc;
	}
	struct actf_fld_loc len_fld_loc;
	if ((rc =
	     json_object_key_get_fld_loc(fc_jobj, "length-field-location", &len_fld_loc, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("length-field-location", "dynamic-length-string field class", e);
		return ACTF_MISSING_PROPERTY;
	}

	dyn_len_str->base = base;
	dyn_len_str->len_fld_loc = len_fld_loc;
	return ACTF_OK;
}

static void dyn_len_str_fld_cls_free(struct dyn_len_str_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	str_fld_cls_free(&fc->base);
	actf_fld_loc_free(&fc->len_fld_loc);
}

static int static_len_blob_fld_cls_parse(struct json_object *fc_jobj,
					 struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct static_len_blob_fld_cls *static_len_blob = &fc->cls.static_len_blob;
	uint64_t len;
	if ((rc = json_object_key_get_gtez_uint64(fc_jobj, "length", &len, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("length", "static-length-blob field class", e);
		return ACTF_MISSING_PROPERTY;
	}
	const char *media_type = DEFAULT_MEDIA_TYPE;
	if ((rc = json_object_key_get_string(fc_jobj, "media-type", &media_type, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	enum actf_role roles = 0;
	if ((rc = json_object_get_roles(fc_jobj, &roles, e)) < 0) {
		return rc;
	}
	static_len_blob->len = len;
	static_len_blob->media_type = media_type ? c_strdup(media_type) : NULL;
	static_len_blob->roles = roles;
	return ACTF_OK;
}

static void static_len_blob_fld_cls_free(struct static_len_blob_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	free(fc->media_type);
}

static int dyn_len_blob_fld_cls_parse(struct json_object *fc_jobj,
				      struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct dyn_len_blob_fld_cls *dyn_len_blob = &fc->cls.dyn_len_blob;

	const char *media_type = DEFAULT_MEDIA_TYPE;
	if ((rc = json_object_key_get_string(fc_jobj, "media-type", &media_type, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	struct actf_fld_loc len_fld_loc;
	if ((rc =
	     json_object_key_get_fld_loc(fc_jobj, "length-field-location", &len_fld_loc, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("length-field-location", "dynamic-length-blob field class", e);
		return ACTF_MISSING_PROPERTY;
	}

	dyn_len_blob->len_fld_loc = len_fld_loc;
	dyn_len_blob->media_type = c_strdup(media_type);
	return ACTF_OK;
}

static void dyn_len_blob_fld_cls_free(struct dyn_len_blob_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	actf_fld_loc_free(&fc->len_fld_loc);
	free(fc->media_type);
}

static int variant_fld_cls_opt_parse(struct json_object *opt_jobj,
				     const struct actf_metadata *metadata,
				     struct variant_fld_cls_opt *opt, struct error *e)
{
	int rc;
	if (!json_object_is_type(opt_jobj, json_type_object)) {
		print_wrong_json_type("variant field class option", json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	struct json_object *fc_jobj;
	if (!json_object_object_get_ex(opt_jobj, "field-class", &fc_jobj)) {
		print_missing_key("field-class", "variant field class option", e);
		return ACTF_MISSING_PROPERTY;
	}
	struct actf_fld_cls fc;
	if ((rc = actf_fld_cls_parse(fc_jobj, metadata, &fc, e)) < 0) {
		return rc;
	}
	struct actf_rng_set sel_fld_rng_set;
	if ((rc =
	     json_object_key_get_rng_set(opt_jobj, "selector-field-ranges", &sel_fld_rng_set,
					 e)) < 0 && rc != ACTF_NOT_FOUND) {
		actf_fld_cls_free(&fc);
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("selector-field-ranges", "variant field class option", e);
		actf_fld_cls_free(&fc);
		return ACTF_MISSING_PROPERTY;
	}
	const char *name = NULL;
	if ((rc = json_object_key_get_string(opt_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		actf_rng_set_free(&sel_fld_rng_set);
		actf_fld_cls_free(&fc);
		return rc;
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(opt_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		eprependf(e, "attributes of variant field class option");
		actf_rng_set_free(&sel_fld_rng_set);
		actf_fld_cls_free(&fc);
		return rc;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(opt_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		eprependf(e, "extensions of variant field class option");
		ctfjson_free(attributes);
		actf_rng_set_free(&sel_fld_rng_set);
		actf_fld_cls_free(&fc);
		return rc;
	}

	opt->fc = fc;
	opt->sel_fld_rng_set = sel_fld_rng_set;
	opt->name = name ? c_strdup(name) : NULL;
	opt->attributes = attributes;
	opt->extensions = extensions;
	return ACTF_OK;
}

static void variant_fld_cls_opt_free(struct variant_fld_cls_opt *opt)
{
	if (!opt) {
		return;
	}
	actf_fld_cls_free(&opt->fc);
	actf_rng_set_free(&opt->sel_fld_rng_set);
	free(opt->name);
	ctfjson_free(opt->attributes);
	ctfjson_free(opt->extensions);
}

static void struct_fld_member_cls_free(struct struct_fld_member_cls *mc);

static int struct_fld_cls_parse_member_clses(struct json_object *member_clses_jobj,
					     const struct actf_metadata *metadata,
					     struct struct_fld_member_cls **clses_out,
					     size_t *n_members, struct error *e)
{
	int rc;
	if (!json_object_is_type(member_clses_jobj, json_type_array)) {
		eprintf(e, "member-classes is not an array");
		return ACTF_JSON_WRONG_TYPE;
	}
	size_t len = json_object_array_length(member_clses_jobj);
	struct struct_fld_member_cls *clses = malloc(len * sizeof(*clses));
	if (!clses) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	size_t i;
	for (i = 0; i < len; i++) {
		struct struct_fld_member_cls *member_cls = &clses[i];
		struct json_object *member_cls_jobj =
		    json_object_array_get_idx(member_clses_jobj, i);
		if (!json_object_is_type(member_cls_jobj, json_type_object)) {
			eprintf(e, "member-class is not an object");
			rc = ACTF_JSON_WRONG_TYPE;
			goto err;
		}
		const char *name;
		if ((rc = json_object_key_get_string(member_cls_jobj, "name", &name, e)) < 0 &&
		    rc != ACTF_NOT_FOUND) {
			goto err;
		} else if (rc == ACTF_NOT_FOUND) {
			print_missing_key("name", "structure member class", e);
			rc = ACTF_MISSING_PROPERTY;
			goto err;
		}
		struct json_object *fc_jobj;
		if (!json_object_object_get_ex(member_cls_jobj, "field-class", &fc_jobj)) {
			eprintf(e,
				"required key field-class is not available in structure member %s",
				name);
			rc = ACTF_MISSING_PROPERTY;
			goto err;
		}
		struct actf_fld_cls fc;
		if ((rc = actf_fld_cls_parse(fc_jobj, metadata, &fc, e)) < 0) {
			eprependf(e, "field-class of structure member %s", name);
			goto err;
		}
		struct ctfjson *attributes = NULL;
		if ((rc =
		     json_object_key_get_ctfjson(member_cls_jobj, "attributes", &attributes, e)) < 0
		    && rc != ACTF_NOT_FOUND) {
			eprependf(e, "attributes of structure member %s", name);
			actf_fld_cls_free(&fc);
			goto err;
		}
		struct ctfjson *extensions = NULL;
		if ((rc =
		     json_object_key_get_ctfjson(member_cls_jobj, "extensions", &extensions, e)) < 0
		    && rc != ACTF_NOT_FOUND) {
			eprependf(e, "extensions of structure member %s", name);
			ctfjson_free(attributes);
			actf_fld_cls_free(&fc);
			goto err;
		}
		member_cls->cls = fc;
		member_cls->name = c_strdup(name);
		member_cls->attributes = attributes;
		member_cls->extensions = extensions;
	}

	*clses_out = clses;
	*n_members = len;
	return ACTF_OK;

err:
	for (size_t j = 0; j < i; j++) {
		struct_fld_member_cls_free(&clses[j]);
	}
	free(clses);
	return rc;
}

/* Frees the content of the provided member class. */
static void struct_fld_member_cls_free(struct struct_fld_member_cls *mc)
{
	if (!mc) {
		return;
	}
	free(mc->name);
	actf_fld_cls_free(&mc->cls);
	ctfjson_free(mc->attributes);
	ctfjson_free(mc->extensions);
}

static int struct_fld_cls_parse(struct json_object *struct_fc_jobj,
				const struct actf_metadata *metadata,
				struct actf_fld_cls *struct_fc, struct error *e)
{
	int rc;
	uint64_t min_align = DEFAULT_ALIGNMENT;
	if ((rc =
	     json_object_key_get_alignment(struct_fc_jobj, "minimum-alignment", &min_align, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	}

	struct struct_fld_member_cls *clses = NULL;
	size_t n_members = 0;
	struct json_object *member_clses_jobj;
	if (json_object_object_get_ex(struct_fc_jobj, "member-classes", &member_clses_jobj)) {
		if ((rc = struct_fld_cls_parse_member_clses(member_clses_jobj, metadata,
							    &clses, &n_members, e)) < 0) {
			eprependf(e, "member-classes of structure");
			return rc;
		}
	}

	/* Precalculate align which is the maximum value of:
	   - The value 'minimum-alignment'
	   - The alignment requirements of the instances of the field-class property
	   of each member class of the member-classes property.
	 */
	size_t align = min_align;
	for (size_t i = 0; i < n_members; i++) {
		size_t member_align = actf_fld_cls_get_align_req(&clses[i].cls);
		align = MAX(member_align, align);
	}

	struct_fc->cls.struct_.min_align = min_align;
	struct_fc->cls.struct_.align = align;
	struct_fc->cls.struct_.member_clses = clses;
	struct_fc->cls.struct_.n_members = n_members;
	return ACTF_OK;
}

static void struct_fld_cls_free(struct struct_fld_cls *struct_fc)
{
	if (!struct_fc) {
		return;
	}
	if (struct_fc->member_clses) {
		for (size_t i = 0; i < struct_fc->n_members; i++) {
			struct_fld_member_cls_free(&struct_fc->member_clses[i]);
		}
		free(struct_fc->member_clses);
	}
}

static int arr_fld_cls_parse(struct json_object *fc_jobj,
			     const struct actf_metadata *metadata,
			     struct arr_fld_cls *arr_fc, struct error *e)
{
	int rc;
	uint64_t min_align = DEFAULT_ALIGNMENT;
	if ((rc = json_object_key_get_alignment(fc_jobj, "minimum-alignment", &min_align, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	struct json_object *ele_fc_jobj;
	if (!json_object_object_get_ex(fc_jobj, "element-field-class", &ele_fc_jobj)) {
		print_missing_key("element-field-class", "array field class", e);
		return ACTF_MISSING_PROPERTY;
	}
	struct actf_fld_cls ele_fc;
	if ((rc = actf_fld_cls_parse(ele_fc_jobj, metadata, &ele_fc, e)) < 0) {
		return rc;
	}

	arr_fc->min_align = min_align;
	arr_fc->ele_fld_cls = malloc(sizeof(*arr_fc->ele_fld_cls));
	*arr_fc->ele_fld_cls = ele_fc;
	return ACTF_OK;
}

static void arr_fld_cls_free(struct arr_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	actf_fld_cls_free(fc->ele_fld_cls);
	free(fc->ele_fld_cls);
}

static int static_len_arr_fld_cls_parse(struct json_object *fc_jobj,
					const struct actf_metadata *metadata,
					struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct static_len_arr_fld_cls *static_len_arr = &fc->cls.static_len_arr;

	struct arr_fld_cls base;
	if ((rc = arr_fld_cls_parse(fc_jobj, metadata, &base, e)) < 0) {
		return rc;
	}
	uint64_t len;
	if ((rc = json_object_key_get_gtez_uint64(fc_jobj, "length", &len, e)) < 0) {
		arr_fld_cls_free(&base);
		return rc;
	}

	static_len_arr->base = base;
	static_len_arr->len = len;
	return ACTF_OK;
}

static void static_len_arr_fld_cls_free(struct static_len_arr_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	arr_fld_cls_free(&fc->base);
}

static int dyn_len_arr_fld_cls_parse(struct json_object *fc_jobj,
				     const struct actf_metadata *metadata,
				     struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct dyn_len_arr_fld_cls *dyn_len_arr = &fc->cls.dyn_len_arr;

	struct arr_fld_cls base;
	if ((rc = arr_fld_cls_parse(fc_jobj, metadata, &base, e)) < 0) {
		return rc;
	}
	struct actf_fld_loc len_fld_loc;
	if ((rc =
	     json_object_key_get_fld_loc(fc_jobj, "length-field-location", &len_fld_loc, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		arr_fld_cls_free(&base);
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("length-field-location", "dynamic-length-array field class", e);
		arr_fld_cls_free(&base);
		return ACTF_MISSING_PROPERTY;
	}

	dyn_len_arr->base = base;
	dyn_len_arr->len_fld_loc = len_fld_loc;
	return ACTF_OK;
}

static void dyn_len_arr_fld_cls_free(struct dyn_len_arr_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	arr_fld_cls_free(&fc->base);
	actf_fld_loc_free(&fc->len_fld_loc);
}

static int optional_fld_cls_parse(struct json_object *fc_jobj,
				  const struct actf_metadata *metadata,
				  struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct optional_fld_cls *optional = &fc->cls.optional;
	struct json_object *opt_fc_jobj;
	if (!json_object_object_get_ex(fc_jobj, "field-class", &opt_fc_jobj)) {
		print_missing_key("field-class", "optional field class", e);
		return ACTF_MISSING_PROPERTY;
	}
	struct actf_fld_cls opt_fc;
	if ((rc = actf_fld_cls_parse(opt_fc_jobj, metadata, &opt_fc, e)) < 0) {
		return rc;
	}

	struct actf_fld_loc sel_fld_loc;
	if ((rc =
	     json_object_key_get_fld_loc(fc_jobj, "selector-field-location", &sel_fld_loc, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		actf_fld_cls_free(&opt_fc);
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("selector-field-location", "optional field class", e);
		actf_fld_cls_free(&opt_fc);
		return ACTF_MISSING_PROPERTY;
	}

	/* This field is not mandatory if the `sel_fld_loc` points to a
	 * fxd_len_bool, but we do not know that by know (the metadata
	 * available here won't have partially parsed data from the
	 * current actf_fld_cls.)
	 */
	struct actf_rng_set sel_fld_rng_set;
	CLEAR(sel_fld_rng_set);
	if ((rc =
	     json_object_key_get_rng_set(fc_jobj, "selector-field-ranges", &sel_fld_rng_set, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		eprependf(e, "selector-field-ranges of optional field-class");
		actf_fld_loc_free(&sel_fld_loc);
		actf_fld_cls_free(&opt_fc);
		return rc;
	}

	optional->fld_cls = malloc(sizeof(*optional->fld_cls));	// check?
	*optional->fld_cls = opt_fc;
	optional->sel_fld_loc = sel_fld_loc;
	optional->sel_fld_rng_set = sel_fld_rng_set;
	return ACTF_OK;
}

static void optional_fld_cls_free(struct optional_fld_cls *fc)
{
	if (!fc) {
		return;
	}
	actf_fld_cls_free(fc->fld_cls);
	free(fc->fld_cls);
	actf_fld_loc_free(&fc->sel_fld_loc);
	actf_rng_set_free(&fc->sel_fld_rng_set);
}

static int variant_fld_cls_parse(struct json_object *fc_jobj,
				 const struct actf_metadata *metadata,
				 struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	struct variant_fld_cls *variant = &fc->cls.variant;

	struct variant_fld_cls_opt *opts;
	struct json_object *opts_jobj;
	if (!json_object_object_get_ex(fc_jobj, "options", &opts_jobj)) {
		print_missing_key("options", "variant field class", e);
		return ACTF_MISSING_PROPERTY;
	}
	if (!json_object_is_type(opts_jobj, json_type_array)) {
		print_wrong_json_type("options", json_type_array, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	size_t n_opts = json_object_array_length(opts_jobj);
	if (!n_opts) {
		eprintf(e, "options in variant field class has no elements");
		return ACTF_INVALID_VARIANT;
	}
	opts = malloc(n_opts * sizeof(*opts));
	if (!opts) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	size_t i;
	for (i = 0; i < n_opts; i++) {
		struct json_object *option_jobj = json_object_array_get_idx(opts_jobj, i);
		if ((rc = variant_fld_cls_opt_parse(option_jobj, metadata, &opts[i], e)) < 0) {
			goto err;
		}
	}

	/* The selector-field-ranges of all opts MUST NOT intersect */
	for (size_t j = 0; j < n_opts; j++) {
		struct variant_fld_cls_opt *opt_a = &opts[j];
		for (size_t k = j + 1; k < n_opts; k++) {
			if (j == k) {
				continue;
			}
			struct variant_fld_cls_opt *opt_b = &opts[k];
			if (actf_rng_set_intersect_rng_set(
				    &opt_a->sel_fld_rng_set, &opt_b->sel_fld_rng_set)) {
				eprintf(e,
					"the selector-field-ranges of two variant options intersect");
				rc = ACTF_INVALID_VARIANT;
				goto err;
			}
		}
	}

	/* The types of all possible selector fields must either be
	 * unsigned, signed or boolean. We cannot determine that here, so
	 * the "true" signedness of the selector ranges is unknown until
	 * we actually decode them. */
	struct actf_fld_loc sel_fld_loc;
	if ((rc =
	     json_object_key_get_fld_loc(fc_jobj, "selector-field-location", &sel_fld_loc, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("selector-field-location", "variant field class", e);
		return ACTF_MISSING_PROPERTY;
	}

	variant->opts = opts;
	variant->n_opts = n_opts;
	variant->sel_fld_loc = sel_fld_loc;
	return ACTF_OK;

err:
	for (size_t j = 0; j < i; j++) {
		variant_fld_cls_opt_free(&opts[j]);
	}
	free(opts);
	return rc;
}

static void variant_fld_cls_free(struct variant_fld_cls *fc)
{
	if (!fc) {
		return;
	}

	for (size_t i = 0; i < fc->n_opts; i++) {
		variant_fld_cls_opt_free(&fc->opts[i]);
	}
	free(fc->opts);
	actf_fld_loc_free(&fc->sel_fld_loc);
}
