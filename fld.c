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
#include <ctype.h>
#include <float.h>
#include <iconv.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fld_cls_int.h"
#include "fld_int.h"

const struct actf_fld_cls *actf_fld_fld_cls(const struct actf_fld *fld)
{
	return fld->cls;
}

enum actf_fld_type actf_fld_type(const struct actf_fld *fld)
{
	return fld->type;
}

const char *actf_fld_type_name(enum actf_fld_type type)
{
	switch (type) {
	case ACTF_FLD_TYPE_NIL:
		return "nil";
	case ACTF_FLD_TYPE_BOOL:
		return "bool";
	case ACTF_FLD_TYPE_SINT:
		return "signed int";
	case ACTF_FLD_TYPE_UINT:
		return "unsigned int";
	case ACTF_FLD_TYPE_BIT_MAP:
		return "bit map";
	case ACTF_FLD_TYPE_REAL:
		return "real";
	case ACTF_FLD_TYPE_STR:
		return "string";
	case ACTF_FLD_TYPE_BLOB:
		return "blob";
	case ACTF_FLD_TYPE_ARR:
		return "array";
	case ACTF_FLD_TYPE_STRUCT:
		return "struct";
	}
	return NULL;
}

bool actf_fld_bool(const struct actf_fld *fld)
{
	switch (fld->type) {
	case ACTF_FLD_TYPE_BOOL:
		return fld->d.bool_.val;
	case ACTF_FLD_TYPE_SINT:
		return !!fld->d.int_.val;
	case ACTF_FLD_TYPE_UINT:
		return !!fld->d.uint.val;
	default:
		return false;
	}
}

uint64_t actf_fld_uint64(const struct actf_fld *fld)
{
	switch (fld->type) {
	case ACTF_FLD_TYPE_BOOL:
		return fld->d.bool_.val;
	case ACTF_FLD_TYPE_SINT:
		if (fld->d.int_.val < 0) {
			return 0;
		}
		return fld->d.int_.val;
	case ACTF_FLD_TYPE_UINT:
		return fld->d.uint.val;
	case ACTF_FLD_TYPE_BIT_MAP:
		return fld->d.bit_map.val;
	default:
		return UINT64_MAX;
	}
}

int64_t actf_fld_int64(const struct actf_fld *fld)
{
	switch (fld->type) {
	case ACTF_FLD_TYPE_BOOL:
		return fld->d.bool_.val;
	case ACTF_FLD_TYPE_SINT:
		return fld->d.int_.val;
	case ACTF_FLD_TYPE_UINT:
		if (fld->d.uint.val >= INT64_MAX) {
			return INT64_MAX;
		}
		return fld->d.uint.val;
	case ACTF_FLD_TYPE_BIT_MAP:
		if (fld->d.bit_map.val >= INT64_MAX) {
			return INT64_MAX;
		}
		return fld->d.bit_map.val;
	default:
		return INT64_MAX;
	}
}

float actf_fld_float(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_REAL || fld->cls->type != ACTF_FLD_CLS_FXD_LEN_FLOAT) {
		return FLT_MAX;
	}
	if (fld->cls->cls.fxd_len_float.bit_arr.len == 32) {
		return fld->d.real.f32;
	} else if (fld->cls->cls.fxd_len_float.bit_arr.len == 64) {
		return fld->d.real.f64;
	} else {
		return FLT_MAX;
	}
}

double actf_fld_double(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_REAL || fld->cls->type != ACTF_FLD_CLS_FXD_LEN_FLOAT) {
		return DBL_MAX;
	}
	if (fld->cls->cls.fxd_len_float.bit_arr.len == 32) {
		return fld->d.real.f32;
	} else if (fld->cls->cls.fxd_len_float.bit_arr.len == 64) {
		return fld->d.real.f64;
	} else {
		return DBL_MAX;
	}
}

const char *actf_fld_str_raw(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_STR) {
		return NULL;
	}
	return (const char *) fld->d.str.ptr;
}

size_t actf_fld_str_sz(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_STR) {
		return 0;
	}
	return fld->d.str.len;
}

const void *actf_fld_blob(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_BLOB) {
		return NULL;
	}
	return fld->d.blob.ptr;
}

size_t actf_fld_blob_sz(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_BLOB) {
		return 0;
	}
	return fld->d.blob.len;
}

size_t actf_fld_arr_len(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_ARR) {
		return 0;
	}
	return fld->d.arr.n_vals;
}

const struct actf_fld *actf_fld_arr_idx(const struct actf_fld *fld, size_t i)
{
	if (fld->type != ACTF_FLD_TYPE_ARR || i >= fld->d.arr.n_vals) {
		return NULL;
	}
	return &fld->d.arr.vals[i];
}

const struct actf_fld *actf_fld_struct_fld(const struct actf_fld *fld, const char *key)
{
	if (fld->type != ACTF_FLD_TYPE_STRUCT || fld->cls->type != ACTF_FLD_CLS_STRUCT) {
		return NULL;
	}
	for (size_t i = 0; i < fld->cls->cls.struct_.n_members; i++) {
		if (strcmp(fld->cls->cls.struct_.member_clses[i].name, key) == 0) {
			return &fld->d.struct_.vals[i];
		}
	}
	return NULL;
}

size_t actf_fld_struct_len(const struct actf_fld *fld)
{
	if (fld->type != ACTF_FLD_TYPE_STRUCT || fld->cls->type != ACTF_FLD_CLS_STRUCT) {
		return 0;
	}
	return fld->cls->cls.struct_.n_members;
}

const struct actf_fld *actf_fld_struct_fld_idx(const struct actf_fld *fld, size_t i)
{
	if (fld->type != ACTF_FLD_TYPE_STRUCT ||
	    fld->cls->type != ACTF_FLD_CLS_STRUCT || i >= fld->cls->cls.struct_.n_members) {
		return NULL;
	}
	return &fld->d.struct_.vals[i];
}

const char *actf_fld_struct_fld_name_idx(const struct actf_fld *fld, size_t i)
{
	if (fld->type != ACTF_FLD_TYPE_STRUCT ||
	    fld->cls->type != ACTF_FLD_CLS_STRUCT || i >= fld->cls->cls.struct_.n_members) {
		return NULL;
	}
	return fld->cls->cls.struct_.member_clses[i].name;
}
