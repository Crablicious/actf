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

#include <stdlib.h>

#include "event_generator.h"
#include "event_int.h"


struct actf_event **actf_event_arr_alloc(size_t evs_cap)
{
    struct actf_event *inner_evs = malloc(evs_cap * sizeof(*inner_evs));
    if (! inner_evs) {
	return NULL;
    }
    struct actf_event **evs = malloc(evs_cap * sizeof(*evs));
    if (! evs) {
	free(inner_evs);
	return NULL;
    }
    for (size_t i = 0; i < evs_cap; i++) {
	evs[i] = &inner_evs[i];
    }
    return evs;
}

void actf_event_arr_free(struct actf_event **evs)
{
    if (evs) {
	free(evs[0]);
	free(evs);
    }
}
