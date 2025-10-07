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

/**
 * @file
 * Field related methods.
 */
#ifndef ACTF_FLD_H
#define ACTF_FLD_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "fld_cls.h"
#include "types.h"

/** Field types */
enum actf_fld_type {
	ACTF_FLD_TYPE_NIL,
	ACTF_FLD_TYPE_BOOL,
	ACTF_FLD_TYPE_SINT,
	ACTF_FLD_TYPE_UINT,
	ACTF_FLD_TYPE_BIT_MAP,
	ACTF_FLD_TYPE_REAL,
	ACTF_FLD_TYPE_STR,
	ACTF_FLD_TYPE_BLOB,
	ACTF_FLD_TYPE_ARR,
	ACTF_FLD_TYPE_STRUCT,
};

/**
 * Get the class of a field
 * @param fld the field
 * @return the field's class
 */
const actf_fld_cls *actf_fld_fld_cls(const actf_fld *fld);

/**
 * Get the type of a field
 * @param fld the field
 * @return the field's type
 */
enum actf_fld_type actf_fld_type(const actf_fld *fld);

/**
 * Get the name of a type
 * @param type the type
 * @return the type's name
 */
const char *actf_fld_type_name(enum actf_fld_type type);

/**
 * Get a bool representation of a field
 *
 * - BOOL  -> Returned as is.
 * - INT   -> Converted to bool (non zero is true)
 * - UINT  -> Converted to bool (non zero is true)
 * - Other -> false
 *
 * @param fld the field
 * @return the bool representation of the field
 */
bool actf_fld_bool(const actf_fld *fld);

/**
 * Get a uint64 representation of the field
 *
 * - BOOL     -> Returned as 0/1
 * - INT      -> Positive values returned as is, negative values returned as 0.
 * - UINT     -> Returned as is.
 * - BIT_MAP  -> Returned as is.
 * - Other    -> UINT64_MAX
 *
 * @param fld the field
 * @return the uint64 representation of the field
 */
uint64_t actf_fld_uint64(const actf_fld *fld);

/**
 * Get an int64 representation of the field
 *
 * - BOOL    -> Returned as 0/1
 * - INT     -> Returned as is.
 * - UINT    -> Truncated to INT64_MAX.
 * - BIT_MAP -> Truncated to INT64_MAX.
 * - Other   -> UINT64_MAX
 *
 * @param fld the field
 * @return the int64 representation of the field
 */
int64_t actf_fld_int64(const actf_fld *fld);

/**
 * Get a float representation of the field
 *
 * - REAL(float)  -> Returned as is.
 * - REAL(double) -> Converted to float and returned.
 * - Other        -> FLT_MAX
 *
 * NOTE: Requires class introspection to know which one is "true" for
 * a field.
 *
 * @param fld the field
 * @return the float representation of the field
 */
float actf_fld_float(const actf_fld *fld);

/**
 * Get a double representation of the field
 *
 * - REAL(float)  -> Converted to double and returned.
 * - REAL(double) -> Returned as is.
 * - Other        -> DBL_MAX
 *
 * NOTE: Requires class introspection to know which one is "true" for
 * a field.
 *
 * @param fld the field
 * @return the double representation of the field
 */
double actf_fld_double(const actf_fld *fld);

/**
 * Get the raw string data representation of a field.
 *
 * - STR   -> Pointer to the start of the string
 * - Other -> NULL
 *
 * The returned data is read directly from the data stream and is NOT
 * guaranteed to be a null-terminated string. The CTF specification
 * puts no requirements on static or dynamic length strings being
 * null-terminated. The encoding of the returned string is determined
 * by the field class, see actf_fld_cls_encoding().
 *
 * Use actf_fld_str_raw() together with actf_fld_str_sz() for
 * thrilling zero-copy action with this footgun.
 *
 * @param fld the string field
 * @return the raw string data of the field
 */
const char *actf_fld_str_raw(const actf_fld *fld);

/**
 * Get the number of bytes the string data of a field occupies,
 * including the null-terminator if it exists.
 *
 * - STR   -> Number of bytes
 * - Other -> 0
 *
 * @param fld the string field
 * @return the size of the raw string data
 */
size_t actf_fld_str_sz(const actf_fld *fld);

/**
 * Get data of a blob field
 *
 * - BLOB  -> Pointer to the start of the blob
 * - Other -> NULL
 *
 * Use actf_fld_blob_sz() to determine the size of the returned data.
 *
 * @param fld the blob field
 * @return the blob data
 */
const void *actf_fld_blob(const actf_fld *fld);

/**
 * Get the number of bytes a blob field occupies
 *
 * - BLOB  -> Number of bytes
 * - Other -> 0
 *
 * @param fld the blob field
 * @return the size of the blob data
 */
size_t actf_fld_blob_sz(const actf_fld *fld);

/**
 * Get the number of elements in an array field
 *
 * - ARR   -> Number of elements in array
 * - Other -> 0
 *
 * @param fld the array field
 * @return the number of elements in the array field
 */
size_t actf_fld_arr_len(const actf_fld *fld);

/**
 * Get the element of a specific index in an array field
 *
 * - ARR   -> Element in index position i if within array bounds, else NULL
 * - Other -> NULL
 *
 * @param fld the array field
 * @param i the index of the element
 * @return the element at the index
 */
const actf_fld *actf_fld_arr_idx(const actf_fld *fld, size_t i);

/**
 * Get the member with a name matching key in a struct field.
 *
 * - STRUCT -> Member with a name matching key
 * - Other  -> NULL
 *
 * @param fld the struct field
 * @param key the member name to find
 * @return the member field
 */
const actf_fld *actf_fld_struct_fld(const actf_fld *fld, const char *key);

/**
 * Get the number of members in a struct field
 *
 * - STRUCT -> Number of elements
 * - Other  -> 0
 *
 * @param fld the struct field
 * @return the number of members in the struct field
 */
size_t actf_fld_struct_len(const actf_fld *fld);

/**
 * Get the i:th member of a struct field
 *
 * - STRUCT -> The member at index i if within struct bounds, else NULL
 * - Other  -> NULL
 *
 * @param fld the struct field
 * @param i the index of the member
 * @return the member at the index
 */
const actf_fld *actf_fld_struct_fld_idx(const actf_fld *fld, size_t i);

/**
 * Get the name of the ith member in a struct field
 *
 * - STRUCT -> The member name at index i if within struct bounds, else NULL
 * - Other  -> NULL
 *
 * @param fld the struct field
 * @param i the index of the member
 * @return the name of the member at the index
 */
const char *actf_fld_struct_fld_name_idx(const actf_fld *fld, size_t i);

#endif /* ACTF_FLD_H */
