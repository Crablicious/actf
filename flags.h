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
 * Bit map flags related methods.
 */
#ifndef ACTF_FLAGS_H
#define ACTF_FLAGS_H

#include <stdint.h>
#include <unistd.h>

#include "types.h"

/** Flags map names to bit index range sets. */
typedef struct actf_flags actf_flags;

/**
 * Find the next flag name matching the provided value.
 *
 * A value can be mapped to several flags, so to read them all out,
 * you should call actf_flags_find() repeatedly with the same iterator
 * until NULL is returned.
 *
 * @param f the flags
 * @param val the value
 * @param it the iterator to use, it must be zero initialized on the
 * first call and then unmodified for subsequent calls.
 * @return the flag name or NULL if no more matches exist
 */
const char *actf_flags_find(const actf_flags *f, uint64_t val, actf_it *it);

/**
 * Find the first flag name matching the provided value.
 *
 * Convenience function for when you only care about the first
 * matching name.
 *
 * @param f the flags
 * @param val the value
 * @return the flag name or NULL if no matches exist
 */
const char *actf_flags_find_first_match(const actf_flags *f, uint64_t val);

#endif /* ACTF_FLAGS_H */
