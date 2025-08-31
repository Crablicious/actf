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

#ifndef TEST_FREADER_H
#define TEST_FREADER_H

#include <CUnit/TestDB.h>

#include "event_generator.h"
#include "crust/common.h"

extern CU_SuiteInfo test_freader_suite;

/* test reading and seeking on an erronous trace. it has 101 events in
 * total. */
__maybe_unused
static const char *philo_nok_seek_test_trace = "testdata/ctfs/philo_nok";
void philo_nok_seek_test(struct actf_event_generator gen);

#endif /* TEST_FREADER_H */
