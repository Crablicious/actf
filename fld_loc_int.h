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

#ifndef ACTF_FLD_LOC_INT_H
#define ACTF_FLD_LOC_INT_H

#include "fld_loc.h"
#include "crust/common.h"


struct actf_fld_loc {
	enum actf_fld_loc_origin origin;
	char **path;
	size_t path_len;
};

const char *actf_fld_loc_origin_name(enum actf_fld_loc_origin origin);

/* Returns ACTF_FLD_LOC_ORIGIN_NONE if no match. */
enum actf_fld_loc_origin actf_fld_loc_origin_name_to_type(const char *name);

/* Frees the content of `actf_fld_loc`. */
void actf_fld_loc_free(struct actf_fld_loc *loc);

#endif /* ACTF_FLD_LOC_INT_H */
