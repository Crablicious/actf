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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crust/common.h"
#include "types.h"
#include "error.h"


int error_init(size_t sz, struct error *e)
{
    char *buf = malloc(sz);
    if (! buf) {
	return ACTF_OOM;
    }
    if (sz) {
	buf[0] = '\0';
    }
    e->buf = buf;
    e->sz = sz;
    return ACTF_OK;
}

int eprintf(struct error *e, const char *fmt, ...)
{
    if (! e) {
	return ACTF_OK;
    }
    va_list ap;
    va_start(ap, fmt);
    int sz = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (sz < 0) {
	return ACTF_ERROR;
    }
    sz++;
    if (e->sz < (size_t)sz) {
	size_t newsz = MAX(sz, ERROR_DEFAULT_START_SZ);
	char *newbuf = realloc(e->buf, newsz);
	if (! newbuf) {
	    vsnprintf(e->buf, e->sz, fmt, ap);
	    return ACTF_OOM;
	}
	e->buf = newbuf;
	e->sz = newsz;
    }
    va_start(ap, fmt);
    sz = vsnprintf(e->buf, e->sz, fmt, ap);
    va_end(ap);
    if (sz < 0) {
	return ACTF_ERROR;
    }
    return ACTF_OK;
}

int eprependf(struct error *e, const char *fmt, ...)
{
    if (! e) {
	return ACTF_OK;
    }
    const char *delim = ": ";
    va_list ap;
    size_t cur_len = e->sz ? strlen(e->buf) : 0;
    va_start(ap, fmt);
    int in_len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (in_len < 0) {
	return ACTF_ERROR;
    }
    size_t req_sz = in_len + strlen(delim) + cur_len + 1;
    if (e->sz < req_sz) {
	char *newbuf = realloc(e->buf, req_sz);
	if (! newbuf) {
	    return ACTF_OOM;
	}
	e->buf = newbuf;
	e->sz = req_sz;
    }
    memmove(e->buf + in_len + strlen(delim), e->buf, cur_len);
    e->buf[req_sz - 1] = '\0';
    va_start(ap, fmt);
    int ret = vsnprintf(e->buf, e->sz, fmt, ap);
    va_end(ap);
    if (ret < 0) {
	return ACTF_ERROR;
    }
    memcpy(e->buf + in_len, delim, strlen(delim));
    return ACTF_OK;
}

const char *actf_errstr(int rc)
{
    static const char *const strs[] = {
	"not an error", // ACTF_OK
	"generic error", // ACTF_ERROR
	"internal error", // ACTF_INTERNAL
	"out of memory", // ACTF_OOM
	"property was not found", // ACTF_NOTFOUND
	"the json could not be parsed", // ACTF_JSON_PARSE_ERROR
	"incompatible json value", // ACTF_JSON_ERROR
	"incompatible type of json value", // ACTF_JSON_WRONG_TYPE
	"a value is not greater or equal than zero", // ACTF_JSON_NOT_GTEZ
	"a value is not greater than zero", // ACTF_JSON_NOT_GTZ
	"an alignment is not a power of two", // ACTF_INVALID_ALIGNMENT
	"non-existent byte order", // ACTF_INVALID_BYTE_ORDER
	"non-existent bit order",  // ACTF_INVALID_BIT_ORDER
	"invalid range", // ACTF_INVALID_RANGE
	"invalid range set", // ACTF_INVALID_RANGE_SET
	"invalid uuid", // ACTF_INVALID_UUID
	"invalid mapping", // ACTF_INVALID_MAPPING
	"invalid field location", // ACTF_INVALID_FLD_LOC
	"invalid field class", // ACTF_INVALID_FLD_CLS
	"invalid flags", // ACTF_INVALID_FLAGS
	"invalid role", // ACTF_INVALID_ROLE
	"non-existent base", // ACTF_INVALID_BASE
	"unsupported integer or float length", // ACTF_UNSUPPORTED_LENGTH
	"invalid encoding", // ACTF_INVALID_ENCODING
	"invalid environment", // ACTF_INVALID_ENVIRONMENT
	"invalid variant", // ACTF_INVALID_VARIANT
	"cycle offset greater than or equal to frequency", // ACTF_CC_GTE_FREQ_ERROR
	"non-existent field class alias is referred to", // ACTF_NO_SUCH_ALIAS
	"required property is missing", // ACTF_MISSING_PROPERTY
	"an extension is enabled which is not supported", // ACTF_UNSUPPORTED_EXTENSION
	"unknown clock origin", // ACTF_NO_SUCH_ORIGIN
	"default timestamp role specified but the data stream has no default clock", // ACTF_NO_DEFAULT_CLOCK
	"invalid \"metadata-stream-uuid\" role, no uuid in preamble or type?", // ACTF_INVALID_UUID_ROLE
	"invalid \"packet-magic-number\" role, should be first member in struct", // ACTF_INVALID_MAGIC_ROLE
	"top-level field class is not a struct when it should be", // ACTF_NOT_A_STRUCT
	"duplicate fragment, field class, name or id", // ACTF_DUPLICATE_ERROR
	"non-existent field class is referred to by id", // ACTF_NO_SUCH_ID
	"the CTF version is not supported",  // ACTF_UNSUPPORTED_VERSION
	"no preamble", // ACTF_NO_PREAMBLE
	"a selection field or a length indicator field has the wrong type", // ACTF_WRONG_FLD_TYPE
	"field location not found", // ACTF_MISSING_FLD_LOC
	"not enough bits in the data stream", // ACTF_NOT_ENOUGH_BITS
	"the byte-order changed in the middle of a byte", // ACTF_MID_BYTE_ENDIAN_SWAP
	"a string has a length which is not compatible with its encoding", // ACTF_INVALID_STR_LEN
	"the packet magic number is incorrect", // ACTF_MAGIC_MISMATCH
	"the data stream UUID does not match the metadata UUID", // ACTF_UUID_MISMATCH
	"a selector field is not found for an optional or variant", // ACTF_NO_SELECTOR_FLD
	"a packet content length is larger than its total length", // ACTF_INVALID_CONTENT_LEN
	"a metadata packet is not valid", // ACTF_INVALID_METADATA_PKT
    };
    if (rc >= 0 && rc < (int)(sizeof(strs) / sizeof(*strs))) {
	return strs[rc];
    } else {
	return "unknown error";
    }
}

void error_free(struct error *e)
{
    if (! e) {
	return;
    }
    free(e->buf);
}
