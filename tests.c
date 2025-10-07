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
#include <CUnit/Basic.h>

#include "test_breader.h"
#include "test_decoder.h"
#include "test_filter.h"
#include "test_fld_cls.h"
#include "test_freader.h"
#include "test_ctfjson.h"
#include "test_metadata.h"
#include "test_rng.h"
#include "test_error.h"
#include "test_prio_queue.h"


int main(void)
{
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	CU_SuiteInfo suites[] = {
		test_metadata_suite,
		test_breader_suite,
		test_filter_suite,
		test_decoder_suite,
		test_fld_cls_suite,
		test_freader_suite,
		test_ctfjson_suite,
		test_rng_suite,
		test_error_suite,
		test_prio_queue_suite,
		CU_SUITE_INFO_NULL,
	};

	CU_ErrorCode error = CU_register_suites(suites);
	if (error != CUE_SUCCESS) {
		CU_cleanup_registry();
		return error;
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	unsigned n_fails = CU_get_number_of_tests_failed();
	CU_cleanup_registry();
	if (CU_get_error() != CUE_SUCCESS) {
		fprintf(stderr, "Cunit framework error after cleanup: %s\n", CU_get_error_msg());
	}
	return n_fails;
}
