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
#include <json-c/json.h>
#include <stdio.h>

#include "test_fld_cls.h"
#include "fld_cls_int.h"


const char *fxd_len_int_attributes_path = "testdata/test_ctf_fld_cls_fxd_len_int_attributes.json";
const char *struct_attributes_path = "testdata/test_ctf_fld_cls_struct_attributes.json";
const char *variant_attributes_path = "testdata/test_ctf_fld_cls_variant_attributes.json";
const char *null_term_str_extensions_path =
    "testdata/test_ctf_fld_cls_null_term_str_extensions.json";

static int test_fld_cls_suite_init(void)
{
	return 0;
}

static int test_fld_cls_suite_clean(void)
{
	return 0;
}

static void test_fld_cls_test_setup(void)
{
	return;
}

static void test_fld_cls_test_teardown(void)
{
	return;
}

static void test_fld_cls_fxd_len_int_attributes(void)
{
	struct json_object *jobj = json_object_from_file(fxd_len_int_attributes_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct actf_fld_cls fc;
	CU_ASSERT_FATAL(actf_fld_cls_parse(jobj, NULL, &fc, NULL) == 0);

	const struct actf_fld *attrs = actf_fld_cls_attributes(&fc);
	CU_ASSERT_PTR_NOT_NULL_FATAL(attrs);
	const struct actf_fld *mytracer = actf_fld_struct_fld(attrs, "my.tracer");
	CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer);
	const struct actf_fld *isnice = actf_fld_struct_fld(mytracer, "is-nice");
	CU_ASSERT_PTR_NOT_NULL_FATAL(isnice);
	CU_ASSERT_TRUE(actf_fld_bool(isnice));

	json_object_put(jobj);
	actf_fld_cls_free(&fc);
}

static void test_fld_cls_struct_attributes(void)
{
	struct json_object *jobj = json_object_from_file(struct_attributes_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct actf_fld_cls struct_fc;
	CU_ASSERT_FATAL(actf_fld_cls_parse(jobj, NULL, &struct_fc, NULL) == 0);

	const struct actf_fld *struct_attrs = actf_fld_cls_attributes(&struct_fc);
	CU_ASSERT_PTR_NOT_NULL_FATAL(struct_attrs);
	const struct actf_fld *mytracer = actf_fld_struct_fld(struct_attrs, "my.tracer");
	CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer);
	const struct actf_fld *version = actf_fld_struct_fld(mytracer, "version");
	CU_ASSERT_PTR_NOT_NULL_FATAL(version);
	CU_ASSERT(actf_fld_uint64(version) == 4);

	CU_ASSERT(actf_fld_cls_members_len(&struct_fc) == 2);
	CU_ASSERT_PTR_NULL(actf_fld_cls_members_attributes_idx(&struct_fc, 0));

	const struct actf_fld *casgrain_attrs = actf_fld_cls_members_attributes_idx(&struct_fc, 1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(casgrain_attrs);
	const struct actf_fld *mytracer2 = actf_fld_struct_fld(casgrain_attrs, "my.tracer");
	CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer2);
	const struct actf_fld *ismask = actf_fld_struct_fld(mytracer2, "is-mask");
	CU_ASSERT_PTR_NOT_NULL_FATAL(ismask);
	CU_ASSERT_TRUE(actf_fld_bool(ismask));

	json_object_put(jobj);
	actf_fld_cls_free(&struct_fc);
}

static void test_fld_cls_variant_attributes(void)
{
	struct json_object *jobj = json_object_from_file(variant_attributes_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct actf_fld_cls variant_fc;
	CU_ASSERT_FATAL(actf_fld_cls_parse(jobj, NULL, &variant_fc, NULL) == 0);
	CU_ASSERT_PTR_NULL(actf_fld_cls_attributes(&variant_fc));

	CU_ASSERT(actf_fld_cls_options_len(&variant_fc) == 2);

	CU_ASSERT_STRING_EQUAL(actf_fld_cls_options_name_idx(&variant_fc, 0), "juice");
	const struct actf_fld *juice_attrs = actf_fld_cls_options_attributes_idx(&variant_fc, 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(juice_attrs);
	const struct actf_fld *mytracer = actf_fld_struct_fld(juice_attrs, "my.tracer");
	CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer);
	const struct actf_fld *uuid = actf_fld_struct_fld(mytracer, "uuid");
	CU_ASSERT_PTR_NOT_NULL_FATAL(uuid);
	const uint64_t expected_uuid[] =
	    { 243, 97, 0, 184, 236, 54, 72, 97, 141, 107, 169, 214, 171, 137, 115, 201 };
	for (size_t i = 0; i < actf_fld_struct_len(uuid); i++) {
		const struct actf_fld *uuid_i = actf_fld_struct_fld_idx(uuid, i);
		CU_ASSERT_EQUAL(actf_fld_uint64(uuid_i), expected_uuid[i]);
	}
	const struct actf_fld *isdid = actf_fld_struct_fld(mytracer, "is-did");
	CU_ASSERT_PTR_NOT_NULL_FATAL(isdid);
	CU_ASSERT_TRUE(actf_fld_bool(isdid));

	json_object_put(jobj);
	actf_fld_cls_free(&variant_fc);
}

static void test_fld_cls_null_term_str_extensions(void)
{
	struct json_object *jobj = json_object_from_file(null_term_str_extensions_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct actf_fld_cls str_fc;
	CU_ASSERT_FATAL(actf_fld_cls_parse(jobj, NULL, &str_fc, NULL) == 0);
	const struct actf_fld *exts = actf_fld_cls_extensions(&str_fc);
	CU_ASSERT_PTR_NOT_NULL_FATAL(exts);
	CU_ASSERT(actf_fld_struct_len(exts) == 2);

	json_object_put(jobj);
	actf_fld_cls_free(&str_fc);
}

static CU_TestInfo test_fld_cls_tests[] = {
	{ "fxd len int with attributes", test_fld_cls_fxd_len_int_attributes },
	{ "struct with attributes", test_fld_cls_struct_attributes },
	{ "variant with attributes", test_fld_cls_variant_attributes },
	{ "null term string with extensions", test_fld_cls_null_term_str_extensions },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_fld_cls_suite = {
	"Field Class", test_fld_cls_suite_init, test_fld_cls_suite_clean,
	test_fld_cls_test_setup, test_fld_cls_test_teardown, test_fld_cls_tests
};
