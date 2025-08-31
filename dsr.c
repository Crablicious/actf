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
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "decoder.h"
#include "metadata.h"
#include "event_generator.h"
#include "print.h"


void print_usage(void)
{
    fprintf(stderr, "Usage: dsr [-q] METADATA_PATH DS_PATH\n"
	    " The options are:\n"
	    "  -s <ns>      Seek to this ns offset\n"
	    "  -q           Quiet, do not print events\n");
}

struct flags {
    bool quiet;
    uint64_t seek_off;
    char *metadata_path;
    char *ds_path;
};

/* set_flags populates f or fails fatally. */
void set_flags(int argc, char *argv[], struct flags *f)
{
    int opt;
    while ((opt = getopt(argc, argv, "s:qh")) != -1) {
	switch (opt) {
	case 's':
	    errno = 0;
	    f->seek_off = (uint64_t)strtoull(optarg, NULL, 0);
	    if (errno != 0) {
		perror("strtoul of seek offset");
		exit(-1);
	    }
	    break;
	case 'q':
	    f->quiet = true;
	    break;
	case 'h':
	    print_usage();
	    exit(0);
	default: /* '?' */
	    print_usage();
	    exit(-1);
	}
    }

    if ((argc - optind) != 2) {
	fprintf(stderr, "Expected 2 positional arguments\n");
	print_usage();
	exit(-1);
    }
    f->metadata_path = argv[optind++];
    f->ds_path = argv[optind++];
}

int mmap_file(const char *path, void **pa_out, size_t *len_out)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "failed to open data stream at %s: %s\n",
		path, strerror(errno));
	return -1;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
	fprintf(stderr, "failed to stat %s: %s\n", path, strerror(errno));
	close(fd);
	return -1;
    }
    if (st.st_size == 0) {
	*pa_out = NULL;
	*len_out = 0;
	close(fd);
	return 0;
    }
    void *pa = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (pa == MAP_FAILED) {
	fprintf(stderr, "failed to mmap %s: %s\n", path, strerror(errno));
	close(fd);
	return -1;
    }
    close(fd);
    *pa_out = pa;
    *len_out = st.st_size;
    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    struct flags flags = {0};
    set_flags(argc, argv, &flags);

    struct actf_metadata *metadata = actf_metadata_init();
    if (! metadata) {
	rc = -1;
	goto err;
    }
    rc = actf_metadata_parse_file(metadata, flags.metadata_path);
    if (rc < 0) {
	fprintf(stderr, "Failed to read metadata: %s\n", actf_metadata_last_error(metadata));
	rc = -1;
	goto err;
    }

    void *pa;
    size_t pa_len;
    rc = mmap_file(flags.ds_path, &pa, &pa_len);
    if (rc < 0) {
	rc = -1;
	goto err;
    }

    struct actf_decoder *dec = actf_decoder_init(pa, pa_len, 0, metadata);
    if (! dec) {
	fprintf(stderr, "Failed to create actf_decoder\n");
	rc = -1;
	goto err_munmap;
    }

    uint64_t count = 0;
    actf_printer *p = actf_printer_init(ACTF_PRINT_ALL | ACTF_PRINT_PROP_LABELS);
    if (! p) {
	rc = -1;
	goto err_decoder;
    }

    if (flags.seek_off) {
	rc = actf_decoder_seek_ns_from_origin(dec, flags.seek_off);
	if (rc < 0) {
	    fprintf(stderr, "Failed to seek: %s\n", actf_decoder_last_error(dec));
	    goto err_printer;
	}
    }

    actf_event **evs = NULL;
    size_t evs_len = 0;
    while ((rc = actf_decoder_decode(dec, &evs, &evs_len)) == 0 && evs_len) {
	for (size_t i = 0; i < evs_len; i++) {
	    if (! flags.quiet) {
		actf_print_event(p, evs[i]);
		printf("\n");
	    }
	    count++;
	}
    }
    if (rc < 0) {
	fprintf(stderr, "Error decoding data stream: %s\n", actf_decoder_last_error(dec));
	rc = -1;
    }
    printf("%" PRIu64 " events decoded\n", count);

err_printer:
    actf_printer_free(p);
err_decoder:
    actf_decoder_free(dec);
err_munmap:
    if (pa && munmap(pa, pa_len) < 0) {
	perror("failed to munmap datastream");
    }
err:
    actf_metadata_free(metadata);
    return rc;
}
