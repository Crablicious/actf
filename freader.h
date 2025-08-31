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
 * A file-based CTF2 reader and its methods.
 *
 * The reader implements a CTF2-FS-1.0 consumer:
 * https://diamon.org/ctf/files/CTF2-FS-1.0.html
 */
#ifndef ACTF_FREADER_H
#define ACTF_FREADER_H

#include <stdbool.h>
#include <stddef.h>

#include "event.h"

/** CTF2 FS Reader */
typedef struct actf_freader actf_freader;

/** The configuration of a CTF2 FS reader. It can be initialized to
 * zero to get the default values. */
struct actf_freader_cfg {
    /** The name of the metadata file located at
     * `path`/`metadata_filename`. If metadata_filename is NULL, the
     * default value is "metadata". */
    const char *metadata_filename;
    /** The number of events used in the buffer for each data stream
     * file. If zero, the default value is ACTF_DEFAULT_EVS_CAP. */
    size_t dstream_evs_cap;
    /** The number of events used in the buffer for the muxer. If
     * zero, the default value is ACTF_DEFAULT_EVS_CAP. */
    size_t muxer_evs_cap;
};

/**
 * Initialize a CTF2 FS reader
 * @param cfg the configuration
 * @return the reader or NULL with errno set. A returned reader should
 * be freed with actf_freader_free().
 */
actf_freader *actf_freader_init(struct actf_freader_cfg cfg);

/**
 * Open a CTF2 trace directory and sets up generators to read it.
 *
 * Does the following:
 * 1. Reads the CTF2 metadata
 * 2. Constructs a decoder for each data stream file
 * 3. Hooks up all decoders to a muxer
 *
 * @param rd the reader
 * @param path the path to a CTF2 trace directory
 * @return ACTF_OK on success or an error code. On error, see actf_last_error().
 */
int actf_freader_open_folder(actf_freader *rd, char *path);

/** Like actf_freader_open_folder() but allows multiple CTF2
 * directories to be specified. The data streams of all CTF2
 * directories will be hooked up to the same muxer. */
int actf_freader_open_folders(actf_freader *rd, char **paths, size_t len);

/** @see actf_event_generate */
int actf_freader_read(actf_freader *rd, actf_event ***evs, size_t *evs_len);

/** @see actf_seek_ns_from_origin */
int actf_freader_seek_ns_from_origin(actf_freader *rd, int64_t tstamp);

/** @see actf_last_error */
const char *actf_freader_last_error(actf_freader *rd);

/**
 * Free a reader
 * @param rd the reader
 */
void actf_freader_free(actf_freader *rd);

/**
 * Create an event generator based on a reader
 *
 * The reader is owned by the caller and must be kept alive as long as
 * the event generator is in use.
 *
 * @param rd the reader
 * @return an event generator
 */
struct actf_event_generator actf_freader_to_generator(actf_freader *rd);

#endif /* ACTF_FREADER_H */
