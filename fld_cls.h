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
 * Field class related methods
 */
#ifndef ACTF_FLD_CLS_H
#define ACTF_FLD_CLS_H

#include "flags.h"
#include "fld_loc.h"
#include "mappings.h"
#include "rng.h"
#include "types.h"


/** The possible roles of a field class. A field class can have
 * multiple roles ORed together. */
enum actf_role {
	ACTF_ROLE_NIL = 0,
	ACTF_ROLE_DSTREAM_CLS_ID = (1 << 0),
	ACTF_ROLE_DSTREAM_ID = (1 << 1),
	ACTF_ROLE_PKT_MAGIC_NUM = (1 << 2),
	ACTF_ROLE_METADATA_STREAM_UUID = (1 << 3),
	ACTF_ROLE_DEF_CLK_TSTAMP = (1 << 4),
	ACTF_ROLE_DISC_EVENT_CNT_SNAPSHOT = (1 << 5),
	ACTF_ROLE_PKT_CONTENT_LEN  = (1 << 6),
	ACTF_ROLE_PKT_END_DEF_CLK_TSTAMP  = (1 << 7),
	ACTF_ROLE_PKT_SEQ_NUM = (1 << 8),
	ACTF_ROLE_PKT_TOT_LEN = (1 << 9),
	ACTF_ROLE_EVENT_CLS_ID = (1 << 10),
};

/** A field class type. */
enum actf_fld_cls_type {
	ACTF_FLD_CLS_NIL,
	ACTF_FLD_CLS_FXD_LEN_BIT_ARR,
	ACTF_FLD_CLS_FXD_LEN_BIT_MAP,
	ACTF_FLD_CLS_FXD_LEN_UINT,
	ACTF_FLD_CLS_FXD_LEN_SINT,
	ACTF_FLD_CLS_FXD_LEN_BOOL,
	ACTF_FLD_CLS_FXD_LEN_FLOAT,
	ACTF_FLD_CLS_VAR_LEN_UINT,
	ACTF_FLD_CLS_VAR_LEN_SINT,
	ACTF_FLD_CLS_NULL_TERM_STR,
	ACTF_FLD_CLS_STATIC_LEN_STR,
	ACTF_FLD_CLS_DYN_LEN_STR,
	ACTF_FLD_CLS_STATIC_LEN_BLOB,
	ACTF_FLD_CLS_DYN_LEN_BLOB,
	ACTF_FLD_CLS_STRUCT,
	ACTF_FLD_CLS_STATIC_LEN_ARR,
	ACTF_FLD_CLS_DYN_LEN_ARR,
	ACTF_FLD_CLS_OPTIONAL,
	ACTF_FLD_CLS_VARIANT,
	ACTF_N_FLD_CLSES,
};

/**
 * Get the type of a field class
 * @param fc the field class
 * @return the type
 */
enum actf_fld_cls_type actf_fld_cls_type(const actf_fld_cls *fc);

/**
 * Get the length property of the field class.
 *
 * The following field classes return length in bits:
 * - ACTF_FLD_CLS_FXD_LEN_BIT_ARR
 * - ACTF_FLD_CLS_FXD_LEN_BIT_MAP
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_FXD_LEN_BOOL
 * - ACTF_FLD_CLS_FXD_LEN_FLOAT
 *
 * The following field classes return length in bytes:
 * - ACTF_FLD_CLS_STATIC_LEN_BLOB
 * - ACTF_FLD_CLS_STATIC_LEN_STR
 * - ACTF_FLD_CLS_STATIC_LEN_ARR
 *
 * Any other field class returns 0.
 *
 * @param fc the field class
 * @return the length
 */
size_t actf_fld_cls_len(const actf_fld_cls *fc);

/**
 * Get the byte order property of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_BIT_ARR
 * - ACTF_FLD_CLS_FXD_LEN_BIT_MAP
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_FXD_LEN_BOOL
 * - ACTF_FLD_CLS_FXD_LEN_FLOAT
 *
 * Any other field class returns ACTF_LIL_ENDIAN.
 *
 * @param fc the field class
 * @return the byte order
 */
enum actf_byte_order actf_fld_cls_byte_order(const actf_fld_cls *fc);

/**
 * Get the bit order property of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_BIT_ARR
 * - ACTF_FLD_CLS_FXD_LEN_BIT_MAP
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_FXD_LEN_BOOL
 * - ACTF_FLD_CLS_FXD_LEN_FLOAT
 *
 * Any other field class returns ACTF_FIRST_TO_LAST.
 *
 * @param fc the field class
 * @return the bit order
 */
enum actf_bit_order actf_fld_cls_bit_order(const actf_fld_cls *fc);

/**
 * Get the alignment property of the field class. This is not the same
 * as the effective alignment requirement of a field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_BIT_ARR
 * - ACTF_FLD_CLS_FXD_LEN_BIT_MAP
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_FXD_LEN_BOOL
 * - ACTF_FLD_CLS_FXD_LEN_FLOAT
 *
 * Any other field class returns 0.
 *
 * @param fc the field class
 * @return the alignment
 */
size_t actf_fld_cls_alignment(const actf_fld_cls *fc);

/**
 * Get the flags of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_BIT_MAP
 *
 * Any other field class returns NULL.

 * @param fc the field class
 * @return the flags
 */
const actf_flags *actf_fld_cls_bit_map_flags(const actf_fld_cls *fc);

/**
 * Get the preferred display base of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_VAR_LEN_UINT
 * - ACTF_FLD_CLS_VAR_LEN_SINT
 *
 * Any other field class returns ACTF_BASE_DECIMAL.
 *
 * @param fc the field class
 * @return the preferred display base
 */
enum actf_base actf_fld_cls_pref_display_base(const actf_fld_cls *fc);

/**
 * Get any available mappings of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_SINT
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_VAR_LEN_SINT
 * - ACTF_FLD_CLS_VAR_LEN_UINT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the mappings
 */
const actf_mappings *actf_fld_cls_mappings(const actf_fld_cls *fc);

/**
 * Get the roles of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_FXD_LEN_UINT
 * - ACTF_FLD_CLS_STATIC_LEN_BLOB
 * - ACTF_FLD_CLS_VAR_LEN_UINT
 *
 * Any other field class returns ACTF_ROLE_NIL.
 *
 * @param fc the field class
 * @return the roles
 */
enum actf_role actf_fld_cls_roles(const actf_fld_cls *fc);

/**
 * Get the character encoding of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_NULL_TERM_STR
 * - ACTF_FLD_CLS_STATIC_LEN_STR
 * - ACTF_FLD_CLS_DYN_LEN_STR
 *
 * Any other field class returns ACTF_ENCODING_UTF8.
 *
 * @param fc the field class
 * @return the character encoding
 */
enum actf_encoding actf_fld_cls_encoding(const actf_fld_cls *fc);

/**
 * Get the IANA media type of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STATIC_LEN_BLOB
 * - ACTF_FLD_CLS_DYN_LEN_BLOB
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the media type
 */
const char *actf_fld_cls_media_type(const actf_fld_cls *fc);

/**
 * Get the field location denoting length of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_DYN_LEN_STR
 * - ACTF_FLD_CLS_DYN_LEN_BLOB
 * - ACTF_FLD_CLS_DYN_LEN_ARR
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the length field location
 */
const actf_fld_loc *actf_fld_cls_len_fld_loc(const actf_fld_cls *fc);

/**
 * Get the minimum alignment property of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 * - ACTF_FLD_CLS_STATIC_LEN_ARR
 * - ACTF_FLD_CLS_DYN_LEN_ARR
 *
 * Any other field class returns 0.
 *
 * @param fc the field class
 * @return the minimum alignment
 */
uint64_t actf_fld_cls_min_alignment(const actf_fld_cls *fc);

/**
 * Get the number of members of the struct field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 *
 * Any other field class returns 0.
 *
 * @param fc the field class
 * @return the number of members
 */
size_t actf_fld_cls_members_len(const actf_fld_cls *fc);

/**
 * Get the name of the member with the specified index of the struct
 * field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the member name
 */
const char *actf_fld_cls_members_name_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the field class of the member with the specified index of the
 * struct field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the member field class
 */
const actf_fld_cls *actf_fld_cls_members_fld_cls_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the attributes of the member with the specified index of the
 * struct field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the member attributes
 */
const actf_fld *actf_fld_cls_members_attributes_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the extensions of the member with the specified index of the
 * struct field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STRUCT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the member extensions
 */
const actf_fld *actf_fld_cls_members_extensions_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the element field class of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_STATIC_LEN_ARR
 * - ACTF_FLD_CLS_DYN_LEN_ARR
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the element field class
 */
const actf_fld_cls *actf_fld_cls_element_fld_cls(const actf_fld_cls *fc);

/**
 * Get the field location denoting selector field of the field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_OPTIONAL
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the selector field location
 */
const actf_fld_loc *actf_fld_cls_selector_fld_loc(const actf_fld_cls *fc);

/**
 * Get the selector ranges which determine if an optional field is
 * enabled or not when the selector field is an integer. The range set
 * will be empty if the selector field is a boolean.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_OPTIONAL
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @return the selector range set
 */
const actf_rng_set *actf_fld_cls_selector_rng_set(const actf_fld_cls *fc);

/**
 * Get a the conditional field class of an optional field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_OPTIONAL
 *
 * Any other field class returns NULL.
 * @param fc the field class
 * @return the optional field class
 */
const actf_fld_cls *actf_fld_cls_optional_fld_cls(const actf_fld_cls *fc);

/**
 * Get the number of options of the variant field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns 0.
 *
 * @param fc the field class
 * @return the number of options
 */
size_t actf_fld_cls_options_len(const actf_fld_cls *fc);

/**
 * Get the name of the option with the specified index of the variant
 * field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the option's name
 */
const char *actf_fld_cls_options_name_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the field class of the option with the specified index of the
 * variant field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the option's field class
 */
const actf_fld_cls *actf_fld_cls_options_fld_cls_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the selector range set of the option with the specified index
 * of the variant field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the option's selector range set
 */
const actf_rng_set *actf_fld_cls_variant_options_selector_rng_set_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the attributes of the option with the specified index of the
 * variant field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the option's attributes or NULL
 */
const actf_fld *actf_fld_cls_options_attributes_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the extensions of the option with the specified index of the
 * variant field class.
 *
 * Applicable to:
 * - ACTF_FLD_CLS_VARIANT
 *
 * Any other field class returns NULL.
 *
 * @param fc the field class
 * @param i the index
 * @return the option's extensions or NULL
 */
const actf_fld *actf_fld_cls_options_extensions_idx(const actf_fld_cls *fc, size_t i);

/**
 * Get the alias of the field class. Applicable to ALL field
 * classes.
 *
 * @param fc the field class
 * @return the alias or NULL
 */
const char *actf_fld_cls_aliased_name(const actf_fld_cls *fc);

/**
 * Get the attributes of the field class.
 *
 * Applicable to ALL field classes.
 *
 * @param fc the field class
 * @return the attributes or NULL
 */
const actf_fld *actf_fld_cls_attributes(const actf_fld_cls *fc);

/**
 * Get the extensions of the field class.
 *
 * Applicable to ALL field classes.
 *
 * @param fc the field class
 * @return the extensions or NULL
 */
const actf_fld *actf_fld_cls_extensions(const actf_fld_cls *fc);

#endif /* ACTF_FLD_CLS_H */
