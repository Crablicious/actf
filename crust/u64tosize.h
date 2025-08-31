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

#ifndef U64TOSIZE_H
#define U64TOSIZE_H

/* Map from a uint64_t id to a size_t index. */

#include <stdint.h>
#include <stddef.h>

#define MAP_NAME u64tosize
#define MAP_KEY_TYPE uint64_t
#define MAP_KEY_CMP uint64cmp
#define MAP_VAL_TYPE size_t
#define MAP_HASH hash_uint64

#include "map.h"

#endif /* U64TOSIZE_H */
