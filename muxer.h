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
 * A muxer and its methods.
 */
#ifndef ACTF_MUXER_H
#define ACTF_MUXER_H

#include <stdint.h>

#include "decoder.h"
#include "event_generator.h"

/** A muxer, implements an actf_event_generator */
typedef struct actf_muxer actf_muxer;

/**
 * Initialize a muxer
 *
 * A muxer multiplexes the events of its generators in a time-ordered
 * fashion based on their timestamp in nanoseconds from origin.
 *
 * The generators must be kept available by the caller until it no
 * longer wants to use the muxer.
 *
 * @param gens the generators to multiplex
 * @param gens_len the number of generators
 * @param evs_cap the max event array capacity. If zero,
 * ACTF_DEFAULT_EVS_CAP will be used.
 * @return a muxer or NULL with errno set. A returned muxer should
 * be freed with actf_muxer_free().
 */
actf_muxer *actf_muxer_init(struct actf_event_generator *gens, size_t gens_len,
			    size_t evs_cap);

/** @see actf_event_generate */
int actf_muxer_mux(actf_muxer *m, actf_event ***evs, size_t *evs_len);

/** @see actf_seek_ns_from_origin */
int actf_muxer_seek_ns_from_origin(actf_muxer *m, int64_t tstamp);

/** @see actf_last_error */
const char *actf_muxer_last_error(actf_muxer *m);

/**
 * Free a muxer
 * @param m the muxer
 */
void actf_muxer_free(actf_muxer *m);

/**
 * Create an event generator based on a muxer
 *
 * The muxer is owned by the caller and must be kept alive as long as
 * the event generator is in use.
 *
 * @param m the muxer
 * @return an event generator
 */
struct actf_event_generator actf_muxer_to_generator(actf_muxer *m);

#endif /* ACTF_MUXER_H */
