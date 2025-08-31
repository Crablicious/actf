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

#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include <stdint.h>

#include "metadata_int.h"


struct event_state {
    uint64_t id;
    struct actf_event_cls *cls;
    uint64_t def_clk_val;
};

static inline void event_state_init(struct event_state *ev)
{
    ev->id = 0;
    ev->cls = NULL;
    ev->def_clk_val = 0;
}

#endif /* EVENT_STATE_H */
