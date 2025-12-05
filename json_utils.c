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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

#include "crust/common.h"
#include "fld_loc_int.h"
#include "rng_int.h"
#include "types.h"
#include "error.h"
#include "json_utils.h"


void print_missing_key(const char *key, const char *where, struct error *e)
{
	eprintf(e, "required key %s is not available in %s", key, where);
}

static const char *json_type_to_str(enum json_type type)
{
	switch (type) {
	case json_type_null:
		return "null";
	case json_type_boolean:
		return "boolean";
	case json_type_double:
		return "double";
	case json_type_int:
		return "int";
	case json_type_object:
		return "object";
	case json_type_array:
		return "array";
	case json_type_string:
		return "string";
	}
	return "unknown";
}

void print_wrong_json_type(const char *key, enum json_type expected_type, struct error *e)
{
	eprintf(e, "%s is not a JSON %s", key, json_type_to_str(expected_type));
}

int json_object_key_get_string(const struct json_object *jobj, const char *key,
			       const char **str, struct error *e)
{
	struct json_object *key_jobj;
	if (!json_object_object_get_ex(jobj, key, &key_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(key_jobj, json_type_string)) {
		print_wrong_json_type(key, json_type_string, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	*str = json_object_get_string(key_jobj);
	return ACTF_OK;
}

static bool align_is_valid(uint64_t n)
{
	return (n & (n - 1)) == 0;
}

int json_object_key_get_alignment(const struct json_object *jobj, const char *key,
				  uint64_t *align, struct error *e)
{
	struct json_object *align_jobj;
	if (!json_object_object_get_ex(jobj, key, &align_jobj)) {
		eprintf(e, "%s is not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(align_jobj, json_type_int)) {
		print_wrong_json_type(key, json_type_int, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	uint64_t jalign = json_object_get_uint64(align_jobj);
	if (!align_is_valid(jalign)) {
		eprintf(e, "%s is not a power of two: %" PRIu64 "", key, jalign);
		return ACTF_INVALID_ALIGNMENT;
	}
	*align = jalign;
	return ACTF_OK;
}

int json_object_key_get_bo(const struct json_object *jobj, const char *key,
			   enum actf_byte_order *bo, struct error *e)
{
	struct json_object *bo_jobj;
	if (!json_object_object_get_ex(jobj, key, &bo_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(bo_jobj, json_type_string)) {
		print_wrong_json_type(key, json_type_string, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	const char *bo_str = json_object_get_string(bo_jobj);
	if (strcmp(bo_str, "big-endian") == 0) {
		*bo = ACTF_BIG_ENDIAN;
		return ACTF_OK;
	} else if (strcmp(bo_str, "little-endian") == 0) {
		*bo = ACTF_LIL_ENDIAN;
		return ACTF_OK;
	} else {
		eprintf(e, "%s is not a valid byte-order: %s", key, bo_str);
		return ACTF_INVALID_BYTE_ORDER;
	}
}

int json_object_key_get_bito(const struct json_object *jobj, const char *key,
			     enum actf_bit_order *bito, struct error *e)
{
	struct json_object *bito_jobj;
	if (!json_object_object_get_ex(jobj, key, &bito_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(bito_jobj, json_type_string)) {
		print_wrong_json_type(key, json_type_string, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	const char *bito_str = json_object_get_string(bito_jobj);
	if (strcmp(bito_str, "first-to-last") == 0) {
		*bito = ACTF_FIRST_TO_LAST;
		return ACTF_OK;
	} else if (strcmp(bito_str, "last-to-first") == 0) {
		*bito = ACTF_LAST_TO_FIRST;
		return ACTF_OK;
	} else {
		eprintf(e, "%s is not a valid bit-order: %s", key, bito_str);
		return ACTF_INVALID_BIT_ORDER;
	}
}

int json_object_key_get_uuid(struct json_object *jobj, const char *key,
			     struct actf_uuid *uuid, struct error *e)
{
	struct json_object *uuid_jobj;
	if (!json_object_object_get_ex(jobj, key, &uuid_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(uuid_jobj, json_type_array)) {
		print_wrong_json_type(key, json_type_array, e);
		return ACTF_JSON_WRONG_TYPE;
	}

	if (json_object_array_length(uuid_jobj) != ACTF_UUID_N_BYTES) {
		eprintf(e, "%s should be an array of length %d but has %d elements",
			key, ACTF_UUID_N_BYTES, json_object_array_length(uuid_jobj));
		return ACTF_INVALID_UUID;
	}
	for (int i = 0; i < ACTF_UUID_N_BYTES; i++) {
		struct json_object *val_jobj = json_object_array_get_idx(uuid_jobj, i);
		if (!json_object_is_type(val_jobj, json_type_int)) {
			eprintf(e, "%s should contain values which are integers", key);
			return ACTF_INVALID_UUID;
		}
		uuid->d[i] = (uint8_t) json_object_get_uint64(val_jobj);
	}
	return ACTF_OK;
}

int json_object_key_get_gtez_uint64(const struct json_object *jobj, const char *key,
				    uint64_t *val, struct error *e)
{
	struct json_object *val_jobj;
	if (!json_object_object_get_ex(jobj, key, &val_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(val_jobj, json_type_int)) {
		print_wrong_json_type(key, json_type_int, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	/* This dance is required to know if the int is negative or not.
	 * json_object_get_uint64 will return negative integers as zero.
	 */
	int64_t ival = json_object_get_int64(val_jobj);
	if (ival < 0) {
		eprintf(e, "%s is a negative number", key);
		return ACTF_JSON_NOT_GTEZ;
	}
	*val = json_object_get_uint64(val_jobj);
	return ACTF_OK;
}

int json_object_key_get_gtz_uint64(const struct json_object *jobj, const char *key,
				   uint64_t *val, struct error *e)
{
	uint64_t jval;
	int rc = json_object_key_get_gtez_uint64(jobj, key, &jval, e);
	if (rc < 0 && rc != ACTF_JSON_NOT_GTEZ) {
		return rc;
	} else if (rc == ACTF_JSON_NOT_GTEZ || jval == 0) {
		eprintf(e, "%s is not greater than zero", key);
		return ACTF_JSON_NOT_GTZ;
	}
	*val = jval;
	return ACTF_OK;
}

int json_object_key_get_int64(const struct json_object *jobj, const char *key,
			      int64_t *val, struct error *e)
{
	struct json_object *val_jobj;
	if (!json_object_object_get_ex(jobj, key, &val_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(val_jobj, json_type_int)) {
		print_wrong_json_type(key, json_type_int, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	*val = json_object_get_int64(val_jobj);
	return ACTF_OK;
}

struct rng {
	enum rng_type type;
	union {
		struct srng s;
		struct urng u;
	} d;
};

/* rng_parse reads out a range from rng_jobj and populates rng_out.
 * The signedness of the rng is determined based on if the JSON
 * integers are negative unless force_unsigned is true. */
static int rng_parse(struct json_object *rng_jobj, bool force_unsigned,
		     struct rng *rng_out, struct error *e)
{
	if (!json_object_is_type(rng_jobj, json_type_array)) {
		print_wrong_json_type("integer range", json_type_array, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	size_t n_eles = json_object_array_length(rng_jobj);
	if (n_eles != 2) {
		eprintf(e, "integer range should be composed of two elements but has %zu elements",
			n_eles);
		return ACTF_INVALID_RANGE;
	}
	struct json_object *lower_jobj = json_object_array_get_idx(rng_jobj, 0);
	if (!json_object_is_type(lower_jobj, json_type_int)) {
		eprintf(e, "integer range element is not an integer");
		return ACTF_INVALID_RANGE;
	}
	struct json_object *upper_jobj = json_object_array_get_idx(rng_jobj, 1);
	if (!json_object_is_type(upper_jobj, json_type_int)) {
		eprintf(e, "integer range element is not an integer");
		return ACTF_INVALID_RANGE;
	}
	struct rng rng;

	/* The first (lower) element should indicate signed or not since
	 * it's the smallest.
	 */
	int64_t slower = json_object_get_int64(lower_jobj);
	rng.type = slower < 0 && !force_unsigned ? RNG_SINT : RNG_UINT;
	if (rng.type == RNG_SINT) {
		rng.d.s.lower = slower;
		rng.d.s.upper = json_object_get_int64(upper_jobj);
	} else {
		rng.d.u.lower = json_object_get_uint64(lower_jobj);
		rng.d.u.upper = json_object_get_uint64(upper_jobj);
	}

	if (rng.type == RNG_SINT && rng.d.s.upper < rng.d.s.lower) {
		eprintf(e,
			"lower bound of integer range is smaller than upper bound: [%" PRIi64 ", %"
			PRIi64 "]", rng.d.s.lower, rng.d.s.upper);
		return ACTF_INVALID_RANGE;
	} else if (rng.type == RNG_UINT && rng.d.u.upper < rng.d.u.lower) {
		eprintf(e,
			"lower bound of integer range is smaller than upper bound: [%" PRIu64 ", %"
			PRIu64 "]", rng.d.u.lower, rng.d.u.upper);
		return ACTF_INVALID_RANGE;
	}

	*rng_out = rng;
	return ACTF_OK;
}

static int rngs_to_rng_set(struct rng *rngs, size_t n_rngs,
			   enum rng_type sign, struct actf_rng_set *rs, struct error *e)
{
	rs->sign = sign;
	if (rs->sign == RNG_SINT) {
		struct srng_set *srs = &rs->d.srs;
		srs->rngs = malloc(n_rngs * sizeof(*rs->d.srs.rngs));
		if (!srs->rngs) {
			eprintf(e, "malloc: %s", strerror(errno));
			return ACTF_OOM;
		}
		srs->len = n_rngs;

		for (size_t i = 0; i < n_rngs; i++) {
			if (rngs[i].type == RNG_SINT) {
				srs->rngs[i] = rngs[i].d.s;
			} else {
				srs->rngs[i].lower = rngs[i].d.u.lower;
				srs->rngs[i].upper = rngs[i].d.u.upper;
			}
		}
	} else {		// UINT
		struct urng_set *urs = &rs->d.urs;
		urs->rngs = malloc(n_rngs * sizeof(*rs->d.urs.rngs));
		if (!urs->rngs) {
			eprintf(e, "malloc: %s", strerror(errno));
			return ACTF_OOM;
		}
		urs->len = n_rngs;

		for (size_t i = 0; i < n_rngs; i++) {
			if (rngs[i].type == RNG_SINT) {
				urs->rngs[i].lower = rngs[i].d.s.lower;
				urs->rngs[i].upper = rngs[i].d.s.upper;
			} else {
				urs->rngs[i] = rngs[i].d.u;
			}
		}
	}
	return ACTF_OK;
}

int json_object_key_get_rng_set(const struct json_object *jobj, const char *key,
				struct actf_rng_set *rs, struct error *e)
{
	int rc;
	struct json_object *rs_jobj;
	if (!json_object_object_get_ex(jobj, key, &rs_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(rs_jobj, json_type_array)) {
		print_wrong_json_type(key, json_type_array, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	size_t n_rngs = json_object_array_length(rs_jobj);
	if (!n_rngs) {
		eprintf(e, "\"%s\" contains zero ranges", key);
		return ACTF_INVALID_RANGE_SET;
	}
	struct rng *rngs = malloc(n_rngs * sizeof(*rngs));
	if (!rngs) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	bool contains_negative = false;
	bool contains_larger_than_int64 = false;
	for (size_t i = 0; i < n_rngs; i++) {
		struct json_object *rng_jobj = json_object_array_get_idx(rs_jobj, i);
		if ((rc = rng_parse(rng_jobj, false, &rngs[i], e)) < 0) {
			free(rngs);
			return rc;
		}
		if (rngs[i].type == RNG_SINT && rngs[i].d.s.lower < 0) {
			contains_negative = true;
		}
		if (rngs[i].type == RNG_UINT && rngs[i].d.u.upper > (uint64_t) INT64_MAX) {
			contains_larger_than_int64 = true;
		}
	}
	if (contains_negative && contains_larger_than_int64) {
		eprintf(e, "Ranges contain both a negative value and a value larger than INT64_MAX."
			" Unable to represent it correctly.");
		free(rngs);
		return ACTF_INVALID_RANGE_SET;
	}

	enum rng_type sign = contains_negative ? RNG_SINT : RNG_UINT;
	if ((rc = rngs_to_rng_set(rngs, n_rngs, sign, rs, e)) < 0) {
		free(rngs);
		return rc;
	}

	free(rngs);
	return ACTF_OK;
}

/* Returns ACTF_INVALID_RANGE if the rng is equivalent to no srng. */
static int rng_to_srng(const struct rng *rng, struct srng *srng)
{
	if (rng->type == RNG_SINT) {
		*srng = rng->d.s;
	} else {
		if (rng->d.u.lower > (uint64_t) INT64_MAX) {
			return ACTF_INVALID_RANGE;
		}
		srng->lower = rng->d.u.lower;
		srng->upper = rng->d.u.upper > (uint64_t) INT64_MAX ? INT64_MAX : rng->d.u.upper;
	}
	return ACTF_OK;
}

/* Returns ACTF_INVALID_RANGE if the rng is equivalent to no urng. */
static int rng_to_urng(const struct rng *rng, struct urng *urng)
{
	if (rng->type == RNG_SINT) {
		if (rng->d.s.upper < 0) {
			return ACTF_INVALID_RANGE;
		}
		urng->lower = rng->d.s.lower < 0 ? 0 : rng->d.s.lower;
		urng->upper = rng->d.s.upper;
	} else {
		*urng = rng->d.u;
	}
	return ACTF_OK;
}

/* maps must have been allocated with the appropriate number of range
 * sets for maps_jobj. Sibling with `umaps_parse`, keep consistent! */
static int smaps_parse(const struct json_object *maps_jobj, const char *key_ctx,
		       struct smappings *maps, struct error *e)
{
	int rc;
	size_t i = 0;
	struct json_object_iter iter;
	json_object_object_foreachC(maps_jobj, iter) {
		const char *name = iter.key;
		struct json_object *rs_jobj = iter.val;
		if (!json_object_is_type(rs_jobj, json_type_array)) {
			print_wrong_json_type(name, json_type_array, e);
			return ACTF_JSON_WRONG_TYPE;
		}
		size_t rngs_len = json_object_array_length(rs_jobj);
		if (!rngs_len) {
			eprintf(e, "\"%s\" contains zero ranges.", key_ctx);
			rc = ACTF_INVALID_RANGE_SET;
			goto err;
		}
		struct srng *rngs = malloc(rngs_len * sizeof(*rngs));
		if (!rngs) {
			eprintf(e, "malloc: %s", strerror(errno));
			rc = ACTF_OOM;
			goto err;
		}
		size_t n_valid = 0;
		for (size_t j = 0; j < rngs_len; j++) {
			struct json_object *rng_jobj = json_object_array_get_idx(rs_jobj, j);
			struct rng rng;
			if ((rc = rng_parse(rng_jobj, false, &rng, e)) < 0) {
				free(rngs);
				goto err;
			}
			n_valid += (rng_to_srng(&rng, &rngs[j]) == 0);
		}
		maps->rng_sets[i].len = n_valid;
		maps->rng_sets[i].rngs = rngs;
		maps->names[i] = c_strdup(name);
		i++;
	}
	return ACTF_OK;

      err:
	for (size_t j = 0; j < i; j++) {
		srng_set_free(&maps->rng_sets[j]);
		free(maps->names[j]);
	}
	return rc;
}

/* maps must have been allocated with the appropriate number of range
 * sets for maps_jobj. Sibling with `smaps_parse`, keep consistent! */
static int umaps_parse(const struct json_object *maps_jobj, const char *key_ctx,
		       struct umappings *maps, struct error *e)
{
	int rc;
	size_t i = 0;
	struct json_object_iter iter;
	json_object_object_foreachC(maps_jobj, iter) {
		const char *name = iter.key;
		struct json_object *rs_jobj = iter.val;
		if (!json_object_is_type(rs_jobj, json_type_array)) {
			print_wrong_json_type(name, json_type_array, e);
			rc = ACTF_JSON_WRONG_TYPE;
			goto err;
		}
		size_t rngs_len = json_object_array_length(rs_jobj);
		if (!rngs_len) {
			eprintf(e, "\"%s\" contains zero ranges.", key_ctx);
			rc = ACTF_INVALID_RANGE_SET;
			goto err;
		}
		struct urng *rngs = malloc(rngs_len * sizeof(*rngs));
		if (!rngs) {
			eprintf(e, "malloc: %s", strerror(errno));
			rc = ACTF_OOM;
			goto err;
		}
		size_t n_valid = 0;
		for (size_t j = 0; j < rngs_len; j++) {
			struct json_object *rng_jobj = json_object_array_get_idx(rs_jobj, j);
			struct rng rng;
			if ((rc = rng_parse(rng_jobj, true, &rng, e)) < 0) {
				free(rngs);
				goto err;
			}
			n_valid += (rng_to_urng(&rng, &rngs[j]) == 0);
		}
		maps->rng_sets[i].len = n_valid;
		maps->rng_sets[i].rngs = rngs;
		maps->names[i] = c_strdup(name);
		i++;
	}
	return ACTF_OK;

      err:
	for (size_t j = 0; j < i; j++) {
		urng_set_free(&maps->rng_sets[j]);
		free(maps->names[j]);
	}
	return rc;
}

int json_object_key_get_mappings(const struct json_object *jobj, const char *key,
				 enum rng_type sign, struct mappings *maps, struct error *e)
{
	int rc;
	struct json_object *maps_jobj;
	if (!json_object_object_get_ex(jobj, key, &maps_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(maps_jobj, json_type_object)) {
		print_wrong_json_type(key, json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}

	int maps_len = json_object_object_length(maps_jobj);
	char **names = malloc(maps_len * sizeof(*names));
	if (!names) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	if (sign == RNG_SINT) {
		maps->sign = RNG_SINT;
		maps->d.smaps.len = maps_len;
		maps->d.smaps.names = names;
		maps->d.smaps.rng_sets = malloc(maps_len * sizeof(*maps->d.smaps.rng_sets));
		if (!maps->d.smaps.rng_sets) {
			eprintf(e, "malloc: %s", strerror(errno));
			free(names);
			return ACTF_OOM;
		}
		if ((rc = smaps_parse(maps_jobj, key, &maps->d.smaps, e)) < 0) {
			free(maps->d.smaps.names);
			free(maps->d.smaps.rng_sets);
			return rc;
		}
	} else {
		maps->sign = RNG_UINT;
		maps->d.umaps.len = maps_len;
		maps->d.umaps.names = names;
		maps->d.umaps.rng_sets = malloc(maps_len * sizeof(*maps->d.umaps.rng_sets));
		if (!maps->d.umaps.rng_sets) {
			eprintf(e, "malloc: %s", strerror(errno));
			free(names);
			return ACTF_OOM;
		}
		if ((rc = umaps_parse(maps_jobj, key, &maps->d.umaps, e)) < 0) {
			free(maps->d.umaps.names);
			free(maps->d.umaps.rng_sets);
			return rc;
		}
	}
	return ACTF_OK;
}

static int actf_fld_loc_parse(struct json_object *fld_loc_jobj,
			      struct actf_fld_loc *fld_loc, struct error *e)
{
	int rc;
	if (!json_object_is_type(fld_loc_jobj, json_type_object)) {
		print_wrong_json_type("field location", json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	enum actf_fld_loc_origin origin = ACTF_FLD_LOC_ORIGIN_NONE;
	const char *origin_name = NULL;
	if ((rc = json_object_key_get_string(fld_loc_jobj, "origin", &origin_name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return ACTF_INVALID_FLD_LOC;
	}
	if (origin_name) {
		origin = actf_fld_loc_origin_name_to_type(origin_name);
		if (origin == ACTF_FLD_LOC_ORIGIN_NONE) {
			eprintf(e,
				"\"origin\" specified in field location but \"%s\" is not a known origin",
				origin_name);
			return ACTF_INVALID_FLD_LOC;
		}
	} else {
		origin = ACTF_FLD_LOC_ORIGIN_NONE;
	}

	char **path;
	struct json_object *path_jobj;
	if (!json_object_object_get_ex(fld_loc_jobj, "path", &path_jobj)) {
		print_missing_key("path", "field location", e);
		return ACTF_INVALID_FLD_LOC;
	}
	if (!json_object_is_type(path_jobj, json_type_array)) {
		print_wrong_json_type("path", json_type_array, e);
		return ACTF_INVALID_FLD_LOC;
	}
	size_t path_len = json_object_array_length(path_jobj);
	if (!path_len) {
		eprintf(e, "\"path\" in field location does not contain any elements");
		return ACTF_INVALID_FLD_LOC;
	}
	path = malloc(path_len * sizeof(*path));
	if (!path) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	size_t i;
	for (i = 0; i < path_len; i++) {
		struct json_object *path_ele_jobj = json_object_array_get_idx(path_jobj, i);
		enum json_type type = json_object_get_type(path_ele_jobj);
		if (type == json_type_string) {
			path[i] = c_strdup(json_object_get_string(path_ele_jobj));
		} else if (type == json_type_null) {
			path[i] = NULL;
		} else {
			eprintf(e, "path element in field location is not a string or null");
			goto err;
		}
	}
	if (path[path_len - 1] == NULL) {
		eprintf(e, "last path element of field location is null");
		goto err;
	}

	fld_loc->origin = origin;
	fld_loc->path = path;
	fld_loc->path_len = path_len;
	return ACTF_OK;
      err:
	for (size_t j = 0; j < i; j++) {
		free(path[j]);
	}
	free(path);
	return ACTF_INVALID_FLD_LOC;
}

int json_object_key_get_fld_loc(const struct json_object *jobj, const char *key,
				struct actf_fld_loc *fld_loc, struct error *e)
{
	int rc;
	struct json_object *fld_loc_jobj;
	if (!json_object_object_get_ex(jobj, key, &fld_loc_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if ((rc = actf_fld_loc_parse(fld_loc_jobj, fld_loc, e)) < 0) {
		return rc;
	}
	return ACTF_OK;
}

int json_object_key_get_ctfjson(const struct json_object *jobj, const char *key,
				struct ctfjson **j, struct error *e)
{
	int rc;
	struct json_object *val_jobj;
	if (!json_object_object_get_ex(jobj, key, &val_jobj)) {
		eprintf(e, "%s not found in JSON object", key);
		return ACTF_NOT_FOUND;
	}
	if (!json_object_is_type(val_jobj, json_type_object)) {
		print_wrong_json_type(key, json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	struct ctfjson *tmpj = NULL;
	if ((rc = ctfjson_init(val_jobj, &tmpj, e)) < 0) {
		eprependf(e, "parsing %s into CTF fields");
		return rc;
	}
	*j = tmpj;
	return ACTF_OK;
}
