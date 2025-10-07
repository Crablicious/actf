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

#include <assert.h>
#include <errno.h>
#include <iconv.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "crust/common.h"
#include "print.h"
#include "types.h"


#define DEFAULT_CONVBUF_SZ 1024

struct actf_printer {
	iconv_t to_utf8[ACTF_N_ENCODINGS];
	char *convbuf;
	size_t convbuf_sz;

	int flags;
	int pkt_to_print_len;
	enum actf_pkt_prop pkt_to_print[ACTF_PKT_N_PROPS];
	int ev_to_print_len;
	enum actf_event_prop ev_to_print[ACTF_EVENT_N_PROPS];
	/* The last printed event's timestamp. Used for timestamp deltas.
	 * Type determined by ACTF_PRINT_TSTAMP_CC. */
	union {
		int64_t ns_from_origin;
		uint64_t cc;
	} last_tstamp;
	bool has_last_tstamp;
};


actf_printer *actf_printer_init(int flags)
{
	actf_printer *p = malloc(sizeof(*p));
	if (!p) {
		return NULL;
	}
	for (int i = 0; i < ACTF_N_ENCODINGS; i++) {
		p->to_utf8[i] = (iconv_t) - 1;
	}
	p->convbuf = malloc(DEFAULT_CONVBUF_SZ);
	if (!p->convbuf) {
		free(p);
		return NULL;
	}
	p->convbuf_sz = DEFAULT_CONVBUF_SZ;

	p->flags = flags;

	p->pkt_to_print_len = 0;
	if (p->flags & ACTF_PRINT_PKT_HEADER) {
		p->pkt_to_print[p->pkt_to_print_len++] = ACTF_PKT_PROP_HEADER;
	}
	if (p->flags & ACTF_PRINT_PKT_CTX) {
		p->pkt_to_print[p->pkt_to_print_len++] = ACTF_PKT_PROP_CTX;
	}

	p->ev_to_print_len = 0;
	if (p->flags & ACTF_PRINT_EVENT_HEADER) {
		p->ev_to_print[p->ev_to_print_len++] = ACTF_EVENT_PROP_HEADER;
	}
	if (p->flags & ACTF_PRINT_EVENT_COMMON_CTX) {
		p->ev_to_print[p->ev_to_print_len++] = ACTF_EVENT_PROP_COMMON_CTX;
	}
	if (p->flags & ACTF_PRINT_EVENT_SPECIFIC_CTX) {
		p->ev_to_print[p->ev_to_print_len++] = ACTF_EVENT_PROP_SPECIFIC_CTX;
	}
	if (p->flags & ACTF_PRINT_EVENT_PAYLOAD) {
		p->ev_to_print[p->ev_to_print_len++] = ACTF_EVENT_PROP_PAYLOAD;
	}

	p->has_last_tstamp = false;
	return p;
}

void actf_printer_free(actf_printer *p)
{
	if (!p) {
		return;
	}
	for (int i = 0; i < ACTF_N_ENCODINGS; i++) {
		if (p->to_utf8[i] != (iconv_t) - 1) {
			iconv_close(p->to_utf8[i]);
		}
	}
	free(p->convbuf);
	free(p);
}

/* init_to_utf8_iconv initializes the converter from enc to utf-8 if
 * it is not already initialized. */
int init_to_utf8_iconv(actf_printer *p, enum actf_encoding enc)
{
	if (p->to_utf8[enc] != (iconv_t) - 1) {
		return ACTF_OK;
	}
	iconv_t cd = (iconv_t) - 1;
	switch (enc) {
	case ACTF_ENCODING_UTF16BE:
		cd = iconv_open("UTF-8//TRANSLIT", "UTF-16BE");
		break;
	case ACTF_ENCODING_UTF16LE:
		cd = iconv_open("UTF-8//TRANSLIT", "UTF-16LE");
		break;
	case ACTF_ENCODING_UTF32BE:
		cd = iconv_open("UTF-8//TRANSLIT", "UTF-32BE");
		break;
	case ACTF_ENCODING_UTF32LE:
		cd = iconv_open("UTF-8//TRANSLIT", "UTF-32LE");
		break;
	case ACTF_ENCODING_UTF8:
		cd = iconv_open("UTF-8//TRANSLIT", "UTF-8");
		break;
	case ACTF_N_ENCODINGS:
		return ACTF_OK;
	}
	if (cd == (iconv_t) - 1) {
		return ACTF_ERROR;
	}
	p->to_utf8[enc] = cd;
	return ACTF_OK;
}

static size_t umappings_fprint_names(FILE *s, const struct actf_mappings *maps, uint64_t val)
{
	const char *name;
	size_t n_prints = 0;
	actf_it it = { 0 };
	while ((name = actf_mappings_find_uint(maps, val, &it))) {
		const char *fmt = n_prints > 0 ? ",\"%s\"" : "\"%s\"";
		fprintf(s, fmt, name);
		n_prints++;
	}
	return n_prints;
}

static size_t smappings_fprint_names(FILE *s, const struct actf_mappings *maps, int64_t val)
{
	const char *name;
	size_t n_prints = 0;
	actf_it it = { 0 };
	while ((name = actf_mappings_find_sint(maps, val, &it))) {
		const char *fmt = n_prints > 0 ? ",\"%s\"" : "\"%s\"";
		fprintf(s, fmt, name);
		n_prints++;
	}
	return n_prints;
}

static size_t actf_flags_fprint_names(FILE *s, const struct actf_flags *bm, uint64_t val)
{
	const char *name;
	size_t n_prints = 0;
	actf_it it = { 0 };
	while ((name = actf_flags_find(bm, val, &it))) {
		const char *fmt = n_prints > 0 ? ",\"%s\"" : "\"%s\"";
		fprintf(s, fmt, name);
		n_prints++;
	}
	return n_prints;
}

static uint8_t clz_lookup[256] = {
	8, 7, 6, 6, 5, 5, 5, 5,
	4, 4, 4, 4, 4, 4, 4, 4,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

static int clz(uint64_t n)
{
	if (n == 0) {
		return 64;
	}
	int r = 0;
	if ((n & UINT64_C(0xFFFFFFFF00000000)) == 0) {
		r += 32;
		n <<= 32;
	}
	if ((n & UINT64_C(0xFFFF000000000000)) == 0) {
		r += 16;
		n <<= 16;
	}
	if ((n & UINT64_C(0xFF00000000000000)) == 0) {
		r += 8;
		n <<= 8;
	}
	r += clz_lookup[n >> 56];
	return r;
}

static int sprint_uint_bin(char str[66], uint64_t n)
{
	str[0] = '0';
	str[1] = 'b';
	int len = MAX(64 - clz(n), 1);
	for (int i = 0; i < len; i++) {
		str[i + 2] = ((n >> (len - 1 - i)) & 1) + '0';
	}
	return len + 2;
}

static void fprint_sint(FILE *s, int64_t v, enum actf_base base)
{
	switch (base) {
	case ACTF_BASE_BINARY: {
		char str[67];
		int len = 0;
		if (v >= 0) {
			len += sprint_uint_bin(str, (uint64_t) v);
		} else {
			str[len++] = '-';
			len += sprint_uint_bin(str + len, (uint64_t) (-v));
		}
		fprintf(s, "%.*s", len, str);
		break;
	}
	case ACTF_BASE_OCTAL:
		if (v >= 0) {
			fprintf(s, "%#" PRIo64, (uint64_t) v);
		} else {
			fprintf(s, "-%#" PRIo64, (uint64_t) (-v));
		}
		break;
	case ACTF_BASE_DECIMAL:
		fprintf(s, "%" PRIi64, v);
		break;
	case ACTF_BASE_HEXADECIMAL:
		if (v >= 0) {
			fprintf(s, "%#" PRIx64, (uint64_t) v);
		} else {
			fprintf(s, "-%#" PRIx64, (uint64_t) (-v));
		}
		break;
	default:		// fall back to decimal
		fprintf(s, "%" PRIi64, v);
		break;
	}
}

static void fprint_uint(FILE *s, uint64_t v, enum actf_base base)
{
	switch (base) {
	case ACTF_BASE_BINARY: {
		char str[66];
		int len = sprint_uint_bin(str, v);
		fprintf(s, "%.*s", len, str);
		break;
	}
	case ACTF_BASE_OCTAL:
		fprintf(s, "%#" PRIo64, v);
		break;
	case ACTF_BASE_DECIMAL:
		fprintf(s, "%" PRIu64, v);
		break;
	case ACTF_BASE_HEXADECIMAL:
		fprintf(s, "%#" PRIx64, v);
		break;
	default:		// fall back to decimal
		fprintf(s, "%" PRIu64, v);
		break;
	}
}

int actf_fprint_fld(actf_printer *p, FILE *s, const actf_fld *fld)
{
	int rc = ACTF_OK;
	const char *NIL = "nil";
	switch (actf_fld_type(fld)) {
	case ACTF_FLD_TYPE_NIL:
		fprintf(s, NIL);
		break;
	case ACTF_FLD_TYPE_SINT: {
		int64_t v = actf_fld_int64(fld);
		const actf_fld_cls *cls = actf_fld_fld_cls(fld);
		enum actf_base base = actf_fld_cls_pref_display_base(cls);
		const struct actf_mappings *maps = actf_fld_cls_mappings(cls);
		fprint_sint(s, v, base);
		if (maps && actf_mappings_len(maps)) {
			fprintf(s, ": ");
			if (smappings_fprint_names(s, maps, v) == 0) {
				fprintf(s, NIL);
			}
		}
		break;
	}
	case ACTF_FLD_TYPE_UINT: {
		uint64_t v = actf_fld_uint64(fld);
		const actf_fld_cls *cls = actf_fld_fld_cls(fld);
		enum actf_base base = actf_fld_cls_pref_display_base(cls);
		const struct actf_mappings *maps =
			actf_fld_cls_mappings(actf_fld_fld_cls(fld));
		fprint_uint(s, v, base);
		if (maps && actf_mappings_len(maps)) {
			fprintf(s, ": ");
			if (umappings_fprint_names(s, maps, v) == 0) {
				fprintf(s, NIL);
			}
		}
		break;
	}
	case ACTF_FLD_TYPE_BIT_MAP: {
		uint64_t v = actf_fld_uint64(fld);
		const struct actf_flags *flags =
			actf_fld_cls_bit_map_flags(actf_fld_fld_cls(fld));
		fprint_uint(s, v, ACTF_BASE_HEXADECIMAL);
		if (flags) {
			fprintf(s, ": ");
			if (actf_flags_fprint_names(s, flags, v) == 0) {
				fprintf(s, NIL);
			}
		} else {
			assert(!"A bit map field should always have flags");
		}
		break;
	}
	case ACTF_FLD_TYPE_REAL: {
		fprintf(s, "%f", actf_fld_double(fld));
		break;
	}
	case ACTF_FLD_TYPE_ARR:
		fprintf(s, "[");
		for (size_t i = 0; i < actf_fld_arr_len(fld); i++) {
			if (i != 0) {
				fprintf(s, ", ");
			}
			actf_fprint_fld(p, s, actf_fld_arr_idx(fld, i));
		}
		fprintf(s, "]");
		break;
	case ACTF_FLD_TYPE_BOOL:
		fprintf(s, "%d", actf_fld_bool(fld));
		break;
	case ACTF_FLD_TYPE_STR: {
		const char *str = actf_fld_str_raw(fld);
		size_t sz = actf_fld_str_sz(fld);
		enum actf_encoding enc = actf_fld_cls_encoding(actf_fld_fld_cls(fld));
		if (enc == ACTF_ENCODING_UTF8) {
			fprintf(s, "\"%.*s\"", (int) sz, str);
			break;
		}
		rc = init_to_utf8_iconv(p, enc);
		if (rc < 0) {
			break;
		}

		char *inbuf = (char *) str;
		char *outbuf = p->convbuf;
		size_t inbytesleft = sz;
		size_t outbytesleft = p->convbuf_sz;
		fprintf(s, "\"");
		while (inbytesleft && rc == ACTF_OK) {
			size_t irc =
				iconv(p->to_utf8[enc], &inbuf, &inbytesleft, &outbuf,
				      &outbytesleft);
			if (irc == (size_t) -1) {
				if (errno == E2BIG) {
					fprintf(s, "%.*s",
						(int) (p->convbuf_sz - outbytesleft),
						p->convbuf);
					outbuf = p->convbuf;
					outbytesleft = p->convbuf_sz;
				} else {
					fprintf(s, "?");
					rc = ACTF_ERROR;
				}
			} else {
				fprintf(s, "%.*s", (int) (p->convbuf_sz - outbytesleft),
					p->convbuf);
			}
		}
		if (rc == ACTF_OK) {
			/* Flush out any partially converted input. */
			outbuf = p->convbuf;
			outbytesleft = p->convbuf_sz;
			inbytesleft = 0;
			size_t irc =
				iconv(p->to_utf8[enc], NULL, &inbytesleft, &outbuf,
				      &outbytesleft);
			if (irc == (size_t) -1) {
				fprintf(s, "?");
				rc = ACTF_ERROR;
			} else {
				fprintf(s, "%.*s", (int) (p->convbuf_sz - outbytesleft),
					p->convbuf);
			}
		}
		fprintf(s, "\"");
		break;
	}
	case ACTF_FLD_TYPE_BLOB: {
		const uint8_t *data = actf_fld_blob(fld);
		for (size_t i = 0; i < actf_fld_blob_sz(fld); i++) {
			fprintf(s, "%02x", data[i]);
		}
		break;
	}
	case ACTF_FLD_TYPE_STRUCT:
		fprintf(s, "{ ");
		size_t len = actf_fld_struct_len(fld);
		for (size_t i = 0; i < len; i++) {
			fprintf(s, "%s: ", actf_fld_struct_fld_name_idx(fld, i));
			actf_fprint_fld(p, s, actf_fld_struct_fld_idx(fld, i));
			if (i != (len - 1)) {
				fprintf(s, ", ");
			}
		}
		fprintf(s, " }");
		break;
	}

	return rc;
}

int actf_print_fld(actf_printer *p, const actf_fld *fld)
{
	return actf_fprint_fld(p, stdout, fld);
}

static const char *actf_pkt_prop_to_name(enum actf_pkt_prop prop)
{
	switch (prop) {
	case ACTF_PKT_PROP_HEADER:
		return "packet-header";
	case ACTF_PKT_PROP_CTX:
		return "packet-context";
	case ACTF_PKT_N_PROPS:
		return "";
	}
	return "";
}

static const char *actf_event_prop_to_name(enum actf_event_prop prop)
{
	switch (prop) {
	case ACTF_EVENT_PROP_HEADER:
		return "event-header";
	case ACTF_EVENT_PROP_COMMON_CTX:
		return "event-common-context";
	case ACTF_EVENT_PROP_SPECIFIC_CTX:
		return "event-specific-context";
	case ACTF_EVENT_PROP_PAYLOAD:
		return "event-payload";
	case ACTF_EVENT_N_PROPS:
		return "";
	}
	return "";
}

static void fprint_tstamp_ns(FILE *s, int64_t tstamp_ns, int flags)
{
	time_t sec = tstamp_ns / INT64_C(1000000000);
	int64_t nsec = tstamp_ns % INT64_C(1000000000);
	if (flags & ACTF_PRINT_TSTAMP_SEC) {
		fprintf(s, "[%" PRIi64 ".%09lld] ", (int64_t) sec, llabs(nsec));
		return;
	}
	if (nsec < 0) {
		sec--;
		nsec = 1000000000 + nsec;
	}
	struct tm tm;
	if (flags & ACTF_PRINT_TSTAMP_UTC) {
		gmtime_r(&sec, &tm);
	} else {
		localtime_r(&sec, &tm);
	}
	if (flags & ACTF_PRINT_TSTAMP_DATE) {
		char yyyymmddhhmmss[22];
		strftime(yyyymmddhhmmss, 22, "%Y-%m-%d %H:%M:%S", &tm);
		fprintf(s, "[%s.%09" PRIi64 "] ", yyyymmddhhmmss, nsec);
	} else {
		char hhmmss[10];
		strftime(hhmmss, 10, "%H:%M:%S", &tm);
		fprintf(s, "[%s.%09" PRIi64 "] ", hhmmss, nsec);
	}
}

int actf_fprint_event(actf_printer *p, FILE *s, const actf_event *ev)
{
	const actf_event_cls *evc = actf_event_event_cls(ev);
	const actf_dstream_cls *dsc = actf_event_cls_dstream_cls(evc);
	const actf_clk_cls *clkc = actf_dstream_cls_clk_cls(dsc);

	if (clkc) {
		if ((p->flags & ACTF_PRINT_TSTAMP_CC) == 0) {
			int64_t tstamp = actf_event_tstamp_ns_from_origin(ev);
			fprint_tstamp_ns(s, tstamp, p->flags);
			if (p->flags & ACTF_PRINT_TSTAMP_DELTA) {
				if (p->has_last_tstamp) {
					fprintf(s, "(+%010" PRIi64 ") ",
						tstamp - p->last_tstamp.ns_from_origin);
				} else {
					fprintf(s, "(+\?\?\?\?\?\?\?\?\?\?) ");
					p->has_last_tstamp = true;
				}
				p->last_tstamp.ns_from_origin = tstamp;
			}
		} else {
			uint64_t tstamp = actf_event_tstamp(ev);
			fprintf(s, "[%020" PRIu64 "] ", tstamp);
			if (p->flags & ACTF_PRINT_TSTAMP_DELTA) {
				if (p->has_last_tstamp) {
					fprintf(s, "(+%010" PRIu64 ") ",
						tstamp - p->last_tstamp.cc);
				} else {
					fprintf(s, "(+\?\?\?\?\?\?\?\?\?\?) ");
					p->has_last_tstamp = true;
				}
				p->last_tstamp.cc = tstamp;
			}
		}
	}

	if (actf_event_cls_namespace(evc) && actf_event_cls_name(evc)) {
		fprintf(s, "%s::%s: ", actf_event_cls_namespace(evc), actf_event_cls_name(evc));
	} else if (actf_event_cls_name(evc)) {
		fprintf(s, "%s: ", actf_event_cls_name(evc));
	}

	fprintf(s, "{ ");
	bool do_comma = false;
	actf_pkt *pkt = actf_event_pkt(ev);
	for (int i = 0; i < p->pkt_to_print_len; i++) {
		const actf_fld *fld = actf_pkt_prop(pkt, p->pkt_to_print[i]);
		if (actf_fld_type(fld) == ACTF_FLD_TYPE_NIL) {
			continue;
		}
		if (do_comma) {
			fprintf(s, ", ");
		}
		if (p->flags & ACTF_PRINT_PROP_LABELS) {
			fprintf(s, "%s: ", actf_pkt_prop_to_name(p->pkt_to_print[i]));
		}
		actf_fprint_fld(p, s, fld);
		do_comma = true;
	}

	for (int i = 0; i < p->ev_to_print_len; i++) {
		const actf_fld *fld = actf_event_prop(ev, p->ev_to_print[i]);
		if (actf_fld_type(fld) == ACTF_FLD_TYPE_NIL) {
			continue;
		}
		if (do_comma) {
			fprintf(s, ", ");
		}
		if (p->flags & ACTF_PRINT_PROP_LABELS) {
			fprintf(s, "%s: ", actf_event_prop_to_name(p->ev_to_print[i]));
		}
		actf_fprint_fld(p, s, fld);
		do_comma = true;
	}
	fprintf(s, " }");
	return ACTF_OK;
}

int actf_print_event(actf_printer *p, const actf_event *ev)
{
	return actf_fprint_event(p, stdout, ev);
}
