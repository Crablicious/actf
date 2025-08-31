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
 * The main API entrypoint for actf, only this file should be included.
 *
 * If you have a file-based CTF2 trace, you should look at @ref
 * freader.h to read it. It will read the metadata, setup decoders for
 * each data stream and connect them to a muxer for you.
 *
 * If you have a CTF2 data stream in memory, you need to manually
 * initialize a metadata using @ref metadata.h and then decode the
 * data stream/s using @ref decoder.h.
 */
#ifndef ACTF_H
#define ACTF_H

#include "types.h"
#include "decoder.h"
#include "event.h"
#include "event_generator.h"
#include "filter.h"
#include "flags.h"
#include "fld.h"
#include "fld_cls.h"
#include "fld_loc.h"
#include "freader.h"
#include "mappings.h"
#include "metadata.h"
#include "muxer.h"
#include "rng.h"
#include "pkt.h"
#include "print.h"

#endif /* ACTF_H */
