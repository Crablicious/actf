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

#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <limits.h>
#include <stdint.h>

#include "rng_int.h"
#include "test_rng.h"


static int test_rng_suite_init(void)
{
	return 0;
}

static int test_rng_suite_clean(void)
{
	return 0;
}

static void test_rng_test_setup(void)
{
	return;
}

static void test_rng_test_teardown(void)
{
	return;
}

static void test_rng_set_intersect_uint(void)
{
	struct urng a = {.lower = 1,.upper = 200 };
	struct urng b = {.lower = 150,.upper = 300 };
	struct urng rngs[] = { a, b };
	struct urng_set urs = {.rngs = rngs,.len = sizeof(rngs) / sizeof(*rngs) };
	struct actf_rng_set rs = {.sign = RNG_UINT,.d.urs = urs };
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 1));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 100));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 150));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 200));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 300));

	CU_ASSERT_FALSE(actf_rng_set_intersect_uint(&rs, 0));
	CU_ASSERT_FALSE(actf_rng_set_intersect_uint(&rs, 301));
	CU_ASSERT_FALSE(actf_rng_set_intersect_uint(&rs, UINT_MAX));

	CU_ASSERT_TRUE(actf_rng_set_intersect_sint(&rs, 1));
	CU_ASSERT_TRUE(actf_rng_set_intersect_sint(&rs, 300));
	CU_ASSERT_FALSE(actf_rng_set_intersect_sint(&rs, 0));
	CU_ASSERT_FALSE(actf_rng_set_intersect_sint(&rs, INT64_MAX));
	CU_ASSERT_FALSE(actf_rng_set_intersect_sint(&rs, -20));
}

static void test_rng_set_intersect_sint(void)
{
	struct srng a = {.lower = -100,.upper = 200 };
	struct srng b = {.lower = INT64_MAX,.upper = INT64_MAX };
	struct srng c = {.lower = INT64_MIN,.upper = INT64_MIN + 200 };
	struct srng rngs[] = { a, b, c };
	struct srng_set srs = {.rngs = rngs,.len = sizeof(rngs) / sizeof(*rngs) };
	struct actf_rng_set rs = {.sign = RNG_SINT,.d.srs = srs };
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 100));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 0));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, 200));
	CU_ASSERT_TRUE(actf_rng_set_intersect_uint(&rs, (uint64_t) INT64_MAX));

	CU_ASSERT_TRUE(actf_rng_set_intersect_sint(&rs, 0));
	CU_ASSERT_TRUE(actf_rng_set_intersect_sint(&rs, -100));
	CU_ASSERT_TRUE(actf_rng_set_intersect_sint(&rs, INT64_MAX));

	CU_ASSERT_FALSE(actf_rng_set_intersect_uint(&rs, (uint64_t) INT64_MAX + 1));
}

static void test_rng_set_intersect_rng_set(void)
{
	struct urng a0 = {.lower = 1,.upper = 200 };
	struct urng a1 = {.lower = 150,.upper = 300 };
	struct urng rngs_a[] = { a0, a1 };
	struct urng_set urs_a = {.rngs = rngs_a,.len = sizeof(rngs_a) / sizeof(*rngs_a) };
	struct actf_rng_set rs_a = {.sign = RNG_UINT,.d.urs = urs_a };

	struct urng b0 = {.lower = 250,.upper = 250 };
	struct urng b1 = {.lower = 600,.upper = 700 };
	struct urng rngs_b[] = { b0, b1 };
	struct urng_set urs_b = {.rngs = rngs_b,.len = sizeof(rngs_b) / sizeof(*rngs_b) };
	struct actf_rng_set rs_b = {.sign = RNG_UINT,.d.urs = urs_b };

	struct urng c0 = {.lower = 301,.upper = 400 };
	struct urng c1 = {.lower = 700,.upper = 800 };
	struct urng rngs_c[] = { c0, c1 };
	struct urng_set urs_c = {.rngs = rngs_c,.len = sizeof(rngs_c) / sizeof(*rngs_c) };
	struct actf_rng_set rs_c = {.sign = RNG_UINT,.d.urs = urs_c };

	CU_ASSERT_TRUE(actf_rng_set_intersect_rng_set(&rs_a, &rs_b));
	CU_ASSERT_TRUE(actf_rng_set_intersect_rng_set(&rs_b, &rs_c));
	CU_ASSERT_FALSE(actf_rng_set_intersect_rng_set(&rs_a, &rs_c));
}

static CU_TestInfo test_rng_tests[] = {
	{ "intersect uint", test_rng_set_intersect_uint },
	{ "intersect sint", test_rng_set_intersect_sint },
	{ "intersect actf_rng set", test_rng_set_intersect_rng_set },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_rng_suite = {
	"Range", test_rng_suite_init, test_rng_suite_clean, test_rng_test_setup,
	test_rng_test_teardown, test_rng_tests
};
