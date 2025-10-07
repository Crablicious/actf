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
 * Basic types used by ACTF.
 */
#ifndef ACTF_TYPES_H
#define ACTF_TYPES_H

#include <stdint.h>


/* Return codes. */
#define ACTF_OK                      0 /**< OK */
#define ACTF_ERROR                  -1 /**< Error */
#define ACTF_INTERNAL               -2 /**< Internal logic error */
#define ACTF_OOM                    -3 /**< Failed to allocate memory */
#define ACTF_NOT_FOUND              -4 /**< The queried property does not exist. */
#define ACTF_JSON_PARSE_ERROR       -5 /**< An error occurred when parsing the JSON data. */
#define ACTF_JSON_ERROR             -6 /**< Incompatible JSON content */
#define ACTF_JSON_WRONG_TYPE        -7 /**< A JSON object is of the wrong type. */
#define ACTF_JSON_NOT_GTEZ          -8 /**< A value is not greater than or equal to zero */
#define ACTF_JSON_NOT_GTZ           -9 /**< A value is not greater than zero */
#define ACTF_INVALID_ALIGNMENT     -10 /**< An alignment is not a power of two */
#define ACTF_INVALID_BYTE_ORDER    -11 /**< An invalid byte order is specified */
#define ACTF_INVALID_BIT_ORDER     -12 /**< An invalid bit order is specified */
#define ACTF_INVALID_RANGE         -13 /**< An invalid range is specified */
#define ACTF_INVALID_RANGE_SET     -14 /**< An invalid range set is specified */
#define ACTF_INVALID_UUID          -15 /**< An invalid uuid is specified */
#define ACTF_INVALID_MAPPING       -16 /**< An invalid mapping is specified */
#define ACTF_INVALID_FLD_LOC       -17 /**< An invalid field location is specified */
#define ACTF_INVALID_FLD_CLS       -18 /**< An invalid field class is specified */
#define ACTF_INVALID_FLAGS         -19 /**< An invalid set of bit map field class flags is specified */
#define ACTF_INVALID_ROLE          -20 /**< An invalid role is specified */
#define ACTF_INVALID_BASE          -21 /**< An invalid base is specified */
#define ACTF_UNSUPPORTED_LENGTH    -22 /**< An invalid or unsupported length is specified */
#define ACTF_INVALID_ENCODING      -23 /**< An invalid encoding is specified */
#define ACTF_INVALID_ENVIRONMENT   -24 /**< An invalid environment is specified */
#define ACTF_INVALID_VARIANT       -25 /**< An invalid variant is specified */
#define ACTF_CC_GTE_FREQ_ERROR     -26 /**< The cycle offest is greater than or equal to the frequency of a clock class */
#define ACTF_NO_SUCH_ALIAS         -27 /**< Referring to an alias which does not exist */
#define ACTF_MISSING_PROPERTY      -28 /**< A required property is not available */
#define ACTF_UNSUPPORTED_EXTENSION -29 /**< An extension is enabled which is not supported */
#define ACTF_NO_SUCH_ORIGIN        -30 /**< A clock origin which does not exist is specified */
#define ACTF_NO_DEFAULT_CLOCK      -31 /**< A default-clock-timestamp role is specified without the data-stream having a default clock */
#define ACTF_INVALID_UUID_ROLE     -32 /**< A "metadata-stream-uuid" role is specified, but it is invalid.  */
#define ACTF_INVALID_MAGIC_ROLE    -33 /**< A "packet-magic-number" role is specified, but it is invalid.  */
#define ACTF_NOT_A_STRUCT          -34 /**< A field class which is required to be a struct is in fact not a struct */
#define ACTF_DUPLICATE_ERROR       -35 /**< A duplicate of an id, name or field class is specified in the metadata. */
#define ACTF_NO_SUCH_ID            -36 /**< An id referring to a field class is specified but no such id exists */
#define ACTF_UNSUPPORTED_VERSION   -37 /**< The metadata stream has an unsupported version. */
#define ACTF_NO_PREAMBLE           -38 /**< No preamble is specified. */
#define ACTF_WRONG_FLD_TYPE        -39 /**< Wrong type of field */
#define ACTF_MISSING_FLD_LOC       -40 /**< Field location is not found */
#define ACTF_NOT_ENOUGH_BITS       -41 /**< Trying to read more bits than what's available in the packet */
#define ACTF_MID_BYTE_ENDIAN_SWAP  -42 /**< The byte-order is changed in the middle of a byte */
#define ACTF_INVALID_STR_LEN       -43 /**< A string length is encountered which is not compatible with its encoding */
#define ACTF_MAGIC_MISMATCH        -44 /**< The packet magic number is incorrect */
#define ACTF_UUID_MISMATCH         -45 /**< The data stream UUID does not match the metadata UUID */
#define ACTF_NO_SELECTOR_FLD       -46 /**< A selector field is not found for an optional or variant */
#define ACTF_INVALID_CONTENT_LEN   -47 /**< A packet's content length is larger than total length of packet */
#define ACTF_INVALID_METADATA_PKT  -48 /**< A metadata packet is invalid. */

/**
 * Convert a return code into an error string.
 * @param rc the return code
 * @return the error string
 */
const char *actf_errstr(int rc);

/** A field */
typedef struct actf_fld actf_fld;
/** A field class */
typedef struct actf_fld_cls actf_fld_cls;

/** Byte orders */
enum actf_byte_order {
	ACTF_LIL_ENDIAN,
	ACTF_BIG_ENDIAN,
};

/** Bit orders */
enum actf_bit_order {
	ACTF_FIRST_TO_LAST,
	ACTF_LAST_TO_FIRST,
};

/** Character encodings */
enum actf_encoding {
	ACTF_ENCODING_UTF8,
	ACTF_ENCODING_UTF16BE,
	ACTF_ENCODING_UTF16LE,
	ACTF_ENCODING_UTF32BE,
	ACTF_ENCODING_UTF32LE,
	ACTF_N_ENCODINGS,
};

/** Bases */
enum actf_base {
	ACTF_BASE_BINARY = 2,
	ACTF_BASE_OCTAL = 8,
	ACTF_BASE_DECIMAL = 10,
	ACTF_BASE_HEXADECIMAL = 16,
};

/** Number of bytes in a UUID */
#define ACTF_UUID_N_BYTES 16

/** A UUID */
struct actf_uuid {
	/** UUID data */
	uint8_t d[ACTF_UUID_N_BYTES];
};

/** An iterator. Must be zero-initialized on the first call to an
 * iterator function. */
typedef struct actf_it {
	/** Opaque pointer */
	void *data;
} actf_it;

#endif /* ACTF_TYPES_H */
