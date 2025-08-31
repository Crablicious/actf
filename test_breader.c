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
#include <stdint.h>

#include "breader.h"
#include "test_breader.h"


static struct breader br;


static int test_breader_suite_init(void)
{
    return 0;
}

static int test_breader_suite_clean(void)
{
    return 0;
}

static void test_breader_test_setup(void)
{
    return;
}

static void test_breader_test_teardown(void)
{
    return;
}

static void test_breader_basic_le(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    breader_init(data, sizeof(data), ACTF_LIL_ENDIAN, &br);
    CU_ASSERT_EQUAL(breader_refill(&br), MAX_READ_BITS);

    CU_ASSERT_EQUAL(breader_peek(&br, 1), 1);
    breader_consume(&br, 1);

    CU_ASSERT_EQUAL(breader_peek(&br, 7), 0);
    breader_consume(&br, 7);

    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0302);
    breader_consume(&br, 16);

    CU_ASSERT_EQUAL(breader_peek(&br, 32), 0x07060504);

    CU_ASSERT_EQUAL(breader_refill(&br), MAX_READ_BITS);

    CU_ASSERT_EQUAL(breader_peek(&br, 32), 0x07060504);
    breader_consume(&br, 32);

    CU_ASSERT_EQUAL(breader_peek(&br, 24), 0xab0908);
    breader_consume(&br, 24);

    CU_ASSERT_EQUAL(breader_refill(&br), 16);
    CU_ASSERT_EQUAL(breader_peek(&br, 12), 0xfcd);
    breader_consume(&br, 12);
    CU_ASSERT_EQUAL(breader_peek(&br, 4), 0xe);
    breader_consume(&br, 4);

    CU_ASSERT_EQUAL(breader_refill(&br), 0);

    breader_free(&br);
}

static void test_breader_basic_be(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    CU_ASSERT_EQUAL(breader_refill(&br), MAX_READ_BITS);

    CU_ASSERT_EQUAL(breader_peek(&br, 1), 0);
    breader_consume(&br, 1);

    CU_ASSERT_EQUAL(breader_peek(&br, 7), 0x1);
    breader_consume(&br, 7);

    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0203);
    breader_consume(&br, 16);

    CU_ASSERT_EQUAL(breader_peek(&br, 32), 0x04050607);

    CU_ASSERT_EQUAL(breader_refill(&br), MAX_READ_BITS);

    CU_ASSERT_EQUAL(breader_peek(&br, 32), 0x04050607);
    breader_consume(&br, 32);

    CU_ASSERT_EQUAL(breader_peek(&br, 24), 0x0809ab);
    breader_consume(&br, 24);

    CU_ASSERT_EQUAL(breader_refill(&br), 16);
    CU_ASSERT_EQUAL(breader_peek(&br, 12), 0xcde);
    breader_consume(&br, 12);
    CU_ASSERT_EQUAL(breader_peek(&br, 4), 0xf);
    breader_consume(&br, 4);

    CU_ASSERT_EQUAL(breader_refill(&br), 0);

    breader_free(&br);
}

static void test_breader_swap_bo(void)
{
    uint8_t data[] = {0x01, 0x02};
    breader_init(data, sizeof(data), ACTF_LIL_ENDIAN, &br);
    CU_ASSERT_EQUAL(breader_refill(&br), 16);

    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0201);

    /* "Swapping" to same bo should do nothing. */
    breader_set_bo(&br, ACTF_LIL_ENDIAN);
    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0201);

    breader_set_bo(&br, ACTF_BIG_ENDIAN);
    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0102);

    breader_set_bo(&br, ACTF_LIL_ENDIAN);
    CU_ASSERT_EQUAL(breader_peek(&br, 16), 0x0201);

    breader_consume(&br, 16);
    CU_ASSERT_EQUAL(breader_refill(&br), 0);

    breader_free(&br);
}

static void test_breader_bits_remaining(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    size_t n_bits = sizeof(data) * 8;
    CU_ASSERT_EQUAL(breader_bits_remaining(&br), n_bits);
    for (size_t i = 1; i <= n_bits; i++) {
	breader_refill(&br);
	breader_consume(&br, 1);
	CU_ASSERT_EQUAL(breader_bits_remaining(&br), n_bits - i);
    }
    breader_free(&br);

    /* Copy pasted for lil endian */
    breader_init(data, sizeof(data), ACTF_LIL_ENDIAN, &br);
    n_bits = sizeof(data) * 8;
    CU_ASSERT_EQUAL(breader_bits_remaining(&br), n_bits);
    for (size_t i = 1; i <= n_bits; i++) {
	breader_refill(&br);
	breader_consume(&br, 1);
	CU_ASSERT_EQUAL(breader_bits_remaining(&br), n_bits - i);
    }
    breader_free(&br);
}

static void test_breader_consume_checked(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    /* Test even byte boundary */
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 8), 0x01);
    breader_consume(&br, 8);

    breader_consume_checked(&br, 9 * 8);
    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 8), 0xcd);
    breader_consume(&br, 8);
    breader_free(&br);

    /* Test end mid-byte. */
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    breader_consume_checked(&br, 12);
    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 4), 0x2);
    breader_consume(&br, 4);
    breader_free(&br);

    /* Test overconsumption */
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    breader_consume_checked(&br, 20000);
    CU_ASSERT_EQUAL(breader_refill(&br), 0);
    breader_free(&br);
}

static void test_breader_read_bytes(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);
    uint8_t *ptr = breader_read_bytes(&br, 4);
    CU_ASSERT(memcmp(ptr, data, 4) == 0);

    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 7), 0x2);
    breader_consume(&br, 7);

    breader_align(&br, 8);

    ptr = breader_read_bytes(&br, 1);
    CU_ASSERT_EQUAL(*ptr, 0x06);

    ptr = breader_read_bytes(&br, 2);
    CU_ASSERT_EQUAL(*ptr, 0x07);
    CU_ASSERT_EQUAL(*(ptr+1), 0x08);

    CU_ASSERT_PTR_NULL(breader_read_bytes(&br, 5));

    ptr = breader_read_bytes(&br, 4);
    CU_ASSERT(memcmp(ptr, data + 8, 4) == 0);

    CU_ASSERT_PTR_NULL(breader_read_bytes(&br, 1));

    breader_free(&br);
}

static void test_breader_seek(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0xab, 0xcd, 0xef};
    breader_init(data, sizeof(data), ACTF_BIG_ENDIAN, &br);

    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 8), 0x1);

    breader_seek(&br, 2, BREADER_SEEK_SET);
    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 8), 0x3);

    breader_seek(&br, 0, BREADER_SEEK_END);
    CU_ASSERT_FALSE(breader_has_bits_remaining(&br));

    breader_seek(&br, 0, BREADER_SEEK_SET);
    breader_refill(&br);
    CU_ASSERT_EQUAL(breader_peek(&br, 8), 0x1);

    breader_seek(&br, 20, BREADER_SEEK_SET);
    CU_ASSERT_FALSE(breader_has_bits_remaining(&br));

    breader_seek(&br, 20, BREADER_SEEK_CUR);
    CU_ASSERT_FALSE(breader_has_bits_remaining(&br));

    breader_seek(&br, 0, BREADER_SEEK_SET);
    CU_ASSERT_TRUE(breader_has_bits_remaining(&br));
    breader_seek(&br, 1000, BREADER_SEEK_CUR);
    CU_ASSERT_FALSE(breader_has_bits_remaining(&br));

    breader_free(&br);
}

static CU_TestInfo test_breader_tests[] = {
    {"basic le", test_breader_basic_le},
    {"basic be", test_breader_basic_be},
    {"swap bo", test_breader_swap_bo},
    {"bits remaining", test_breader_bits_remaining},
    {"consume checked", test_breader_consume_checked},
    {"read bytes", test_breader_read_bytes},
    {"seek", test_breader_seek},
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_breader_suite = {
    "Breader", test_breader_suite_init, test_breader_suite_clean, test_breader_test_setup, test_breader_test_teardown, test_breader_tests
};
