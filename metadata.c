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
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <json-c/json.h>

#include "crust/common.h"
#include "metadata_int.h"
#include "fld_cls_int.h"
#include "json_utils.h"


#define ACTF_MAJOR_VERSION 2


#define METADATA_PKT_HDR_MAGIC 0x75d11d57
#define METADATA_PKT_HDR_MAJOR 2
#define METADATA_PKT_HDR_MINOR 0
#define METADATA_PKT_HDR_SZ_BITS 352

/* metadata_pkt_hdr represents a metadata packet header used by
 * packetized metadata streams (CTF2-PMETA-1.0). A packetized metadata
 * stream is a sequence of metadata packets. Each metadata packet
 * consists of a header followed by content. */
struct metadata_pkt_hdr {
	/* Must match `METADATA_PKT_HDR_MAGIC`. The byte-order of this
	 * field decides the byte-order of the rest of the header. */
	uint32_t magic;
	/* UUID for all metadata packets wrapping a metadata stream */
	struct actf_uuid uuid;
	/* Must ignore */
	uint32_t checksum;
	/* Size of metadata packet including this header. Must be multiple of
	 * 8. */
	uint32_t content_sz_bits;
	/* Total size of the whole metadata packet including this header
	 * and padding. Must be a multiple of 8. */
	uint32_t total_sz_bits;
	/* Must be zero (no compression) */
	uint8_t compression_scheme;
	/* Must be zero (no encryption) */
	uint8_t encryption_scheme;
	/* Must be zero (no checksum) */
	uint8_t content_checksum;
	/* Must be set to 2 */
	uint8_t major;
	/* Must be set to 0 */
	uint8_t minor;
	uint8_t reserved[3];
	/* Size of whole header. Must be 352 (44 bytes). */
	uint32_t hdr_sz_bits;
} __attribute__((__packed__));


const struct actf_fld_cls_alias *actf_metadata_find_fld_cls_alias(const struct actf_metadata
								  *metadata, const char *name)
{
	const struct actf_fld_cls_alias_vec *aliases = &metadata->fld_cls_aliases;
	for (size_t i = 0; i < aliases->len; i++) {
		const struct actf_fld_cls_alias *alias = actf_fld_cls_alias_vec_idx(aliases, i);
		if (strcmp(alias->name, name) == 0) {
			return alias;
		}
	}
	return NULL;
}

struct actf_metadata *actf_metadata_init(void)
{
	struct actf_metadata *metadata = calloc(1, sizeof(*metadata));
	if (!metadata) {
		return NULL;
	}
	if (u64todsc_init_cap(&metadata->idtodsc, 4) < 0) {
		free(metadata);
		return NULL;
	}
	metadata->err = ERROR_EMPTY;
	return metadata;
}

/* Returns the first found enabled extension. An extension is enabled
 * when it is declared by both namespace and name in the preamble
 * fragment. */
static const char *first_extension_enabled(struct ctfjson *extensions)
{
	size_t namespace_len = actf_fld_struct_len(&extensions->jval.val);
	for (size_t i = 0; i < namespace_len; i++) {
		/* The member name is the "namespace".
		 * The member value is the "namespaced extensions object" */
		const struct actf_fld *extension =
		    actf_fld_struct_fld_idx(&extensions->jval.val, i);
		/* Due to ctfjson representing JSON arrays as structs, we can
		 * not verify that the extension was a JSON object rather than
		 * a JSON array. */
		if (actf_fld_type(extension) == ACTF_FLD_TYPE_STRUCT
		    && actf_fld_struct_len(extension) > 0) {
			return actf_fld_struct_fld_name_idx(extension, 0);
		}
	}
	return NULL;
}

static int actf_preamble_parse(struct json_object *preamble_jobj, struct actf_preamble *preamble,
			       struct error *e)
{
	int rc;
	struct json_object *version_jobj;
	if (!json_object_object_get_ex(preamble_jobj, "version", &version_jobj)) {
		print_missing_key("version", "preamble", e);
		return ACTF_MISSING_PROPERTY;
	}
	if (!json_object_is_type(version_jobj, json_type_int)) {
		print_wrong_json_type("version", json_type_int, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	struct actf_uuid uuid = { 0 };
	if ((rc = json_object_key_get_uuid(preamble_jobj, "uuid", &uuid, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	bool has_uuid = (rc == ACTF_OK);
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(preamble_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(preamble_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		ctfjson_free(attributes);
		return rc;
	} else if (rc == ACTF_OK && extensions) {
		const char *unsupported_extension = first_extension_enabled(extensions);
		if (unsupported_extension) {
			eprintf(e,
				"unsupported extension \"%s\" enabled in preamble, unable to consume data stream",
				unsupported_extension);
			ctfjson_free(attributes);
			ctfjson_free(extensions);
			return ACTF_UNSUPPORTED_EXTENSION;
		}
	}

	preamble->version = json_object_get_int(version_jobj);
	preamble->uuid = uuid;
	preamble->has_uuid = has_uuid;
	preamble->attributes = attributes;
	preamble->extensions = extensions;
	return ACTF_OK;
}

static void actf_preamble_free(struct actf_preamble *preamble)
{
	if (!preamble) {
		return;
	}
	ctfjson_free(preamble->attributes);
	ctfjson_free(preamble->extensions);
}

static int actf_fld_cls_alias_parse(const struct json_object *fc_alias_jobj,
				    struct actf_metadata *metadata,
				    struct actf_fld_cls_alias *alias)
{
	int rc;
	struct error *e = &metadata->err;
	const char *name;
	if ((rc = json_object_key_get_string(fc_alias_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("name", "field-class-alias", e);
		return ACTF_MISSING_PROPERTY;
	}
	struct actf_fld_cls fc;
	CLEAR(fc);
	struct json_object *fc_jobj;
	if (!json_object_object_get_ex(fc_alias_jobj, "field-class", &fc_jobj)) {
		print_missing_key("field-class", "field-class-alias", e);
		return ACTF_MISSING_PROPERTY;
	}
	if ((rc = actf_fld_cls_parse(fc_jobj, metadata, &fc, e)) < 0) {
		eprependf(e, "field-class of field-class-alias %s", name);
		return rc;
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(fc_alias_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		actf_fld_cls_free(&fc);
		return rc;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(fc_alias_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		ctfjson_free(attributes);
		actf_fld_cls_free(&fc);
		return rc;
	}

	alias->name = name ? c_strdup(name) : NULL;
	alias->fld_cls = fc;
	alias->attributes = attributes;
	alias->extensions = extensions;
	return ACTF_OK;
}

static void actf_fld_cls_alias_free(struct actf_fld_cls_alias *alias)
{
	if (!alias) {
		return;
	}
	free(alias->name);
	actf_fld_cls_free(&alias->fld_cls);
	ctfjson_free(alias->attributes);
	ctfjson_free(alias->extensions);
}

/* Returns if the environment property of a trace class is formed
 * correctly. */
static bool environment_valid(struct ctfjson *environment, struct error *e)
{
	size_t len = actf_fld_struct_len(&environment->jval.val);
	for (size_t i = 0; i < len; i++) {
		const struct actf_fld *val = actf_fld_struct_fld_idx(&environment->jval.val, i);
		enum actf_fld_type type = actf_fld_type(val);
		if (type != ACTF_FLD_TYPE_SINT &&
		    type != ACTF_FLD_TYPE_UINT && type != ACTF_FLD_TYPE_STR) {
			eprintf(e,
				"environment is invalid in trace class, only JSON integers and strings are allowed.");
			return false;
		}
	}
	return true;
}

static int actf_trace_cls_parse(const struct json_object *tc_jobj,
				struct actf_metadata *metadata, struct actf_trace_cls *tc)
{
	int rc;
	struct error *e = &metadata->err;
	const char *namespace = NULL;
	if ((rc = json_object_key_get_string(tc_jobj, "namespace", &namespace, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	const char *name = NULL;
	if ((rc = json_object_key_get_string(tc_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	const char *uid = NULL;
	if ((rc = json_object_key_get_string(tc_jobj, "uid", &uid, e)) < 0 && rc != ACTF_NOT_FOUND) {
		return rc;
	}
	struct actf_fld_cls pkt_hdr;
	CLEAR(pkt_hdr);
	struct json_object *pkt_hdr_jobj;
	if (json_object_object_get_ex(tc_jobj, "packet-header-field-class", &pkt_hdr_jobj)) {
		if ((rc = actf_fld_cls_parse(pkt_hdr_jobj, metadata, &pkt_hdr, e)) < 0) {
			eprependf(e, "packet-header-field-class");
			return rc;
		}
	}
	struct ctfjson *environment = NULL;
	if ((rc = json_object_key_get_ctfjson(tc_jobj, "environment", &environment, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		actf_fld_cls_free(&pkt_hdr);
		return rc;
	} else if (rc == ACTF_OK && environment && !environment_valid(environment, e)) {
		actf_fld_cls_free(&pkt_hdr);
		ctfjson_free(environment);
		return ACTF_INVALID_ENVIRONMENT;
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(tc_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		actf_fld_cls_free(&pkt_hdr);
		ctfjson_free(environment);
		return rc;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(tc_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		ctfjson_free(attributes);
		actf_fld_cls_free(&pkt_hdr);
		ctfjson_free(environment);
		return rc;
	}

	tc->namespace = namespace ? c_strdup(namespace) : NULL;
	tc->name = name ? c_strdup(name) : NULL;
	tc->uid = uid ? c_strdup(uid) : NULL;
	tc->pkt_hdr = pkt_hdr;
	tc->environment = environment;
	tc->attributes = attributes;
	tc->extensions = extensions;
	return ACTF_OK;
}

/* Frees the content of `actf_trace_cls`. */
static void actf_trace_cls_free(struct actf_trace_cls *tc)
{
	if (!tc) {
		return;
	}
	free(tc->namespace);
	free(tc->name);
	free(tc->uid);
	actf_fld_cls_free(&tc->pkt_hdr);
	ctfjson_free(tc->environment);
	ctfjson_free(tc->attributes);
	ctfjson_free(tc->extensions);
}

static const char *actf_clk_origin_type_to_name(enum actf_clk_origin_type type)
{
	switch (type) {
	case ACTF_CLK_ORIGIN_TYPE_UNIX_EPOCH:
		return "unix-epoch";
	case ACTF_CLK_ORIGIN_TYPE_NONE:
	case ACTF_CLK_ORIGIN_TYPE_CUSTOM:
	case ACTF_CLK_ORIGIN_N_TYPES:
		return NULL;
	}
	return NULL;
}

static enum actf_clk_origin_type actf_clk_origin_name_to_type(const char *name)
{
	for (int i = 0; i < ACTF_CLK_ORIGIN_N_TYPES; i++) {
		const char *type_name = actf_clk_origin_type_to_name(i);
		if (type_name && strcmp(name, type_name) == 0) {
			return i;
		}
	}
	return ACTF_CLK_ORIGIN_TYPE_NONE;
}

static bool actf_clk_origin_eq(const struct actf_clk_origin *o1, const struct actf_clk_origin *o2)
{
	if (o1->type != o2->type) {
		return false;
	}
	if (o1->type != ACTF_CLK_ORIGIN_TYPE_CUSTOM) {
		return true;
	}
	if ((!!o1->custom.namespace != !!o2->custom.namespace) ||
	    (o1->custom.namespace && strcmp(o1->custom.namespace, o2->custom.namespace))) {
		return false;
	}
	if (strcmp(o1->custom.name, o2->custom.name)) {
		return false;
	}
	if (strcmp(o1->custom.uid, o2->custom.uid)) {
		return false;
	}
	return true;
}

bool actf_clk_cls_eq_identities(const struct actf_clk_cls *clkc1, const struct actf_clk_cls *clkc2)
{
	if ((!!clkc1->namespace != !!clkc2->namespace) ||
	    (clkc1->namespace && strcmp(clkc1->namespace, clkc2->namespace))) {
		return false;
	}
	if ((!!clkc1->name != !!clkc2->name) || (clkc1->name && strcmp(clkc1->name, clkc2->name))) {
		return false;
	}
	if ((!!clkc1->uid != !!clkc2->uid) || (clkc1->uid && strcmp(clkc1->uid, clkc2->uid))) {
		return false;
	}
	return true;
}

bool actf_clk_cls_eq_identities_strict(const struct actf_clk_cls *clkc1,
				       const struct actf_clk_cls *clkc2)
{
	if (!actf_clk_cls_eq_identities(clkc1, clkc2)) {
		return false;
	}
	if (clkc1->freq != clkc2->freq) {
		return false;
	}
	if ((clkc1->has_precision != clkc2->has_precision) ||
	    (clkc1->has_precision && clkc1->precision != clkc2->precision)) {
		return false;
	}
	if ((clkc1->has_accuracy != clkc2->has_accuracy) ||
	    (clkc1->has_accuracy && clkc1->accuracy != clkc2->accuracy)) {
		return false;
	}
	return actf_clk_origin_eq(&clkc1->origin, &clkc2->origin);
}

int64_t actf_clk_cls_cc_to_ns_from_origin(const struct actf_clk_cls *clkc, uint64_t tstamp_cc)
{
	uint64_t freq = clkc->freq;
	int64_t s_off = clkc->off_from_origin.seconds;
	uint64_t cc_off = clkc->off_from_origin.cycles;

	uint64_t cc = tstamp_cc + cc_off;
	int64_t s = s_off + (int64_t) (cc / freq);
	int64_t ns = s * INT64_C(1000000000) + ((cc % freq) * INT64_C(1000000000)) / freq;
	return ns;
}

static int actf_clk_origin_parse(struct json_object *clk_origin_jobj,
				 struct actf_clk_origin *origin, struct error *e)
{
	int rc;
	enum json_type jtype = json_object_get_type(clk_origin_jobj);
	if (jtype == json_type_string) {
		const char *name = json_object_get_string(clk_origin_jobj);
		enum actf_clk_origin_type type = actf_clk_origin_name_to_type(name);
		if (type == ACTF_CLK_ORIGIN_TYPE_NONE) {
			eprintf(e, "clock origin has name \"%s\" but no origin has that name",
				name);
			return ACTF_NO_SUCH_ORIGIN;
		}
		origin->type = type;
	} else if (jtype == json_type_object) {
		const char *namespace = NULL;
		if ((rc =
		     json_object_key_get_string(clk_origin_jobj, "namespace", &namespace, e)) < 0
		    && rc != ACTF_NOT_FOUND) {
			return rc;
		}
		const char *name;
		if ((rc = json_object_key_get_string(clk_origin_jobj, "name", &name, e)) < 0 &&
		    rc != ACTF_NOT_FOUND) {
			return rc;
		} else if (rc == ACTF_NOT_FOUND) {
			print_missing_key("name", "clock origin", e);
			return ACTF_MISSING_PROPERTY;
		}
		const char *uid;
		if ((rc = json_object_key_get_string(clk_origin_jobj, "uid", &uid, e)) < 0 &&
		    rc != ACTF_NOT_FOUND) {
			return rc;
		} else if (rc == ACTF_NOT_FOUND) {
			print_missing_key("uid", "clock origin", e);
			return ACTF_MISSING_PROPERTY;
		}
		origin->type = ACTF_CLK_ORIGIN_TYPE_CUSTOM;
		origin->custom.namespace = namespace ? c_strdup(namespace) : NULL;
		origin->custom.name = name ? c_strdup(name) : NULL;
		origin->custom.uid = uid ? c_strdup(uid) : NULL;
	} else {
		eprintf(e, "clock origin is not a string or an object");
		return ACTF_JSON_WRONG_TYPE;
	}
	return ACTF_OK;
}

static void actf_clk_origin_free(struct actf_clk_origin *origin)
{
	if (!origin) {
		return;
	}
	if (origin->type == ACTF_CLK_ORIGIN_TYPE_CUSTOM) {
		free(origin->custom.namespace);
		free(origin->custom.name);
		free(origin->custom.uid);
	}
}

static int actf_clk_offset_parse(const struct json_object *clk_off_jobj,
				 struct actf_clk_offset *off, struct error *e)
{
	int rc;
	if (!json_object_is_type(clk_off_jobj, json_type_object)) {
		print_wrong_json_type("clock offset", json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	int64_t seconds = 0;
	if ((rc = json_object_key_get_int64(clk_off_jobj, "seconds", &seconds, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}
	uint64_t cycles = 0;
	if ((rc = json_object_key_get_gtez_uint64(clk_off_jobj, "cycles", &cycles, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		return rc;
	}

	off->seconds = seconds;
	off->cycles = cycles;
	return ACTF_OK;
}

static int actf_clk_cls_parse(const struct json_object *clk_cls_jobj,
			      struct actf_clk_cls **clkcp, struct error *e)
{
	int rc;
	struct actf_clk_cls *clkc = malloc(sizeof(*clkc));
	if (!clkc) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	const char *id = NULL;
	if ((rc = json_object_key_get_string(clk_cls_jobj, "id", &id, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("id", "clock-class", e);
		rc = ACTF_MISSING_PROPERTY;
		goto err;
	}
	const char *namespace = NULL;
	if ((rc = json_object_key_get_string(clk_cls_jobj, "namespace", &namespace, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *name = NULL;
	if ((rc = json_object_key_get_string(clk_cls_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *uid = NULL;
	if ((rc = json_object_key_get_string(clk_cls_jobj, "uid", &uid, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	uint64_t freq = 0;
	if ((rc = json_object_key_get_gtz_uint64(clk_cls_jobj, "frequency", &freq, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	} else if (rc == ACTF_NOT_FOUND) {
		print_missing_key("frequency", "clock-class", e);
		rc = ACTF_MISSING_PROPERTY;
		goto err;
	}
	uint64_t precision = 0;
	if ((rc = json_object_key_get_gtez_uint64(clk_cls_jobj, "precision", &precision, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	bool has_precision = (rc == ACTF_OK);
	uint64_t accuracy = 0;
	if ((rc = json_object_key_get_gtez_uint64(clk_cls_jobj, "accuracy", &accuracy, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	bool has_accuracy = (rc == ACTF_OK);
	const char *desc = NULL;
	if ((rc = json_object_key_get_string(clk_cls_jobj, "description", &desc, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	struct actf_clk_origin origin;
	CLEAR(origin);
	struct json_object *origin_jobj;
	if (json_object_object_get_ex(clk_cls_jobj, "origin", &origin_jobj)) {
		if ((rc = actf_clk_origin_parse(origin_jobj, &origin, e)) < 0) {
			goto err;
		}
	}
	struct actf_clk_offset off_from_origin;
	CLEAR(off_from_origin);
	struct json_object *off_from_origin_jobj;
	if (json_object_object_get_ex(clk_cls_jobj, "offset-from-origin", &off_from_origin_jobj)) {
		if ((rc = actf_clk_offset_parse(off_from_origin_jobj, &off_from_origin, e)) < 0) {
			goto err;
		}
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(clk_cls_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(clk_cls_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto errattr;
	}

	clkc->id = id ? c_strdup(id) : NULL;
	clkc->namespace = namespace ? c_strdup(namespace) : NULL;
	clkc->name = name ? c_strdup(name) : NULL;
	clkc->uid = uid ? c_strdup(uid) : NULL;
	clkc->freq = freq;
	clkc->origin = origin;
	clkc->off_from_origin = off_from_origin;
	clkc->precision = precision;
	clkc->has_precision = has_precision;
	clkc->accuracy = accuracy;
	clkc->has_accuracy = has_accuracy;
	clkc->desc = desc ? c_strdup(desc) : NULL;
	clkc->attributes = attributes;
	clkc->extensions = extensions;
	*clkcp = clkc;
	return ACTF_OK;

      errattr:
	ctfjson_free(attributes);
      err:
	free(clkc);
	return rc;
}

static void actf_clk_cls_free(struct actf_clk_cls *clkc)
{
	if (!clkc) {
		return;
	}
	free(clkc->id);
	free(clkc->namespace);
	free(clkc->name);
	free(clkc->uid);
	actf_clk_origin_free(&clkc->origin);
	free(clkc->desc);
	ctfjson_free(clkc->attributes);
	ctfjson_free(clkc->extensions);
	free(clkc);
}

static int actf_event_cls_parse(const struct json_object *evc_jobj,
				const struct actf_metadata *metadata,
				struct actf_event_cls **evcp, struct error *e)
{
	int rc;
	struct actf_event_cls *evc = malloc(sizeof(*evc));
	if (!evc) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	uint64_t id = 0;
	if ((rc = json_object_key_get_gtez_uint64(evc_jobj, "id", &id, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	uint64_t dsc_id = 0;
	if ((rc = json_object_key_get_gtez_uint64(evc_jobj, "data-stream-class-id", &dsc_id, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *namespace = NULL;
	if ((rc = json_object_key_get_string(evc_jobj, "namespace", &namespace, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *name = NULL;
	if ((rc = json_object_key_get_string(evc_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *uid = NULL;
	if ((rc = json_object_key_get_string(evc_jobj, "uid", &uid, e)) < 0 && rc != ACTF_NOT_FOUND) {
		goto err;
	}
	struct actf_fld_cls payload;
	CLEAR(payload);
	struct json_object *payload_jobj;
	if (json_object_object_get_ex(evc_jobj, "payload-field-class", &payload_jobj)) {
		if ((rc = actf_fld_cls_parse(payload_jobj, metadata, &payload, e)) < 0) {
			eprependf(e, "payload-field-class");
			goto err;
		}
	}
	struct actf_fld_cls spec_ctx;
	CLEAR(spec_ctx);
	struct json_object *spec_ctx_jobj;
	if (json_object_object_get_ex(evc_jobj, "specific-context-field-class", &spec_ctx_jobj)) {
		if ((rc = actf_fld_cls_parse(spec_ctx_jobj, metadata, &spec_ctx, e)) < 0) {
			eprependf(e, "specific-context-field-class");
			goto errpayload;
		}
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(evc_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto errspecctx;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(evc_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto errattributes;
	}

	evc->id = id;
	evc->dsc_id = dsc_id;
	evc->dsc = NULL;
	evc->namespace = namespace ? c_strdup(namespace) : NULL;
	evc->name = name ? c_strdup(name) : NULL;
	evc->uid = uid ? c_strdup(uid) : NULL;
	evc->payload = payload;
	evc->spec_ctx = spec_ctx;
	evc->attributes = attributes;
	evc->extensions = extensions;
	*evcp = evc;
	return ACTF_OK;

      errattributes:
	ctfjson_free(attributes);
      errspecctx:
	actf_fld_cls_free(&spec_ctx);
      errpayload:
	actf_fld_cls_free(&payload);
      err:
	free(evc);
	return rc;
}

void actf_event_cls_free(struct actf_event_cls *evc)
{
	if (!evc) {
		return;
	}
	free(evc->namespace);
	free(evc->name);
	free(evc->uid);
	actf_fld_cls_free(&evc->payload);
	actf_fld_cls_free(&evc->spec_ctx);
	ctfjson_free(evc->attributes);
	ctfjson_free(evc->extensions);
	free(evc);
}

static int actf_dstream_cls_parse(const struct json_object *ds_cls_jobj,
				  const struct actf_metadata *metadata,
				  struct actf_dstream_cls **dscp, struct error *e)
{
	int rc;
	struct actf_dstream_cls *dsc = malloc(sizeof(*dsc));
	if (!dsc) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	uint64_t id = 0;
	if ((rc = json_object_key_get_gtez_uint64(ds_cls_jobj, "id", &id, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *namespace = NULL;
	if ((rc = json_object_key_get_string(ds_cls_jobj, "namespace", &namespace, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *name = NULL;
	if ((rc = json_object_key_get_string(ds_cls_jobj, "name", &name, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *uid = NULL;
	if ((rc = json_object_key_get_string(ds_cls_jobj, "uid", &uid, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto err;
	}
	const char *def_clkc_id = NULL;
	if ((rc =
	     json_object_key_get_string(ds_cls_jobj, "default-clock-class-id", &def_clkc_id, e)) < 0
	    && rc != ACTF_NOT_FOUND) {
		goto err;
	}
	struct actf_fld_cls pkt_ctx;
	CLEAR(pkt_ctx);
	struct json_object *pkt_ctx_jobj;
	if (json_object_object_get_ex(ds_cls_jobj, "packet-context-field-class", &pkt_ctx_jobj)) {
		if ((rc = actf_fld_cls_parse(pkt_ctx_jobj, metadata, &pkt_ctx, e)) < 0) {
			eprependf(e, "packet-context-field-class");
			goto err;
		}
	}
	struct actf_fld_cls event_hdr;
	CLEAR(event_hdr);
	struct json_object *event_hdr_jobj;
	if (json_object_object_get_ex(ds_cls_jobj, "event-record-header-field-class",
				      &event_hdr_jobj)) {
		if ((rc = actf_fld_cls_parse(event_hdr_jobj, metadata, &event_hdr, e)) < 0) {
			eprependf(e, "event-record-header-field-class");
			goto errpktctx;
		}
	}
	struct actf_fld_cls event_common_ctx;
	CLEAR(event_common_ctx);
	struct json_object *event_common_ctx_jobj;
	if (json_object_object_get_ex(ds_cls_jobj, "event-record-common-context-field-class",
				      &event_common_ctx_jobj)) {
		if ((rc =
		     actf_fld_cls_parse(event_common_ctx_jobj, metadata, &event_common_ctx,
					e)) < 0) {
			eprependf(e, "event-record-common-context-field-class");
			goto erreventhdr;
		}
	}
	struct ctfjson *attributes = NULL;
	if ((rc = json_object_key_get_ctfjson(ds_cls_jobj, "attributes", &attributes, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto errcommonctx;
	}
	struct ctfjson *extensions = NULL;
	if ((rc = json_object_key_get_ctfjson(ds_cls_jobj, "extensions", &extensions, e)) < 0 &&
	    rc != ACTF_NOT_FOUND) {
		goto errattributes;
	}
	u64toevc idtoevc;
	if (u64toevc_init(&idtoevc) < 0) {
		rc = ACTF_ERROR;
		goto errextensions;
	}

	dsc->id = id;
	dsc->namespace = namespace ? c_strdup(namespace) : NULL;
	dsc->name = name ? c_strdup(name) : NULL;
	dsc->uid = uid ? c_strdup(uid) : NULL;
	dsc->pkt_ctx = pkt_ctx;
	dsc->event_hdr = event_hdr;
	dsc->event_common_ctx = event_common_ctx;
	dsc->def_clkc_id = def_clkc_id ? c_strdup(def_clkc_id) : NULL;
	dsc->def_clkc = NULL;
	dsc->attributes = attributes;
	dsc->extensions = extensions;
	dsc->idtoevc = idtoevc;
	dsc->metadata = metadata;
	*dscp = dsc;
	return ACTF_OK;

      errextensions:
	ctfjson_free(extensions);
      errattributes:
	ctfjson_free(attributes);
      errcommonctx:
	actf_fld_cls_free(&event_common_ctx);
      erreventhdr:
	actf_fld_cls_free(&event_hdr);
      errpktctx:
	actf_fld_cls_free(&pkt_ctx);
      err:
	free(dsc);
	return rc;
}

void actf_dstream_cls_free(struct actf_dstream_cls *dsc)
{
	if (!dsc) {
		return;
	}
	free(dsc->namespace);
	free(dsc->name);
	free(dsc->uid);
	free(dsc->def_clkc_id);
	actf_fld_cls_free(&dsc->pkt_ctx);
	actf_fld_cls_free(&dsc->event_hdr);
	actf_fld_cls_free(&dsc->event_common_ctx);
	ctfjson_free(dsc->attributes);
	ctfjson_free(dsc->extensions);
	u64toevc_free(&dsc->idtoevc);
	free(dsc);
}

static int verify_clk_roles(struct actf_dstream_cls *dsc, struct actf_fld_cls *fc,
			    const char *ctx, struct error *e)
{
	int rc;
	if (fc->type == ACTF_FLD_CLS_STRUCT) {
		for (size_t i = 0; i < fc->cls.struct_.n_members; i++) {
			struct struct_fld_member_cls *member = &fc->cls.struct_.member_clses[i];
			if ((rc = verify_clk_roles(dsc, &member->cls, ctx, e)) < 0) {
				return rc;
			}
		}
		return ACTF_OK;
	} else {
		enum actf_role roles = actf_fld_cls_roles(fc);
		if (roles & ACTF_ROLE_DEF_CLK_TSTAMP && !dsc->def_clkc_id) {
			eprintf(e,
				"%s has role \"default-clock-timestamp\" but data-stream-class has no default clock",
				ctx);
			return ACTF_NO_DEFAULT_CLOCK;
		}
		if (roles & ACTF_ROLE_PKT_END_DEF_CLK_TSTAMP && !dsc->def_clkc_id) {
			eprintf(e,
				"%s has role \"packet-end-default-clock-timestamp\" but data-stream-class has no default clock",
				ctx);
			return ACTF_NO_DEFAULT_CLOCK;
		}
		return ACTF_OK;
	}
}

static int verify_metadata_stream_uuid_role(struct actf_preamble *preamble,
					    struct actf_fld_cls *fc, struct error *e)
{
	int rc;
	if (fc->type == ACTF_FLD_CLS_STRUCT) {
		for (size_t i = 0; i < fc->cls.struct_.n_members; i++) {
			struct struct_fld_member_cls *member = &fc->cls.struct_.member_clses[i];
			if ((rc = verify_metadata_stream_uuid_role(preamble, &member->cls, e)) < 0) {
				return rc;
			}
		}
		return ACTF_OK;
	} else {
		enum actf_role roles = actf_fld_cls_roles(fc);
		if (roles & ACTF_ROLE_METADATA_STREAM_UUID) {
			if (!preamble->has_uuid) {
				eprintf(e,
					"packet-header-field-class has role \"metadata-stream-uuid\" but preamble has no uuid");
				return ACTF_INVALID_UUID_ROLE;
			}
			if (fc->type != ACTF_FLD_CLS_STATIC_LEN_BLOB) {
				eprintf(e,
					"packet-header-field-class has role \"metadata-stream-uuid\" but it is not a static-length-blob");
				return ACTF_INVALID_UUID_ROLE;
			}
			if (fc->cls.static_len_blob.len != 16) {
				eprintf(e,
					"packet-header-field-class has role \"metadata-stream-uuid\" but its length is not the required 16");
				return ACTF_INVALID_UUID_ROLE;
			}
		}
		return ACTF_OK;
	}
}

static int int_verify_pkt_magic_num_role(struct actf_fld_cls *fc, bool is_top_lvl_struct,
					 bool is_first_member, struct error *e)
{
	int rc;
	if (fc->type == ACTF_FLD_CLS_STRUCT) {
		for (size_t i = 0; i < fc->cls.struct_.n_members; i++) {
			struct struct_fld_member_cls *member = &fc->cls.struct_.member_clses[i];
			bool is_top_level = is_top_lvl_struct
			    && (member->cls.type != ACTF_FLD_CLS_STRUCT);
			if ((rc =
			     int_verify_pkt_magic_num_role(&member->cls, is_top_level, i == 0,
							   e)) < 0) {
				return rc;
			}
		}
		return ACTF_OK;
	} else {
		enum actf_role roles = actf_fld_cls_roles(fc);
		if (roles & ACTF_ROLE_PKT_MAGIC_NUM && (!is_top_lvl_struct || !is_first_member)) {
			eprintf(e,
				"packet-header-field-class has role \"packet-magic-number\" but it is not its first member");
			return ACTF_INVALID_MAGIC_ROLE;
		}
	}
	return ACTF_OK;
}

/* Packet magic number MUST be the first member of the top-level structure field. */
static int verify_pkt_magic_num_role(struct actf_fld_cls *fc, struct error *e)
{
	return int_verify_pkt_magic_num_role(fc, true, false, e);
}

static int frag_parse(struct json_object *frag_jobj, struct actf_metadata *metadata)
{
	int rc;
	struct error *e = &metadata->err;
	if (!json_object_is_type(frag_jobj, json_type_object)) {
		print_wrong_json_type("metadata", json_type_object, e);
		return ACTF_JSON_WRONG_TYPE;
	}
	const char *type;
	if (json_object_key_get_string(frag_jobj, "type", &type, e) < 0) {
		print_missing_key("type", "metadata", e);
		return ACTF_MISSING_PROPERTY;
	}

	if (strcmp(type, "preamble") == 0) {
		if (metadata->preamble_is_set) {
			eprintf(e, "multiple preambles,"
				" a metadata stream must contain exactly one preamble");
			return ACTF_DUPLICATE_ERROR;
		}
		struct actf_preamble preamble;
		if ((rc = actf_preamble_parse(frag_jobj, &preamble, e)) < 0) {
			eprependf(e, "preamble");
			return rc;
		}
		if (preamble.version != ACTF_MAJOR_VERSION) {
			eprintf(e, "unsupported metadata stream version %d, only %d is supported",
				preamble.version, ACTF_MAJOR_VERSION);
			return ACTF_UNSUPPORTED_VERSION;
		}
		metadata->preamble = preamble;
		metadata->preamble_is_set = true;
	} else if (!metadata->preamble_is_set) {
		eprintf(e, "preamble is not the first fragment");
		return ACTF_NO_PREAMBLE;
	} else if (strcmp(type, "field-class-alias") == 0) {
		struct actf_fld_cls_alias alias;
		if ((rc = actf_fld_cls_alias_parse(frag_jobj, metadata, &alias)) < 0) {
			eprependf(e, "field-class-alias");
			return rc;
		}
		for (size_t i = 0; i < metadata->fld_cls_aliases.len; i++) {
			if (strcmp(actf_fld_cls_alias_vec_idx(&metadata->fld_cls_aliases, i)->name,
				   alias.name) == 0) {
				eprintf(e, "multiple field-class-alias with name %s", alias.name);
				actf_fld_cls_alias_free(&alias);
				return ACTF_DUPLICATE_ERROR;
			}
		}
		actf_fld_cls_alias_vec_push(&metadata->fld_cls_aliases, alias);
	} else if (strcmp(type, "trace-class") == 0) {
		if (metadata->trace_cls_is_set) {
			eprintf(e, "multiple trace classes,"
				" a metadata stream must contain max one trace-class");
			return ACTF_DUPLICATE_ERROR;
		}
		struct actf_trace_cls tc;
		if ((rc = actf_trace_cls_parse(frag_jobj, metadata, &tc)) < 0) {
			eprependf(e, "trace class");
			return rc;
		}
		if (tc.pkt_hdr.type != ACTF_FLD_CLS_NIL && tc.pkt_hdr.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e, "packet-header-field-class is not a structure field class");
			actf_trace_cls_free(&tc);
			return ACTF_NOT_A_STRUCT;
		}
		if ((rc = verify_pkt_magic_num_role(&tc.pkt_hdr, e)) < 0) {
			actf_trace_cls_free(&tc);
			return rc;
		}
		if ((rc =
		     verify_metadata_stream_uuid_role(&metadata->preamble, &tc.pkt_hdr, e)) < 0) {
			actf_trace_cls_free(&tc);
			return rc;
		}
		metadata->trace_cls = tc;
		metadata->trace_cls_is_set = true;
	} else if (strcmp(type, "clock-class") == 0) {
		struct actf_clk_cls *clkc = NULL;
		if ((rc = actf_clk_cls_parse(frag_jobj, &clkc, e)) < 0) {
			eprependf(e, "clock-class");
			return rc;
		}
		if (clkc->off_from_origin.cycles >= clkc->freq) {
			eprintf(e,
				"clock-class has a cycle offset %" PRIu64
				" that is greater than or equal the frequency %" PRIu64 "",
				clkc->off_from_origin.cycles, clkc->freq);
			actf_clk_cls_free(clkc);
			return ACTF_CC_GTE_FREQ_ERROR;
		}
		for (size_t i = 0; i < metadata->clk_clses.len; i++) {
			if (strcmp((*actf_clk_cls_vec_idx(&metadata->clk_clses, i))->id, clkc->id)
			    == 0) {
				eprintf(e, "multiple clock classes with id %s", clkc->id);
				actf_clk_cls_free(clkc);
				return ACTF_DUPLICATE_ERROR;
			}
		}
		actf_clk_cls_vec_push(&metadata->clk_clses, clkc);
	} else if (strcmp(type, "data-stream-class") == 0) {
		struct actf_dstream_cls *dsc = NULL;
		if ((rc = actf_dstream_cls_parse(frag_jobj, metadata, &dsc, e)) < 0) {
			eprependf(e, "data-stream-class");
			return rc;
		}
		if (dsc->pkt_ctx.type != ACTF_FLD_CLS_NIL &&
		    dsc->pkt_ctx.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e, "packet-context-field-class is not a structure field class");
			actf_dstream_cls_free(dsc);
			return ACTF_NOT_A_STRUCT;
		}
		if ((rc = verify_clk_roles(dsc, &dsc->pkt_ctx, "packet-context", e)) < 0) {
			actf_dstream_cls_free(dsc);
			return rc;
		}
		if (dsc->event_hdr.type != ACTF_FLD_CLS_NIL &&
		    dsc->event_hdr.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e,
				"event-record-header-field-class is not a structure field class");
			actf_dstream_cls_free(dsc);
			return ACTF_NOT_A_STRUCT;
		}
		if ((rc = verify_clk_roles(dsc, &dsc->event_hdr, "event-record-header", e)) < 0) {
			actf_dstream_cls_free(dsc);
			return rc;
		}
		if (dsc->event_common_ctx.type != ACTF_FLD_CLS_NIL &&
		    dsc->event_common_ctx.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e,
				"event-record-common-context-field-class is not a structure field class");
			actf_dstream_cls_free(dsc);
			return ACTF_NOT_A_STRUCT;
		}
		if (u64todsc_contains(&metadata->idtodsc, dsc->id)) {
			eprintf(e, "multiple data stream classes with the same id %" PRIu64 "",
				dsc->id);
			actf_dstream_cls_free(dsc);
			return ACTF_DUPLICATE_ERROR;
		}
		if (dsc->def_clkc_id) {
			for (size_t i = 0; i < metadata->clk_clses.len; i++) {
				struct actf_clk_cls **clkcp =
				    actf_clk_cls_vec_idx(&metadata->clk_clses, i);
				if (strcmp((*clkcp)->id, dsc->def_clkc_id) == 0) {
					dsc->def_clkc = *clkcp;
					break;
				}
			}
			if (!dsc->def_clkc) {
				eprintf(e,
					"data-stream-class (id %" PRIu64
					") refers to clock-class %s which does not exist",
					dsc->def_clkc_id, dsc->id);
				actf_dstream_cls_free(dsc);
				return ACTF_NO_SUCH_ID;
			}
		}
		rc = u64todsc_insert(&metadata->idtodsc, dsc->id, dsc);
		if (rc < 0) {
			eprintf(e,
				"unable to insert data stream class id %" PRIu64 " into hash map",
				dsc->id);
			actf_dstream_cls_free(dsc);
			return ACTF_ERROR;
		}
	} else if (strcmp(type, "event-record-class") == 0) {
		struct actf_event_cls *evc = NULL;
		if ((rc = actf_event_cls_parse(frag_jobj, metadata, &evc, e)) < 0) {
			eprependf(e, "event-record-class");
			free(evc);
			return rc;
		}
		if (evc->payload.type != ACTF_FLD_CLS_NIL &&
		    evc->payload.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e, "payload-field-class is not a structure field class");
			actf_event_cls_free(evc);
			return ACTF_NOT_A_STRUCT;
		}
		if (evc->spec_ctx.type != ACTF_FLD_CLS_NIL &&
		    evc->spec_ctx.type != ACTF_FLD_CLS_STRUCT) {
			eprintf(e, "specific-context-field-class is not a structure field class");
			actf_event_cls_free(evc);
			return ACTF_NOT_A_STRUCT;
		}
		struct actf_dstream_cls **dscp = u64todsc_find(&metadata->idtodsc, evc->dsc_id);
		if (!dscp) {
			eprintf(e,
				"event-record-class (id %" PRIu64
				") refers to data-stream-class id %" PRIu64 " which does not exist",
				evc->id, evc->dsc_id);
			actf_event_cls_free(evc);
			return ACTF_NO_SUCH_ID;
		}
		evc->dsc = *dscp;
		if (u64toevc_contains(&(*dscp)->idtoevc, evc->id)) {
			eprintf(e, "multiple event record classes with the same id %" PRIu64,
				evc->id);
			actf_event_cls_free(evc);
			return ACTF_DUPLICATE_ERROR;
		}

		rc = u64toevc_insert(&(*dscp)->idtoevc, evc->id, evc);
		if (rc < 0) {
			eprintf(e, "unable to insert event class id %" PRIu64 " into hash map",
				evc->id);
			actf_event_cls_free(evc);
			return ACTF_ERROR;
		}
	} else {
		eprintf(e, "%s is not a valid fragment type", type);
		return ACTF_JSON_ERROR;
	}

	return ACTF_OK;
}

/* parse_json_frags parses any found json objects as metadata
 * fragments. parse_json_frags and tok supports reading partial data,
 * which means buf does not need to contain a full json object or
 * fragment. If partial data is fed, parse_json_frags will return an
 * error and the error returned from json_tokener_get_error will be
 * json_tokener_continue. parse_json_frags does not write to
 * metadata->err since it is unable to determine if partial data is
 * valid or not. */
static int parse_json_frags(struct json_tokener *tok, const char *buf, size_t buf_len,
			    struct actf_metadata *metadata)
{
	int rc;
	const char *prev_loc = buf;

	while (prev_loc < (buf + buf_len)) {
		/* loc points TO the matching byte
		   | x | x | x | 0x1e | x | x | x |
		   0x0         0x3
		   prev_loc    loc
		 */
		const char *loc = memchr(prev_loc, 0x1e, buf_len - (prev_loc - buf));
		struct json_object *jobj;
		size_t frag_len;
		if (loc) {
			if (loc != prev_loc) {
				frag_len = loc - prev_loc;
			} else {
				frag_len = 0;
			}
		} else {	// All remaining data is assumed to be a frag
			frag_len = buf_len - (prev_loc - buf);
		}
		if (frag_len) {
			jobj = json_tokener_parse_ex(tok, prev_loc, frag_len);
			if (!jobj) {
				return ACTF_JSON_PARSE_ERROR;
			}

			if ((rc = frag_parse(jobj, metadata)) < 0) {
				json_object_put(jobj);
				return rc;
			}

			json_object_put(jobj);
		}

		prev_loc += frag_len + 1;
	}
	return ACTF_OK;
}

static inline bool is_metadata_stream_magic_valid(uint32_t magic)
{
	return (magic == METADATA_PKT_HDR_MAGIC || magic == bswap32(METADATA_PKT_HDR_MAGIC));
}

static bool is_metadata_stream_packetized(const char *b, size_t len)
{
	uint32_t magic;
	if (len < sizeof(magic)) {
		return false;
	}
	memcpy(&magic, b, sizeof(magic));
	return is_metadata_stream_magic_valid(magic);
}

/* read_metadata_pkt_hdr reads a full packet header.  */
static int read_metadata_pkt_hdr(const char *b, size_t len, struct metadata_pkt_hdr *hdr,
				 struct error *e)
{
	if (len < sizeof(*hdr)) {
		eprintf(e, "not enough bytes to read metadata packet header");
		return ACTF_INVALID_METADATA_PKT;
	}
	memcpy(hdr, b, sizeof(*hdr));
	if (!is_metadata_stream_magic_valid(hdr->magic)) {
		eprintf(e, "magic value in metadata packet header is incorrect; is %#"
			PRIx32 "; must be %#" PRIx32 " or %#" PRIx32, hdr->magic,
			METADATA_PKT_HDR_MAGIC, bswap32(METADATA_PKT_HDR_MAGIC));
		return ACTF_INVALID_METADATA_PKT;
	}
	/* The byte-order of magic determines the byte-order of the rest
	 * of the fields. */
	if ((hdr->magic == bswap32(METADATA_PKT_HDR_MAGIC))) {
		hdr->content_sz_bits = bswap32(hdr->content_sz_bits);
		hdr->total_sz_bits = bswap32(hdr->total_sz_bits);
		hdr->hdr_sz_bits = bswap32(hdr->hdr_sz_bits);
	}
	if (hdr->major != METADATA_PKT_HDR_MAJOR) {
		eprintf(e, "metadata packet header has unsupported major version %d", hdr->major);
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->minor != METADATA_PKT_HDR_MINOR) {
		eprintf(e, "metadata packet header has unsupported minor version %d", hdr->minor);
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->content_sz_bits % 8 != 0) {
		eprintf(e, "metadata packet content size is not a multiple of 8");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->total_sz_bits % 8 != 0) {
		eprintf(e, "metadata packet total size is not a multiple of 8");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->compression_scheme != 0) {
		eprintf(e, "metadata packet header is compressed");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->encryption_scheme != 0) {
		eprintf(e, "metadata packet header is encrypted");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->content_checksum != 0) {
		eprintf(e, "metadata packet header has a checksum");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->hdr_sz_bits != METADATA_PKT_HDR_SZ_BITS) {
		eprintf(e, "metadata packet header size is incorrect; is %#"
			PRIx32 "; must be %#" PRIx32, hdr->hdr_sz_bits, METADATA_PKT_HDR_SZ_BITS);
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->content_sz_bits < hdr->hdr_sz_bits) {
		eprintf(e, "metadata packet header's content size is smaller than header size");
		return ACTF_INVALID_METADATA_PKT;
	}
	if (hdr->total_sz_bits < hdr->content_sz_bits) {
		eprintf(e, "metadata packet header's total size is smaller than content size");
		return ACTF_INVALID_METADATA_PKT;
	}
	return 0;
}

/* unpack_packetized_metadata_stream reads the packetized metadata
 * stream in b and unpacks their content from the start of b without
 * any padding. The new length of the buffer is written to len. */
static int unpack_packetized_metadata_stream(const char *b, size_t len,
					     struct actf_metadata *metadata)
{
	int rc = 0;
	struct error *e = &metadata->err;
	size_t cur = 0;
	struct json_tokener *tok = json_tokener_new();
	enum json_tokener_error last_err = json_tokener_success;
	while (cur < len) {
		struct metadata_pkt_hdr hdr;
		rc = read_metadata_pkt_hdr(b + cur, len - cur, &hdr, e);
		if (rc < 0) {
			goto out;
		}
		size_t hdr_sz = hdr.hdr_sz_bits / 8;
		size_t content_sz = hdr.content_sz_bits / 8;
		size_t total_sz = hdr.total_sz_bits / 8;
		if (cur + content_sz > len) {
			eprintf(e, "not enough bytes to read metadata packet content");
			rc = ACTF_INVALID_METADATA_PKT;
			goto out;
		}
		size_t metadata_sz = content_sz - hdr_sz;
		const char *metadata_b = b + cur + hdr_sz;
		rc = parse_json_frags(tok, metadata_b, metadata_sz, metadata);
		last_err = json_tokener_get_error(tok);
		if (rc < 0 && last_err != json_tokener_continue) {
			eprintf(e, "json tokener parsing: %s", json_tokener_error_desc(last_err));
			goto out;
		}
		cur += total_sz;
	}
	if (last_err == json_tokener_continue) {
		eprintf(e, "json tokener parsing: not enough data");
	}
      out:
	json_tokener_free(tok);
	return rc;
}

int actf_metadata_parse_fd(struct actf_metadata *metadata, int fd)
{
	int rc;
	struct error *e = &metadata->err;
	struct stat sb;
	if (fstat(fd, &sb) < 0) {
		eprintf(e, "fstat: %s", strerror(errno));
		return ACTF_ERROR;
	}
	char *b = malloc(sb.st_size);
	if (!b) {
		eprintf(e, "malloc: %s", strerror(errno));
		return ACTF_OOM;
	}
	ssize_t n_read = read(fd, b, sb.st_size);
	if (n_read < 0) {
		free(b);
		eprintf(e, "read: %s", strerror(errno));
		return ACTF_ERROR;
	}

	rc = actf_metadata_nparse(metadata, b, n_read);

	free(b);
	return rc;
}

int actf_metadata_parse_file(struct actf_metadata *metadata, const char *path)
{
	int rc;
	struct error *e = &metadata->err;
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		eprintf(e, "unable to open metadata file for reading: %s", strerror(errno));
		return ACTF_ERROR;
	}

	rc = actf_metadata_parse_fd(metadata, fd);

	close(fd);
	return rc;
}

int actf_metadata_parse(struct actf_metadata *metadata, const char *str)
{
	return actf_metadata_nparse(metadata, str, strlen(str));
}

int actf_metadata_nparse(struct actf_metadata *metadata, const char *str, size_t len)
{
	int rc = 0;
	struct error *e = &metadata->err;
	if (is_metadata_stream_packetized(str, len)) {
		rc = unpack_packetized_metadata_stream(str, len, metadata);
	} else {
		struct json_tokener *tok = json_tokener_new();
		rc = parse_json_frags(tok, str, len, metadata);
		if (rc == ACTF_JSON_PARSE_ERROR) {
			enum json_tokener_error err = json_tokener_get_error(tok);
			const char *msg;
			if (err == json_tokener_continue) {
				msg = "not enough data";
			} else {
				msg = json_tokener_error_desc(err);
			}
			eprintf(e, "json tokener parsing: %s", msg);
		}
		json_tokener_free(tok);
	}
	return rc;
}

const char *actf_metadata_last_error(struct actf_metadata *metadata)
{
	if (!metadata || metadata->err.buf[0] == '\0') {
		return NULL;
	}
	return metadata->err.buf;
}

void actf_metadata_free(struct actf_metadata *metadata)
{
	if (!metadata) {
		return;
	}
	actf_preamble_free(&metadata->preamble);
	actf_trace_cls_free(&metadata->trace_cls);
	for (size_t i = 0; i < metadata->fld_cls_aliases.len; i++) {
		actf_fld_cls_alias_free(actf_fld_cls_alias_vec_idx(&metadata->fld_cls_aliases, i));
	}
	actf_fld_cls_alias_vec_free(&metadata->fld_cls_aliases);
	for (size_t i = 0; i < metadata->clk_clses.len; i++) {
		actf_clk_cls_free(*actf_clk_cls_vec_idx(&metadata->clk_clses, i));
	}
	actf_clk_cls_vec_free(&metadata->clk_clses);
	u64todsc_free(&metadata->idtodsc);
	error_free(&metadata->err);
	free(metadata);
}

const struct actf_preamble *actf_metadata_preamble(const struct actf_metadata *metadata)
{
	return metadata->preamble_is_set ? &metadata->preamble : NULL;
}

const struct actf_uuid *actf_preamble_uuid(const struct actf_preamble *preamble)
{
	return preamble->has_uuid ? &preamble->uuid : NULL;
}

const struct actf_fld *actf_preamble_attributes(const struct actf_preamble *preamble)
{
	return preamble->attributes ? &preamble->attributes->jval.val : NULL;
}

const struct actf_fld *actf_preamble_extensions(const struct actf_preamble *preamble)
{
	return preamble->extensions ? &preamble->extensions->jval.val : NULL;
}

const struct actf_trace_cls *actf_metadata_trace_cls(const struct actf_metadata *metadata)
{
	return metadata->trace_cls_is_set ? &metadata->trace_cls : NULL;
}

const char *actf_trace_cls_namespace(const struct actf_trace_cls *tc)
{
	return tc->namespace;
}

const char *actf_trace_cls_name(const struct actf_trace_cls *tc)
{
	return tc->name;
}

const char *actf_trace_cls_uid(const struct actf_trace_cls *tc)
{
	return tc->uid;
}

const struct actf_fld_cls *actf_trace_cls_pkt_hdr(const struct actf_trace_cls *tc)
{
	return &tc->pkt_hdr;
}

const struct actf_fld *actf_trace_cls_environment(const struct actf_trace_cls *tc)
{
	return tc->environment ? &tc->environment->jval.val : NULL;
}

const struct actf_fld *actf_trace_cls_attributes(const struct actf_trace_cls *tc)
{
	return tc->attributes ? &tc->attributes->jval.val : NULL;
}

const struct actf_fld *actf_trace_cls_extensions(const struct actf_trace_cls *tc)
{
	return tc->extensions ? &tc->extensions->jval.val : NULL;
}

size_t actf_metadata_fld_cls_aliases_len(const struct actf_metadata *metadata)
{
	return metadata->fld_cls_aliases.len;
}

const struct actf_fld_cls_alias *actf_metadata_fld_cls_aliases_idx(const struct actf_metadata
								   *metadata, size_t i)
{
	return actf_fld_cls_alias_vec_idx(&metadata->fld_cls_aliases, i);
}

const char *actf_fld_cls_alias_name(const struct actf_fld_cls_alias *fc_alias)
{
	return fc_alias->name;
}

const struct actf_fld_cls *actf_fld_cls_alias_fld_cls(const struct actf_fld_cls_alias *fc_alias)
{
	return &fc_alias->fld_cls;
}

const struct actf_fld *actf_fld_cls_alias_attributes(const struct actf_fld_cls_alias *fc_alias)
{
	return fc_alias->attributes ? &fc_alias->attributes->jval.val : NULL;
}

const struct actf_fld *actf_fld_cls_alias_extensions(const struct actf_fld_cls_alias *fc_alias)
{
	return fc_alias->extensions ? &fc_alias->extensions->jval.val : NULL;
}

size_t actf_metadata_clk_clses_len(const struct actf_metadata *metadata)
{
	return metadata->clk_clses.len;
}

const struct actf_clk_cls *actf_metadata_clk_clses_idx(const struct actf_metadata *metadata,
						       size_t i)
{
	return *actf_clk_cls_vec_idx(&metadata->clk_clses, i);
}

const char *actf_clk_cls_id(const struct actf_clk_cls *clkc)
{
	return clkc->id;
}

const char *actf_clk_cls_namespace(const struct actf_clk_cls *clkc)
{
	return clkc->namespace;
}

const char *actf_clk_cls_name(const struct actf_clk_cls *clkc)
{
	return clkc->name;
}

const char *actf_clk_cls_uid(const struct actf_clk_cls *clkc)
{
	return clkc->uid;
}

uint64_t actf_clk_cls_frequency(const struct actf_clk_cls *clkc)
{
	return clkc->freq;
}

const struct actf_clk_origin *actf_clk_cls_origin(const struct actf_clk_cls *clkc)
{
	return &clkc->origin;
}

const struct actf_clk_offset *actf_clk_cls_offset(const struct actf_clk_cls *clkc)
{
	return &clkc->off_from_origin;
}

uint64_t actf_clk_cls_precision(const struct actf_clk_cls *clkc)
{
	return clkc->has_precision ? clkc->precision : 0;
}

bool actf_clk_cls_has_precision(const struct actf_clk_cls *clkc)
{
	return clkc->has_precision;
}

uint64_t actf_clk_cls_accuracy(const struct actf_clk_cls *clkc)
{
	return clkc->has_accuracy ? clkc->accuracy : 0;
}

bool actf_clk_cls_has_accuracy(const struct actf_clk_cls *clkc)
{
	return clkc->has_accuracy;
}

const char *actf_clk_cls_description(const struct actf_clk_cls *clkc)
{
	return clkc->desc;
}

const struct actf_fld *actf_clk_cls_attributes(const struct actf_clk_cls *clkc)
{
	return clkc->attributes ? &clkc->attributes->jval.val : NULL;
}

const struct actf_fld *actf_clk_cls_extensions(const struct actf_clk_cls *clkc)
{
	return clkc->extensions ? &clkc->extensions->jval.val : NULL;
}

enum actf_clk_origin_type actf_clk_origin_type(const struct actf_clk_origin *origin)
{
	return origin->type;
}

const char *actf_clk_origin_custom_namespace(const struct actf_clk_origin *origin)
{
	if (origin->type != ACTF_CLK_ORIGIN_TYPE_CUSTOM) {
		return NULL;
	}
	return origin->custom.namespace;
}

const char *actf_clk_origin_custom_name(const struct actf_clk_origin *origin)
{
	if (origin->type != ACTF_CLK_ORIGIN_TYPE_CUSTOM) {
		return NULL;
	}
	return origin->custom.name;
}

const char *actf_clk_origin_custom_uid(const struct actf_clk_origin *origin)
{
	if (origin->type != ACTF_CLK_ORIGIN_TYPE_CUSTOM) {
		return NULL;
	}
	return origin->custom.uid;
}

int64_t actf_clk_offset_seconds(const struct actf_clk_offset *offset)
{
	return offset->seconds;
}

uint64_t actf_clk_offset_cycles(const struct actf_clk_offset *offset)
{
	return offset->cycles;
}

size_t actf_metadata_dstream_clses_len(const struct actf_metadata *metadata)
{
	return metadata->idtodsc.len;
}

const struct actf_dstream_cls *actf_metadata_dstream_clses_next(const struct actf_metadata
								*metadata, struct actf_it *it)
{
	struct u64todsc_it tmpit = {.i = (uintptr_t) it->data };
	uint64_t id;
	struct actf_dstream_cls *next = NULL;
	u64todsc_foreach(&metadata->idtodsc, &id, &next, &tmpit);
	it->data = (void *) (uintptr_t) tmpit.i;
	return next;
}

uint64_t actf_dstream_cls_id(const struct actf_dstream_cls *dsc)
{
	return dsc->id;
}

const char *actf_dstream_cls_namespace(const struct actf_dstream_cls *dsc)
{
	return dsc->namespace;
}

const char *actf_dstream_cls_name(const struct actf_dstream_cls *dsc)
{
	return dsc->name;
}

const char *actf_dstream_cls_uid(const struct actf_dstream_cls *dsc)
{
	return dsc->uid;
}

const char *actf_dstream_cls_clk_cls_id(const struct actf_dstream_cls *dsc)
{
	return dsc->def_clkc_id;
}

const actf_clk_cls *actf_dstream_cls_clk_cls(const actf_dstream_cls *dsc)
{
	return dsc->def_clkc;
}

const struct actf_fld_cls *actf_dstream_cls_pkt_ctx(const struct actf_dstream_cls *dsc)
{
	return &dsc->pkt_ctx;
}

const struct actf_fld_cls *actf_dstream_cls_event_hdr(const struct actf_dstream_cls *dsc)
{
	return &dsc->event_hdr;
}

const struct actf_fld_cls *actf_dstream_cls_event_common_ctx(const struct actf_dstream_cls *dsc)
{
	return &dsc->event_common_ctx;
}

const struct actf_fld *actf_dstream_cls_attributes(const struct actf_dstream_cls *dsc)
{
	return dsc->attributes ? &dsc->attributes->jval.val : NULL;
}

const struct actf_fld *actf_dstream_cls_extensions(const struct actf_dstream_cls *dsc)
{
	return dsc->extensions ? &dsc->extensions->jval.val : NULL;
}

size_t actf_dstream_cls_event_clses_len(const struct actf_dstream_cls *dsc)
{
	return dsc->idtoevc.len;
}

const struct actf_event_cls *actf_dstream_cls_event_clses_next(const struct actf_dstream_cls *dsc,
							       struct actf_it *it)
{
	struct u64toevc_it tmpit = {.i = (uintptr_t) it->data };
	uint64_t id;
	struct actf_event_cls *next = NULL;
	u64toevc_foreach(&dsc->idtoevc, &id, &next, &tmpit);
	it->data = (void *) (uintptr_t) tmpit.i;
	return next;
}

const actf_metadata *actf_dstream_cls_metadata(const actf_dstream_cls *dsc)
{
	return dsc->metadata;
}

uint64_t actf_event_cls_id(const struct actf_event_cls *evc)
{
	return evc->id;
}

uint64_t actf_event_cls_dstream_cls_id(const struct actf_event_cls *evc)
{
	return evc->dsc_id;
}

const actf_dstream_cls *actf_event_cls_dstream_cls(const actf_event_cls *evc)
{
	return evc->dsc;
}

const char *actf_event_cls_namespace(const struct actf_event_cls *evc)
{
	return evc->namespace;
}

const char *actf_event_cls_name(const struct actf_event_cls *evc)
{
	return evc->name;
}

const char *actf_event_cls_uid(const struct actf_event_cls *evc)
{
	return evc->uid;
}

const struct actf_fld_cls *actf_event_cls_spec_ctx(const struct actf_event_cls *evc)
{
	return &evc->spec_ctx;
}

const struct actf_fld_cls *actf_event_cls_payload(const struct actf_event_cls *evc)
{
	return &evc->payload;
}

const struct actf_fld *actf_event_cls_attributes(const struct actf_event_cls *evc)
{
	return evc->attributes ? &evc->attributes->jval.val : NULL;
}

const struct actf_fld *actf_event_cls_extensions(const struct actf_event_cls *evc)
{
	return evc->extensions ? &evc->extensions->jval.val : NULL;
}
