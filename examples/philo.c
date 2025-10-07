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

/* philo is a program that analyzes the dining philosophers trace in
 * <ACTF-ROOT>/testdata/ctfs/philo to showcase reading events, their
 * values and timestamps. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include actf, this is the only header that must be included. */
#include <actf/actf.h>

/* Each philosopher is represented by a thread with a tid in the
 * event-record-common-context of events. Each event has a field name
 * that describes what is happening. */

/* The states of the philosophers we're tracking. A philosopher
 * thinks, becomes hungry, grabs two forks, eats and then drops the
 * forks. We only consider the thinking and eating part. */
enum state { NONE, THINKING, EATING };

/* A philosopher */
struct philo {
	enum state state;
	int64_t state_begin;
	uint64_t total_thinking;
	uint64_t total_eating;
};

/* Define a hash map from tid to a philosopher */
#define MAP_NAME tidtophilo
#define MAP_KEY_TYPE uint64_t
#define MAP_KEY_CMP uint64cmp
#define MAP_VAL_TYPE struct philo *
#define MAP_VAL_FREE free
#define MAP_HASH hash_uint64
#include "../crust/map.h"


static uint64_t fld_tid(actf_event *ev)
{
	const actf_fld *f = actf_event_fld(ev, "tid");
	if (!f) {
		return UINT64_MAX;
	}
	return actf_fld_uint64(f);
}

static const char *fld_name(actf_event *ev)
{
	const actf_fld *f = actf_event_fld(ev, "name");
	if (!f) {
		return NULL;
	}
	return actf_fld_str_raw(f);
}

static struct philo *get_or_make_philo(tidtophilo *t2p, uint64_t tid)
{
	struct philo **pp = tidtophilo_find(t2p, tid);
	if (pp) {
		return *pp;
	}
	struct philo *p = calloc(1, sizeof(*p));
	tidtophilo_insert(t2p, tid, p);
	return p;
}

static struct philo *get_philo(tidtophilo *t2p, uint64_t tid)
{
	struct philo **pp = tidtophilo_find(t2p, tid);
	if (pp) {
		return *pp;
	}
	return NULL;
}

static void analyze(actf_event *ev, tidtophilo *t2p)
{
	const char *evname = actf_event_cls_name(actf_event_event_cls(ev));
	if (strcmp(evname, "begin") == 0) {
		const char *name = fld_name(ev);
		if (strcmp(name, "eating") == 0) {
			struct philo *p = get_or_make_philo(t2p, fld_tid(ev));
			p->state = EATING;
			p->state_begin = actf_event_tstamp_ns_from_origin(ev);
		} else if (strcmp(name, "thinking") == 0) {
			struct philo *p = get_or_make_philo(t2p, fld_tid(ev));
			p->state = THINKING;
			p->state_begin = actf_event_tstamp_ns_from_origin(ev);
		}
	} else if (strcmp(evname, "end") == 0) {
		struct philo *p = get_philo(t2p, fld_tid(ev));
		if (!p) {
			return;
		}
		const char *name = fld_name(ev);
		if (strcmp(name, "eating") == 0 && p->state == EATING) {
			p->total_eating += actf_event_tstamp_ns_from_origin(ev) - p->state_begin;
		} else if (strcmp(name, "thinking") == 0 && p->state == THINKING) {
			p->total_thinking += actf_event_tstamp_ns_from_origin(ev) - p->state_begin;
		}
		p->state = NONE;
	}
}

int main(int argc, char *argv[])
{
	int rc = 1;
	if (argc != 2) {
		fprintf(stderr,
			"Usage: %s PHILO_CTF_PATH\n"
			"Call with trace located at <ACTF-ROOT>/testdata/ctfs/philo\n", argv[0]);
		return rc;
	}

	/* Initialize a CTF-FS reader and open the provided folders with
	 * it. */
	struct actf_freader_cfg cfg = { 0 };
	actf_freader *rd = actf_freader_init(cfg);
	if (!rd) {
		perror("actf_freader_init");
		return rc;
	}
	if (actf_freader_open_folders(rd, argv + 1, argc - 1) < 0) {
		fprintf(stderr, "actf_freader_open_folder: %s\n", actf_freader_last_error(rd));
		goto err;
	}

	/* Initialize a tid to philosopher hash map. */
	struct tidtophilo t2p;
	rc = tidtophilo_init_cap(&t2p, 16);
	if (rc < 0) {
		goto err;
	}

	/* Read all events and count them. */
	size_t evs_len = 0;
	actf_event **evs = NULL;
	while ((rc = actf_freader_read(rd, &evs, &evs_len)) == 0 && evs_len) {
		for (size_t i = 0; i < evs_len; i++) {
			analyze(evs[i], &t2p);
		}
	}
	if (rc < 0) {
		fprintf(stderr, "actf_freader_read: %s\n", actf_freader_last_error(rd));
	}

	/* Loop through all keys and values in the hash map and print
	 * them. */
	uint64_t tid;
	struct philo *p;
	struct tidtophilo_it it = { 0 };
	while (tidtophilo_foreach(&t2p, &tid, &p, &it) == 0) {
		printf("tid %10" PRIu64 ": thought for %" PRIu64 " ns and ate for %" PRIu64 " ns\n",
		       tid, p->total_thinking, p->total_eating);
	}

	tidtophilo_free(&t2p);
err:
	actf_freader_free(rd);
	return rc;
}
