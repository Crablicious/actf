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

#ifndef ACTF_EVENT_INT_H
#define ACTF_EVENT_INT_H

#include <stdint.h>

#include "event.h"
#include "fld.h"
#include "pkt_int.h"
#include "event_state.h"


struct actf_event {
    struct event_state ev_s;
    struct actf_pkt *pkt;
    struct actf_fld props[ACTF_EVENT_N_PROPS];
};

/* Initializes an event. */
void actf_event_init(struct actf_event *ev, struct actf_pkt *pkt);

#endif /* ACTF_EVENT_INT_H */
