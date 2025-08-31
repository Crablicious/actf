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
#include <stdio.h>
#include <time.h>

#include "uival.h"
#include "rng_int.h"


#define N_QUERIES 1000000
#define N_RNGS 1024


enum meas_point {
    INSERTION,
    QUERYING,
    DELETION,
};

struct meas {
    const char *label;
    clock_t begin;
    clock_t end;
};

struct meas measurements[] = {
    [INSERTION] = {"insertion"},
    [QUERYING] = {"querying"},
    [DELETION] = {"deletion"},
};


static inline void begin_meas(enum meas_point pt)
{
    measurements[pt].begin = clock();
}

static inline void end_meas(enum meas_point pt)
{
    measurements[pt].end = clock();
}

static int benchmark_rb_tree(struct urng *rngs, size_t rngs_len,
			     uint64_t *queries, size_t n_queries)
{
    struct uival *ivals = malloc(rngs_len * sizeof(*ivals));
    if (! ivals) {
	perror("malloc");
	exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < rngs_len; i++) {
	ivals[i] = (struct uival){.lower = rngs[i].lower, .upper = rngs[i].upper};
    }
    struct rb_tree ivt;
    rb_tree_init(&ivt);

    begin_meas(INSERTION);
    for (size_t i = 0; i < rngs_len; i++) {
	uivalt_insert(&ivt, &ivals[i]);
    }
    end_meas(INSERTION);

    begin_meas(QUERYING);
    int total_intersects = 0;
    for (size_t i = 0; i < n_queries; i++) {
	struct uival *match = NULL;
	while ((match = uivalt_intersect_pt(&ivt, queries[i], match))) {
	    total_intersects++;
	}
    }
    end_meas(QUERYING);

    begin_meas(DELETION);
    for (size_t i = 0; i < rngs_len; i++) {
	uivalt_delete(&ivt, &ivals[i]);
    }
    end_meas(DELETION);

    free(ivals);
    return total_intersects;
}

static bool urng_set_intersect2(const struct urng_set *rs, uint64_t val, size_t *it)
{
    for (size_t i = *it; i < rs->len; i++) {
	struct urng rng = rs->rngs[i];
	if (val >= rng.lower && val <= rng.upper) {
	    *it = i + 1;
	    return true;
	}
    }
    return false;
}

static int benchmark_urng(struct urng *rngs, size_t rngs_len,
			  uint64_t *queries, size_t n_queries)
{
    struct urng_set rs = {.rngs = rngs, .len = rngs_len};
    begin_meas(INSERTION);
    end_meas(INSERTION);

    begin_meas(QUERYING);
    int total_intersects = 0;
    for (size_t i = 0; i < n_queries; i++) {
	size_t j = 0;
	while (urng_set_intersect2(&rs, queries[i], &j)) {
	    total_intersects++;
	}
    }
    end_meas(QUERYING);

    begin_meas(DELETION);
    end_meas(DELETION);
    return total_intersects;
}

static void print_measurements(const char *victim, int total_intersects)
{
    printf("%s\n", victim);
    printf("intersects: %d / %d\n", total_intersects, N_QUERIES);
    for (size_t i = 0; i < sizeof(measurements)/sizeof(*measurements); i++) {
	double used_time = ((double)(measurements[i].end - measurements[i].begin)) / CLOCKS_PER_SEC;
	printf("%s: %f\n", measurements[i].label, used_time);
    }
}

int main(int argc, char *argv[])
{
    (void)argc; // shush compiler
    (void)argv; // shush compiler
    printf("querying %d ranges %d times\n", N_RNGS, N_QUERIES);
    int total_intersects;
    struct urng rngs[N_RNGS];
    uint64_t next_lower = 0;
    for (size_t i = 0; i < N_RNGS; i++) {
	rngs[i].lower = next_lower;
	rngs[i].upper = next_lower + (rand() % 4);
	next_lower = rngs[i].upper + 2; // leave a gap to avoid 100% coverage
    }
    uint64_t queries[N_QUERIES];
    for (size_t i = 0; i < N_QUERIES; i++) {
	queries[i] = rand() % next_lower;
    }

    total_intersects = benchmark_urng(rngs, N_RNGS, queries, N_QUERIES);
    print_measurements("urng", total_intersects);

    total_intersects = benchmark_rb_tree(rngs, N_RNGS, queries, N_QUERIES);
    print_measurements("rb_tree", total_intersects);

    return EXIT_SUCCESS;
}
