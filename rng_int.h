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

#ifndef ACTF_RNG_INT_H
#define ACTF_RNG_INT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rng.h"

/* I don't want to make rng_set or rng public to allow for a more
 * optimized implementation in the future. */

struct srng {
	int64_t lower;
	int64_t upper; // inclusive
};

struct urng {
	uint64_t lower;
	uint64_t upper; // inclusive
};

enum rng_type {
	RNG_SINT,
	RNG_UINT,
};

struct srng_set {
	struct srng *rngs;
	size_t len;
};

struct urng_set {
	struct urng *rngs;
	size_t len;
};

struct actf_rng_set {
	enum rng_type sign;
	union {
		struct srng_set srs;
		struct urng_set urs;
	} d;
};

size_t actf_rng_set_len(const struct actf_rng_set *rs);
bool srng_set_intersect(const struct srng_set *rs, int64_t val);
bool urng_set_intersect(const struct urng_set *rs, uint64_t val);

bool urng_intersect_urng(const struct urng *rng_a, const struct urng *rng_b);
bool srng_intersect_srng(const struct srng *rng_a, const struct srng *rng_b);

void actf_rng_set_free(struct actf_rng_set *rs);
void urng_set_free(struct urng_set *rs);
void srng_set_free(struct srng_set *rs);

#endif /* ACTF_RNG_INT_H */
