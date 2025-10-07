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

#ifndef ACTF_MAPPINGS_INT_H
#define ACTF_MAPPINGS_INT_H

#include "mappings.h"
#include "rng_int.h"
#include "crust/uival.h"
#include "error.h"


struct umappings {
	/* Contains an array of `len` elements, each of which corresponds
	 * to a range-set. Each range-set has a name in the same index in
	 * `names`. */
	struct urng_set *rng_sets;
	char **names;
	size_t len;
};

struct smappings {
	/* See actf_umappings */
	struct srng_set *rng_sets;
	char **names;
	size_t len;
};

struct mappings {
	enum rng_type sign;
	union {
		struct umappings umaps;
		struct smappings smaps;
	} d;
};

size_t mappings_len(const struct mappings *maps);

#define MAPPINGS_IT_BEGIN 0
const char *umappings_find_first(const struct umappings *maps, uint64_t val);
/* it must be initialized with MAPPINGS_IT_BEGIN first call. */
const char *umappings_find(const struct umappings *maps, uint64_t val, size_t *it);
const char *smappings_find_first(const struct smappings *maps, int64_t val);
/* it must be initialized with MAPPINGS_IT_BEGIN first call. */
const char *smappings_find(const struct smappings *maps, int64_t val, size_t *it);

void smappings_free(struct smappings *maps);
void umappings_free(struct umappings *maps);
void mappings_free(struct mappings *maps);


struct actf_umappings {
	// ivals for ALL mappings. Each ival has a pointer to its name in
	// names.
	struct uival *ivals;
	size_t ivals_len;
	// interval tree containing all ivals.
	struct rb_tree ivt;
	// names contains a name for each mapping.
	char **names;
	size_t names_len; // equivalent to number of mappings
};

struct actf_mappings {
	enum rng_type sign;
	union {
		struct uival *u;
		struct sival *s;
	} ivals;
	size_t ivals_len;
	struct rb_tree ivt;
	// names contains a name for each mapping.
	char **names;
	size_t names_len; // equivalent to number of mappings
};

/* actf_mappings_init initializes maps using raw_maps. It takes
 * ownership of raw_maps which should not be used anymore. */
int actf_mappings_init(struct mappings *raw_maps, struct actf_mappings *maps);
void actf_mappings_free(struct actf_mappings *maps);


#endif /* ACTF_MAPPINGS_INT_H */
