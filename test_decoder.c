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
#include <fcntl.h>

#include "decoder.h"
#include "event_generator.h"
#include "test_decoder.h"


static char databuf[1024];

static int test_decoder_suite_init(void)
{
	return 0;
}

static int test_decoder_suite_clean(void)
{
	return 0;
}

static void test_decoder_test_setup(void)
{
	return;
}

static void test_decoder_test_teardown(void)
{
	return;
}

/* Reads all data from path into databuf and returns the size. */
static size_t read_file(const char *path)
{
	int fd = open(path, O_RDONLY);
	CU_ASSERT_FATAL(fd >= 0);
	ssize_t n = read(fd, databuf, sizeof(databuf));
	CU_ASSERT_FATAL(n >= 0);
	close(fd);
	return n;
}

static void test_decoder_basic(void)
{
	const char *ds_path = "testdata/ctfs/fxd_len_bit_arr_bo_mix/ds0";
	const char *metadata_path = "testdata/ctfs/fxd_len_bit_arr_bo_mix/metadata";
	struct actf_metadata *metadata = actf_metadata_init();
	CU_ASSERT_PTR_NOT_NULL_FATAL(metadata);
	CU_ASSERT_EQUAL_FATAL(actf_metadata_parse_file(metadata, metadata_path), 0);
	const char *fld_names[] = { "5-bit lil endian", "3-bit lil endian", "8-bit lil endian",
		"8-bit big endian", "3-bit big endian", "5-bit big endian", NULL
	};
	uint64_t ev0_expected_vals[] = { 0x1e, 0x7, 0xca, 0x37, 0x0, 0x13 };
	uint64_t ev1_expected_vals[] = { 0x13, 0x0, 0x37, 0xca, 0x7, 0x1e };
	uint64_t *expected_vals[] = { ev0_expected_vals, ev1_expected_vals };

	size_t len = read_file(ds_path);
	struct actf_decoder *dec = actf_decoder_init(databuf, len, 2, metadata);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dec);

	size_t evs_len;
	struct actf_event **evs;
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL(evs_len, 2);

	for (size_t i = 0; i < evs_len; i++) {
		struct actf_event *ev = evs[i];
		uint64_t *ev_expected_vals = expected_vals[i];
		for (size_t j = 0; fld_names[j]; j++) {
			const char *fld_name = fld_names[j];
			uint64_t expected_val = ev_expected_vals[j];
			const struct actf_fld *fld = actf_event_fld(ev, fld_name);
			CU_ASSERT_PTR_NOT_NULL_FATAL(fld);
			uint64_t val = actf_fld_uint64(fld);
			CU_ASSERT_EQUAL(val, expected_val);
		}
	}

	actf_decoder_free(dec);
	actf_metadata_free(metadata);
}

static void test_decoder_pkt_resumption(void)
{
	const char *ds_path = "testdata/ctfs/pkt_ctxt/ds0";	// 2 packets a 2 events
	const char *metadata_path = "testdata/ctfs/pkt_ctxt/metadata";
	uint64_t expected_vals[] = { 0xdeadbeef, 0xcafebabe, 0xfeedbabe, 0x1337beef };

	struct actf_metadata *metadata = actf_metadata_init();
	CU_ASSERT_PTR_NOT_NULL_FATAL(metadata);
	CU_ASSERT_EQUAL_FATAL(actf_metadata_parse_file(metadata, metadata_path), 0);
	size_t len = read_file(ds_path);
	struct actf_decoder *dec = actf_decoder_init(databuf, len, 1, metadata);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dec);
	size_t evs_len;
	struct actf_event **evs;

	for (int i = 0; i < 4; i++) {
		CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
		CU_ASSERT_EQUAL_FATAL(evs_len, 1);
		const struct actf_fld *fld = actf_event_fld(evs[0], "32-bit lil endian");
		CU_ASSERT_PTR_NOT_NULL_FATAL(fld);
		CU_ASSERT_EQUAL(actf_fld_uint64(fld), expected_vals[i]);
	}
	CU_ASSERT_EQUAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL(evs_len, 0);

	actf_decoder_free(dec);
	actf_metadata_free(metadata);
}

static void test_decoder_pkt_resumption_error(void)
{
	// 1 packet a 2 events, 1 packet a 1 ok, 1 nok event
	const char *ds_path = "testdata/ctfs/pkt_ctxt_cutoff_pkt_nok/ds0";
	const char *metadata_path = "testdata/ctfs/pkt_ctxt_cutoff_pkt_nok/metadata";
	uint64_t expected_vals[] = { 0xdeadbeef, 0xcafebabe, 0xfeedbabe };

	struct actf_metadata *metadata = actf_metadata_init();
	CU_ASSERT_PTR_NOT_NULL_FATAL(metadata);
	CU_ASSERT_EQUAL_FATAL(actf_metadata_parse_file(metadata, metadata_path), 0);
	size_t len = read_file(ds_path);
	struct actf_decoder *dec = actf_decoder_init(databuf, len, 3, metadata);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dec);
	size_t evs_len;
	struct actf_event **evs;

	/* Valid packet with 2 events */
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 2);
	const struct actf_fld *fld = actf_event_fld(evs[0], "32-bit lil endian");
	CU_ASSERT_PTR_NOT_NULL_FATAL(fld);
	CU_ASSERT_EQUAL(actf_fld_uint64(fld), expected_vals[0]);
	fld = actf_event_fld(evs[1], "32-bit lil endian");
	CU_ASSERT_PTR_NOT_NULL_FATAL(fld);
	CU_ASSERT_EQUAL(actf_fld_uint64(fld), expected_vals[1]);

	/* Invalid packet with 1 valid event and 1 invalid event, the
	 * decoding should give us 1 valid event first and then error on
	 * next call. */
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 1);
	fld = actf_event_fld(evs[0], "32-bit lil endian");
	CU_ASSERT_PTR_NOT_NULL_FATAL(fld);
	CU_ASSERT_EQUAL(actf_fld_uint64(fld), expected_vals[2]);
	CU_ASSERT_NOT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);

	actf_decoder_free(dec);
	actf_metadata_free(metadata);
}

static void test_decoder_seek(void)
{
	// 1 packet a 17 ok events, 1 packet a 3 ok, 1 nok event
	const char *ds_path = "testdata/ctfs/philo_nok/tid125101760";
	const char *metadata_path = "testdata/ctfs/philo_nok/metadata";

	struct actf_metadata *metadata = actf_metadata_init();
	CU_ASSERT_PTR_NOT_NULL_FATAL(metadata);
	CU_ASSERT_EQUAL_FATAL(actf_metadata_parse_file(metadata, metadata_path), 0);

	size_t len = read_file(ds_path);
	struct actf_decoder *dec = actf_decoder_init(databuf, len, 20, metadata);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dec);
	size_t evs_len;
	struct actf_event **evs;

	// seek to three last events in first packet
	CU_ASSERT_EQUAL_FATAL(actf_decoder_seek_ns_from_origin(dec, INT64_C(1750742599127877131)),
			      0);
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 3);

	// decode the second packet which will give an error
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 3);
	CU_ASSERT_NOT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);

	// seek to last valid event of second packet
	CU_ASSERT_EQUAL_FATAL(actf_decoder_seek_ns_from_origin(dec, UINT64_C(1750742599528026369)),
			      0);
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 1);
	// we do not trigger the error here, instead we reseek to second
	// event of first packet. this should not yield an error.
	CU_ASSERT_EQUAL_FATAL(actf_decoder_seek_ns_from_origin(dec, UINT64_C(1750742598727750064)),
			      0);
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 16);
	// decode second packet
	CU_ASSERT_EQUAL_FATAL(actf_decoder_decode(dec, &evs, &evs_len), 0);
	CU_ASSERT_EQUAL_FATAL(evs_len, 3);

	actf_decoder_free(dec);
	actf_metadata_free(metadata);
}

static CU_TestInfo test_decoder_tests[] = {
	{ "basic", test_decoder_basic },
	{ "pkt resumption", test_decoder_pkt_resumption },
	{ "pkt resumption error", test_decoder_pkt_resumption_error },
	{ "seek", test_decoder_seek },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_decoder_suite = {
	"Decoder", test_decoder_suite_init, test_decoder_suite_clean,
	test_decoder_test_setup, test_decoder_test_teardown, test_decoder_tests
};
