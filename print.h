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
 * Event and field printer utility
 */
#ifndef ACTF_PRINT_H
#define ACTF_PRINT_H

#include <stdio.h>

#include "event.h"
#include "types.h"


/** A printer */
typedef struct actf_printer actf_printer;

/** Printer flags */
enum actf_printer_flags {
    /** Print packet header */
    ACTF_PRINT_PKT_HEADER = (1 << 0),
    /** Print packet context */
    ACTF_PRINT_PKT_CTX = (1 << 1),
    /** Print event header */
    ACTF_PRINT_EVENT_HEADER = (1 << 2),
    /** Print event common context */
    ACTF_PRINT_EVENT_COMMON_CTX = (1 << 3),
    /** Print event specific context */
    ACTF_PRINT_EVENT_SPECIFIC_CTX = (1 << 4),
    /** Print event payload */
    ACTF_PRINT_EVENT_PAYLOAD = (1 << 5),
    /** Print property labels of packets and events */
    ACTF_PRINT_PROP_LABELS = (1 << 6),
    /** Print a timestamp delta for events */
    ACTF_PRINT_TSTAMP_DELTA = (1 << 7),
    /** Print the timestamp in cycles */
    ACTF_PRINT_TSTAMP_CC = (1 << 8),
    /** Print the timestamp in UTC (default is localtime) */
    ACTF_PRINT_TSTAMP_UTC = (1 << 9),
    /** Print the full date with the timestamp */
    ACTF_PRINT_TSTAMP_DATE = (1 << 10),
    /** Print the timestamp in seconds.nanoseconds */
    ACTF_PRINT_TSTAMP_SEC = (1 << 11),
};

/** Print all packet and event properties */
#define ACTF_PRINT_ALL (ACTF_PRINT_PKT_HEADER | ACTF_PRINT_PKT_CTX | \
			ACTF_PRINT_EVENT_HEADER | ACTF_PRINT_EVENT_COMMON_CTX | \
			ACTF_PRINT_EVENT_SPECIFIC_CTX | ACTF_PRINT_EVENT_PAYLOAD)

/**
 * Initialize a printer with provided flags.
 *
 * Events and fields are printed in utf-8. Strings in a different
 * encoding will be converted to utf-8 using iconv with
 * transliteration (//TRANSLIT) enabled. If an error occurs during
 * conversion, a '?' will be printed instead.
 *
 * The returned printer must be freed using actf_printer_free().
 *
 * @param flags actf_printer_flags ORed together detailing how/what to
 * print
 * @return a printer
 */
actf_printer *actf_printer_init(int flags);

/**
 * Free a printer
 * @param p the printer
 */
void actf_printer_free(actf_printer *p);

/**
 * Print a field to stdout.
 * @param p the printer
 * @param fld the field
 * @return ACTF_OK on success or an error code.
 */
int actf_print_fld(actf_printer *p, const actf_fld *fld);

/**
 * Print a field to a stream.
 * @param p the printer
 * @param stream the stream
 * @param fld the field
 * @return ACTF_OK on success or an error code.
 */
int actf_fprint_fld(actf_printer *p, FILE *stream, const actf_fld *fld);

/**
 * Print an event to stdout.
 * @param p the printer
 * @param ev the event
 * @return ACTF_OK on success or an error code.
 */
int actf_print_event(actf_printer *p, const actf_event *ev);

/**
 * Print an event to a stream.
 * @param p the printer
 * @param stream the stream
 * @param ev the event
 * @return ACTF_OK on success or an error code.
 */
int actf_fprint_event(actf_printer *p, FILE *stream, const actf_event *ev);

#endif /* ACTF_PRINT_H */
