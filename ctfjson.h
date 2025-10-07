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

#ifndef CTFJSON_H
#define CTFJSON_H

/* A JSON to CTF field/field class converter. Main purpose is to
 * facilitate access to environment, attributes and extensions. */

#include <json-c/json_object.h>

#include "fld_cls_int.h"
#include "fld_int.h"
#include "flds_vec.h"
#include "str_vec.h"
#include "error.h"


/* Procedure:
 * 1. Parse the json structure into a field class
 * 2. Parse the json structure again and instantiate the fields of the field class */

/* ctfjson_fld represents a JSON field */
struct ctfjson_fld {
	/* The field values representing the JSON values. */
	struct actf_fld val;
	/* Allocated strings of `val` */
	struct str_vec strs;
	/* Allocated fields of `val` */
	struct actf_flds_vec ctf_flds;
};

/* ctfjson represents JSON values */
struct ctfjson {
	/* fc represents a JSON schema */
	struct actf_fld_cls fc;
	/* jval represents the JSON values */
	struct ctfjson_fld jval;
};

/* Parses the JSON object into field classes / fields:
 * - json_type_null    -> ACTF_FLD_CLS_NIL           / ACTF_FLD_TYPE_NIL
 * - json_type_boolean -> ACTF_FLD_CLS_FXD_LEN_BOOL  / ACTF_FLD_TYPE_BOOL
 * - json_type_double  -> ACTF_FLD_CLS_FXD_LEN_FLOAT / ACTF_FLD_TYPE_REAL
 * - json_type_int(+)  -> ACTF_FLD_CLS_FXD_LEN_UINT  / ACTF_FLD_TYPE_UINT
 * - json_type_int(-)  -> ACTF_FLD_CLS_FXD_LEN_SINT  / ACTF_FLD_TYPE_SINT
 * - json_type_object  -> ACTF_FLD_CLS_STRUCT        / ACTF_FLD_TYPE_STRUCT
 * - json_type_array   -> ACTF_FLD_CLS_STRUCT        / ACTF_FLD_TYPE_STRUCT
 * - json_type_string  -> ACTF_FLD_CLS_NULL_TERM_STR / ACTF_FLD_TYPE_STR
 *
 * JSON arrays are not mono-typed, so they must be represented as a
 * struct.
 *
 * The caller should ensure that jobj is of the appropriate type. For
 * attributes / extensions / environment, it would be a JSON
 * object. */
int ctfjson_init(struct json_object *jobj, struct ctfjson **j, struct error *e);

void ctfjson_free(struct ctfjson *j);


#endif /* CTFJSON_H */
