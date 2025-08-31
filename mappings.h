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
 * Mapping related methods.
 */
#ifndef ACTF_MAPPINGS_H
#define ACTF_MAPPINGS_H

#include <stddef.h>
#include <stdint.h>

#include "types.h"

/** Mappings map names to integer range sets. */
typedef struct actf_mappings actf_mappings;

/**
 * Get the number of mappings.
 * @param maps the mappings
 * @return the number of mappings in maps
 */
size_t actf_mappings_len(const actf_mappings *maps);

/**
 * Find the next name matching the provided value.
 *
 * A value can be mapped to several names, so to read them all out,
 * you should call actf_mappigns_find_uint() repeatedly with the same
 * iterator until NULL is returned.
 *
 * @param maps the mappings
 * @param val the value
 * @param it the iterator to use, it must be zero initialized on the
 * first call and then unmodified for subsequent calls.
 * @return a name or NULL if no more matches exist.
 */
const char *actf_mappings_find_uint(const actf_mappings *maps, uint64_t val, actf_it *it);

/**
 * Find the first name matching the provided value.
 *
 * Convenience function for when you only care about the first
 * matching name.
 *
 * @param maps the mappings
 * @param val the value
 * @return the name or NULL if no matches exist
 */
const char *actf_mappings_find_first_uint(const actf_mappings *maps, uint64_t val);

/** @see actf_mappings_find_uint() */
const char *actf_mappings_find_sint(const actf_mappings *maps, int64_t val, actf_it *it);

/** @see actf_mappings_find_first_uint() */
const char *actf_mappings_find_first_sint(const actf_mappings *maps, int64_t val);

#endif /* ACTF_MAPPINGS_H */
