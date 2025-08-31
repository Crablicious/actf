/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2025  Adam Wendelin <adwe live se>
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

#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "crust/common.h"
#include "filter.h"
#include "freader.h"
#include "test_filter.h"
#include "test_freader.h"

static actf_freader *rd;

static int test_filter_suite_init(void)
{
    return 0;
}

static int test_filter_suite_clean(void)
{
    return 0;
}

static void test_filter_test_setup(void)
{
    struct actf_freader_cfg cfg = {.dstream_evs_cap = 20, .muxer_evs_cap = 20};
    rd = actf_freader_init(cfg);
    CU_ASSERT_PTR_NOT_NULL_FATAL(rd);
    CU_ASSERT_EQUAL_FATAL(actf_freader_open_folder(rd, (char *)philo_nok_seek_test_trace), 0);
}

static void test_filter_test_teardown(void)
{
    actf_freader_free(rd);
}

static void test_filter_seek(void)
{
    struct actf_filter_time_range rng = ACTF_FILTER_TIME_RANGE_ALL;
    actf_filter *flt = actf_filter_init(actf_freader_to_generator(rd), rng);

    philo_nok_seek_test(actf_filter_to_generator(flt));

    actf_filter_free(flt);
}

static void test_filter_filter(void)
{
    struct {
	struct actf_filter_time_range rng;
	size_t tot_evs;
    }tcs[] = {
	{{.begin = INT64_C(1750742598527225323), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 100},
	{{.begin = INT64_C(1750742598527225322), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 101},
	{{.begin = INT64_C(1750742598527225863), .begin_has_date = true,
	     .end = INT64_C(1750742599127893261), .end_has_date = true}, .tot_evs = 64},
	{{.begin = INT64_C(1750742599528018043), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 2},
	{{.begin = INT64_C(1750742599528018044), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 1},
	{{.begin = INT64_C(1750742599528026369), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 1},
	{{.begin = INT64_C(1750742599528026370), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 0},
	{{.begin = INT64_C(1750742783000000000), .begin_has_date = true,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 0},
	{{.begin = INT64_MIN, .begin_has_date = true,
	     .end = INT64_C(1750742599328014055), .end_has_date = true}, .tot_evs = 95},
	// no date is based on ns from origin without any timezone
	// adjustments.
	{{.begin = INT64_C(19398527225322), .begin_has_date = false,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 101},
	{{.begin = INT64_C(19398527225863), .begin_has_date = false,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 100},
	{{.begin = INT64_C(19399127915322), .begin_has_date = false,
	     .end = INT64_MAX, .end_has_date = true}, .tot_evs = 30},
	{{.begin = INT64_C(19398527225322), .begin_has_date = false,
	     .end = 19398527225322, .end_has_date = false}, .tot_evs = 1},
	{{.begin = INT64_C(19398527225322), .begin_has_date = false,
	     .end = 19398527225863, .end_has_date = false}, .tot_evs = 2},
    };

    for (size_t i = 0; i < ARRLEN(tcs); i++) {
	int rc;
	size_t tot_evs = 0, evs_len = 0;
	actf_event **evs = NULL;
	CU_ASSERT_EQUAL_FATAL(actf_freader_seek_ns_from_origin(rd, INT64_MIN), ACTF_OK);

	actf_filter *flt = actf_filter_init(actf_freader_to_generator(rd), tcs[i].rng);

	while ((rc = actf_filter_filter(flt, &evs, &evs_len)) == 0 && evs_len) { tot_evs += evs_len; }

	if (tot_evs != tcs[i].tot_evs) {
	    printf("[%"PRIi64"(%s), %"PRIi64"(%s)]: expected %zu; got %zu\n",
		   tcs[i].rng.begin, tcs[i].rng.begin_has_date ? "has date" : "no date",
		   tcs[i].rng.end, tcs[i].rng.end_has_date ? "has date" : "no date",
		   tcs[i].tot_evs, tot_evs);
	    CU_FAIL("wrong number of events read");
	}

	actf_filter_free(flt);
    }
}

static CU_TestInfo test_filter_tests[] = {
    {"seek", test_filter_seek},
    {"filter", test_filter_filter},
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_filter_suite = {
    "Filter", test_filter_suite_init, test_filter_suite_clean,
    test_filter_test_setup, test_filter_test_teardown, test_filter_tests
};
