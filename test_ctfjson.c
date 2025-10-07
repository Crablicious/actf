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

#include "ctfjson.h"
#include "test_ctfjson.h"


const char *attributes_path = "testdata/test_ctfjson_attributes.json";
const char *extensions_path = "testdata/test_ctfjson_extensions.json";
const char *types_path = "testdata/test_ctfjson_types.json";

static int test_ctfjson_suite_init(void)
{
	return 0;
}

static int test_ctfjson_suite_clean(void)
{
	return 0;
}

static void test_ctfjson_test_setup(void)
{
	return;
}

static void test_ctfjson_test_teardown(void)
{
	return;
}

static void test_ctfjson_extensions(void)
{
	struct json_object *jobj = json_object_from_file(extensions_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct ctfjson *j;
	CU_ASSERT(ctfjson_init(jobj, &j, NULL) == ACTF_OK);
	json_object_put(jobj);

	struct actf_fld *root = &j->jval.val;
	CU_ASSERT_FATAL(actf_fld_type(root) == ACTF_FLD_TYPE_STRUCT);
	CU_ASSERT_FATAL(actf_fld_struct_len(root) == 2);
	{
		const actf_fld *mytracer = actf_fld_struct_fld_idx(root, 0);
		CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer);
		CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, 0), "my.tracer");
		CU_ASSERT(actf_fld_type(mytracer) == ACTF_FLD_TYPE_STRUCT);
		CU_ASSERT_FATAL(actf_fld_struct_len(mytracer) == 2);
		{
			const actf_fld *piano = actf_fld_struct_fld_idx(mytracer, 0);
			CU_ASSERT_PTR_NOT_NULL_FATAL(piano);
			CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(mytracer, 0), "piano");
			CU_ASSERT(actf_fld_type(piano) == ACTF_FLD_TYPE_STRUCT);
			CU_ASSERT_FATAL(actf_fld_struct_len(piano) == 2);
			{
				const actf_fld *keys = actf_fld_struct_fld_idx(piano, 0);
				CU_ASSERT_PTR_NOT_NULL_FATAL(keys);
				CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(piano, 0),
						       "keys");
				CU_ASSERT(actf_fld_type(keys) == ACTF_FLD_TYPE_UINT);
				CU_ASSERT(actf_fld_int64(keys) == 88);

				const actf_fld *temperament = actf_fld_struct_fld_idx(piano, 1);
				CU_ASSERT_PTR_NOT_NULL_FATAL(temperament);
				CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(piano, 1),
						       "temperament");
				CU_ASSERT(actf_fld_type(temperament) == ACTF_FLD_TYPE_STR);
				CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(temperament), "equal");
			}
			const actf_fld *ramen = actf_fld_struct_fld_idx(mytracer, 1);
			CU_ASSERT_PTR_NOT_NULL_FATAL(ramen);
			CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(mytracer, 1), "ramen");
			CU_ASSERT(actf_fld_type(ramen) == ACTF_FLD_TYPE_SINT);
			CU_ASSERT(actf_fld_int64(ramen) == -23);
		}
	}
	{
		const actf_fld *abcxyz = actf_fld_struct_fld_idx(root, 1);
		CU_ASSERT_PTR_NOT_NULL_FATAL(abcxyz);
		CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, 1), "abc/xyz");
		CU_ASSERT(actf_fld_type(abcxyz) == ACTF_FLD_TYPE_STRUCT);
		CU_ASSERT_FATAL(actf_fld_struct_len(abcxyz) == 1);
		{
			const actf_fld *sax = actf_fld_struct_fld_idx(abcxyz, 0);
			CU_ASSERT_PTR_NOT_NULL_FATAL(sax);
			CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(abcxyz, 0), "sax");
			CU_ASSERT(actf_fld_type(sax) == ACTF_FLD_TYPE_STRUCT);
			CU_ASSERT_FATAL(actf_fld_struct_len(sax) == 1);
			{
				const actf_fld *variant = actf_fld_struct_fld_idx(sax, 0);
				CU_ASSERT_PTR_NOT_NULL_FATAL(variant);
				CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(sax, 0),
						       "variant");
				CU_ASSERT(actf_fld_type(variant) == ACTF_FLD_TYPE_STR);
				CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(variant), "alto");
			}
		}
	}
	ctfjson_free(j);
}

static void test_ctfjson_attributes(void)
{
	struct json_object *jobj = json_object_from_file(attributes_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct ctfjson *j;
	CU_ASSERT(ctfjson_init(jobj, &j, NULL) == ACTF_OK);
	json_object_put(jobj);

	struct actf_fld *root = &j->jval.val;
	CU_ASSERT_FATAL(actf_fld_type(root) == ACTF_FLD_TYPE_STRUCT);
	CU_ASSERT_FATAL(actf_fld_struct_len(root) == 2);
	{
		const actf_fld *mytracer = actf_fld_struct_fld_idx(root, 0);
		CU_ASSERT_PTR_NOT_NULL_FATAL(mytracer);
		CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, 0), "my.tracer");
		CU_ASSERT(actf_fld_type(mytracer) == ACTF_FLD_TYPE_STRUCT);
		CU_ASSERT_FATAL(actf_fld_struct_len(mytracer) == 2);
		{
			const actf_fld *maxcount = actf_fld_struct_fld_idx(mytracer, 0);
			CU_ASSERT_PTR_NOT_NULL_FATAL(maxcount);
			CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(mytracer, 0),
					       "max-count");
			CU_ASSERT(actf_fld_type(maxcount) == ACTF_FLD_TYPE_UINT);
			CU_ASSERT(actf_fld_uint64(maxcount) == 45);

			const actf_fld *module = actf_fld_struct_fld_idx(mytracer, 1);
			CU_ASSERT_PTR_NOT_NULL_FATAL(module);
			CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(mytracer, 1), "module");
			CU_ASSERT(actf_fld_type(module) == ACTF_FLD_TYPE_STR);
			CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(module), "sys");
		}
	}
	{
		const actf_fld *abcxyz = actf_fld_struct_fld_idx(root, 1);
		CU_ASSERT_PTR_NOT_NULL_FATAL(abcxyz);
		const char *abcxyz_name = actf_fld_struct_fld_name_idx(root, 1);
		CU_ASSERT_STRING_EQUAL(abcxyz_name, "abc/xyz");
		CU_ASSERT(actf_fld_type(abcxyz) == ACTF_FLD_TYPE_BOOL);
		CU_ASSERT_TRUE(actf_fld_bool(abcxyz));
	}
	ctfjson_free(j);
}

static void test_ctfjson_types(void)
{
	struct json_object *jobj = json_object_from_file(types_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(jobj);

	struct ctfjson *j;
	CU_ASSERT(ctfjson_init(jobj, &j, NULL) == ACTF_OK);
	json_object_put(jobj);

	struct actf_fld *root = &j->jval.val;
	CU_ASSERT_FATAL(actf_fld_type(root) == ACTF_FLD_TYPE_STRUCT);
	CU_ASSERT_FATAL(actf_fld_struct_len(root) == 8);

	int i = 0;
	const actf_fld *json_type_null = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_null);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_null");
	CU_ASSERT(actf_fld_type(json_type_null) == ACTF_FLD_TYPE_NIL);
	i++;

	const actf_fld *json_type_boolean = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_boolean);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_boolean");
	CU_ASSERT(actf_fld_type(json_type_boolean) == ACTF_FLD_TYPE_BOOL);
	CU_ASSERT_FALSE(actf_fld_bool(json_type_boolean));
	i++;

	const actf_fld *json_type_double = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_double);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_double");
	CU_ASSERT(actf_fld_type(json_type_double) == ACTF_FLD_TYPE_REAL);
	CU_ASSERT_DOUBLE_EQUAL(actf_fld_double(json_type_double), 13.37, 0.0001);
	i++;

	const actf_fld *json_type_uint = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_uint);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_uint");
	CU_ASSERT(actf_fld_type(json_type_uint) == ACTF_FLD_TYPE_UINT);
	CU_ASSERT(actf_fld_uint64(json_type_uint) == 42);
	i++;

	const actf_fld *json_type_sint = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_sint);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_sint");
	CU_ASSERT(actf_fld_type(json_type_sint) == ACTF_FLD_TYPE_SINT);
	CU_ASSERT(actf_fld_int64(json_type_sint) == -42);
	i++;

	const actf_fld *json_type_object = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_object);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_object");
	CU_ASSERT(actf_fld_type(json_type_object) == ACTF_FLD_TYPE_STRUCT);
	CU_ASSERT(actf_fld_struct_len(json_type_object) == 0);
	i++;

	const actf_fld *json_type_array = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_array);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_array");
	CU_ASSERT(actf_fld_type(json_type_array) == ACTF_FLD_TYPE_STRUCT);
	CU_ASSERT(actf_fld_struct_len(json_type_array) == 2);
	i++;
	{
		const actf_fld *i0 = actf_fld_struct_fld_idx(json_type_array, 0);
		CU_ASSERT_PTR_NOT_NULL_FATAL(i0);
		CU_ASSERT(actf_fld_type(i0) == ACTF_FLD_TYPE_UINT);
		CU_ASSERT(actf_fld_uint64(i0) == 12);

		const actf_fld *i1 = actf_fld_struct_fld_idx(json_type_array, 1);
		CU_ASSERT_PTR_NOT_NULL_FATAL(i1);
		CU_ASSERT(actf_fld_type(i1) == ACTF_FLD_TYPE_STR);
		CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(i1), "12");
	}
	const actf_fld *json_type_string = actf_fld_struct_fld_idx(root, i);
	CU_ASSERT_PTR_NOT_NULL_FATAL(json_type_string);
	CU_ASSERT_STRING_EQUAL(actf_fld_struct_fld_name_idx(root, i), "json_type_string");
	CU_ASSERT(actf_fld_type(json_type_string) == ACTF_FLD_TYPE_STR);
	CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(json_type_string), "");
	i++;

	ctfjson_free(j);
}

static CU_TestInfo test_ctfjson_tests[] = {
	{ "extensions", test_ctfjson_extensions },
	{ "attributes", test_ctfjson_attributes },
	{ "types", test_ctfjson_types },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_ctfjson_suite = {
	"CTF JSON", test_ctfjson_suite_init, test_ctfjson_suite_clean,
	test_ctfjson_test_setup, test_ctfjson_test_teardown, test_ctfjson_tests
};
