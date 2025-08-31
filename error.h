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

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>


#define ERROR_DEFAULT_START_SZ 64

#define ERROR_EMPTY (struct error) {0}

struct error {
    size_t sz;
    char *buf;
};

/* Initializes an error with a message buffer of size sz. */
int error_init(size_t sz, struct error *e);

/* Prints to the error buffer, will grow the internal buffer as
 * needed. Passing a NULL error is allowed and will return ACTF_OK. */
int eprintf(struct error *e, const char *fmt, ...);

/* Prepends the provided string to error's current content. A colon
 * and a blank delimits them. Passing a NULL error is allowed and will
 * return ACTF_OK. */
int eprependf(struct error *e, const char *fmt, ...);

/* Frees the error buffer contents. */
void error_free(struct error *e);

#endif /* ERROR_H */
