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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "flags_int.h"
#include "rng_int.h"
#include "types.h"


/* Returns a bit mask representing the provided range set. */
static uint64_t make_bit_mask(struct urng_set *rs)
{
	uint64_t mask = 0;
	for (size_t i = 0; i < rs->len; i++) {
		struct urng rng = rs->rngs[i];
		uint64_t upper_mask;
		if (rng.upper < 63) {
			upper_mask = ((uint64_t) 1 << (rng.upper + 1)) - 1;
		} else {
			upper_mask = (uint64_t) 0xFFFFFFFFFFFFFFFF;
		}
		uint64_t lower_mask;
		if (rng.lower < 64) {
			lower_mask = ~(((uint64_t) 1 << rng.lower) - 1);
		} else {
			lower_mask = 0x0;
		}
		mask |= (upper_mask & lower_mask);
	}
	return mask;
}

int actf_flags_init(struct umappings *maps, struct actf_flags *f)
{
	uint64_t *masks = malloc(maps->len * sizeof(*masks));
	if (!masks) {
		return ACTF_OOM;
	}
	for (size_t i = 0; i < maps->len; i++) {
		struct urng_set *rs = &maps->rng_sets[i];
		masks[i] = make_bit_mask(rs);
	}
	f->maps = *maps;
	f->masks = masks;
	return ACTF_OK;
}

const char *actf_flags_find_first_match(const struct actf_flags *f, uint64_t val)
{
	actf_it it = { 0 };
	return actf_flags_find(f, val, &it);
}

const char *actf_flags_find(const struct actf_flags *f, uint64_t val, actf_it *it)
{
	for (uintptr_t i = (uintptr_t) it->data; i < (uintptr_t) f->maps.len; i++) {
		if (f->masks[i] & val) {
			it->data = (void *) (i + 1);
			return f->maps.names[i];
		}
	}
	return NULL;
}

void actf_flags_free(struct actf_flags *f)
{
	if (!f) {
		return;
	}
	umappings_free(&f->maps);
	free(f->masks);
}
