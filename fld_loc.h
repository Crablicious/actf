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
 * Field location related methods.
 */
#ifndef ACTF_FLD_LOC_H
#define ACTF_FLD_LOC_H

/** A field location */
typedef struct actf_fld_loc actf_fld_loc;

/** Field location origins */
enum actf_fld_loc_origin {
        ACTF_FLD_LOC_ORIGIN_NONE,
	ACTF_FLD_LOC_ORIGIN_PKT_HEADER,
	ACTF_FLD_LOC_ORIGIN_PKT_CTX,
	ACTF_FLD_LOC_ORIGIN_EVENT_HEADER,
	ACTF_FLD_LOC_ORIGIN_EVENT_COMMON_CTX,
	ACTF_FLD_LOC_ORIGIN_EVENT_SPECIFIC_CTX,
	ACTF_FLD_LOC_ORIGIN_EVENT_PAYLOAD,
	ACTF_FLD_LOC_N_ORIGINS,
};

/* Returns where the field location procedure should start. */
/**
 * Get the origin of the field location.
 * @param loc the field location
 * @return the origin
 */
enum actf_fld_loc_origin actf_fld_loc_origin(const actf_fld_loc *loc);

/**
 * Get the path of a field location
 *
 * Each path element is either a string denoting the name of a
 * structure field member to follow or null which means refers to the
 * parent structure field of the current locating context.
 *
 * @param loc the field location
 * @return the list of path elements, length is retrieved from
 * actf_fld_loc_path_len().
 */
char **actf_fld_loc_path(const actf_fld_loc *loc);

/**
 * Get the number of elements in the field location path
 * @param loc the field location
 * @return the number of elements in the path
 */
size_t actf_fld_loc_path_len(const actf_fld_loc *loc);

#endif /* ACTF_FLD_LOC_H */
