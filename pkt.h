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
 * Packet related methods.
 */
#ifndef ACTF_PKT_H
#define ACTF_PKT_H

#include <stdint.h>

#include "metadata.h"

/** A packet */
typedef struct actf_pkt actf_pkt;

/** Packet properties */
enum actf_pkt_prop {
    ACTF_PKT_PROP_HEADER,
    ACTF_PKT_PROP_CTX,
    ACTF_PKT_N_PROPS,
};

/**
 * Search the top-level struct of all packet properties for a field
 * with name key.
 *
 * The packet properties will be searched in the following order:
 * 1. packet header
 * 2. packet context
 *
 * @param pkt the packet
 * @param key the key to search for
 * @return the first found field or NULL
 */
const actf_fld *actf_pkt_fld(const actf_pkt *pkt, const char *key);

/**
 * Search the top-level struct of a packet property for a field with
 * name key.
 * @param pkt the packet
 * @param key the key to search for
 * @param prop the packet property to search through
 * @return the field or NULL
 */
const actf_fld *actf_pkt_prop_fld(const actf_pkt *pkt, const char *key,
				  enum actf_pkt_prop prop);

/**
 * Get the top-level struct of a packet property
 * @param pkt the packet
 * @param prop the packet property
 * @return the top-level field of the packet property
 */
const struct actf_fld *actf_pkt_prop(const struct actf_pkt *pkt,
				     enum actf_pkt_prop prop);

/**
 * Get the sequence number of a packet.
 *
 * The sequence number is only relevant if actf_pkt_has_seq_num()
 * returns true.
 *
 * @param pkt the packet
 * @return the packet's sequence number
 */
uint64_t actf_pkt_seq_num(const actf_pkt *pkt);

/**
 * Check if the packet has a sequence number.
 *
 * @param pkt the packet
 * @return whether the packet has a sequence number
 */
bool actf_pkt_has_seq_num(const actf_pkt *pkt);

/**
 * Get the timestamp of the beginning of a packet in cycles.
 *
 * The returned timestamp is the raw timestamp in cycles without clock
 * class offsets applied to it. In most cases, you probably want to
 * use actf_pkt_begin_tstamp_ns_from_origin() instead.
 *
 * @param pkt the packet
 * @return the raw timestamp of the beginning of the packet.
 */
uint64_t actf_pkt_begin_tstamp(const actf_pkt *pkt);

/**
 * Get the timestamp of the beginning of a packet in nanoseconds from
 * origin.
 *
 * The returned timestamp includes any clock offset of the related
 * clock class.
 *
 * @param pkt the packet
 * @return the nanosecond timestamp of the packet
 */
int64_t actf_pkt_begin_tstamp_ns_from_origin(const actf_pkt *pkt);

/**
 * Get the timestamp of the end of a packet in cycles.
 *
 * The returned timestamp is the raw timestamp in cycles without clock
 * class offsets applied to it. The timestamp is only relevant if
 * actf_pkt_has_end_tstamp() returns true. In most cases, you probably
 * want to use actf_pkt_end_tstamp_ns_from_origin() instead.
 *
 * @param pkt the packet
 * @return the raw timestamp of the end of the packet or zero if the
 * packet has no end timestamp.
 */
uint64_t actf_pkt_end_tstamp(const actf_pkt *pkt);

/**
 * Check if the packet has an end timestamp.
 *
 * @param pkt the packet
 * @return whether the packet has an end timestamp
 */
bool actf_pkt_has_end_tstamp(const actf_pkt *pkt);

/**
 * Get the timestamp of the end of a packet in nanoseconds from
 * origin.
 *
 * The returned timestamp includes any clock offset of the related
 * clock class.
 *
 * @param pkt the packet
 * @return the nanosecond timestamp of the packet or zero if the
 * packet has no end timestamp.
 */
int64_t actf_pkt_end_tstamp_ns_from_origin(const actf_pkt *pkt);

/**
 * Get the discarded event record snapshot of a packet.
 *
 * The discarded event record snapshot is only relevant if
 * actf_pkt_has_disc_event_record_snapshot() returns true.
 *
 * @param pkt the packet
 * @return the packet's discarded event record snapshot
 */
uint64_t actf_pkt_disc_event_record_snapshot(const actf_pkt *pkt);

/**
 * Check if the packet has a discarded event record snapshot.
 *
 * @param pkt the packet
 * @return whether the packet has a discarded event record snapshot.
 */
bool actf_pkt_has_disc_event_record_snapshot(const actf_pkt *pkt);

/**
 * Get the data stream id of the packet.
 *
 * @param pkt the packet
 * @return the data stream id of the packet
 */
uint64_t actf_pkt_dstream_id(const actf_pkt *pkt);

/**
 * Get the data stream class id of the packet.
 *
 * @param pkt the packet
 * @return the packet's data stream class id
 */
uint64_t actf_pkt_dstream_cls_id(const actf_pkt *pkt);

/**
 * Get the data stream class of the packet.
 *
 * @param pkt the packet
 * @return the packet's data stream class
 */
const actf_dstream_cls *actf_pkt_dstream_cls(const actf_pkt *pkt);

/**
 * Check if the packet has an explicit datastream class id.
 *
 * @param pkt the packet
 * @return whether the packet has an explicit datastream class id
 */
bool actf_pkt_has_dstream_id(const actf_pkt *pkt);

#endif /* ACTF_PKT_H */
