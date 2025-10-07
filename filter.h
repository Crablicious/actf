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

/**
 * @file
 * A time-based event filter and its methods.
 */
#ifndef ACTF_FILTER_H
#define ACTF_FILTER_H

#include <stdbool.h>
#include <stdint.h>

#include "decoder.h"
#include "event_generator.h"

/** A time-based event filter, implements an actf_event_generator */
typedef struct actf_filter actf_filter;

/** A time range filter */
struct actf_filter_time_range {
	/** The (inclusive) start time of the filter in nanoseconds from
	 * origin. */
	int64_t begin;
	/** Whether the start time includes a date. If no date, the date
	 * will be based on the date of the first event. */
	bool begin_has_date;
	/** The (inclusive) end time of the filter in nanoseconds from
	 * origin. */
	int64_t end;
	/** Whether the end time includes a date. If no date, the date
	 * will be based on the date of the first event. */
	bool end_has_date;
};

/** A filter accepting all events from start of time to end of
 * time. */
#define ACTF_FILTER_TIME_RANGE_ALL (struct actf_filter_time_range)	\
    {.begin = INT64_MIN, .begin_has_date = true, .end = INT64_MAX, .end_has_date = true};

/**
 * Initialize a filter.
 * @param gen the generator to filter
 * @param range the time range to accept
 * @return a filter or NULL with errno set. A returned filter should
 * be freed with actf_filter_free().
 */
actf_filter *actf_filter_init(struct actf_event_generator gen, struct actf_filter_time_range range);

/** @see actf_event_generate */
int actf_filter_filter(actf_filter *f, actf_event ***evs, size_t *evs_len);

/** @see actf_seek_ns_from_origin */
int actf_filter_seek_ns_from_origin(actf_filter *f, int64_t tstamp);

/** @see actf_last_error */
const char *actf_filter_last_error(actf_filter *f);

/**
 * Free a filter
 * @param f the filter
 */
void actf_filter_free(actf_filter *f);

/**
 * Create an event generator based on a filter
 *
 * The filter is owned by the caller and must be kept alive as long as
 * the event generator is in use.
 *
 * @param f the filter
 * @return an event generator
 */
struct actf_event_generator actf_filter_to_generator(actf_filter *f);

#endif /* ACTF_FILTER_H */
