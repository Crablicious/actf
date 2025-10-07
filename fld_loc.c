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

#include <stdlib.h>
#include <string.h>

#include "fld_loc_int.h"

enum actf_fld_loc_origin actf_fld_loc_origin(const struct actf_fld_loc *loc)
{
	return loc->origin;
}

char **actf_fld_loc_path(const struct actf_fld_loc *loc)
{
	return loc->path;
}

size_t actf_fld_loc_path_len(const struct actf_fld_loc *loc)
{
	return loc->path_len;
}

const char *actf_fld_loc_origin_name(enum actf_fld_loc_origin origin)
{
	switch (origin) {
	case ACTF_FLD_LOC_ORIGIN_NONE:
		return NULL;
	case ACTF_FLD_LOC_ORIGIN_PKT_HEADER:
		return "packet-header";
	case ACTF_FLD_LOC_ORIGIN_PKT_CTX:
		return "packet-context";
	case ACTF_FLD_LOC_ORIGIN_EVENT_HEADER:
		return "event-record-header";
	case ACTF_FLD_LOC_ORIGIN_EVENT_COMMON_CTX:
		return "event-record-common-context";
	case ACTF_FLD_LOC_ORIGIN_EVENT_SPECIFIC_CTX:
		return "event-record-specific-context";
	case ACTF_FLD_LOC_ORIGIN_EVENT_PAYLOAD:
		return "event-record-payload";
	case ACTF_FLD_LOC_N_ORIGINS:
		return NULL;
	}
	return NULL;
}

enum actf_fld_loc_origin actf_fld_loc_origin_name_to_type(const char *name)
{
	for (size_t i = 0; i < ACTF_FLD_LOC_N_ORIGINS; i++) {
		const char *origin_name = actf_fld_loc_origin_name(i);
		if (origin_name && strcmp(name, origin_name) == 0) {
			return i;
		}
	}
	return ACTF_FLD_LOC_ORIGIN_NONE;
}

void actf_fld_loc_free(struct actf_fld_loc *loc)
{
	if (!loc) {
		return;
	}
	for (size_t i = 0; i < loc->path_len; i++) {
		free(loc->path[i]);
	}
	free(loc->path);
}
