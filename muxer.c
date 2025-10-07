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

#include "crust/common.h"
#include "event.h"
#include "muxer.h"
#include "types.h"
#include "prio_queue.h"
#include "error.h"

/* A muxer needs to:
   - Take multiple data streams as input
   - Manage reading of the data streams
   - Generate an event buffer with events mixed from the data streams.
 */

enum muxer_state {
	MUXER_STATE_FRESH,
	MUXER_STATE_ONGOING,
	MUXER_STATE_DONE,
	MUXER_STATE_ERROR,
};

struct actf_muxer {
	struct actf_event_generator *gens;
	size_t gens_len;
	/* in_evs holds all event buffers for all generators. It holds a
	 * total of gens_len buffers. */
	actf_event ***in_evs;
	/* in_evs_lens holds the length of each buffer in in_evs. */
	size_t *in_evs_lens;
	/* in_evs_lens holds the current index of each buffer in
	 * in_evs. */
	size_t *in_evs_cur_is;
	/* evs holds the output buffer */
	actf_event **evs;
	/* evs_cap holds the capacity of evs */
	size_t evs_cap;
	struct error err;
	/* pq represents a priority queue of all event generators. The key
	 * is the timestamp and the value is the event generator index. */
	struct prio_queue pq;
	/* state represents the possible states of the muxer. */
	enum muxer_state state;
	/* pending_gen_i represents a generator pending a generate and a
	 * push to the pq. If pending_gen_i >= gens_len, no generator is
	 * pending. */
	size_t pending_gen_i;
};

actf_muxer *actf_muxer_init(struct actf_event_generator *gens, size_t gens_len, size_t evs_cap)
{
	struct actf_muxer *m = malloc(sizeof(*m));
	if (!m) {
		return NULL;
	}
	actf_event ***in_evs = malloc(gens_len * sizeof(*in_evs));
	if (!in_evs) {
		goto err_free_muxer;
	}
	size_t *in_evs_lens = calloc(gens_len, sizeof(*in_evs_lens));
	if (!in_evs_lens) {
		goto err_free_in_evs;
	}
	size_t *in_evs_cur_is = calloc(gens_len, sizeof(*in_evs_cur_is));
	if (!in_evs_cur_is) {
		goto err_free_in_evs_lens;
	}
	if (evs_cap == 0) {
		evs_cap = ACTF_DEFAULT_EVS_CAP;
	}
	actf_event **evs = actf_event_arr_alloc(evs_cap);
	if (!evs) {
		goto err_free_in_evs_cur_is;
	}
	struct prio_queue pq;
	if (prio_queue_init(&pq, gens_len) < 0) {
		goto err_free_event_arr;
	}
	*m = (struct actf_muxer) {
		.gens = gens,
		.gens_len = gens_len,
		.in_evs = in_evs,
		.in_evs_lens = in_evs_lens,
		.in_evs_cur_is = in_evs_cur_is,
		.evs = evs,
		.evs_cap = evs_cap,
		.err = ERROR_EMPTY,
		.pq = pq,
		.state = MUXER_STATE_FRESH,
		.pending_gen_i = gens_len,
	};
	return m;

      err_free_event_arr:
	actf_event_arr_free(evs);
      err_free_in_evs_cur_is:
	free(in_evs_cur_is);
      err_free_in_evs_lens:
	free(in_evs_lens);
      err_free_in_evs:
	free(in_evs);
      err_free_muxer:
	free(m);
	return NULL;
}

static inline bool has_pending_gen(actf_muxer *m)
{
	return m->pending_gen_i < m->gens_len;
}

static inline void set_no_pending_gen(actf_muxer *m)
{
	m->pending_gen_i = m->gens_len;
}

static inline void set_pending_gen(actf_muxer *m, size_t gen_i)
{
	m->pending_gen_i = gen_i;
}

static int push_fresh_evs_to_pq(actf_muxer *m, size_t gen_i)
{
	int rc;
	actf_event ***in_evs = m->in_evs + gen_i;
	size_t *in_evs_len = m->in_evs_lens + gen_i;
	size_t *in_evs_i = m->in_evs_cur_is + gen_i;
	*in_evs_i = 0;
	rc = m->gens[gen_i].generate(m->gens[gen_i].self, in_evs, in_evs_len);
	if (rc < 0) {
		const char *msg = m->gens[gen_i].last_error(m->gens[gen_i].self);
		eprintf(&m->err, "generate: %s", msg ? msg : "unknown event_generate error");
		m->state = MUXER_STATE_ERROR;
		return rc;
	}
	if (*in_evs_len) {
		prio_queue_push_unchecked(&m->pq, (struct node) {
					  .key = actf_event_tstamp_ns_from_origin((*in_evs)[0]),
					  .value = gen_i
					  });
	}
	return ACTF_OK;
}

int actf_muxer_mux(actf_muxer *m, actf_event ***evs, size_t *evs_len)
{
	int rc;
	switch (m->state) {
	case MUXER_STATE_FRESH:
		// bootstrap pq with all generators.
		for (size_t i = 0; i < m->gens_len; i++) {
			if ((rc = push_fresh_evs_to_pq(m, i)) < 0) {
				return rc;
			}
		}
		m->state = MUXER_STATE_ONGOING;
		// fallthrough
	case MUXER_STATE_ONGOING:
		if (has_pending_gen(m)) {
			if ((rc = push_fresh_evs_to_pq(m, m->pending_gen_i)) < 0) {
				return rc;
			}
			set_no_pending_gen(m);
		}
		// read up to evs_cap events from the priority queue.
		*evs_len = 0;
		*evs = m->evs;
		for (size_t i = 0; i < m->evs_cap && m->pq.len > 0; i++) {
			struct node n = prio_queue_pop_unchecked(&m->pq);
			actf_event **in_evs = m->in_evs[n.value];
			size_t *in_evs_len = m->in_evs_lens + n.value;
			size_t *in_evs_i = m->in_evs_cur_is + n.value;
			actf_event_copy(m->evs[(*evs_len)++], in_evs[(*in_evs_i)++]);
			// push in a new element.
			if (*in_evs_i < *in_evs_len) {
				prio_queue_push_unchecked(&m->pq, (struct node) {
							  .key = actf_event_tstamp_ns_from_origin(in_evs[*in_evs_i]),
							  .value = n.value
							  });
			} else {
				/* Calling generate for the generator whose buffer we
				 * just emptied would invalidate any existing events
				 * belonging to it in evs. Instead we return whatever
				 * events are collected so far and read from the
				 * generator next call. If buffers are made to not be
				 * invalidated after a subsequent generate call, we
				 * could keep more buffers than generators around to
				 * make sure we fill evs properly. Two buffers per
				 * generator would guarantee a minimum filledness of
				 * the output buffer. This is assuming that the
				 * actf_muxer consumer only uses one buffer though.. */
				set_pending_gen(m, n.value);
				break;
			}
		}
		if (*evs_len == 0 && m->evs_cap > 0) {
			m->state = MUXER_STATE_DONE;
		}
		return ACTF_OK;
	case MUXER_STATE_DONE:
		*evs_len = 0;
		return ACTF_OK;
	case MUXER_STATE_ERROR:
		return ACTF_ERROR;
	default:
		return ACTF_INTERNAL;
	}
	return ACTF_OK;
}

static int muxer_mux(void *self, actf_event ***evs, size_t *evs_len)
{
	actf_muxer *m = self;
	return actf_muxer_mux(m, evs, evs_len);
}

static void muxer_reset(actf_muxer *m)
{
	set_no_pending_gen(m);
	prio_queue_clear(&m->pq);
	m->state = MUXER_STATE_FRESH;
	memset(m->in_evs_lens, 0, m->gens_len * sizeof(*m->in_evs_lens));
	memset(m->in_evs_cur_is, 0, m->gens_len * sizeof(*m->in_evs_cur_is));
}

int actf_muxer_seek_ns_from_origin(actf_muxer *m, int64_t tstamp)
{
	for (size_t i = 0; i < m->gens_len; i++) {
		int rc = m->gens[i].seek_ns_from_origin(m->gens[i].self, tstamp);
		if (rc < 0) {
			const char *msg = m->gens[i].last_error(m->gens[i].self);
			eprintf(&m->err, "seek_ns_from_origin: %s",
				msg ? msg : "unknown seek_ns_from_origin error");
			m->state = MUXER_STATE_ERROR;
			return rc;
		}
	}
	muxer_reset(m);
	return ACTF_OK;
}

/* an actf_seek_ns_from_origin */
static int muxer_seek_ns_from_origin(void *self, int64_t tstamp)
{
	actf_muxer *m = self;
	return actf_muxer_seek_ns_from_origin(m, tstamp);
}

const char *actf_muxer_last_error(actf_muxer *m)
{
	if (!m || m->err.buf[0] == '\0') {
		return NULL;
	}
	return m->err.buf;
}

static const char *muxer_last_error(void *self)
{
	actf_muxer *m = self;
	return actf_muxer_last_error(m);
}

void actf_muxer_free(actf_muxer *m)
{
	if (!m) {
		return;
	}
	free(m->in_evs);
	free(m->in_evs_lens);
	free(m->in_evs_cur_is);
	actf_event_arr_free(m->evs);
	error_free(&m->err);
	prio_queue_free(&m->pq);
	free(m);
}

struct actf_event_generator actf_muxer_to_generator(actf_muxer *m)
{
	return (struct actf_event_generator) {
		.generate = muxer_mux,
		.seek_ns_from_origin = muxer_seek_ns_from_origin,
		.last_error = muxer_last_error,
		.self = m,
	};
}
