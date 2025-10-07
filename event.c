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

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "metadata_int.h"
#include "event_int.h"
#include "fld_cls_int.h"
#include "fld_int.h"


void actf_event_init(struct actf_event *ev, struct actf_pkt *pkt)
{
	event_state_init(&ev->ev_s);
	for (int i = 0; i < ACTF_EVENT_N_PROPS; i++) {
		ev->props[i].type = ACTF_FLD_TYPE_NIL;
		ev->props[i].cls = NULL;
		ev->props[i].parent = NULL;
	}
	ev->pkt = pkt;
}

const struct actf_fld *actf_event_fld(const struct actf_event *ev, const char *key)
{
	for (int i = 0; i < ACTF_EVENT_N_PROPS; i++) {
		const struct actf_fld *fld = actf_event_prop_fld(ev, key, i);
		if (fld) {
			return fld;
		}
	}
	return NULL;
}

const struct actf_fld *actf_event_prop_fld(const struct actf_event *ev, const char *key,
					   enum actf_event_prop prop)
{
	switch (prop) {
	case ACTF_EVENT_PROP_HEADER:
	case ACTF_EVENT_PROP_COMMON_CTX:
	case ACTF_EVENT_PROP_SPECIFIC_CTX:
	case ACTF_EVENT_PROP_PAYLOAD:
		return actf_fld_struct_fld(&ev->props[prop], key);
	case ACTF_EVENT_N_PROPS:
		return NULL;
	}
	return NULL;
}

const struct actf_fld *actf_event_prop(const struct actf_event *ev, enum actf_event_prop prop)
{
	switch (prop) {
	case ACTF_EVENT_PROP_HEADER:
	case ACTF_EVENT_PROP_COMMON_CTX:
	case ACTF_EVENT_PROP_SPECIFIC_CTX:
	case ACTF_EVENT_PROP_PAYLOAD:
		return &ev->props[prop];
	case ACTF_EVENT_N_PROPS:
		return NULL;
	}
	return NULL;
}

uint64_t actf_event_tstamp(const struct actf_event *ev)
{
	return ev->ev_s.def_clk_val;
}

int64_t actf_event_tstamp_ns_from_origin(const actf_event *ev)
{
	const actf_dstream_cls *dsc = actf_event_cls_dstream_cls(ev->ev_s.cls);
	const actf_clk_cls *clkc = actf_dstream_cls_clk_cls(dsc);
	if (!clkc) {
		return 0;
	}
	return actf_clk_cls_cc_to_ns_from_origin(clkc, ev->ev_s.def_clk_val);
}

const actf_event_cls *actf_event_event_cls(const actf_event *ev)
{
	return ev->ev_s.cls;
}

struct actf_pkt *actf_event_pkt(const struct actf_event *ev)
{
	return ev->pkt;
}

void actf_event_copy(actf_event *dest, const actf_event *src)
{
	*dest = *src;
}
