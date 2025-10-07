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

#ifndef PKT_STATE_H
#define PKT_STATE_H

#include <stdint.h>

#include "crust/common.h"
#include "metadata_int.h"


static const uint64_t PACKET_MAGIC_NUMBER = 0xc1fc1fc1;

enum pkt_state_opt {
	PKT_DISC_ER_SNAP = 1,
	PKT_DSTREAM_CLS = 2,
	PKT_DSTREAM_ID = 4,
	PKT_LAST_BO = 8,
	PKT_END_DEF_CLK_VAL = 16,
	PKT_SEQ_NUM = 32,
};

struct pkt_state {
	uint64_t bit_off; // Bit offset to start of packet
	uint64_t def_clk_val;
	uint64_t begin_def_clk_val;
	uint64_t disc_er_snap;
	struct {
		struct actf_dstream_cls *cls;
		uint64_t id;
	} dsc;
	uint64_t ds_id;
	enum actf_byte_order last_bo;
	uint64_t content_len;
	uint64_t end_def_clk_val;
	uint64_t seq_num;
	uint64_t tot_len;

	/* OR'd existance checks for optional fields. */
	enum pkt_state_opt opt_flags;
};


static inline void pkt_state_init(struct pkt_state *pkt)
{
	pkt->bit_off = 0;
	pkt->def_clk_val = 0;
	pkt->begin_def_clk_val = 0;
	pkt->dsc.id = 0;
	pkt->dsc.cls = NULL;
	pkt->content_len = UINT64_MAX;
	pkt->tot_len = UINT64_MAX;
	pkt->opt_flags = 0;
}

#endif /* PKT_STATE_H */
