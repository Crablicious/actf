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

/**
 * @file
 * A lua event filter and its methods.
 *
 * The lua filter reads in a lua file and filters event based on it.
 * Three separate functions are supported in the lua file, actf.init,
 * actf.filter and actf.fini. actf.filter is mandatory.
 *
 * The lua filter is designed to be used for quick ad-hoc filtering or
 * processing, therefore the full actf API is not exposed to it. The
 * lua bindings are focused around accessing an event's fields and
 * values.
 *
 * # Lua Filter Callbacks
 *
 * ## actf.init(...)
 *
 * actf.init is is called by actf_lua_filter_lua_init(), takes a
 * variable number of string arguments as input and returns a status
 * code, zero or nil means success.
 *
 * ## actf.filter(ev)
 *
 * actf.filter is called for every event entering the filter. The
 * given event and its components (packet, fields) are only valid
 * during this call and should not be kept around. Values, such as
 * numbers and strings, are copied into lua and can be kept for later
 * referencing. It should return 0 to keep the event, 1 to filter the
 * event and <0 to error out; nil is classified as 1.
 *
 * ## actf.fini()
 *
 * actf.fini is called as a finalization step by
 * actf_lua_filter_lua_fini(). It should return a status code, zero or nil
 * means success.
 *
 * # Example filter
 *
 * The following filter prints the arguments it was given during
 * initialization and performs the simple task of counting the number
 * of events grouped by their names which it then prints at the end.
 *
 * ```lua
 * function setDefault(t, d)
 *    local mt = {__index = function () return d end}
 *    setmetatable(t, mt)
 * end
 *
 * function actf.init(...)
 *    stats = {}
 *    setDefault(stats, 0)
 *    print("init with "..tostring(select("#", ...)).." args:")
 *    for i,v in ipairs(table.pack(...)) do
 *       print(i, v)
 *    end
 * end
 *
 * function actf.filter(ev)
 *    local name = ev:name() or "UNNAMED EVENT"
 *    stats[name] = stats[name] + 1
 *    return 1
 * end
 *
 * function actf.fini()
 *    for name, cnt in pairs(stats) do
 *       print(name, cnt)
 *    end
 * end
 * ```
 *
 * # Lua Types
 *
 * These are the actf types and their methods that are exposed to lua.
 *
 * ## Field Representations
 *
 * actf fields apart from the compound types are represented as
 * regular lua types. This is what will be returned when you access a
 * field using get(). The signed integer (SINT), unsigned integer
 * (UINT) and bit map types return their integer value followed by all
 * their associated mappings or flags.
 *
 * | actf_fld_type | lua type             |
 * |---------------|----------------------|
 * | NIL           | nil                  |
 * | BOOL          | boolean              |
 * | SINT          | integer, [string...] |
 * | UINT          | integer, [string...] |
 * | BIT_MAP       | integer, [string...] |
 * | REAL          | number               |
 * | STR           | string               |
 * | BLOB          | string               |
 * | STRUCT        | compound field       |
 * | ARR           | compound field       |
 *
 * ## Raw Field Represenations
 *
 * When accessing fields using rawget(), you will receive a field or a
 * compound field rather than a resolved value. This allows you to
 * access more properties of the field such as attributes.
 *
 * | actf_fld_type | lua type       |
 * |---------------|----------------|
 * | NIL           | field          |
 * | BOOL          | field          |
 * | SINT          | field          |
 * | UINT          | field          |
 * | BIT_MAP       | field          |
 * | REAL          | field          |
 * | STR           | field          |
 * | BLOB          | field          |
 * | STRUCT        | compound field |
 * | ARR           | compound field |
 *
 * ## Field
 *
 * A non-compound field is represented with this lua type. This is
 * what can be returned from rawget() methods.
 *
 * | name       | description                           | arguments | return type               |
 * |------------|---------------------------------------|-----------|---------------------------|
 * | value      | resolve the field into a lua value    |           | the field type's lua type |
 * | attributes | get the field's attributes            |           | field or nil              |
 * | extensions | get the field's extensions            |           | field or nil              |
 * | __tostring | get the field's string representation |           | string                    |
 *
 * ## Compound Field
 *
 * A compound field, struct or array, are represented with a special
 * lua type. An array's elements can be accessed using get() with an
 * integer as the argument (1-based indexing). A struct's members can
 * be accessed using get() with either a string or an integer as the
 * argument. A string is matched against the members' names and an
 * integer is interpreted as the Nth member (1-based indexing). The
 * following methods are defined for a compound type:
 *
 * | name       | description                                        | arguments         | return type  |
 * |------------|----------------------------------------------------|-------------------|--------------|
 * | get        | get a field of the compound field                  | string or integer | field or nil |
 * | rawget     | get a non-resolved field of the compound field     | string or integer | field or nil |
 * | attributes | get the field's attributes                         |                   | field or nil |
 * | extensions | get the field's extensions                         |                   | field or nil |
 * | __len      | get the number of elements or members in the field |                   | integer      |
 * | __tostring | get the field's string representation              |                   | string       |
 *
 * ## Event Methods
 *
 * An event has the following methods defined:
 *
 * | name             | description                                                                          | arguments | return type   |
 * |------------------|--------------------------------------------------------------------------------------|-----------|---------------|
 * | namespace        | get the event's namespace                                                            |           | string or nil |
 * | name             | get the event's name                                                                 |           | string or nil |
 * | uid              | get the event's uid                                                                  |           | string or nil |
 * | get              | get a field in the event's header, context, specific context or payload              | string    | field or nil  |
 * | rawget           | get a non-resolved field in the event's header, context, specific context or payload | string    | field or nil  |
 * | header           | get the event's header                                                               |           | field or nil  |
 * | context          | get the event's context                                                              |           | field or nil  |
 * | specific_context | get the event's specific context                                                     |           | field or nil  |
 * | payload          | get the event's payload                                                              |           | field or nil  |
 * | packet           | get the event's packet                                                               |           | packet        |
 * | timestamp        | get the event's raw timestamp                                                        |           | integer       |
 * | timestamp_ns     | get the event's timestamp in nanoseconds from origin                                 |           | integer       |
 * | attributes       | get the event's attributes                                                           |           | field or nil  |
 * | extensions       | get the event's extensions                                                           |           | field or nil  |
 * | __tostring       | get the event's string representation                                                |           | string        |
 *
 * ## Packet Methods
 *
 * A packet has the following methods defined:
 *
 * | name               | description                                                     | arguments | return type    |
 * |--------------------|-----------------------------------------------------------------|-----------|----------------|
 * | get                | get a field of the packet in its header or context              | string    | field or nil   |
 * | rawget             | get a non-resolved field of the packet in its header or context | string    | field or nil   |
 * | header             | get the packet's header                                         |           | field or nil   |
 * | context            | get the packet's context                                        |           | field or nil   |
 * | sequence_number    | get the packet's sequence number                                |           | integer or nil |
 * | begin_timestamp    | get the packet's raw begin timestamp                            |           | integer        |
 * | begin_timestamp_ns | get the packet's begin timestamp in nanoseconds from origin     |           | integer        |
 * | end_timestamp      | get the packet's raw end timestamp                              |           | integer or nil |
 * | end_timestamp_ns   | get the packet's end timestamp in nanoseconds from origin       |           | integer or nil |
 * | discard_snapshot   | get the packet's discarded event snapshot                       |           | integer or nil |
 *
 */
#ifndef LUA_FILTER_H
#define LUA_FILTER_H

#include "event.h"
#include "event_generator.h"


/** A lua filter, implements an actf_event_generator
 *
 * It should be setup and used in the following order:
 * 1. actf_lua_filter_init
 * 2. actf_lua_filter_lua_init
 * 3. <event processing usage>
 * 4. actf_lua_filter_lua_fini
 * 5. actf_lua_filter_free
 */
typedef struct actf_lua_filter actf_lua_filter;

/**
 * Initialize a lua filter struct. The lua state and filter should be
 * loaded subsequently in actf_lua_filter_lua_init(). The
 * initialization is a two-step process to allow us to return detailed
 * error messages using actf_lua_filter_last_error().
 * @param gen the generator to filter
 * @param evs_cap the max event array capacity. If zero,
 * ACTF_DEFAULT_EVS_CAP will be used. This should in general be set to
 * the same capacity as the provided generator.
 * @return a filter or NULL with errno set. A returned filter should
 * be freed with actf_filter_free().
 */
actf_lua_filter *actf_lua_filter_init(struct actf_event_generator gen,
				      size_t evs_cap);

/**
 * Initialize the lua part of the filter and call actf.init.
 * @param f the lua filter struct
 * @param filter_path the path of the lua filter
 * @param filter_argc the number of args given to the filter
 * @param filter_argv the args given to the filter
 * @return ACTF_OK on success or an error code. On error, see actf_last_error().
 */
int actf_lua_filter_lua_init(actf_lua_filter *f, const char *filter_path,
			     int filter_argc, char *filter_argv[]);

/** @see actf_event_generate. Will call actf.filter to filter each event. */
int actf_lua_filter_filter(actf_lua_filter *f, actf_event ***evs, size_t *evs_len);

/** @see actf_seek_ns_from_origin */
int actf_lua_filter_seek_ns_from_origin(actf_lua_filter *f, int64_t tstamp);

/** @see actf_last_error */
const char *actf_lua_filter_last_error(actf_lua_filter *f);

/** Finalize a filter, calling actf.fini */
int actf_lua_filter_lua_fini(actf_lua_filter *f);

/**
 * Free a filter
 * @param f the filter
 */
void actf_lua_filter_free(actf_lua_filter *f);

/**
 * Create an event generator based on a filter
 *
 * The filter is owned by the caller and must be kept alive as long as
 * the event generator is in use.
 *
 * @param f the filter
 * @return an event generator
 */
struct actf_event_generator actf_lua_filter_to_generator(actf_lua_filter *f);

#endif /* LUA_FILTER_H */
