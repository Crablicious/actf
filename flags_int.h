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

#ifndef ACTF_FLAGS_INT_H
#define ACTF_FLAGS_INT_H

#include "flags.h"

#include "mappings_int.h"


struct actf_flags {
    struct umappings maps; // The "raw" bit map ranges
    uint64_t *masks;   // Inclusive masks for the flag ranges. Length
		       // matching maps.len.
};

/* ctf flags takes ownership of the input maps */
int actf_flags_init(struct umappings *maps, struct actf_flags *f);

void actf_flags_free(struct actf_flags *f);

#endif /* ACTF_FLAGS_INT_H */
