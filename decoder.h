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
 * A CTF2 decoder and its methods.
 *
 * Decodes data based on the CTF2-SPEC-2.0rA specification:
 * https://diamon.org/ctf/files/CTF2-SPEC-2.0rA.html
 */
#ifndef ACTF_DECODER_H
#define ACTF_DECODER_H

#include "metadata.h"
#include "event_generator.h"
#include "event.h"

/** A CTF2 decoder, implements an actf_event_generator */
typedef struct actf_decoder actf_decoder;

/**
 * Initialize a decoder
 * @param data the ctf2 data to process
 * @param data_len the length of the data
 * @param evs_cap the max event array capacity. If zero,
 * ACTF_DEFAULT_EVS_CAP will be used.
 * @param metadata the metadata describing the data
 * @return a decoder or NULL with errno set. A returned decoder should
 * be freed with actf_decoder_free().
 */
actf_decoder *actf_decoder_init(void *data, size_t data_len, size_t evs_cap,
				const actf_metadata *metadata);

/** @see actf_event_generate */
int actf_decoder_decode(actf_decoder *dec, actf_event ***evs, size_t *evs_len);

/** @see actf_seek_ns_from_origin */
int actf_decoder_seek_ns_from_origin(actf_decoder *dec, int64_t tstamp);

/** @see actf_last_error */
const char *actf_decoder_last_error(actf_decoder *dec);

/**
 * Free a decoder
 * @param dec the decoder
 */
void actf_decoder_free(actf_decoder *dec);

/**
 * Create an event generator based on a decoder
 *
 * The decoder is owned by the caller and must be kept alive as long
 * as the event generator is in use.
 *
 * @param dec the decoder
 * @return an event generator
 */
struct actf_event_generator actf_decoder_to_generator(actf_decoder *dec);

#endif /* ACTF_DECODER_H */
