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

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <json-c/json_object.h>
#include <stdint.h>

#include "ctfjson.h"
#include "mappings_int.h"
#include "types.h"
#include "fld_loc.h"
#include "rng.h"
#include "error.h"


/* Prints that `key` is missing in `where` to stderr. */
void print_missing_key(const char *key, const char *where, struct error *e);

/* Prints that `key` is expected to be of `expected_type`. */
void print_wrong_json_type(const char *key, enum json_type expected_type, struct error *e);

/* Retrives the string at `key` in `jobj` and puts it in `str`.
 * Returns ACTF_OK on success, ACTF_NOT_FOUND if `key` does not exist
 * and ACTF_JSON_WRONG_TYPE/ACTF_JSON_ERROR if there was an error
 * parsing an existing key.
 */
int json_object_key_get_string(const struct json_object *jobj, const char *key,
			       const char **str, struct error *e);

/* Retrieves and sanity-checks the alignment value in `key`. Returns
 * ACTF_OK on success, ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_JSON_WRONG_TYPE/ACTF_INVALID_ALIGNMENT/ACTF_JSON_ERROR if there
 * was an error parsing an existing key.
 */
int json_object_key_get_alignment(const struct json_object *jobj, const char *key,
				  uint64_t *align, struct error *e);

/* Retrieves the byte-order in `key`. Returns ACTF_OK on success,
 * ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_INVALID_BYTE_ORDER/ACTF_WRONG_TYPE/ACTF_JSON_ERROR if there was
 * an error parsing an existing key.
 */
int json_object_key_get_bo(const struct json_object *jobj, const char *key,
			   enum actf_byte_order *bo, struct error *e);

/* Retrieves the bit-order in `key`. Returns ACTF_OK on success,
 * ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_INVALID_BIT_ORDER/ACTF_WRONG_TYPE/ACTF_JSON_ERROR if there was an
 * error parsing an existing key.
 */
int json_object_key_get_bito(const struct json_object *jobj, const char *key,
			     enum actf_bit_order *bito, struct error *e);

/* Parses a uuid at `key` in `jobj` and puts it in `uuid`. Returns
 * ACTF_OK on success, ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_INVALID_UUID/ACTF_JSON_WRONG_TYPE/ACTF_JSON_ERROR if there was an
 * error parsing an existing key.
 */
int json_object_key_get_uuid(struct json_object *jobj, const char *key,
			     struct actf_uuid *uuid, struct error *e);

/* Retrieves the greater than zero uint64_t in `key`. Returns ACTF_OK
 * on success, ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_JSON_NOT_GTZ/ACTF_JSON_WRONG_TYPE/ACTF_JSON_ERROR if there was an
 * error parsing an existing key.
 */
int json_object_key_get_gtz_uint64(const struct json_object *jobj, const char *key,
				   uint64_t *val, struct error *e);

/* Retrieves the greater than or equal to zero uint64_t in `key`.
 * Returns ACTF_OK on success, ACTF_NOT_FOUND if `key` does not exist
 * and ACTF_JSON_NOT_GTEZ/ACTF_JSON_WRONG_TYPE/ACTF_JSON_ERROR if there
 * was an error parsing an existing key.
 */
int json_object_key_get_gtez_uint64(const struct json_object *jobj, const char *key,
				   uint64_t *val, struct error *e);

/* Retrieves the int64_t in `key`. Returns ACTF_OK on success,
 * ACTF_NOT_FOUND if `key` does not exist and ACTF_JSON_ERROR if there
 * was an error parsing an existing key.
 */
int json_object_key_get_int64(const struct json_object *jobj, const char *key,
			      int64_t *val, struct error *e);

/* Retrieves the range set in `key`. Returns ACTF_OK on success,
 * ACTF_NOT_FOUND if `key` does not exist and ACTF_JSON_ERROR if there
 * was an error parsing an existing key. Special funny parsing logic.
 * Will parse the range as signed/unsigned based on if its members
 * contains a negative number or not. If succesful, rs will be
 * allocated and should eventually be freed by the caller with a
 * `actf_rng_set_free`.
 */
int json_object_key_get_rng_set(const struct json_object *jobj, const char *key,
				struct actf_rng_set *rs, struct error *e);

/* Retrieves the mappings in `key` and parses them according into a
 * mapping with signedness based on input sign. Returns ACTF_OK on
 * success, ACTF_NOT_FOUND if `key` does not exist and ACTF_JSON_ERROR
 * if there was an error parsing an existing key. If successful, maps
 * will be populated with dynamic memory which must be freed with
 * `mappings_free`.
 */
int json_object_key_get_mappings(const struct json_object *jobj, const char *key,
				 enum rng_type sign, struct mappings *maps,
				 struct error *e);

/* Retrives the field-location in `key`. Returns ACTF_OK on success,
 * ACTF_NOT_FOUND if `key` does not exist and ACTF_JSON_ERROR if there
 * was an error parsing an existing key. On success, the returned
 * `actf_fld_loc` must be freed with a call to `actf_fld_loc_free`.
 */
int json_object_key_get_fld_loc(const struct json_object *jobj, const char *key,
				struct actf_fld_loc *fld_loc, struct error *e);

/* Retrieves the CTF JSON representation of value in `key`. Puts a
 * requirement that the top-level JSON value is an object. Returns
 * ACTF_OK on success, ACTF_NOT_FOUND if `key` does not exist and
 * ACTF_JSON_ERROR if there was an error parsing an existing key. On
 * success, the returned `ctfjson` must be freed with a call to
 * `ctfjson_free`. */
int json_object_key_get_ctfjson(const struct json_object *jobj, const char *key,
				struct ctfjson **j, struct error *e);

#endif /* JSON_UTILS_H */
