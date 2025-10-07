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

#ifndef ACTF_PKT_INT_H
#define ACTF_PKT_INT_H

#include "fld_int.h"
#include "pkt.h"
#include "pkt_state.h"


struct actf_pkt {
	struct pkt_state *pkt_s;
	struct actf_fld props[ACTF_PKT_N_PROPS];
};


/* Initializes a packet. */
void actf_pkt_init(struct actf_pkt *pkt, struct pkt_state *pkt_s);

#endif /* ACTF_PKT_INT_H */
