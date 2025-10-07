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

#include "error.h"
#include "test_error.h"


static int test_error_suite_init(void)
{
	return 0;
}

static int test_error_suite_clean(void)
{
	return 0;
}

static void test_error_test_setup(void)
{
	return;
}

static void test_error_test_teardown(void)
{
	return;
}

static void test_eprintf(void)
{
	int rc;
	struct error e;
	size_t szes[] = { ERROR_DEFAULT_START_SZ, 0, 3 };
	for (size_t i = 0; i < sizeof(szes) / sizeof(*szes); i++) {
		rc = error_init(szes[i], &e);
		CU_ASSERT(rc == 0);
		rc = eprintf(&e, "");
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "");
		rc = eprintf(&e, "fortunate!");
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "fortunate!");
		rc = eprintf(&e, "uh oh, somebody did an oopsie: %s", "unlucky");
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "uh oh, somebody did an oopsie: unlucky");
		error_free(&e);
	}

	rc = eprintf(NULL, "null is fine", 1337);
	CU_ASSERT(rc == 0);
}

static void test_eprependf(void)
{
	int rc;
	struct error e;
	size_t szes[] = { ERROR_DEFAULT_START_SZ, 0, 4 };
	for (size_t i = 0; i < sizeof(szes) / sizeof(*szes); i++) {
		rc = error_init(szes[i], &e);
		CU_ASSERT(rc == 0);
		rc = eprintf(&e, "a");
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "a");
		rc = eprependf(&e, "b%d", 2);
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "b2: a");
		rc = eprependf(&e, "c");
		CU_ASSERT(rc == 0);
		CU_ASSERT_STRING_EQUAL(e.buf, "c: b2: a");
		error_free(&e);
	}

	rc = eprependf(NULL, "null is fine", 1337);
	CU_ASSERT(rc == 0);
}

static CU_TestInfo test_error_tests[] = {
	{ "eprintf", test_eprintf },
	{ "eprependf", test_eprependf },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_error_suite = {
	"Error", test_error_suite_init, test_error_suite_clean, test_error_test_setup,
	    test_error_test_teardown, test_error_tests
};
