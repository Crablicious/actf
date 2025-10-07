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

#include <stdint.h>

#include "pkt_int.h"


void actf_pkt_init(struct actf_pkt *pkt, struct pkt_state *pkt_s)
{
	for (int i = 0; i < ACTF_PKT_N_PROPS; i++) {
		pkt->props[i].type = ACTF_FLD_TYPE_NIL;
		pkt->props[i].cls = NULL;
		pkt->props[i].parent = NULL;
	}
	pkt->pkt_s = pkt_s;
}

const struct actf_fld *actf_pkt_fld(const struct actf_pkt *pkt, const char *key)
{
	for (int i = 0; i < ACTF_PKT_N_PROPS; i++) {
		const struct actf_fld *fld = actf_pkt_prop_fld(pkt, key, i);
		if (fld) {
			return fld;
		}
	}
	return NULL;
}

const struct actf_fld *actf_pkt_prop_fld(const struct actf_pkt *pkt, const char *key,
					 enum actf_pkt_prop prop)
{
	switch (prop) {
	case ACTF_PKT_PROP_HEADER:
	case ACTF_PKT_PROP_CTX:
		return actf_fld_struct_fld(&pkt->props[prop], key);
	case ACTF_PKT_N_PROPS:
		return NULL;
	}
	return NULL;
}

const struct actf_fld *actf_pkt_prop(const struct actf_pkt *pkt, enum actf_pkt_prop prop)
{
	switch (prop) {
	case ACTF_PKT_PROP_HEADER:
	case ACTF_PKT_PROP_CTX:
		return &pkt->props[prop];
	case ACTF_PKT_N_PROPS:
		return NULL;
	}
	return NULL;
}

uint64_t actf_pkt_seq_num(const actf_pkt *pkt)
{
	if (pkt->pkt_s->opt_flags & PKT_SEQ_NUM) {
		return pkt->pkt_s->seq_num;
	} else {
		return 0;
	}
}

bool actf_pkt_has_seq_num(const actf_pkt *pkt)
{
	return pkt->pkt_s->opt_flags & PKT_SEQ_NUM;
}

uint64_t actf_pkt_begin_tstamp(const struct actf_pkt *pkt)
{
	return pkt->pkt_s->begin_def_clk_val;
}

int64_t actf_pkt_begin_tstamp_ns_from_origin(const actf_pkt *pkt)
{
	if (!(pkt->pkt_s->opt_flags & PKT_DSTREAM_CLS)) {
		return 0;
	}
	const actf_clk_cls *clkc = actf_dstream_cls_clk_cls(pkt->pkt_s->dsc.cls);
	if (!clkc) {
		return 0;
	}
	return actf_clk_cls_cc_to_ns_from_origin(clkc, pkt->pkt_s->begin_def_clk_val);
}

uint64_t actf_pkt_end_tstamp(const struct actf_pkt *pkt)
{
	if (pkt->pkt_s->end_def_clk_val & PKT_END_DEF_CLK_VAL) {
		return pkt->pkt_s->end_def_clk_val;
	} else {
		return 0;
	}
}

bool actf_pkt_has_end_tstamp(const actf_pkt *pkt)
{
	return pkt->pkt_s->opt_flags & PKT_END_DEF_CLK_VAL;
}

int64_t actf_pkt_end_tstamp_ns_from_origin(const actf_pkt *pkt)
{
	if (!(pkt->pkt_s->opt_flags & PKT_DSTREAM_CLS) ||
	    !(pkt->pkt_s->opt_flags & PKT_END_DEF_CLK_VAL)) {
		return 0;
	}
	const actf_clk_cls *clkc = actf_dstream_cls_clk_cls(pkt->pkt_s->dsc.cls);
	if (!clkc) {
		return 0;
	}
	return actf_clk_cls_cc_to_ns_from_origin(clkc, pkt->pkt_s->end_def_clk_val);
}

uint64_t actf_pkt_disc_event_record_snapshot(const struct actf_pkt *pkt)
{
	if (pkt->pkt_s->opt_flags & PKT_DISC_ER_SNAP) {
		return pkt->pkt_s->disc_er_snap;
	} else {
		return 0;
	}
}

bool actf_pkt_has_disc_event_record_snapshot(const actf_pkt *pkt)
{
	return pkt->pkt_s->opt_flags & PKT_DISC_ER_SNAP;
}

uint64_t actf_pkt_dstream_id(const struct actf_pkt *pkt)
{
	if (pkt->pkt_s->opt_flags & PKT_DSTREAM_ID) {
		return pkt->pkt_s->ds_id;
	} else {
		return 0;
	}
}

uint64_t actf_pkt_dstream_cls_id(const struct actf_pkt *pkt)
{
	return pkt->pkt_s->dsc.id;
}

const actf_dstream_cls *actf_pkt_dstream_cls(const actf_pkt *pkt)
{
	return (pkt->pkt_s->opt_flags & PKT_DSTREAM_CLS) ? pkt->pkt_s->dsc.cls : NULL;
}

bool actf_pkt_has_dstream_id(const actf_pkt *pkt)
{
	return pkt->pkt_s->opt_flags & PKT_DSTREAM_ID;
}
