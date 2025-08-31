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
 * Range set related methods.
 */
#ifndef ACTF_RNG_H
#define ACTF_RNG_H

#include <stdbool.h>
#include <stdint.h>


/** A range set. */
typedef struct actf_rng_set actf_rng_set;

/**
 * Check whether two range sets intersect.
 * @param rs_a range set a
 * @param rs_b range set b
 * @return whether range set a intersects range set b
 */
bool actf_rng_set_intersect_rng_set(const actf_rng_set *restrict rs_a, const actf_rng_set *restrict rs_b);

/**
 * Check if a range set intersects with a point
 * @param rs the range set
 * @param pt the point
 * @return whether the range intersects a point
 */
bool actf_rng_set_intersect_sint(const actf_rng_set *rs, int64_t pt);

/**
 * Check if a range set intersects with a point
 * @param rs the range set
 * @param pt the point
 * @return whether the range intersects a point
 */
bool actf_rng_set_intersect_uint(const actf_rng_set *rs, uint64_t pt);

#endif /* ACTF_RNG_H */
