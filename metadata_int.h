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

#ifndef ACTF_METADATA_INT_H
#define ACTF_METADATA_INT_H

#include <stdbool.h>
#include <stdint.h>

#include "ctfjson.h"
#include "fld_cls_int.h"
#include "error.h"
#include "metadata.h"
#include "types.h"


struct actf_preamble {
	int version;
	struct actf_uuid uuid;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
	bool has_uuid;
};

struct actf_fld_cls_alias {
	char *name;
	struct actf_fld_cls fld_cls;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

struct actf_trace_cls {
	char *namespace;
	char *name;
	char *uid;
	struct ctfjson *environment;
	struct actf_fld_cls pkt_hdr;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

struct actf_clk_origin { // For correlation with other clocks.
	enum actf_clk_origin_type type;
	struct {
		char *namespace;
		char *name;
		char *uid;
	} custom; // Only used for ACTF_CLK_ORIGIN_TYPE_CUSTOM
};

struct actf_clk_offset {
	int64_t seconds;
	uint64_t cycles;
};

struct actf_clk_cls {
	char *id;
	char *namespace;
	char *name;
	char *uid;
	uint64_t freq;
	struct actf_clk_origin origin;
	struct actf_clk_offset off_from_origin;
	uint64_t precision;
	uint64_t accuracy;
	char *desc;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
	bool has_precision;
	bool has_accuracy;
};

/* dstream cls requires event_cls to be fully declared, but event_cls
 * wants to reference dstream_cls. */
struct actf_dstream_cls;

struct actf_event_cls {
	uint64_t id;
	uint64_t dsc_id;
	/* dsc is not a real property and purely here for convenience */
	struct actf_dstream_cls *dsc;
	char *namespace;
	char *name;
	char *uid;
	struct actf_fld_cls spec_ctx;
	struct actf_fld_cls payload;
	struct ctfjson *attributes;
	struct ctfjson *extensions;
};

void actf_event_cls_free(struct actf_event_cls *evc);

#define MAP_NAME u64toevc
#define MAP_KEY_TYPE uint64_t
#define MAP_KEY_CMP uint64cmp
#define MAP_VAL_TYPE struct actf_event_cls *
#define MAP_VAL_FREE actf_event_cls_free
#define MAP_HASH hash_uint64
#include "crust/map.h"

/* to allow dstream_cls to reference the metadata. */
struct actf_metadata;

/* TODO: I never explicitly "instantiate" this, should an event be
 * linked to a dstream which could hold source-unique properties
 * (e.g. trace = <ctf-dir>, stream = <ctf-dir>/<ds-file>). Who links to metadata?
 */
struct actf_dstream_cls {
	uint64_t id;
	char *name;
	char *namespace;
	char *uid;
	char *def_clkc_id;
	/* def_clkc is not a real property and purely here for
	 * convenience */
	struct actf_clk_cls *def_clkc;
	struct actf_fld_cls pkt_ctx;
	struct actf_fld_cls event_hdr;
	struct actf_fld_cls event_common_ctx;
	struct ctfjson *attributes;
	struct ctfjson *extensions;

	/* Holds all event record classes of this data stream class. Maps
	 * event class id to event class. */
	u64toevc idtoevc;
	const struct actf_metadata *metadata;
};

void actf_dstream_cls_free(struct actf_dstream_cls *dsc);

#define TYPE struct actf_fld_cls_alias
#define TYPED_NAME(x) actf_fld_cls_alias_##x
#include "crust/vec.h"

#define TYPE struct actf_clk_cls *
#define TYPED_NAME(x) actf_clk_cls_##x
#include "crust/vec.h"

#define MAP_NAME u64todsc
#define MAP_KEY_TYPE uint64_t
#define MAP_KEY_CMP uint64cmp
#define MAP_VAL_TYPE struct actf_dstream_cls *
#define MAP_VAL_FREE actf_dstream_cls_free
#define MAP_HASH hash_uint64
#include "crust/map.h"

struct actf_metadata {
	struct actf_preamble preamble; // MUST contain exactly one
	struct actf_fld_cls_alias_vec fld_cls_aliases; // MAY contain one or more
	struct actf_trace_cls trace_cls; // MAY contain one
	struct actf_clk_cls_vec clk_clses; // MAY contain one or more which MUST occur before a ds referring to it
	u64todsc idtodsc; // MUST contain one or more which MUST follow trace_cls
	bool preamble_is_set;
	bool trace_cls_is_set;
	struct error err;
};

const struct actf_fld_cls_alias *actf_metadata_find_fld_cls_alias(const struct actf_metadata
								  *metadata, const char *name);

#endif /* ACTF_METADATA_INT_H */
