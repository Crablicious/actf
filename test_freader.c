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
#include <stdio.h>

#include "event_generator.h"
#include "freader.h"
#include "test_freader.h"

static int test_freader_suite_init(void)
{
    return 0;
}

static int test_freader_suite_clean(void)
{
    return 0;
}

static void test_freader_test_setup(void)
{
    return;
}

static void test_freader_test_teardown(void)
{
    return;
}

void philo_nok_seek_test(struct actf_event_generator gen)
{
    int rc;
    size_t tot_evs = 0;
    size_t evs_len = 0;
    actf_event **evs = NULL;
    while ((rc = gen.generate(gen.self, &evs, &evs_len)) == 0 && evs_len) { tot_evs += evs_len; }
    CU_ASSERT_EQUAL_FATAL(tot_evs, 101);

    /* seek to mid of trace */
    tot_evs = 0;
    CU_ASSERT_EQUAL_FATAL(gen.seek_ns_from_origin(gen.self, INT64_C(1750742598927896798)), ACTF_OK);
    while ((rc = gen.generate(gen.self, &evs, &evs_len)) == 0 && evs_len) { tot_evs += evs_len; }
    CU_ASSERT_EQUAL_FATAL(tot_evs, 49);

    /* seek again to mid of trace, a read should be successful */
    tot_evs = 0;
    CU_ASSERT_EQUAL_FATAL(gen.seek_ns_from_origin(gen.self, INT64_C(1750742599127915322)), ACTF_OK);
    CU_ASSERT_EQUAL_FATAL(gen.generate(gen.self, &evs, &evs_len), ACTF_OK);
    CU_ASSERT_FATAL(evs_len > 0);

    /* seek to the last event before the error */
    tot_evs = 0;
    CU_ASSERT_EQUAL_FATAL(gen.seek_ns_from_origin(gen.self, INT64_C(1750742599528026369)), ACTF_OK);
    while ((rc = gen.generate(gen.self, &evs, &evs_len)) == 0 && evs_len) { tot_evs += evs_len; }
    CU_ASSERT_EQUAL_FATAL(tot_evs, 1);

    /* seek past the corrupt data. this used to give an error when we
     * read every event while seeking, but now we hop packet to packet
     * where none contains the selected time. perhaps it should be an
     * error because the number of available bits in the ds is less
     * than the packet length? */
    CU_ASSERT_EQUAL_FATAL(gen.seek_ns_from_origin(gen.self, INT64_C(1750802783000000000)), ACTF_OK);

    /* seek to the start and read whole trace again */
    CU_ASSERT_EQUAL_FATAL(gen.seek_ns_from_origin(gen.self, INT64_C(0)), ACTF_OK);
    tot_evs = 0;
    while ((rc = gen.generate(gen.self, &evs, &evs_len)) == 0 && evs_len) { tot_evs += evs_len; }
    CU_ASSERT_EQUAL_FATAL(tot_evs, 101);
}

static void test_freader_seek(void)
{
    // 101 events total
    struct actf_freader_cfg cfg = {.dstream_evs_cap = 20, .muxer_evs_cap = 20};
    actf_freader *rd = actf_freader_init(cfg);
    CU_ASSERT_PTR_NOT_NULL_FATAL(rd);
    CU_ASSERT_EQUAL_FATAL(actf_freader_open_folder(rd, (char *)philo_nok_seek_test_trace), 0);

    philo_nok_seek_test(actf_freader_to_generator(rd));

    actf_freader_free(rd);
}

static CU_TestInfo test_freader_tests[] = {
    {"seek", test_freader_seek},
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_freader_suite = {
    "Freader", test_freader_suite_init, test_freader_suite_clean,
    test_freader_test_setup, test_freader_test_teardown, test_freader_tests
};
