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
 * A CTF2 metadata representation.
 *
 * Provides metadata based on the CTF2-SPEC-2.0rA specification:
 * https://diamon.org/ctf/files/CTF2-SPEC-2.0rA.html
 *
 * How events and packets are linked to metadata-classes:
 * @verbatim
 * event --> event class --> data stream class --> clock class
 *                                            \--> metadata
 *
 * packet --> data stream class --> clock class
 *                             \--> metadata
 * @endverbatim
 */
#ifndef ACTF_METADATA_H
#define ACTF_METADATA_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "fld.h"
#include "fld_cls.h"
#include "types.h"

/** A CTF2 metadata */
typedef struct actf_metadata actf_metadata;
/** A preamble */
typedef struct actf_preamble actf_preamble;
/** A trace class */
typedef struct actf_trace_cls actf_trace_cls;
/** A clock class */
typedef struct actf_clk_cls actf_clk_cls;
/** A clock class origin */
typedef struct actf_clk_origin actf_clk_origin;
/** A clock class offset */
typedef struct actf_clk_offset actf_clk_offset;
/** A field class alias */
typedef struct actf_fld_cls_alias actf_fld_cls_alias;
/** A data stream class */
typedef struct actf_dstream_cls actf_dstream_cls;
/** An event class */
typedef struct actf_event_cls actf_event_cls;

/** Clock origin types */
enum actf_clk_origin_type {
    ACTF_CLK_ORIGIN_TYPE_NONE,
    ACTF_CLK_ORIGIN_TYPE_UNIX_EPOCH,
    ACTF_CLK_ORIGIN_TYPE_CUSTOM,
    ACTF_CLK_ORIGIN_N_TYPES,
};

/*****************************************************************************/
/*                                  Metadata                                 */
/*****************************************************************************/

/**
 * Initialize a metadata.
 * @return a metadata or NULL with errno set. A returned metadata
 * should be freed with actf_metadata_free().
 */
actf_metadata *actf_metadata_init(void);

/**
 * Parse the metadata of a file.
 * @param metadata the metadata
 * @param path the path to a file containing metadata.
 * @return ACTF_OK on success or an error code. On error, see
 * actf_metadata_last_error().
 */
int actf_metadata_parse_file(actf_metadata *metadata, const char *path);

/**
 * Parse the metadata from a file descriptor.
 * @param metadata the metadata
 * @param fd the fd to read for metadata.
 * @return ACTF_OK on success or an error code. On error, see
 * actf_metadata_last_error().
 */
int actf_metadata_parse_fd(actf_metadata *metadata, int fd);

/**
 * Parse the metadata from a string.
 * @param metadata the metadata
 * @param str the string containing metadata. Must be null-terminated.
 * @return ACTF_OK on success or an error code. On error, see
 * actf_metadata_last_error().
 */
int actf_metadata_parse(actf_metadata *metadata, const char *str);

/**
 * Parse the metadata from a string with explicit length.
 *
 * @param metadata the metadata
 * @param str the string containing metadata. No null-termination needed.
 * @param len the length of the string
 * @return ACTF_OK on success or an error code. On error, see
 * actf_metadata_last_error().
 */
int actf_metadata_nparse(struct actf_metadata *metadata, const char *str,
			 size_t len);

/**
 * Get the last error message of the metadata
 * A metadata method returning an error can store an error message
 * which is returned by this function. The returned string's memory is
 * handled internally and should not be freed, and it might be
 * overwritten or freed on subsequent API calls.
 *
 * @param metadata the metadata
 * @return the last error message or NULL
 */
const char *actf_metadata_last_error(actf_metadata *metadata);

/**
 * Free a metadata
 * @param metadata the metadata
 */
void actf_metadata_free(actf_metadata *metadata);

/*****************************************************************************/
/*                                  Preamble                                 */
/*****************************************************************************/

/**
 * Get the preamble of a metadata
 * @param metadata the metadata
 * @return the preamble
 */
const actf_preamble *actf_metadata_preamble(const actf_metadata *metadata);

/**
 * Get the UUID of a preamble
 * @param preamble the preamble
 * @return the UUID
 */
const struct actf_uuid *actf_preamble_uuid(const actf_preamble *preamble);

/**
 * Get the attributes of a preamble
 * @param preamble the preamble
 * @return the attributes
 */
const actf_fld *actf_preamble_attributes(const actf_preamble *preamble);

/**
 * Get the extensions of a preamble
 * @param preamble the preamble
 * @return the extensions
 */
const actf_fld *actf_preamble_extensions(const actf_preamble *preamble);

/*****************************************************************************/
/*                                Trace Class                                */
/*****************************************************************************/

/**
 * Get the trace class of a metadata
 * @param metadata the metadata
 * @return the trace class
 */
const actf_trace_cls *actf_metadata_trace_cls(const actf_metadata *metadata);

/**
 * Get the namespace of a trace class
 * @param tc the trace class
 * @return the namespace
 */
const char *actf_trace_cls_namespace(const actf_trace_cls *tc);

/**
 * Get the name of a trace class
 * @param tc the trace class
 * @return the name
 */
const char *actf_trace_cls_name(const actf_trace_cls *tc);

/**
 * Get the uid of a trace class
 * @param tc the trace class
 * @return the uid
 */
const char *actf_trace_cls_uid(const actf_trace_cls *tc);

/**
 * Get the packet header field class of a trace class
 * @param tc the trace class
 * @return the packet header field class
 */
const actf_fld_cls *actf_trace_cls_pkt_hdr(const actf_trace_cls *tc);

/**
 * Get the environment of a trace class
 * @param tc the trace class
 * @return the environment
 */
const actf_fld *actf_trace_cls_environment(const actf_trace_cls *tc);

/**
 * Get the attributes of a trace class
 * @param tc the trace class
 * @return the attributes
 */
const actf_fld *actf_trace_cls_attributes(const actf_trace_cls *tc);

/**
 * Get the extensions of a trace class
 * @param tc the trace class
 * @return the extensions
 */
const actf_fld *actf_trace_cls_extensions(const actf_trace_cls *tc);

/*****************************************************************************/
/*                             Field Class Alias                             */
/*****************************************************************************/

/**
 * Get the number of field class aliases of the metadata
 * @param metadata the metadata
 * @return the number of field class aliases
 */
size_t actf_metadata_fld_cls_aliases_len(const actf_metadata *metadata);

/**
 * Get the ith field class alias of the metadata
 * @param metadata the metadata
 * @param i the index
 * @return the field class alias
 */
const actf_fld_cls_alias *actf_metadata_fld_cls_aliases_idx(const actf_metadata *metadata, size_t i);

/**
 * Get the field class alias name
 * @param fc_alias the field class alias
 * @return the name
 */
const char *actf_fld_cls_alias_name(const actf_fld_cls_alias *fc_alias);

/**
 * Get the aliased field class of a field class alias
 * @param fc_alias the field class alias
 * @return the aliased field class
 */
const actf_fld_cls *actf_fld_cls_alias_fld_cls(const actf_fld_cls_alias *fc_alias);

/**
 * Get the field class alias attributes
 * @param fc_alias the field class alias
 * @return the attributes
 */
const actf_fld *actf_fld_cls_alias_attributes(const actf_fld_cls_alias *fc_alias);

/**
 * Get the field class alias extensions
 * @param fc_alias the field class alias
 * @return the extensions
 */
const actf_fld *actf_fld_cls_alias_extensions(const actf_fld_cls_alias *fc_alias);

/*****************************************************************************/
/*                                Clock Class                                */
/*****************************************************************************/

/**
 * Get the number of clock classes of the metadata
 * @param metadata the metadata
 * @return the number of clock classes
 */
size_t actf_metadata_clk_clses_len(const actf_metadata *metadata);

/**
 * Get the ith clock class of the metadata
 * @param metadata the metadata
 * @param i the index
 * @return the clock class
 */
const actf_clk_cls *actf_metadata_clk_clses_idx(const actf_metadata *metadata, size_t i);

/**
 * Get the clock class id
 * @param clkc the clock class
 * @return the id
 */
const char *actf_clk_cls_id(const actf_clk_cls *clkc);

/**
 * Get the clock class namespace
 * @param clkc the clock class
 * @return the namespace
 */
const char *actf_clk_cls_namespace(const actf_clk_cls *clkc);

/**
 * Get the clock class name
 * @param clkc the clock class
 * @return the name
 */
const char *actf_clk_cls_name(const actf_clk_cls *clkc);

/**
 * Get the clock class uid
 * @param clkc the clock class
 * @return the uid
 */
const char *actf_clk_cls_uid(const actf_clk_cls *clkc);

/**
 * Get the clock class frequency
 * @param clkc the clock class
 * @return the frequency
 */
uint64_t actf_clk_cls_frequency(const actf_clk_cls *clkc);

/**
 * Get the clock class origin
 * @param clkc the clock class
 * @return the origin
 */
const actf_clk_origin *actf_clk_cls_origin(const actf_clk_cls *clkc);

/**
 * Get the clock class offset
 * @param clkc the clock class
 * @return the offset
 */
const actf_clk_offset *actf_clk_cls_offset(const actf_clk_cls *clkc);

/**
 * Get the clock class precision
 * @param clkc the clock class
 * @return the precision or zero if no specified precision
 */
uint64_t actf_clk_cls_precision(const actf_clk_cls *clkc);

/**
 * Check whether the clock class has a precision
 * @param clkc the clock class
 * @return true if the clock class has a precision
 */
bool actf_clk_cls_has_precision(const actf_clk_cls *clkc);

/**
 * Get the clock class accuracy
 * @param clkc the clock class
 * @return the accuracy or zero if no specified accuracy
 */
uint64_t actf_clk_cls_accuracy(const struct actf_clk_cls *clkc);

/**
 * Check whether the clock class has an accuracy
 * @param clkc the clock class
 * @return true if the clock class has an accuracy
 */
bool actf_clk_cls_has_accuracy(const struct actf_clk_cls *clkc);

/**
 * Get the clock class description
 * @param clkc the clock class
 * @return the description
 */
const char *actf_clk_cls_description(const actf_clk_cls *clkc);

/**
 * Get the clock class attributes
 * @param clkc the clock class
 * @return the attributes
 */
const actf_fld *actf_clk_cls_attributes(const actf_clk_cls *clkc);

/**
 * Get the clock class extensions
 * @param clkc the clock class
 * @return the extensions
 */
const actf_fld *actf_clk_cls_extensions(const actf_clk_cls *clkc);

/**
 * Check whether two clock classes have the same identity.
 *
 * Two clock classes have the same identity if namespace, name and uid
 * are equal.
 *
 * @param clkc1 the clock class 1
 * @param clkc2 the clock class 2
 * @return whether the identities are equal
 */
bool actf_clk_cls_eq_identities(const struct actf_clk_cls *clkc1,
				const struct actf_clk_cls *clkc2);

/**
 * Check whether two clock classes have the same identity in a
 * stricter fashion than actf_clk_cls_eq_identities().
 *
 * Two clock classes have the same identity if namespace, name and uid
 * are equal. In addition, this function verifies that the frequency,
 * precision, accuracy and origin are equal.
 *
 * @param clkc1 the clock class 1
 * @param clkc2 the clock class 2
 * @return whether the identities and specified fields are equal.
 */
bool actf_clk_cls_eq_identities_strict(const struct actf_clk_cls *clkc1,
				       const struct actf_clk_cls *clkc2);

/**
 * Convert a timestamp in cycles to nanoseconds from origin
 * @param clkc the clock class
 * @param tstamp_cc the timestamp in cycles
 * @return timestamp in nanosecond from origin
 */
int64_t actf_clk_cls_cc_to_ns_from_origin(const struct actf_clk_cls *clkc,
					  uint64_t tstamp_cc);

/**
 * Get the clock origin type
 * @param origin the clock origin
 * @return the type
 */
enum actf_clk_origin_type actf_clk_origin_type(const actf_clk_origin *origin);

/**
 * Get the clock origin custom namespace
 * @param origin the clock origin of type ACTF_CLK_ORIGIN_TYPE_CUSTOM
 * @return the namespace
 */
const char *actf_clk_origin_custom_namespace(const actf_clk_origin *origin);

/**
 * Get the clock origin custom name
 * @param origin the clock origin of type ACTF_CLK_ORIGIN_TYPE_CUSTOM
 * @return the name
 */
const char *actf_clk_origin_custom_name(const actf_clk_origin *origin);

/**
 * Get the clock origin custom uid
 * @param origin the clock origin of type ACTF_CLK_ORIGIN_TYPE_CUSTOM
 * @return the uid
 */
const char *actf_clk_origin_custom_uid(const actf_clk_origin *origin);

/**
 * Get the seconds component of the clock offset
 * @param offset the clock offset
 * @return the seconds
 */
int64_t actf_clk_offset_seconds(const actf_clk_offset *offset);

/**
 * Get the cycles component of the clock offset
 * @param offset the clock offset
 * @return the cycles
 */
uint64_t actf_clk_offset_cycles(const actf_clk_offset *offset);

/*****************************************************************************/
/*                             Data Stream Class                             */
/*****************************************************************************/

/**
 * Get the number of data stream classes of the metadata
 * @param metadata the metadata
 * @return the number of data stream classes
 */
size_t actf_metadata_dstream_clses_len(const actf_metadata *metadata);

/**
 * Iterate over the data stream classes.
 * @param metadata the metadata
 * @param it the iterator to use, it must be zero initialized on the
 * first call and then unmodified for subsequent calls.
 * @return the data stream class or NULL if no more
 */
const actf_dstream_cls *actf_metadata_dstream_clses_next(const actf_metadata *metadata,
							 actf_it *it);

/**
 * Get the data stream class id
 * @param dsc the data stream class
 * @return the id
 */
uint64_t actf_dstream_cls_id(const actf_dstream_cls *dsc);

/**
 * Get the data stream class namespace
 * @param dsc the data stream class
 * @return the namespace
 */
const char *actf_dstream_cls_namespace(const actf_dstream_cls *dsc);

/**
 * Get the data stream class name
 * @param dsc the data stream class
 * @return the name
 */
const char *actf_dstream_cls_name(const actf_dstream_cls *dsc);

/**
 * Get the data stream class uid
 * @param dsc the data stream class
 * @return the uid
 */
const char *actf_dstream_cls_uid(const actf_dstream_cls *dsc);

/**
 * Get the data stream class clock class id
 * @param dsc the data stream class
 * @return the clock class id
 */
const char *actf_dstream_cls_clk_cls_id(const actf_dstream_cls *dsc);

/**
 * Get the data stream class clock class
 * @param dsc the data stream class
 * @return the clock class
 */
const actf_clk_cls *actf_dstream_cls_clk_cls(const actf_dstream_cls *dsc);

/**
 * Get the data stream class packet context
 * @param dsc the data stream class
 * @return the packet context
 */
const actf_fld_cls *actf_dstream_cls_pkt_ctx(const actf_dstream_cls *dsc);

/**
 * Get the data stream class event header
 * @param dsc the data stream class
 * @return the event header
 */
const actf_fld_cls *actf_dstream_cls_event_hdr(const actf_dstream_cls *dsc);

/**
 * Get the data stream class event common context
 * @param dsc the data stream class
 * @return the event common context
 */
const actf_fld_cls *actf_dstream_cls_event_common_ctx(const actf_dstream_cls *dsc);

/**
 * Get the data stream class attributes
 * @param dsc the data stream class
 * @return the attributes
 */
const actf_fld *actf_dstream_cls_attributes(const actf_dstream_cls *dsc);

/**
 * Get the data stream class extensions
 * @param dsc the data stream class
 * @return the extensions
 */
const actf_fld *actf_dstream_cls_extensions(const actf_dstream_cls *dsc);

/**
 * Get the number of event classes of the data stream class
 * @param dsc the data stream class
 * @return the number of event classes
 */
size_t actf_dstream_cls_event_clses_len(const actf_dstream_cls *dsc);

/**
 * Iterate over the event classes.
 * @param dsc the data stream class
 * @param it the iterator to use, it must be zero initialized on the
 * first call and then unmodified for subsequent calls.
 * @return the event class or NULL if no more
 */
 const actf_event_cls *actf_dstream_cls_event_clses_next(const actf_dstream_cls *dsc,
							 actf_it *it);

/**
 * Get the metadata of the data stream class
 * @param dsc the data stream class
 * @return the metadata
 */
const actf_metadata *actf_dstream_cls_metadata(const actf_dstream_cls *dsc);

/*****************************************************************************/
/*                             Event Record Class                            */
/*****************************************************************************/

/**
 * Get the event class id
 * @param evc the event class
 * @return the id
 */
uint64_t actf_event_cls_id(const actf_event_cls *evc);

/**
 * Get the event class data stream class id
 * @param evc the event class
 * @return the data stream class id
 */
uint64_t actf_event_cls_dstream_cls_id(const actf_event_cls *evc);

/**
 * Get the event class data stream class
 * @param evc the event class
 * @return the data stream class
 */
const actf_dstream_cls *actf_event_cls_dstream_cls(const actf_event_cls *evc);

/**
 * Get the event class namespace
 * @param evc the event class
 * @return the namespace
 */
const char *actf_event_cls_namespace(const actf_event_cls *evc);

/**
 * Get the event class name
 * @param evc the event class
 * @return the name
 */
const char *actf_event_cls_name(const actf_event_cls *evc);

/**
 * Get the event class uid
 * @param evc the event class
 * @return the uid
 */
const char *actf_event_cls_uid(const actf_event_cls *evc);

/**
 * Get the event class specific context
 * @param evc the event class
 * @return the specific context
 */
const actf_fld_cls *actf_event_cls_spec_ctx(const actf_event_cls *evc);

/**
 * Get the event class payload
 * @param evc the event class
 * @return the payload
 */
const actf_fld_cls *actf_event_cls_payload(const actf_event_cls *evc);

/**
 * Get the event class attributes
 * @param evc the event class
 * @return the attributes
 */
const actf_fld *actf_event_cls_attributes(const actf_event_cls *evc);

/**
 * Get the event class extensions
 * @param evc the event class
 * @return the extensions
 */
const actf_fld *actf_event_cls_extensions(const actf_event_cls *evc);

#endif /* ACTF_METADATA_H */
