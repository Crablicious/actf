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
 * Event related methods.
 */
#ifndef ACTF_EVENT_H
#define ACTF_EVENT_H

#include <stdint.h>

#include "fld.h"
#include "metadata.h"
#include "pkt.h"

/** An event */
typedef struct actf_event actf_event;

/** Event properties */
enum actf_event_prop {
	ACTF_EVENT_PROP_HEADER,
	ACTF_EVENT_PROP_COMMON_CTX,
	ACTF_EVENT_PROP_SPECIFIC_CTX,
	ACTF_EVENT_PROP_PAYLOAD,
	ACTF_EVENT_N_PROPS,
};

/**
 * Search the top-level struct of all event properties for a
 * field with name key.
 *
 * The event properties will be searched in the following order:
 * 1. event header
 * 2. event common context
 * 3. event specific context
 * 4. event payload
 *
 * @param ev the event
 * @param key the key to search for
 * @return the first found field or NULL
 */
const actf_fld *actf_event_fld(const actf_event *ev, const char *key);

/**
 * Search the top-level struct of an event property for a field with
 * name key.
 * @param ev the event
 * @param key the key to search for
 * @param prop the event property to search through
 * @return the field or NULL
 */
const actf_fld *actf_event_prop_fld(const actf_event *ev, const char *key,
				    enum actf_event_prop prop);

/**
 * Get the top-level struct of an event property
 * @param ev the event
 * @param prop the event property
 * @return the top-level field of the event property
 */
const struct actf_fld *actf_event_prop(const struct actf_event *ev, enum actf_event_prop prop);

/**
 * Get the timestamp of an event in cycles.
 *
 * The returned timestamp is the raw timestamp in cycles without clock
 * class offsets applied to it. In most cases, you probably want to
 * use actf_event_tstamp_ns_from_origin() instead.
 *
 * @param ev the event
 * @return the raw timestamp of the event or zero if the event has no
 * timestamp.
 */
uint64_t actf_event_tstamp(const actf_event *ev);

/**
 * Get the timestamp of an event in nanoseconds from origin.
 *
 * The returned timestamp includes any clock offset of the related
 * clock class.
 *
 * @param ev the event
 * @return the nanosecond timestamp of the event or zero if the event
 * has no timestamp.
 */
int64_t actf_event_tstamp_ns_from_origin(const actf_event *ev);

/**
 * Get the class of an event.
 * @param ev the event
 * @return the event's class
 */
const actf_event_cls *actf_event_event_cls(const actf_event *ev);

/**
 * Get the packet of an event.
 * @param ev the event
 * @return the event's packet
 */
actf_pkt *actf_event_pkt(const actf_event *ev);

/**
 * Perform a shallow copy of an event.
 * @param dest the event to be copied to
 * @param src the event to be copied
 */
void actf_event_copy(actf_event *dest, const actf_event *src);

#endif /* ACTF_EVENT_H */
