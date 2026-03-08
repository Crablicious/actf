/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2026  Adam Wendelin <adwe live se>
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

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lua_filter.h"
#include "error.h"
#include "event.h"
#include "print.h"
#include "crust/common.h"


#define ALUA_GLOBAL_TABLE "actf"

/* To access fields of structs and arrays we can either use array
 * access notation (using __index) or use an explicit get method and
 * return multiple values
 *
 *   val, name = ev:payload():get("enumfield") -- we go for this
 * VS
 *   val, name = ev:payload()["enumfield"]:value()
 * VS
 *   val = ev:payload()["enumfield"]:integer()
 *   name = ev:payload()["enumfield"]:string()
 */

/* type for compound field */
static const char *CFIELD_M_TBL = "actf.cfield";
/* type for non-compound field */
static const char *FIELD_M_TBL = "actf.field";
/* type for event */
static const char *EVENT_M_TBL = "actf.event";
/* type for packet */
static const char *PACKET_M_TBL = "actf.packet";
/* type for printer */
static const char *PRINTER_M_TBL = "actf.printer";

typedef struct alua_cfield {
	const actf_fld *fld;
} alua_cfield;

typedef struct alua_field {
	const actf_fld *fld;
} alua_field;

typedef struct alua_event {
	const actf_event *ev;
} alua_event;

typedef struct alua_packet {
	const actf_pkt *pkt;
} alua_packet;

typedef struct alua_printer {
	actf_printer *pr;
} alua_printer;

static actf_printer *printer(lua_State *L);
static int new_field(lua_State *L, const actf_fld *fld);
static int new_cfield(lua_State *L, const actf_fld *fld);

static int return_field(lua_State *L, const actf_fld *fld)
{
	if (! fld) {
		lua_pushnil(L);
		return 1;
	}
	int n_rets = 1;
	switch (actf_fld_type(fld)) {
	case ACTF_FLD_TYPE_NIL:
		lua_pushnil(L);
		break;
	case ACTF_FLD_TYPE_BOOL:
		lua_pushboolean(L, actf_fld_bool(fld));
		break;
	case ACTF_FLD_TYPE_SINT: {
		int64_t val = actf_fld_int64(fld);
		lua_pushinteger(L, (lua_Integer)val);
		const struct actf_mappings *maps =
			actf_fld_cls_mappings(actf_fld_fld_cls(fld));
		if (maps) {
			const char *name;
			actf_it it = { 0 };
			while ((name = actf_mappings_find_sint(maps, val, &it))) {
				lua_pushstring(L, name);
				n_rets++;
			}
		}
		break;
	}
	case ACTF_FLD_TYPE_UINT: {
		uint64_t val = actf_fld_uint64(fld);
		lua_Integer lval = (lua_Integer) (val < INT64_MAX ? val : INT64_MAX);
		lua_pushinteger(L, lval);
		const struct actf_mappings *maps =
			actf_fld_cls_mappings(actf_fld_fld_cls(fld));
		if (maps) {
			const char *name;
			actf_it it = { 0 };
			while ((name = actf_mappings_find_uint(maps, val, &it))) {
				lua_pushstring(L, name);
				n_rets++;
			}
		}
		break;
	}
	case ACTF_FLD_TYPE_BIT_MAP: {
		uint64_t val = actf_fld_uint64(fld);
		lua_pushinteger(L, (lua_Integer)val);
		const struct actf_flags *flags =
			actf_fld_cls_bit_map_flags(actf_fld_fld_cls(fld));
		if (flags) {
			const char *name;
			actf_it it = { 0 };
			while ((name = actf_flags_find(flags, val, &it))) {
				lua_pushstring(L, name);
				n_rets++;
			}
		}
		break;
	}
	case ACTF_FLD_TYPE_REAL:
		lua_pushnumber(L, (lua_Number)actf_fld_double(fld));
		break;
	case ACTF_FLD_TYPE_STR: {
		const actf_fld_cls *fc = actf_fld_fld_cls(fld);
		enum actf_encoding enc = actf_fld_cls_encoding(fc);
		if (enc == ACTF_ENCODING_UTF8) {
			const char *s = actf_fld_str_raw(fld);
			size_t sz = actf_fld_str_sz(fld);
			size_t len = sz ? sz - (s[sz - 1] == '\0') : 0;
			lua_pushlstring(L, s, len);
			break;
		}
		actf_printer *pr = printer(L);

		char *s = NULL;
		size_t sz = 0;
		FILE *f = open_memstream(&s, &sz);
		if (! f) {
			return luaL_error(L, "open_memstream: %s", strerror(errno));
		}
		int rc = actf_fprint_fld(pr, f, fld);
		if (rc < 0) {
			fclose(f);
			free(s);
			return luaL_error(L, "actf_fprint_event: %s", actf_errstr(rc));
		}
		fclose(f);

		lua_pushlstring(L, s + 1, sz - 1); // remove quotes
		free(s);
		break;
	}
	case ACTF_FLD_TYPE_BLOB:
		lua_pushlstring(L, actf_fld_blob(fld),
				actf_fld_blob_sz(fld));
		break;
	case ACTF_FLD_TYPE_STRUCT:
	case ACTF_FLD_TYPE_ARR:
		new_cfield(L, fld);
		break;
	}
	return n_rets;
}

static int return_rawfield(lua_State *L, const actf_fld *fld)
{
	if (! fld) {
		lua_pushnil(L);
		return 1;
	}
	switch (actf_fld_type(fld)) {
	case ACTF_FLD_TYPE_STRUCT:
	case ACTF_FLD_TYPE_ARR:
		new_cfield(L, fld);
		break;
	default:
		new_field(L, fld);
		break;
	}
	return 1;
}

static const actf_fld *fld_get(lua_State *L)
{
	alua_cfield *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`cfield' expected");

	switch (actf_fld_type(lfld->fld)) {
	case ACTF_FLD_TYPE_STRUCT: {
		switch (lua_type(L, 2)) {
		case LUA_TNUMBER: {
			lua_Integer idx = lua_tointeger(L, 2);
			const actf_fld *fld = NULL;
			if (idx > 0) {
				/* Offset to make indexing 1-based. */
				fld = actf_fld_struct_fld_idx(lfld->fld, (size_t)idx - 1);
			}
			return fld;
		}
		case LUA_TSTRING: {
			size_t len = 0;
			const char *s = lua_tolstring(L, 2, &len);
			return actf_fld_struct_fldn(lfld->fld, s, len);
		}
		default:
			luaL_argerror(L, 2, "`integer' or `string' expected");
			return NULL;
		}
	}
	case ACTF_FLD_TYPE_ARR: {
		int isnum = false;
		lua_Integer idx = lua_tointegerx(L, 2, &isnum);
		luaL_argcheck(L, isnum, 2, "`integer' expected");
		const actf_fld *fld = NULL;
		if (idx > 0) {
			/* Offset to make indexing 1-based. */
			fld = actf_fld_arr_idx(lfld->fld, (size_t)idx - 1);
		}
		return fld;
	}
	default:
		return NULL;
	}
}

static int cfield_get(lua_State *L)
{
	return return_field(L, fld_get(L));
}

static int cfield_rawget(lua_State *L)
{
	return return_rawfield(L, fld_get(L));
}

static int cfield_attributes(lua_State *L)
{
	alua_cfield *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return return_field(L, actf_fld_cls_attributes(actf_fld_fld_cls(lfld->fld)));
}

static int cfield_extensions(lua_State *L)
{
	alua_cfield *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return return_field(L, actf_fld_cls_extensions(actf_fld_fld_cls(lfld->fld)));
}

static int cfield_len(lua_State *L)
{
	alua_cfield *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`cfield' expected");

	switch (actf_fld_type(lfld->fld)) {
	case ACTF_FLD_TYPE_STRUCT:
		lua_pushinteger(L, actf_fld_struct_len(lfld->fld));
		break;
	case ACTF_FLD_TYPE_ARR:
		lua_pushinteger(L, actf_fld_arr_len(lfld->fld));
		break;
	default:
		lua_pushnil(L);
		break;
	}
	return 1;
}

static int fld_tostring(lua_State *L, const actf_fld *fld)
{
	actf_printer *pr = printer(L);

	char *s = NULL;
	size_t sz = 0;
	FILE *f = open_memstream(&s, &sz);
	if (! f) {
		return luaL_error(L, "open_memstream: %s", strerror(errno));
	}
	int rc = actf_fprint_fld(pr, f, fld);
	if (rc < 0) {
		fclose(f);
		free(s);
		return luaL_error(L, "actf_fprint_event: %s", actf_errstr(rc));
	}
	fclose(f);

	lua_pushstring(L, s);
	free(s);

	return 1;
}

static int cfield_tostring(lua_State *L)
{
	alua_cfield *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return fld_tostring(L, lfld->fld);
}

static const luaL_Reg cfieldlib_m [] = {
	/* explicit get rather than __index to support multiple return
	 * values for bit maps and enums. */
	{"get", cfield_get},
	{"rawget", cfield_rawget},
	{"attributes", cfield_attributes},
	{"extensions", cfield_extensions},
	{"__len", cfield_len},
	{"__tostring", cfield_tostring},
	{NULL, NULL}
};

static int new_cfield(lua_State *L, const actf_fld *fld)
{
	alua_cfield *lfld = lua_newuserdata(L, sizeof(struct alua_cfield));
	lfld->fld = fld;

	luaL_getmetatable(L, CFIELD_M_TBL);
	lua_setmetatable(L, -2);

	return 1;
}

static int field_value(lua_State *L)
{
	alua_field *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return return_field(L, lfld->fld);
}

static int field_attributes(lua_State *L)
{
	alua_field *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return return_field(L, actf_fld_cls_attributes(actf_fld_fld_cls(lfld->fld)));
}

static int field_extensions(lua_State *L)
{
	alua_field *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return return_field(L, actf_fld_cls_extensions(actf_fld_fld_cls(lfld->fld)));
}

static int field_tostring(lua_State *L)
{
	alua_field *lfld = lua_touserdata(L, 1);
	luaL_argcheck(L, lfld != NULL, 1, "`field' expected");
	return fld_tostring(L, lfld->fld);
}

static const luaL_Reg fieldlib_m [] = {
	{"value", field_value},
	{"attributes", field_attributes},
	{"extensions", field_extensions},
	{"__tostring", field_tostring},
	{NULL, NULL}
};

static int new_field(lua_State *L, const actf_fld *fld)
{
	alua_field *lfld = lua_newuserdata(L, sizeof(struct alua_field));
	lfld->fld = fld;

	luaL_getmetatable(L, FIELD_M_TBL);
	lua_setmetatable(L, -2);

	return 1;
}

static const actf_fld *pkt_fld(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");

	size_t len = 0;
	const char *key = luaL_checklstring(L, 2, &len);
	if (! key) {
		luaL_argerror(L, 2, "`string' expected");
		return NULL;
	}
	return actf_pkt_fldn(lpkt->pkt, key, len);
}

static int packet_get(lua_State *L)
{
	return return_field(L, pkt_fld(L));
}

static int packet_rawget(lua_State *L)
{
	return return_rawfield(L, pkt_fld(L));
}

static int packet_prop(lua_State *L, enum actf_pkt_prop prop)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");

	const actf_fld *fld = actf_pkt_prop(lpkt->pkt, prop);
	if (! fld) {
		lua_pushnil(L);
		return 1;
	}
	return return_field(L, fld);
}

static int packet_header(lua_State *L)
{
	return packet_prop(L, ACTF_PKT_PROP_HEADER);
}

static int packet_context(lua_State *L)
{
	return packet_prop(L, ACTF_PKT_PROP_CTX);
}

static int packet_sequencenumber(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	if (actf_pkt_has_seq_num(lpkt->pkt)) {
		lua_pushinteger(L, (lua_Integer)actf_pkt_seq_num(lpkt->pkt));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int packet_begintimestamp(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	lua_pushinteger(L, (lua_Integer)actf_pkt_begin_tstamp(lpkt->pkt));
	return 1;
}

static int packet_begintimestampns(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	lua_pushinteger(L, (lua_Integer)actf_pkt_begin_tstamp_ns_from_origin(lpkt->pkt));
	return 1;
}

static int packet_endtimestamp(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	if (actf_pkt_has_end_tstamp(lpkt->pkt)) {
		lua_pushinteger(L, (lua_Integer)actf_pkt_end_tstamp(lpkt->pkt));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int packet_endtimestampns(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	if (actf_pkt_has_end_tstamp(lpkt->pkt)) {
		lua_pushinteger(L, (lua_Integer)actf_pkt_end_tstamp_ns_from_origin(lpkt->pkt));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int packet_discardsnapshot(lua_State *L)
{
	alua_packet *lpkt = lua_touserdata(L, 1);
	luaL_argcheck(L, lpkt != NULL, 1, "`packet' expected");
	if (actf_pkt_has_disc_event_record_snapshot(lpkt->pkt)) {
		lua_pushinteger(L, (lua_Integer)actf_pkt_disc_event_record_snapshot(lpkt->pkt));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static const luaL_Reg packetlib_m [] = {
	{"get", packet_get},
	{"rawget", packet_rawget},
	{"header", packet_header},
	{"context", packet_context},
	{"sequence_number", packet_sequencenumber},
	{"begin_timestamp", packet_begintimestamp},
	{"begin_timestamp_ns", packet_begintimestampns},
	{"end_timestamp", packet_endtimestamp},
	{"end_timestamp_ns", packet_endtimestampns},
	{"discard_snapshot", packet_discardsnapshot},
	{NULL, NULL}
};

static int newpacket(lua_State *L, const actf_pkt *pkt)
{
	alua_packet *lpkt = lua_newuserdata(L, sizeof(struct alua_packet));
	lpkt->pkt = pkt;

	luaL_getmetatable(L, PACKET_M_TBL);
	lua_setmetatable(L, -2);

	return 1;
}

static int event_namespace(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	lua_pushstring(L, actf_event_cls_namespace(actf_event_event_cls(lev->ev)));
	return 1;
}

static int event_name(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	lua_pushstring(L, actf_event_cls_name(actf_event_event_cls(lev->ev)));
	return 1;
}

static int event_uid(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	lua_pushstring(L, actf_event_cls_uid(actf_event_event_cls(lev->ev)));
	return 1;
}

static const actf_fld *event_fld(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");

	size_t len = 0;
	const char *key = luaL_checklstring(L, 2, &len);
	if (! key) {
		luaL_argerror(L, 2, "`string' expected");
	}
	const actf_fld *fld = actf_event_fldn(lev->ev, key, len);
	if (! fld) {
		fld = actf_pkt_fldn(actf_event_pkt(lev->ev), key, len);
	}
	return fld;
}

static int event_get(lua_State *L)
{
	return return_field(L, event_fld(L));
}

static int event_rawget(lua_State *L)
{
	return return_rawfield(L, event_fld(L));
}

static int event_prop(lua_State *L, enum actf_event_prop prop)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	return return_field(L, actf_event_prop(lev->ev, prop));
}

static int event_header(lua_State *L)
{
	return event_prop(L, ACTF_EVENT_PROP_HEADER);
}

static int event_context(lua_State *L)
{
	return event_prop(L, ACTF_EVENT_PROP_COMMON_CTX);
}

static int event_specificcontext(lua_State *L)
{
	return event_prop(L, ACTF_EVENT_PROP_SPECIFIC_CTX);
}

static int event_payload(lua_State *L)
{
	return event_prop(L, ACTF_EVENT_PROP_PAYLOAD);
}

static int event_packet(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	newpacket(L, actf_event_pkt(lev->ev));
	return 1;
}

static int event_timestamp(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	lua_pushinteger(L, (lua_Integer)actf_event_tstamp(lev->ev));
	return 1;
}

static int event_timestampns(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	lua_pushinteger(L, (lua_Integer)actf_event_tstamp_ns_from_origin(lev->ev));
	return 1;
}

static int event_attributes(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	return return_field(L, actf_event_cls_attributes(actf_event_event_cls(lev->ev)));
}

static int event_extensions(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");
	return return_field(L, actf_event_cls_extensions(actf_event_event_cls(lev->ev)));
}

static int event_tostring(lua_State *L)
{
	alua_event *lev = lua_touserdata(L, 1);
	luaL_argcheck(L, lev != NULL, 1, "`event' expected");

	actf_printer *pr = printer(L);

	char *s = NULL;
	size_t sz = 0;
	FILE *f = open_memstream(&s, &sz);
	if (! f) {
		return luaL_error(L, "open_memstream: %s", strerror(errno));
	}
	int rc = actf_fprint_event(pr, f, lev->ev);
	if (rc < 0) {
		fclose(f);
		free(s);
		return luaL_error(L, "actf_fprint_event: %s", actf_errstr(rc));
	}
	fclose(f);

	lua_pushstring(L, s);
	free(s);

	return 1;
}

static const luaL_Reg eventlib_m [] = {
	{"namespace", event_namespace},
	{"name", event_name},
	{"uid", event_uid},
	{"get", event_get},
	{"rawget", event_rawget},
	{"header", event_header},
	{"context", event_context},
	{"specific_context", event_specificcontext},
	{"payload", event_payload},
	{"packet", event_packet},
	{"timestamp", event_timestamp},
	{"timestamp_ns", event_timestampns},
	{"attributes", event_attributes},
	{"extensions", event_extensions},
	{"__tostring", event_tostring},
	{NULL, NULL}
};

static int new_event(lua_State *L, const actf_event *ev)
{
	alua_event *lev = lua_newuserdata(L, sizeof(struct alua_event));
	lev->ev = ev;

	luaL_getmetatable(L, EVENT_M_TBL);
	lua_setmetatable(L, -2);

	return 1;
}

static int printer_gc(lua_State *L)
{
	alua_printer *lpr = lua_touserdata(L, 1);
	luaL_argcheck(L, lpr != NULL, 1, "`printer' expected");

	actf_printer_free(lpr->pr);
	return 0;
}

static const luaL_Reg printerlib_m [] = {
	{"__gc", printer_gc},
	{NULL, NULL}
};

static int new_printer(lua_State *L, actf_printer *pr)
{
	alua_printer *lpr = lua_newuserdata(L, sizeof(struct alua_printer));
	lpr->pr = pr;

	luaL_getmetatable(L, PRINTER_M_TBL);
	lua_setmetatable(L, -2);

	return 1;
}

static actf_printer *printer(lua_State *L)
{
	lua_pushlightuserdata(L, &PRINTER_M_TBL); // push
	lua_gettable(L, LUA_REGISTRYINDEX); // pops and pushes
	alua_printer *lpr = lua_touserdata(L, -1);
	lua_pop(L, 1); // pops
	return lpr->pr;
}

static int open_lua_actf(lua_State *L)
{
	static const luaL_Reg reg [] = {
		{NULL, NULL}
	};
	luaL_newlib(L, reg);
	return 1;
}

static int alua_init(const char *filter_path, lua_State **outL, struct error *e)
{
	lua_State *L;
	if (! (L = luaL_newstate())) {
		eprintf(e, "luaL_newstate error");
		return ACTF_ERROR;
	}
	luaL_requiref(L, ALUA_GLOBAL_TABLE, open_lua_actf, true);
	lua_pop(L, 1);

	luaL_openlibs(L);
	if (luaL_loadfile(L, filter_path) || lua_pcall(L, 0, 0, 0)) {
		eprintf(e, "lua script error: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);
		return ACTF_ERROR;
	}

	luaL_newmetatable(L, CFIELD_M_TBL);
	luaL_setfuncs(L, cfieldlib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, FIELD_M_TBL);
	luaL_setfuncs(L, fieldlib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, EVENT_M_TBL);
	luaL_setfuncs(L, eventlib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, PACKET_M_TBL);
	luaL_setfuncs(L, packetlib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, PRINTER_M_TBL);
	luaL_setfuncs(L, printerlib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");


	actf_printer *pr = actf_printer_init(ACTF_PRINT_ALL);
	if (! pr) {
		eprintf(e, "actf_printer_init: %s", strerror(errno));
		lua_close(L);
		return ACTF_ERROR;
	}
	lua_pushlightuserdata(L, &PRINTER_M_TBL);
	new_printer(L, pr); // pushes userdata
	lua_settable(L, LUA_REGISTRYINDEX);

	*outL = L;
	return ACTF_OK;
}

static void alua_free(lua_State *L)
{
	if (L) lua_close(L);
}

static int alua_call_init(lua_State *L, int argc, char *argv[], struct error *e)
{
	int rc;
	lua_getglobal(L, ALUA_GLOBAL_TABLE);
	lua_getfield(L, -1, "init");
	if (! lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return ACTF_OK;
	}
	for (int i = 0; i < argc; i++) {
		lua_pushstring(L, argv[i]);
	}
	if ((rc = lua_pcall(L, argc, 1, 0))) {
		eprintf(e, "error running function `"ALUA_GLOBAL_TABLE".init': %s",
			lua_tostring(L, -1));
		lua_pop(L, 2);
		return ACTF_LUA_RUN_ERROR;
	}
	if (lua_isnil(L, -1)) {
		rc = ACTF_OK;
	} else if (lua_isinteger(L, -1)) {
		rc = lua_tointeger(L, -1);
		if (rc != ACTF_OK) {
			eprintf(e, "function `"ALUA_GLOBAL_TABLE".init' returned an error");
			rc = ACTF_ERROR;
		}
	} else {
		eprintf(e, "function `"ALUA_GLOBAL_TABLE".init' must return an integer");
		rc = ACTF_LUA_WRONG_RET_TYPE;
	}
	lua_pop(L, 2);
	return rc;
}

static int alua_call_filter(lua_State *L, const actf_event *ev, struct error *e)
{
	lua_getglobal(L, ALUA_GLOBAL_TABLE);
	lua_getfield(L, -1, "filter");
	if (! lua_isfunction(L, -1)) {
		eprintf(e, "`"ALUA_GLOBAL_TABLE".filter' is not a function");
		lua_pop(L, 2);
		return ACTF_LUA_RUN_ERROR;
	}
	new_event(L, ev);
	if (lua_pcall(L, 1, 1, 0)) {
		eprintf(e, "error running function `"ALUA_GLOBAL_TABLE".filter': %s\n",
			lua_tostring(L, -1));
		lua_pop(L, 2);
		return ACTF_LUA_RUN_ERROR;
	}
	int rc;
	if (lua_isnil(L, -1)) {
		rc = 1;
	} else if (lua_isinteger(L, -1)) {
		rc = lua_tointeger(L, -1);
		if (rc < 0) {
			eprintf(e, "function `"ALUA_GLOBAL_TABLE".filter' returned an error");
			rc = ACTF_ERROR;
		}
	} else {
		eprintf(e, "function `"ALUA_GLOBAL_TABLE".filter' must return an integer\n");
		rc = ACTF_LUA_WRONG_RET_TYPE;
	}
	lua_pop(L, 2);
	return rc;
}

int alua_call_fini(lua_State *L, struct error *e)
{
	lua_getglobal(L, ALUA_GLOBAL_TABLE);
	lua_getfield(L, -1, "fini");
	if (! lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return ACTF_OK;
	}
	if (lua_pcall(L, 0, 1, 0)) {
		eprintf(e, "error running function `"ALUA_GLOBAL_TABLE".fini': %s",
			lua_tostring(L, -1));
		lua_pop(L, 2);
		return ACTF_LUA_RUN_ERROR;
	}
	int rc;
	if (lua_isnil(L, -1)) {
		rc = ACTF_OK;
	} else if (lua_isinteger(L, -1)) {
		rc = lua_tointeger(L, -1);
		if (rc != ACTF_OK) {
			eprintf(e, "function `"ALUA_GLOBAL_TABLE".fini' returned an error");
			rc = ACTF_ERROR;
		}
	} else {
		eprintf(e, "function `"ALUA_GLOBAL_TABLE".fini' must return an integer");
		rc = ACTF_LUA_WRONG_RET_TYPE;
	}
	lua_pop(L, 2);
	return rc;
}

enum lua_filter_state {
	LUA_FILTER_STATE_INIT,
	LUA_FILTER_STATE_ONGOING,
	LUA_FILTER_STATE_DONE,
	LUA_FILTER_STATE_ERROR,
};

struct actf_lua_filter {
	struct actf_event_generator gen;
	lua_State *L;
	struct error err;
	actf_event **in_evs;
	/* in_evs_len holds the length of the buffer in in_evs. */
	size_t in_evs_len;
	/* in_evs_i holds the current index of the buffer in
	 * in_evs. */
	size_t in_evs_i;
	/* evs holds the output buffer */
	actf_event **evs;
	/* evs_cap holds the capacity of evs */
	size_t evs_cap;
	enum lua_filter_state state;
	// stored error code if state == LUA_FILTER_STATE_ERROR to
	// return the correct error even if we have to return a few
	// valid events first.
	int err_rc;
};

actf_lua_filter *actf_lua_filter_init(struct actf_event_generator gen,
				      size_t evs_cap)
{
	actf_lua_filter *f = malloc(sizeof(*f));
	if (! f) {
		return NULL;
	}
	f->gen = gen;
	f->L = NULL;
	f->err = ERROR_EMPTY;
	f->in_evs = NULL;
	f->in_evs_len = 0;
	f->in_evs_i = 0;
	f->evs_cap = evs_cap ? evs_cap : ACTF_DEFAULT_EVS_CAP;
	f->evs = actf_event_arr_alloc(f->evs_cap);
	if (! f->evs) {
		int e = errno;
		free(f);
		errno = e;
		return NULL;
	}
	f->state = LUA_FILTER_STATE_INIT;
	f->err_rc = ACTF_ERROR;
	return f;
}

int actf_lua_filter_lua_init(actf_lua_filter *f, const char *filter_path,
			     int filter_argc, char *filter_argv[])
{
	int rc = ACTF_OK;
	if ((rc = alua_init(filter_path, &f->L, &f->err)) < 0) {
		return rc;
	}
	if ((rc = alua_call_init(f->L, filter_argc, filter_argv, &f->err)) < 0) {
		alua_free(f->L);
		f->L = NULL;
		return rc;
	}
	f->state = LUA_FILTER_STATE_ONGOING;
	return rc;
}

/* an actf_event_generate */
static int lua_filter_filter(void *self, actf_event ***evs, size_t *evs_len)
{
	actf_lua_filter *f = self;
	return actf_lua_filter_filter(f, evs, evs_len);
}

int actf_lua_filter_filter(actf_lua_filter *f, actf_event ***evs, size_t *evs_len)
{
	int rc;
	switch (f->state) {
	case LUA_FILTER_STATE_INIT:
		eprintf(&f->err, "lua_filter not initialized with a lua script");
		return ACTF_ERROR;
	case LUA_FILTER_STATE_ONGOING: {
		size_t out_len = 0;
		do {
			if (f->in_evs_i >= f->in_evs_len) {
				rc = f->gen.generate(f->gen.self, &f->in_evs, &f->in_evs_len);
				if (rc < 0) {
					const char *msg = f->gen.last_error(f->gen.self);
					eprintf(&f->err, "%s", msg ? msg : "unknown actf_event_generate error");
					f->state = LUA_FILTER_STATE_ERROR;
					f->err_rc = rc;
					return rc;
				} else if (f->in_evs_len == 0) {
					f->state = LUA_FILTER_STATE_DONE;
					*evs = f->evs;
					*evs_len = 0;
					return rc;
				}
				f->in_evs_i = 0;
			}
			for (; f->in_evs_i < f->in_evs_len && out_len < f->evs_cap; f->in_evs_i++) {
				rc = alua_call_filter(f->L, f->in_evs[f->in_evs_i], &f->err);
				if (rc < 0) {
					*evs = f->evs;
					*evs_len = out_len;
					f->state = LUA_FILTER_STATE_ERROR;
					f->err_rc = rc;
					return out_len ? ACTF_OK : rc;
				} else if (rc == 0) {
					actf_event_copy(f->evs[out_len], f->in_evs[f->in_evs_i]);
					out_len++;
				}
			}
		} while (out_len == 0);
		*evs = f->evs;
		*evs_len = out_len;
		return ACTF_OK;
	}
	case LUA_FILTER_STATE_DONE:
		*evs_len = 0;
		return ACTF_OK;
	case LUA_FILTER_STATE_ERROR:
		return f->err_rc;
	default:
		return ACTF_INTERNAL;
	}
}

/* an actf_seek_ns_from_origin */
static int lua_filter_seek_ns_from_origin(void *self, int64_t tstamp)
{
	actf_lua_filter *f = self;
	return actf_lua_filter_seek_ns_from_origin(f, tstamp);
}

int actf_lua_filter_seek_ns_from_origin(actf_lua_filter *f, int64_t tstamp)
{
	if (f->state == LUA_FILTER_STATE_INIT) {
		eprintf(&f->err, "lua_filter not initialized with a lua script");
		return ACTF_ERROR;
	}
	int rc = ACTF_OK;
	rc = f->gen.seek_ns_from_origin(f->gen.self, tstamp);
	if (rc < 0) {
		const char *msg = f->gen.last_error(f->gen.self);
		eprintf(&f->err, "%s", msg ? msg : "unknown actf_seek_ns_from_origin error");
		f->state = LUA_FILTER_STATE_ERROR;
		f->err_rc = rc;
		return rc;
	}
	f->in_evs = NULL;
	f->in_evs_len = 0;
	f->in_evs_i = 0;
	f->state = LUA_FILTER_STATE_ONGOING;
	return ACTF_OK;
}

/* an actf_last_error */
const char *lua_filter_last_error(void *self)
{
	actf_lua_filter *f = self;
	return actf_lua_filter_last_error(f);
}

const char *actf_lua_filter_last_error(actf_lua_filter *f)
{
	if (! f || ! f->err.buf || f->err.buf[0] == '\0') {
		return NULL;
	}
	return f->err.buf;
}

int actf_lua_filter_lua_fini(actf_lua_filter *f)
{
	return alua_call_fini(f->L, &f->err);
}

void actf_lua_filter_free(actf_lua_filter *f)
{
	if (! f) {
		return;
	}
	error_free(&f->err);
	actf_event_arr_free(f->evs);
	alua_free(f->L);
	free(f);
}

struct actf_event_generator actf_lua_filter_to_generator(actf_lua_filter *f)
{
	return (struct actf_event_generator) {
		.generate = lua_filter_filter,
		.seek_ns_from_origin = lua_filter_seek_ns_from_origin,
		.last_error = lua_filter_last_error,
		.self = f,
	};
}
