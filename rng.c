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

#include <stddef.h>
#include <stdlib.h>

#include "crust/common.h"
#include "rng_int.h"


size_t actf_rng_set_len(const struct actf_rng_set *rs)
{
    if (rs->sign == RNG_SINT) {
	return rs->d.srs.len;
    } else {
	return rs->d.urs.len;
    }
}

bool urng_intersect_urng(const struct urng *rng_a, const struct urng *rng_b)
{
    return ! (rng_a->upper < rng_b->lower || rng_a->lower > rng_b->upper);
}

bool srng_intersect_srng(const struct srng *rng_a, const struct srng *rng_b)
{
    return ! (rng_a->upper < rng_b->lower || rng_a->lower > rng_b->upper);
}

__maybe_unused
static bool urng_intersect_srng(const struct urng *rng_a, const struct srng *rng_b)
{
    if (rng_b->upper < 0) {
	return false;
    }
    struct urng urng_b = {.lower = MAX(0, rng_b->lower), .upper = rng_b->upper};
    return urng_intersect_urng(rng_a, &urng_b);
}

static bool urng_set_intersect_urng(const struct urng_set *rs, const struct urng *rng)
{
    for (size_t i = 0; i < rs->len; i++) {
	if (urng_intersect_urng(&rs->rngs[i], rng)) {
	    return true;
	}
    }
    return false;
}

static bool srng_set_intersect_srng(const struct srng_set *rs, const struct srng *rng)
{
    for (size_t i = 0; i < rs->len; i++) {
	if (srng_intersect_srng(&rs->rngs[i], rng)) {
	    return true;
	}
    }
    return false;
}

static bool urng_set_intersect_srng(const struct urng_set *rs, const struct srng *rng)
{
    if (rng->upper < 0) {
	return false;
    }
    struct urng urng = {.lower = MAX(0, rng->lower), .upper = rng->upper};
    return urng_set_intersect_urng(rs, &urng);
}

bool actf_rng_set_intersect_rng_set(const struct actf_rng_set *restrict rs_a, const struct actf_rng_set *restrict rs_b)
{
    if (rs_a->sign == RNG_SINT && rs_b->sign == RNG_SINT) {
	const struct srng_set *a = &rs_a->d.srs;
	const struct srng_set *b = &rs_b->d.srs;
	for (size_t i = 0; i < a->len; i++) {
	    if (srng_set_intersect_srng(b, &a->rngs[i])) {
		return true;
	    }
	}
    } else if (rs_a->sign == RNG_UINT && rs_b->sign == RNG_UINT) {
	const struct urng_set *a = &rs_a->d.urs;
	const struct urng_set *b = &rs_b->d.urs;
	for (size_t i = 0; i < a->len; i++) {
	    if (urng_set_intersect_urng(b, &a->rngs[i])) {
		return true;
	    }
	}
    } else if (rs_a->sign == RNG_UINT && rs_b->sign == RNG_SINT) {
	const struct urng_set *a = &rs_a->d.urs;
	const struct srng_set *b = &rs_b->d.srs;
	for (size_t i = 0; i < b->len; i++) {
	    if (urng_set_intersect_srng(a, &b->rngs[i])) {
		return true;
	    }
	}
    } else {
	const struct srng_set *a = &rs_a->d.srs;
	const struct urng_set *b = &rs_b->d.urs;
	for (size_t i = 0; i < a->len; i++) {
	    if (urng_set_intersect_srng(b, &a->rngs[i])) {
		return true;
	    }
	}
    }
    return false;
}

bool actf_rng_set_intersect_sint(const struct actf_rng_set *rs, int64_t pt)
{
    if (rs->sign == RNG_SINT) {
	return srng_set_intersect(&rs->d.srs, pt);
    } else {
	if (pt < 0) {
	    return false;
	}
	uint64_t upt = pt;
	return urng_set_intersect(&rs->d.urs, upt);
    }
}

bool actf_rng_set_intersect_uint(const struct actf_rng_set *rs, uint64_t pt)
{
    if (rs->sign == RNG_SINT) {
	if (pt > INT64_MAX) {
	    return false;
	}
	int64_t ipt = pt;
	return srng_set_intersect(&rs->d.srs, ipt);
    } else {
	return urng_set_intersect(&rs->d.urs, pt);
    }
}

bool urng_set_intersect(const struct urng_set *rs, uint64_t pt)
{
    for (size_t i = 0; i < rs->len; i++) {
	struct urng rng = rs->rngs[i];
	if (pt >= rng.lower && pt <= rng.upper) {
	    return true;
	}
    }
    return false;
}

bool srng_set_intersect(const struct srng_set *rs, int64_t pt)
{
    for (size_t i = 0; i < rs->len; i++) {
	struct srng rng = rs->rngs[i];
	if (pt >= rng.lower && pt <= rng.upper) {
	    return true;
	}
    }
    return false;
}

void actf_rng_set_free(struct actf_rng_set *rs)
{
    if (! rs) {
	return;
    }
    if (rs->sign == RNG_SINT) {
	srng_set_free(&rs->d.srs);
    } else {
	urng_set_free(&rs->d.urs);
    }
}

void urng_set_free(struct urng_set *rs)
{
    if (! rs) {
	return;
    }
    free(rs->rngs);
}

void srng_set_free(struct srng_set *rs)
{
    if (! rs) {
	return;
    }
    free(rs->rngs);
}
