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

#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "actf.h"


#define ARRLEN(a) (sizeof(a)/sizeof(a)[0])

#define NANOSECOND  (1L)
#define MICROSECOND (1000L * NANOSECOND)
#define MILLISECOND (1000L * MICROSECOND)
#define SECOND      (1000L * MILLISECOND)
#define MINUTE      (60L * SECOND)
#define HOUR        (60L * MINUTE)


static void print_usage(void)
{
	fprintf(stderr, "Usage: actf [option(s)] [CTF_PATH(s)]\n"
		" The options are:\n"
		"  -p <opts>   Select what to print. The opts argument is a comma-separated list\n"
		"              with the event property to print. Supported properties are:\n"
		"              packet-header, packet-context, event-header, event-common-context,\n"
		"              event-specific-context, event-payload and all.\n"
		"              For example: -p event-header,event-payload\n"
		"  -l          Print labels for each event property\n"
		"  -d          Print timestamp delta between events\n"
		"  -c          Print timestamps in cycles (default [hh:mm:ss.ns] in localtime)\n"
		"  -g          Print and parse (-b/-e) timestamps in UTC instead of localtime.\n"
		"  -t          Print timestamps with the full date.\n"
		"  -s          Print timestamps in seconds.nanoseconds.\n"
		"  -b <tstamp> Trim events occurring before tstamp. The unit of tstamp is\n"
		"              ns and can be given in the following formats:\n"
		"                yyyy-mm-dd hh:ii[:ss[.nano]]\n"
		"                hh:ii[:ss[.nano]]\n"
		"                [-]sec[.nano]\n"
		"              For the hh:ii[:ss[.nano]] format, the date will be taken from the first event.\n"
		"              For the [-]sec[.nano] format, sec is the number of seconds from origin.\n"
		"              The date is considered localtime. If you want UTC, set environment to TZ=UTC.\n"
		"  -e <tstamp> Trim events occurring after tstamp. See available formats under -b.\n"
		"  -q          Quiet, do not print events\n" "  -h          Print help\n" "");
}

struct flags {
	bool quiet;
	char **ctf_paths;
	size_t ctf_paths_len;
	int printer_flags;
	struct actf_filter_time_range filter_range;
	bool has_filter_range;
};

/* strtoboundi64 parses s as a bounded base10 int64 and puts it in
 * val. The lower and upper bounds are inclusive. */
static int strtoboundi64(const char *s, int64_t lower, int64_t upper, int64_t *val)
{
	errno = 0;
	int64_t v = (int64_t) strtoull(s, NULL, 10);
	if (errno != 0 || v < lower || v > upper) {
		return ACTF_ERROR;
	}
	*val = v;
	return ACTF_OK;
}

static bool is_leap_year(int year)
{
	return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

/* gmtime is not part of POSIX or C-standard, so we implement.
   Based on "Seconds Since the Epoch" from:
   https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15 */
static time_t c_gmtime(struct tm *tm)
{
	static const int days_to_month[] =
	    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	int tm_yday = 0;
	tm_yday += tm->tm_mday - 1;
	tm_yday += days_to_month[tm->tm_mon];
	if (tm->tm_mon > 1 && is_leap_year(tm->tm_year + 1900)) {
		tm_yday++;
	}
	return tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 + tm_yday * 86400 +
	    (tm->tm_year - 70) * 31536000 + ((tm->tm_year - 69) / 4) * 86400 -
	    ((tm->tm_year - 1) / 100) * 86400 + ((tm->tm_year + 299) / 400) * 86400;
}

static int local_time_to_epoch(int hour, int min, int sec, int64_t *epoch_sec)
{
	/* setup a tm with unix epoch to convert the provided time to
	 * utc. */
	struct tm tm = {.tm_sec = sec,.tm_min = min,.tm_hour = hour,
		.tm_mday = 1,.tm_mon = 0,.tm_year = 1970 - 1900,.tm_isdst = -1
	};
	errno = 0;
	time_t epoch_s = mktime(&tm);
	if (epoch_s == (time_t) - 1 && errno) {
		return ACTF_ERROR;
	}
	*epoch_sec = epoch_s;
	return ACTF_OK;
}

static int parse_tstamp_ns(const char *s, bool parse_as_utc, int64_t *ns, bool *has_date)
{
	/* yyyy-mm-dd hh:ii[:ss[.nano]] */
	const char *yyyymmddhhiiss_re =
	    "^([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2})($|:([0-9]{2})($|\\.([0-9]{1,9})$))";
	/* hh:ii[:ss[.nano]] */
	const char *hhiiss_re = "^([0-9]{2}):([0-9]{2})($|:([0-9]{2})($|\\.([0-9]{1,9})$))";
	/* [-]sec[.nano] */
	const char *sec_re = "^(-|)+([0-9]+)($|\\.([0-9]{1,9}))";

	int rc = ACTF_OK;
	regex_t yyyymmddhhiiss_regex, hhiiss_regex, sec_regex;
	regmatch_t pmatch[10];

	if (regcomp(&yyyymmddhhiiss_regex, yyyymmddhhiiss_re, REG_EXTENDED)) {
		fprintf(stderr, "error compiling regex\n");
		rc = ACTF_ERROR;
		goto out;
	}
	if (regcomp(&hhiiss_regex, hhiiss_re, REG_EXTENDED)) {
		fprintf(stderr, "error compiling regex\n");
		rc = ACTF_ERROR;
		goto free_yyyymmddhhiiss_regex;
	}
	if (regcomp(&sec_regex, sec_re, REG_EXTENDED)) {
		fprintf(stderr, "error compiling regex\n");
		rc = ACTF_ERROR;
		goto free_hhiiss_regex;
	}

	if (regexec(&yyyymmddhhiiss_regex, s, ARRLEN(pmatch), pmatch, 0) == 0) {
		regmatch_t year_match = pmatch[1];	// always
		regmatch_t mon_match = pmatch[2];	// always
		regmatch_t day_match = pmatch[3];	// always
		regmatch_t hour_match = pmatch[4];	// always
		regmatch_t min_match = pmatch[5];	// always
		regmatch_t sec_match = pmatch[7];	// maybe
		regmatch_t nsec_match = pmatch[9];	// maybe

		int64_t year, mon, day, hour, min, sec = 0, nsec = 0;
		if (strtoboundi64(s + year_match.rm_so, 0, INT64_MAX, &year) < 0) {	// TBD: year range?
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (strtoboundi64(s + mon_match.rm_so, 1, 12, &mon) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (strtoboundi64(s + day_match.rm_so, 1, 31, &day) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (strtoboundi64(s + hour_match.rm_so, 0, 23, &hour) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (strtoboundi64(s + min_match.rm_so, 0, 59, &min) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (sec_match.rm_so != -1 && strtoboundi64(s + sec_match.rm_so, 0, 60, &sec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (nsec_match.rm_so != -1
		    && strtoboundi64(s + nsec_match.rm_so, 0, INT64_MAX, &nsec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		struct tm tm = {.tm_sec = sec,.tm_min = min,.tm_hour = hour,
			.tm_mday = day,.tm_mon = mon - 1,.tm_year = year - 1900,.tm_isdst = -1
		};
		time_t epoch_sec = parse_as_utc ? c_gmtime(&tm) : mktime(&tm);
		if (epoch_sec == (time_t) - 1) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		*ns = (int64_t) nsec + (int64_t) epoch_sec *(int64_t) SECOND;
		*has_date = true;
	} else if (regexec(&hhiiss_regex, s, ARRLEN(pmatch), pmatch, 0) == 0) {
		regmatch_t hour_match = pmatch[1];	// always
		regmatch_t min_match = pmatch[2];	// always
		regmatch_t sec_match = pmatch[4];	// maybe
		regmatch_t nsec_match = pmatch[6];	// maybe

		int64_t hour, min, sec = 0, nsec = 0;
		if (strtoboundi64(s + hour_match.rm_so, 0, 23, &hour) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (strtoboundi64(s + min_match.rm_so, 0, 59, &min) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (sec_match.rm_so != -1 && strtoboundi64(s + sec_match.rm_so, 0, 60, &sec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (nsec_match.rm_so != -1
		    && strtoboundi64(s + nsec_match.rm_so, 0, INT64_MAX, &nsec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		int64_t epoch_sec;
		if (local_time_to_epoch(hour, min, sec, &epoch_sec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		*ns = epoch_sec * (int64_t) SECOND + nsec;
		*has_date = false;
	} else if (regexec(&sec_regex, s, ARRLEN(pmatch), pmatch, 0) == 0) {
		regmatch_t sign_match = pmatch[1];	// always
		regmatch_t sec_match = pmatch[2];	// always
		regmatch_t nsec_match = pmatch[4];	// maybe

		int64_t sign = *(s + sign_match.rm_so) == '-' ? -1 : 1;
		int64_t sec = 0, nsec;
		if (strtoboundi64(s + sec_match.rm_so, 0, INT64_MAX, &sec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		if (nsec_match.rm_so != -1
		    && strtoboundi64(s + nsec_match.rm_so, 0, INT64_MAX, &nsec) < 0) {
			rc = ACTF_ERROR;
			goto free_sec_regex;
		}
		*ns = sign * (sec * (int64_t) SECOND + nsec);
		*has_date = true;
	} else {
		rc = ACTF_ERROR;
	}

      free_sec_regex:
	regfree(&sec_regex);
      free_hhiiss_regex:
	regfree(&hhiiss_regex);
      free_yyyymmddhhiiss_regex:
	regfree(&yyyymmddhhiiss_regex);
      out:
	return rc;
}

/* parse_flags populates f or fails horribly. */
static void parse_flags(int argc, char *argv[], struct flags *f)
{
	enum {
		PRINT_PKT_HEADER_OPT,
		PRINT_PKT_CTX_OPT,
		PRINT_EVENT_HEADER_OPT,
		PRINT_EVENT_COMMON_CTX_OPT,
		PRINT_EVENT_SPECIFIC_CTX_OPT,
		PRINT_EVENT_PAYLOAD_OPT,
		PRINT_ALL_OPT,
	};
	char *const token[] = {
		[PRINT_PKT_HEADER_OPT] = "packet-header",
		[PRINT_PKT_CTX_OPT] = "packet-context",
		[PRINT_EVENT_HEADER_OPT] = "event-header",
		[PRINT_EVENT_COMMON_CTX_OPT] = "event-common-context",
		[PRINT_EVENT_SPECIFIC_CTX_OPT] = "event-specific-context",
		[PRINT_EVENT_PAYLOAD_OPT] = "event-payload",
		[PRINT_ALL_OPT] = "all",
		NULL
	};
	char *filter_begin = NULL;
	char *filter_end = NULL;
	int opt;
	char *subopts;
	char *value;
	while ((opt = getopt(argc, argv, "p:ldcgtsb:e:qh")) != -1) {
		switch (opt) {
		case 'p':
			subopts = optarg;
			while (*subopts != '\0') {
				switch (getsubopt(&subopts, token, &value)) {
				case PRINT_PKT_HEADER_OPT:
					f->printer_flags |= ACTF_PRINT_PKT_HEADER;
					break;
				case PRINT_PKT_CTX_OPT:
					f->printer_flags |= ACTF_PRINT_PKT_CTX;
					break;
				case PRINT_EVENT_HEADER_OPT:
					f->printer_flags |= ACTF_PRINT_EVENT_HEADER;
					break;
				case PRINT_EVENT_COMMON_CTX_OPT:
					f->printer_flags |= ACTF_PRINT_EVENT_COMMON_CTX;
					break;
				case PRINT_EVENT_SPECIFIC_CTX_OPT:
					f->printer_flags |= ACTF_PRINT_EVENT_SPECIFIC_CTX;
					break;
				case PRINT_EVENT_PAYLOAD_OPT:
					f->printer_flags |= ACTF_PRINT_EVENT_PAYLOAD;
					break;
				case PRINT_ALL_OPT:
					f->printer_flags |= ACTF_PRINT_ALL;
					break;
				default:
					fprintf(stderr, "No match for print option: \"%s\"\n",
						value);
					print_usage();
					exit(-1);
				}
			}
			break;
		case 'l':
			f->printer_flags |= ACTF_PRINT_PROP_LABELS;
			break;
		case 'd':
			f->printer_flags |= ACTF_PRINT_TSTAMP_DELTA;
			break;
		case 'c':
			f->printer_flags |= ACTF_PRINT_TSTAMP_CC;
			break;
		case 'g':
			f->printer_flags |= ACTF_PRINT_TSTAMP_UTC;
			break;
		case 't':
			f->printer_flags |= ACTF_PRINT_TSTAMP_DATE;
			break;
		case 's':
			f->printer_flags |= ACTF_PRINT_TSTAMP_SEC;
			break;
		case 'b':
			filter_begin = optarg;
			break;
		case 'e':
			filter_end = optarg;
			break;
		case 'q':
			f->quiet = true;
			break;
		case 'h':
			print_usage();
			exit(0);
		default:	/* '?' */
			print_usage();
			exit(-1);
		}
	}

	if ((argc - optind) < 1) {
		fprintf(stderr, "Expected one or more positional arguments with CTF directories\n");
		print_usage();
		exit(-1);
	}
	f->ctf_paths = &argv[optind];
	f->ctf_paths_len = argc - optind;

	if ((f->printer_flags & ACTF_PRINT_ALL) == 0) {
		f->printer_flags |= ACTF_PRINT_ALL;
	}

	f->filter_range = ACTF_FILTER_TIME_RANGE_ALL;
	f->has_filter_range = (filter_begin || filter_end);
	const char *errfmt = "invalid timestamp (%s), the formats "
	    "yyyy-mm-dd hh:ii[:ss[.nano]], hh:ii[:ss[.nano]] and [-]sec[.nano] are supported\n";
	bool parse_as_utc = (f->printer_flags & ACTF_PRINT_TSTAMP_UTC);
	if (filter_begin && parse_tstamp_ns(filter_begin, parse_as_utc,
					    &f->filter_range.begin,
					    &f->filter_range.begin_has_date) < 0) {
		fprintf(stderr, errfmt, filter_begin);
		exit(-1);
	}
	if (filter_end && parse_tstamp_ns(filter_end, parse_as_utc,
					  &f->filter_range.end,
					  &f->filter_range.end_has_date) < 0) {
		fprintf(stderr, errfmt, filter_end);
		exit(-1);
	}
}

static int read_events(struct actf_event_generator gen, bool quiet, int printer_flags)
{
	int rc = ACTF_OK;
	uint64_t count = 0;
	uint64_t last_disc_evs = 0;
	actf_printer *p = actf_printer_init(printer_flags);
	if (!p) {
		return ACTF_ERROR;
	}

	uint64_t last_seq_num = 0;
	size_t evs_len = 0;
	actf_event **evs = NULL;
	while ((rc = gen.generate(gen.self, &evs, &evs_len)) == 0 && evs_len) {
		for (size_t i = 0; i < evs_len; i++) {
			if (!quiet) {
				actf_print_event(p, evs[i]);
				printf("\n");
			}
			const actf_pkt *pkt = actf_event_pkt(evs[i]);
			uint64_t seq_num = actf_pkt_seq_num(pkt);
			if (count == 0 || seq_num != last_seq_num) {
				uint64_t disc_evs = actf_pkt_disc_event_record_snapshot(pkt);
				if (!quiet && last_disc_evs < disc_evs) {
					fprintf(stderr,
						"packet %" PRIu64 " has %" PRIu64 " lost events\n",
						seq_num, disc_evs - last_disc_evs);
				}
				last_disc_evs = disc_evs;
			}
			last_seq_num = seq_num;
			count++;
		}
	}
	if (rc < 0) {
		fprintf(stderr, "read error: %s\n", gen.last_error(gen.self));
	}
	fprintf(stderr, "%" PRIu64 " events decoded\n", count);
	if (last_disc_evs) {
		fprintf(stderr, "%" PRIu64 " events discarded\n", last_disc_evs);
	}

	actf_printer_free(p);
	return rc;
}

int main(int argc, char *argv[])
{
	struct flags flags = { 0 };
	parse_flags(argc, argv, &flags);

	struct actf_freader_cfg cfg = { 0 };
	actf_freader *rd = actf_freader_init(cfg);
	if (!rd) {
		fprintf(stderr, "actf_freader_init: %s\n", strerror(errno));
		return ACTF_OOM;
	}
	if (actf_freader_open_folders(rd, flags.ctf_paths, flags.ctf_paths_len) < 0) {
		fprintf(stderr, "actf_freader_open_folder: %s\n", actf_freader_last_error(rd));
		actf_freader_free(rd);
		return ACTF_ERROR;
	}
	struct actf_event_generator gen = actf_freader_to_generator(rd);

	actf_filter *flt = NULL;
	if (flags.has_filter_range) {
		flt = actf_filter_init(gen, flags.filter_range);
		if (!flt) {
			fprintf(stderr, "actf_filter_init: %s\n", strerror(errno));
			actf_freader_free(rd);
			return ACTF_OOM;
		}
		gen = actf_filter_to_generator(flt);
	}

	int rc = read_events(gen, flags.quiet, flags.printer_flags);

	actf_filter_free(flt);
	actf_freader_free(rd);
	return rc;
}
