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
 * Event generator API
 *
 * Event generators are the basic building blocks of actf that are
 * hooked together to produce events, combine generators or perform
 * filtering on events.
 */
#ifndef ACTF_EVENT_GENERATOR_H
#define ACTF_EVENT_GENERATOR_H

#include <stdint.h>
#include "event.h"

/**
 * Generate events.
 *
 * A generator will write events up to its event capacity and provide
 * a pointer to the array holding them. An actf_event_generate() must
 * not return both events and an error code. On error, any valid
 * events should be returned and then on the next call, the error is
 * returned.
 *
 * The generator owns the events and any returned events (or their
 * fields) are invalid after the next actf_event_generate() call.
 *
 * @param self the generator's self parameter
 * @param evs a pointer to be populated with an event array
 * @param evs_len a pointer to be populated with the number of events
 * in the event array
 * @return ACTF_OK on success or an error code. On error, see actf_last_error().
 */
typedef int (*actf_event_generate)(void *self, actf_event ***evs, size_t *evs_len);

/**
 * Seek to the specified timestamp in the event stream.
 *
 * Any events previously returned by an actf_event_generate() are
 * invalid after calling this.
 *
 * @param self the generator's self parameter
 * @param tstamp the nanosecond from origin timestamp to seek to
 * @return ACTF_OK on success or an error code. On error, see actf_last_error().
 */
typedef int (*actf_seek_ns_from_origin)(void *self, int64_t tstamp);

/**
 * Get the last error message of a generator function.
 *
 * A failing generator function can store the most recent error and
 * return it through this function. The returned string is handled
 * internally, should not be freed, and might be overwritten or freed
 * on subsequent API calls.
 *
 * @param self the generator's self parameter
 * @return the last error message or NULL
 */
typedef const char *(*actf_last_error)(void *self);

/** The default capacity of an event array
 *
 * Napkin math for 32 data stream files:
 * - 40 bytes per actf_fld
 * - 16 fields per message allocated up front
 * - 40 * 16 = 640 bytes per event allocated per data stream file
 * - 640 * 32 data stream = 20480 bytes per event for 32 data stream files
 * - 20480 * 64 = 1310720 bytes = 1280 K = 1.25 M total for 32 data stream
 *   files each having 64 events per buffer
 */
#define ACTF_DEFAULT_EVS_CAP 64

/** An event generator */
struct actf_event_generator {
    /** See actf_event_generate() */
    actf_event_generate generate;
    /** See actf_seek_ns_from_origin() */
    actf_seek_ns_from_origin seek_ns_from_origin;
    /** See actf_last_error() */
    actf_last_error last_error;
    /** Opaque data, should be passed to the generator methods. */
    void *self;
};

/**
 * Allocate an event array with provided capacity.
 *
 * @param evs_cap the event capacity
 * @return an event array or NULL with errno set. A returned array
 * should be freed with actf_event_arr_free().
 */
actf_event **actf_event_arr_alloc(size_t evs_cap);

/**
 * Free an event array
 * @param evs the event array
 */
void actf_event_arr_free(actf_event **evs);

#endif /* ACTF_EVENT_GENERATOR_H */
