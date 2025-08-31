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
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>

#include "crust/common.h"
#include "ctfjson.h"


static char *uitoa(uint64_t i)
{
    const char *fmt = PRIu64;
    int sz = snprintf(NULL, 0, fmt, i);
    if (sz < 0) {
	return NULL;
    }
    sz++; // null term
    char *s = malloc(sz);
    if (! s) {
	return NULL;
    }
    if (snprintf(s, sz, fmt, i) < 0) {
	free(s);
	return NULL;
    }
    return s;
}

/* Parses the JSON object into field classes */
static int fld_cls_from_json(struct json_object *jobj, struct actf_fld_cls *fc, struct error *e)
{
    int rc;
    fc->alias = NULL;
    fc->attributes = NULL;
    fc->extensions = NULL;
    enum json_type type = json_object_get_type(jobj);
    switch (type) {
    case json_type_null:
	fc->type = ACTF_FLD_CLS_NIL;
	break;
    case json_type_boolean:
	fc->type = ACTF_FLD_CLS_FXD_LEN_BOOL;
	fc->cls.fxd_len_bool = (struct fxd_len_bool_fld_cls){.bit_arr = {.len = 1, .align = DEFAULT_ALIGNMENT}};
	break;
    case json_type_double:
	fc->type = ACTF_FLD_CLS_FXD_LEN_FLOAT;
	fc->cls.fxd_len_float = (struct fxd_len_float_fld_cls){.bit_arr = {.len = 64, .align = DEFAULT_ALIGNMENT}};
	break;
    case json_type_int:
	if (json_object_get_int64(jobj) < 0) {
	    fc->type = ACTF_FLD_CLS_FXD_LEN_SINT;
	} else {
	    fc->type = ACTF_FLD_CLS_FXD_LEN_UINT;
	}
	fc->cls.fxd_len_int = (struct fxd_len_int_fld_cls){.bit_arr = {.len = 64, .align = DEFAULT_ALIGNMENT},
	    .base = {.pref_display_base = DEFAULT_DISPLAY_BASE}};
	break;
    case json_type_object: {
	size_t n_members = json_object_object_length(jobj);
	struct struct_fld_member_cls *clses = malloc(n_members * sizeof(*clses));
	if (! clses) {
	    eprintf(e, "malloc: %s", strerror(errno));
	    return ACTF_OOM;
	}

	size_t i = 0;
	struct json_object_iter iter;
	json_object_object_foreachC(jobj, iter) {
	    const char *name = iter.key;
	    struct json_object *m_jobj = iter.val;
	    if ((rc = fld_cls_from_json(m_jobj, &clses[i].cls, e)) < 0) {
		for (size_t j = 0; j < i; j++) {
		    actf_fld_cls_free(&clses[j].cls);
		    free(clses[j].name);
		}
		free(clses);
		return rc;
	    }
	    clses[i].name = c_strdup(name); // check?
	    clses[i].attributes = NULL;
	    clses[i].extensions = NULL;
	    i++;
	}

	fc->type = ACTF_FLD_CLS_STRUCT;
	fc->cls.struct_ = (struct struct_fld_cls){.n_members = n_members, .min_align = DEFAULT_ALIGNMENT,
	    .member_clses = clses};
	break;
    }
    case json_type_array: {
	/* A json array is basically handled like an object, but
	 * without user-provided key names. Instead we generate
	 * generic key names based on the index of the array like "0",
	 * "1". We cannot represent a JSON array as an array in CTF
	 * since it may contain different types. */
	size_t n_members = json_object_array_length(jobj);
	struct struct_fld_member_cls *clses = malloc(n_members * sizeof(*clses));
	if (! clses) {
	    eprintf(e, "malloc: %s", strerror(errno));
	    return ACTF_OOM;
	}

	for (size_t i = 0; i < n_members; i++) {
	    struct json_object *m_jobj = json_object_array_get_idx(jobj, i);
	    if ((rc = fld_cls_from_json(m_jobj, &clses[i].cls, e)) < 0) {
		for (size_t j = 0; j < i; j++) {
		    actf_fld_cls_free(&clses[j].cls);
		    free(clses[j].name);
		}
		free(clses);
		return rc;
	    }
	    clses[i].name = uitoa(i); // check?
	    clses[i].attributes = NULL;
	    clses[i].extensions = NULL;
	}

	fc->type = ACTF_FLD_CLS_STRUCT;
	fc->cls.struct_ = (struct struct_fld_cls){.n_members = n_members, .min_align = DEFAULT_ALIGNMENT,
	    .member_clses = clses};
	break;
    }
    case json_type_string:
	fc->type = ACTF_FLD_CLS_NULL_TERM_STR;
	fc->cls.null_term_str = (struct null_term_str_fld_cls){.base = {.enc = ACTF_ENCODING_UTF8}};
	break;
    default:
	assert(! "unrecognized json type");
	break;
    }
    return ACTF_OK;
}

static void eprint_incompatible_fld_cls(struct actf_fld_cls *fc, enum json_type jtype,
					struct error *e)
{
    eprintf(e, "incompatible field-class \"%s\" for JSON %s",
	    actf_fld_cls_type_name(fc->type), json_type_to_name(jtype));
}

/* Parses the JSON object into fields based on the provided field
 * classes. The field classes are presumably generated from
 * `fld_cls_from_json`. */
static int fld_from_json(struct ctfjson_fld *root, struct json_object *jobj,
			 struct actf_fld_cls *fc, struct actf_fld *parent,
			 struct actf_fld *val, struct error *e)
{
    int rc;
    val->parent = parent;
    val->cls = fc;
    enum json_type type = json_object_get_type(jobj);
    switch (type) {
    case json_type_null:
	if (fc->type != ACTF_FLD_CLS_NIL) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_NIL;
	break;
    case json_type_boolean:
	if (fc->type != ACTF_FLD_CLS_FXD_LEN_BOOL) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_BOOL;
	val->d.bool_.val = json_object_get_boolean(jobj);
	break;
    case json_type_double:
	if (fc->type != ACTF_FLD_CLS_FXD_LEN_FLOAT) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_REAL;
	val->d.real.f64 = json_object_get_double(jobj);
	break;
    case json_type_int:
	if (fc->type != ACTF_FLD_CLS_FXD_LEN_UINT &&
	    fc->type != ACTF_FLD_CLS_FXD_LEN_SINT &&
	    fc->type != ACTF_FLD_CLS_VAR_LEN_UINT &&
	    fc->type != ACTF_FLD_CLS_VAR_LEN_SINT) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	if (fc->type == ACTF_FLD_CLS_FXD_LEN_SINT ||
	    fc->type == ACTF_FLD_CLS_VAR_LEN_SINT) {
	    val->type = ACTF_FLD_TYPE_SINT;
	    val->d.int_.val = json_object_get_int64(jobj);
	} else {
	    val->type = ACTF_FLD_TYPE_UINT;
	    val->d.uint.val = json_object_get_uint64(jobj);
	}
	break;
    case json_type_object: {
	if (fc->type != ACTF_FLD_CLS_STRUCT) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_STRUCT;

	size_t n_members = json_object_object_length(jobj);
	if (n_members != fc->cls.struct_.n_members) {
	    eprintf(e, "field-class has %zu members while json object has %zu members",
		    fc->cls.struct_.n_members, n_members);
	    return ACTF_INTERNAL;
	}
	struct actf_fld *vals = malloc(n_members * sizeof(*vals));
	if (! vals) {
	    eprintf(e, "malloc: %s", strerror(errno));
	    return ACTF_OOM;
	}
	val->d.struct_.vals = vals;

	size_t i = 0;
	struct json_object_iter iter;
	json_object_object_foreachC(jobj, iter) {
	    const char *name = iter.key;
	    struct json_object *m_jobj = iter.val;
	    /* NOTE: This assumes the order must be the same. */
	    struct struct_fld_member_cls *mfc = &fc->cls.struct_.member_clses[i];
	    if (strcmp(name, mfc->name) != 0) {
		eprintf(e, "field-class has key %s in index %zu but JSON object has key %s",
			mfc->name, i, name);
		free(vals);
		return ACTF_INTERNAL;
	    }
	    if ((rc = fld_from_json(root, m_jobj, &mfc->cls, val, &vals[i], e)) < 0) {
		free(vals);
		return rc;
	    }
	    i++;
	}
	actf_flds_vec_push(&root->ctf_flds, vals);
	break;
    }
    case json_type_array: {
	if (fc->type != ACTF_FLD_CLS_STRUCT) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_STRUCT;

	size_t n_members = json_object_array_length(jobj);
	if (n_members != fc->cls.struct_.n_members) {
	    eprintf(e, "field-class has %zu members while json object has %zu members",
		    fc->cls.struct_.n_members, n_members);
	    return ACTF_INTERNAL;
	}
	struct actf_fld *vals = malloc(n_members * sizeof(*vals));
	if (! vals) {
	    eprintf(e, "malloc: %s", strerror(errno));
	    return ACTF_OOM;
	}
	val->d.struct_.vals = vals;

	for (size_t i = 0; i < n_members; i++) {
	    struct json_object *m_jobj = json_object_array_get_idx(jobj, i);
	    struct struct_fld_member_cls *mfc = &fc->cls.struct_.member_clses[i];
	    if ((rc = fld_from_json(root, m_jobj, &mfc->cls, val, &vals[i], e)) < 0) {
		free(vals);
		return rc;
	    }
	}
	actf_flds_vec_push(&root->ctf_flds, vals);
	break;
    }
    case json_type_string: {
	if (fc->type != ACTF_FLD_CLS_NULL_TERM_STR &&
	    fc->type != ACTF_FLD_CLS_STATIC_LEN_STR &&
	    fc->type != ACTF_FLD_CLS_DYN_LEN_STR) {
	    eprint_incompatible_fld_cls(fc, type, e);
	    return ACTF_INTERNAL;
	}
	val->type = ACTF_FLD_TYPE_STR;

	const char *js = json_object_get_string(jobj);
	size_t sz = json_object_get_string_len(jobj) + 1;
	char *s = malloc(sz);
	if (! s) {
	    eprintf(e, "malloc: %s", strerror(errno));
	    return ACTF_OOM;
	}
	memcpy(s, js, sz);
	val->d.str.ptr = (uint8_t *) s;
	val->d.str.len = sz;
	str_vec_push(&root->strs, s);
	break;
    }
    default:
	assert(! "unrecognized JSON type");
	break;
    }
    return ACTF_OK;
}

/* Frees the content of `ctfjson_fld`. */
static void ctfjson_fld_free(struct ctfjson_fld *val)
{
    for (size_t i = 0; i < val->strs.len; i++) {
	char **s = str_vec_idx(&val->strs, i);
	free(*s);
    }
    str_vec_free(&val->strs);
    for (size_t i = 0; i < val->ctf_flds.len; i++) {
	struct actf_fld **flds = actf_flds_vec_idx(&val->ctf_flds, i);
	free(*flds);
    }
    actf_flds_vec_free(&val->ctf_flds);
}

int ctfjson_init(struct json_object *jobj, struct ctfjson **j, struct error *e)
{
    int rc;
    struct ctfjson *tmpj = calloc(1, sizeof(*tmpj));
    if (! tmpj) {
	eprintf(e, "malloc: %s", strerror(errno));
	return ACTF_OOM;
    }
    if ((rc = fld_cls_from_json(jobj, &tmpj->fc, e)) < 0) {
	free(tmpj);
	return rc;
    }
    if ((rc = fld_from_json(&tmpj->jval, jobj, &tmpj->fc, NULL, &tmpj->jval.val, e)) < 0) {
	actf_fld_cls_free(&tmpj->fc);
	free(tmpj);
	return rc;
    }
    *j = tmpj;
    return ACTF_OK;
}

void ctfjson_free(struct ctfjson *j)
{
    if (! j) {
	return;
    }
    ctfjson_fld_free(&j->jval);
    actf_fld_cls_free(&j->fc);
    free(j);
}
