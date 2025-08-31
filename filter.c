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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "event.h"
#include "event_generator.h"
#include "filter.h"


enum filter_state {
    FILTER_STATE_FRESH,
    FILTER_STATE_ONGOING,
    FILTER_STATE_DONE,
    FILTER_STATE_ERROR,
};

struct actf_filter {
    struct actf_event_generator gen;
    struct actf_filter_time_range range;
    struct error err;
    enum filter_state state;
};

actf_filter *actf_filter_init(struct actf_event_generator gen,
			      struct actf_filter_time_range range)
{
    struct actf_filter *f = malloc(sizeof(*f));
    if (! f) {
	return NULL;
    }
    f->gen = gen;
    f->range = range;
    f->err = ERROR_EMPTY;
    f->state = FILTER_STATE_FRESH;
    return f;
}

/* ensure_range_has_dates makes sure that both boundaries include a
 * date. If they do not, their date is based on the first message of
 * the generator and the generator will have its state modified. */
static int ensure_range_has_dates(actf_filter *f)
{
    int rc = ACTF_OK;
    if (f->range.begin_has_date && f->range.end_has_date) {
	return rc;
    }
    actf_event **evs;
    size_t evs_len;
    rc = f->gen.generate(f->gen.self, &evs, &evs_len);
    if (rc < 0) {
	const char *msg = f->gen.last_error(f->gen.self);
	eprintf(&f->err, "generate: %s", msg ? msg : "unknown actf_event_generate error");
	f->state = FILTER_STATE_ERROR;
	return rc;
    }
    if (! evs_len) {
	return rc;
    }
    int64_t ns_from_origin = actf_event_tstamp_ns_from_origin(evs[0]);

    int64_t hhmmss_ns = INT64_C(24) * INT64_C(60) * INT64_C(60) * INT64_C(1000000000);
    int64_t date_off = ns_from_origin - (ns_from_origin % hhmmss_ns);
    if (! f->range.begin_has_date) {
	f->range.begin += date_off;
	f->range.begin_has_date = true;
    }
    if (! f->range.end_has_date) {
	f->range.end += date_off;
	f->range.end_has_date = true;
    }
    return rc;
}

int actf_filter_filter(actf_filter *f, actf_event ***evs, size_t *evs_len)
{
    int rc = ACTF_OK;
    if (f->state == FILTER_STATE_FRESH) {
	rc = ensure_range_has_dates(f);
	if (rc < 0) {
	    return rc;
	}
	rc = actf_filter_seek_ns_from_origin(f, f->range.begin);
	if (rc < 0) {
	    return rc;
	}
    }
    switch (f->state) {
    case FILTER_STATE_ONGOING:
	rc = f->gen.generate(f->gen.self, evs, evs_len);
	if (rc < 0) {
	    const char *msg = f->gen.last_error(f->gen.self);
	    eprintf(&f->err, "generate: %s", msg ? msg : "unknown actf_event_generate error");
	    f->state = FILTER_STATE_ERROR;
	    return rc;
	}
	/* ensure the end cutoff if set. looking at pkt end tstamp
	 * does not work since the generator can be based on multiple
	 * data streams with different packets. */
	if (f->range.end != INT64_MAX) {
	    size_t i;
	    for (i = 0; i < *evs_len && actf_event_tstamp_ns_from_origin((*evs)[i]) <= f->range.end; i++) {}
	    *evs_len = i;
	}
	if (*evs_len == 0) {
	    f->state = FILTER_STATE_DONE;
	}
	return ACTF_OK;
    case FILTER_STATE_DONE:
	*evs_len = 0;
	return ACTF_OK;
    case FILTER_STATE_ERROR:
	return ACTF_ERROR;
    default:
	return ACTF_INTERNAL;
    }
}

/* an actf_event_generate */
static int filter_filter(void *self, actf_event ***evs, size_t *evs_len)
{
    actf_filter *f = self;
    return actf_filter_filter(f, evs, evs_len);
}

int actf_filter_seek_ns_from_origin(actf_filter *f, int64_t tstamp)
{
    int rc = ACTF_OK;
    rc = ensure_range_has_dates(f);
    if (rc < 0) {
	return rc;
    }
    if (tstamp < f->range.begin) {
	tstamp = f->range.begin;
    } else if (tstamp > f->range.end) {
	f->state = FILTER_STATE_DONE;
	return ACTF_OK;
    }
    rc = f->gen.seek_ns_from_origin(f->gen.self, tstamp);
    if (rc < 0) {
	const char *msg = f->gen.last_error(f->gen.self);
	eprintf(&f->err, "seek_ns_from_origin: %s",
		msg ? msg : "unknown actf_seek_ns_from_origin error");
	f->state = FILTER_STATE_ERROR;
	return rc;
    }
    f->state = FILTER_STATE_ONGOING;
    return ACTF_OK;
}

/* an actf_seek_ns_from_origin */
static int filter_seek_ns_from_origin(void *self, int64_t tstamp)
{
    actf_filter *f = self;
    return actf_filter_seek_ns_from_origin(f, tstamp);
}

const char *actf_filter_last_error(actf_filter *f)
{
    if (! f || f->err.buf[0] == '\0') {
	return NULL;
    }
    return f->err.buf;
}

/* an actf_last_error */
const char *filter_last_error(void *self)
{
    actf_filter *f = self;
    return actf_filter_last_error(f);
}

void actf_filter_free(actf_filter *f)
{
    if (! f) {
	return;
    }
    error_free(&f->err);
    free(f);
}

struct actf_event_generator actf_filter_to_generator(actf_filter *f)
{
    return (struct actf_event_generator) {
	.generate = filter_filter,
	.seek_ns_from_origin = filter_seek_ns_from_origin,
	.last_error = filter_last_error,
	.self = f,
    };
}
