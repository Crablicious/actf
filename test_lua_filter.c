/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2026  Adam Wendelin <adwe live se>
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
#include "lua_filter.h"
#include "freader.h"
#include "test_lua_filter.h"
#include "test_freader.h"

static actf_freader *rd;

static int test_lua_filter_suite_init(void)
{
	return 0;
}

static int test_lua_filter_suite_clean(void)
{
	return 0;
}

static void test_lua_filter_test_setup(void)
{
	struct actf_freader_cfg cfg = {.dstream_evs_cap = 20,.muxer_evs_cap = 20 };
	rd = actf_freader_init(cfg);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rd);
	CU_ASSERT_EQUAL_FATAL(actf_freader_open_folder(rd, (char *) philo_nok_seek_test_trace), 0);
}

static void test_lua_filter_test_teardown(void)
{
	actf_freader_free(rd);
}

static void test_lua_filter_seek(void)
{
	actf_lua_filter *flt = actf_lua_filter_init(actf_freader_to_generator(rd), 20);
	CU_ASSERT_PTR_NOT_NULL_FATAL(flt);
	CU_ASSERT_EQUAL_FATAL(actf_lua_filter_lua_init(flt, "testdata/filter_none.lua", 0, NULL), ACTF_OK);

	philo_nok_seek_test(actf_lua_filter_to_generator(flt));

	CU_ASSERT_EQUAL(actf_lua_filter_lua_fini(flt), ACTF_OK);

	actf_lua_filter_free(flt);
}

static void test_lua_filter_filter(void)
{
	struct {
		char *filter_path;
		size_t tot_evs;
		int argc;
		char *argv[10];
	} tcs[] = {
		{.filter_path = "testdata/filter_all.lua", .tot_evs = 0},
		{.filter_path = "testdata/filter_every_other.lua", .tot_evs = 51},
		{.filter_path = "testdata/filter_none.lua", .tot_evs = 101},
		{.filter_path = "testdata/filter_hungry.lua", .tot_evs = 10},
		{.filter_path = "testdata/filter_timestamps.lua", .tot_evs = 64},
		{.filter_path = "testdata/filter_test.lua", .tot_evs = 101},
		{.filter_path = "testdata/filter_init_based.lua", .tot_evs = 51,
		 .argc = 2, .argv = {"begin", "end"}},
		{.filter_path = "testdata/filter_init_based.lua", .tot_evs = 0},
		{.filter_path = "testdata/filter_fail.lua", .tot_evs = 0},
	};

	for (size_t i = 0; i < ARRLEN(tcs); i++) {
		int rc;
		size_t tot_evs = 0, evs_len = 0;
		actf_event **evs = NULL;
		CU_ASSERT_EQUAL_FATAL(actf_freader_seek_ns_from_origin(rd, INT64_MIN), ACTF_OK);

		actf_lua_filter *flt = actf_lua_filter_init(actf_freader_to_generator(rd), 20);
		CU_ASSERT_PTR_NOT_NULL_FATAL(flt);

		CU_ASSERT_EQUAL_FATAL(actf_lua_filter_lua_init(flt, tcs[i].filter_path,
							       tcs[i].argc, tcs[i].argv), ACTF_OK);

		while ((rc = actf_lua_filter_filter(flt, &evs, &evs_len)) == 0 && evs_len) {
			tot_evs += evs_len;
		}
		/* The trace has an error which is expected, can't
		 * really differentiate it from a lua run error to
		 * print it because it gets coerced into a standard
		 * actf error. */
		/* if (rc < 0) { */
		/* 	printf("lua filter error: %d, %s\n", rc, actf_lua_filter_last_error(flt)); */
		/* } */

		if (tot_evs != tcs[i].tot_evs) {
			printf("%s: expected %zu; got %zu\n",
			       tcs[i].filter_path, tcs[i].tot_evs, tot_evs);
			CU_FAIL("wrong number of events read");
		}

		CU_ASSERT_EQUAL(actf_lua_filter_lua_fini(flt), ACTF_OK);

		actf_lua_filter_free(flt);
	}
}

static void test_lua_filter_init_fail(void)
{
	actf_lua_filter *flt = actf_lua_filter_init(actf_freader_to_generator(rd), 20);
	CU_ASSERT_PTR_NOT_NULL_FATAL(flt);

	const char *path = "testdata/filter_init_fail.lua";
	CU_ASSERT_NOT_EQUAL_FATAL(actf_lua_filter_lua_init(flt, path, 0, NULL),
				  ACTF_OK);

	actf_lua_filter_free(flt);
}

static void test_lua_filter_fini_fail(void)
{
	actf_lua_filter *flt = actf_lua_filter_init(actf_freader_to_generator(rd), 20);
	CU_ASSERT_PTR_NOT_NULL_FATAL(flt);

	const char *path = "testdata/filter_fini_fail.lua";
	CU_ASSERT_EQUAL_FATAL(actf_lua_filter_lua_init(flt, path, 0, NULL), ACTF_OK);

	int rc;
	size_t tot_evs = 0, evs_len = 0;
	actf_event **evs = NULL;
	while ((rc = actf_lua_filter_filter(flt, &evs, &evs_len)) == 0 && evs_len) {
		tot_evs += evs_len;
	}
	CU_ASSERT_EQUAL_FATAL(tot_evs, 101);

	CU_ASSERT_NOT_EQUAL(actf_lua_filter_lua_fini(flt), ACTF_OK);

	actf_lua_filter_free(flt);
}

static CU_TestInfo test_lua_filter_tests[] = {
	{ "seek", test_lua_filter_seek },
	{ "filter", test_lua_filter_filter },
	{ "init fail", test_lua_filter_init_fail },
	{ "fini fail", test_lua_filter_fini_fail },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_lua_filter_suite = {
	"Lua Filter", test_lua_filter_suite_init, test_lua_filter_suite_clean,
	test_lua_filter_test_setup, test_lua_filter_test_teardown, test_lua_filter_tests
};
