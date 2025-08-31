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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "decoder.h"
#include "metadata.h"
#include "muxer.h"
#include "freader.h"
#include "error.h"

struct mmap_s {
    void *pa;
    size_t len;
};

struct muxer_s {
    actf_muxer *muxer;
    struct actf_event_generator *gens;
    size_t gens_len;
};

struct ctf_dir {
    char *path;
    actf_metadata *metadata;
    struct mmap_s *mmaps;
    size_t mmaps_len;
    actf_decoder **decs;
    size_t decs_len;
};

struct actf_freader {
    struct actf_freader_cfg cfg;
    struct ctf_dir *dirs;
    size_t dirs_len;
    struct muxer_s mux;
    struct actf_event_generator active_gen;
    struct error err;
};


/* openat_metadata tries to read a metadata file relative to the
 * directory fd. */
static actf_metadata *openat_metadata(int fd, const char *metadata_filename, struct error *e)
{
    int mfd = openat(fd, metadata_filename, O_RDONLY);
    if (mfd < 0) {
	eprintf(e, "%s openat: %s", metadata_filename, strerror(errno));
	return NULL;
    }
    actf_metadata *m = actf_metadata_init();
    if (! m) {
	close(mfd);
	eprintf(e, "actf_metadata_init: %s", strerror(errno));
	return NULL;
    }
    int rc = actf_metadata_parse_fd(m, mfd);
    if (rc < 0) {
	eprintf(e, actf_metadata_last_error(m));
	actf_metadata_free(m);
	close(mfd);
	return NULL;
    }
    close(mfd);
    return m;
}

/* mmap_datastreams mmaps all regular files in dir. Any file matching
 * `metadata_filename` will not be mmapped. */
static int mmap_datastreams(DIR *dir, const char *metadata_filename,
			    struct mmap_s **mmaps_out, size_t *mmaps_len_out,
			    struct error *e)
{
    int rc;
    struct dirent *dp;
    size_t n_entries = 0;
    while ((dp = readdir(dir)) != NULL) {
	n_entries++; // Allow the waste
    }
    rewinddir(dir);

    size_t mmaps_len = 0;
    struct mmap_s *mmaps = malloc(n_entries * sizeof(*mmaps));
    if (! mmaps) {
	eprintf(e, "malloc: %s", strerror(errno));
	return ACTF_OOM;
    }

    int fd = dirfd(dir);
    if (fd < 0) {
	free(mmaps);
	eprintf(e, "dirfd: %s", strerror(errno));
	return ACTF_ERROR;
    }
    struct stat sb;
    while ((dp = readdir(dir)) != NULL) {
	if (strcmp(dp->d_name, metadata_filename) == 0 || dp->d_name[0] == '.') {
	    continue;
	}
	int dstream_fd = openat(fd, dp->d_name, O_RDONLY);
	if (dstream_fd < 0) {
	    eprintf(e, "openat: %s", strerror(errno));
	    rc = ACTF_ERROR;
	    goto err;
	}
	if (fstat(dstream_fd, &sb) < 0) {
	    close(dstream_fd);
	    eprintf(e, "fstat: %s", strerror(errno));
	    rc = ACTF_ERROR;
	    goto err;
	}
	if (! S_ISREG(sb.st_mode) || sb.st_size == 0) {
	    close(dstream_fd);
	    continue;
	}
	void *pa = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, dstream_fd, 0);
	if (pa == MAP_FAILED) {
	    close(dstream_fd);
	    eprintf(e, "mmap: %s", strerror(errno));
	    rc = ACTF_ERROR;
	    goto err;
	}
	close(dstream_fd);
	mmaps[mmaps_len++] = (struct mmap_s) {
	    .pa = pa,
	    .len = sb.st_size,
	};
    }

    *mmaps_out = mmaps;
    *mmaps_len_out = mmaps_len;
    return ACTF_OK;

err:
    for (size_t i = 0; i < mmaps_len; i++) {
	munmap(mmaps[i].pa, mmaps[i].len);
    }
    free(mmaps);
    return rc;
}

int init_decoders(struct mmap_s *mmaps, size_t mmaps_len,
		  actf_metadata *m, size_t evs_cap, actf_decoder **decs,
		  struct error *e)
{
    int rc = ACTF_ERROR;
    size_t i;
    for (i = 0; i < mmaps_len; i++) {
	actf_decoder *d = actf_decoder_init(mmaps[i].pa, mmaps[i].len, evs_cap, m);
	if (! d) {
	    eprintf(e, "actf_decoder_init: %s", strerror(errno));
	    rc = ACTF_ERROR;
	    goto err;
	}
	decs[i] = d;
    }
    return ACTF_OK;

err:
    for (size_t j = 0; j < i; j++) {
	actf_decoder_free(decs[j]);
    }
    return rc;
}

actf_freader *actf_freader_init(struct actf_freader_cfg cfg)
{
    struct actf_freader *rd = calloc(1, sizeof(*rd));
    if (! rd) {
	return NULL;
    }
    if (cfg.metadata_filename == NULL) {
	cfg.metadata_filename = "metadata";
    }
    rd->cfg = cfg;
    rd->err = ERROR_EMPTY;
    return rd;
}

static int init_muxer(struct ctf_dir *dirs, size_t dirs_len, size_t evs_cap,
		      struct muxer_s *mux, struct error *e)
{
    size_t gens_len = 0;
    for (size_t i = 0; i < dirs_len; i++) {
	gens_len += dirs[i].decs_len;
    }
    struct actf_event_generator *gens = malloc(gens_len * sizeof(*gens));
    if (! gens) {
	eprintf(e, "malloc: %s", strerror(errno));
	return ACTF_OOM;
    }
    for (size_t gens_i = 0, i = 0; i < dirs_len; i++) {
	for (size_t j = 0; j < dirs[i].decs_len; j++) {
	    gens[gens_i++] = actf_decoder_to_generator(dirs[i].decs[j]);
	}
    }
    actf_muxer *m = actf_muxer_init(gens, gens_len, evs_cap);
    if (! m) {
	free(gens);
	eprintf(e, "actf_muxer_init: %s", strerror(errno));
	return ACTF_ERROR;
    }
    *mux = (struct muxer_s) {
	.muxer = m,
	.gens = gens,
	.gens_len = gens_len,
    };
    return ACTF_OK;
}

/* open_ctf_dir opens a ctf directory at path. The metadata file is
 * read, all data stream files are mmapped, and a decoder is setup for
 * each data stream file. */
static int open_ctf_dir(const char *path, struct actf_freader_cfg *cfg,
			struct ctf_dir *cd, struct error *e)
{
    int rc = ACTF_ERROR;
    DIR *dir = opendir(path);
    if (dir == NULL) {
	eprintf(e, "ctf folder opendir: %s", strerror(errno));
	return ACTF_ERROR;
    }
    int fd = dirfd(dir);
    if (fd < 0) {
	eprintf(e, "dirfd: %s", strerror(errno));
	rc = ACTF_ERROR;
	goto err;
    }

    actf_metadata *m = openat_metadata(fd, cfg->metadata_filename, e);
    if (! m) {
	rc = ACTF_ERROR;
	goto err;
    }

    size_t mmaps_len;
    struct mmap_s *mmaps;
    rc = mmap_datastreams(dir, cfg->metadata_filename, &mmaps, &mmaps_len, e);
    if (rc < 0) {
	goto err_mmap;
    }

    actf_decoder **decs = malloc(mmaps_len * sizeof(*decs));
    if (! decs) {
	eprintf(e, "malloc: %s", strerror(errno));
	rc = ACTF_OOM;
	goto err_decs;
    }

    rc = init_decoders(mmaps, mmaps_len, m, cfg->dstream_evs_cap, decs, e);
    if (rc < 0) {
	goto err_decs_init;
    }

    *cd = (struct ctf_dir){
	.path = strdup(path),
	.metadata = m,
	.mmaps = mmaps,
	.mmaps_len = mmaps_len,
	.decs = decs,
	.decs_len = mmaps_len,
    };

    closedir(dir);
    return ACTF_OK;

err_decs_init:
    free(decs);
err_decs:
    for (size_t i = 0; i < mmaps_len; i++) {
	munmap(mmaps[i].pa, mmaps[i].len);
    }
    free(mmaps);
err_mmap:
    actf_metadata_free(m);
err:
    closedir(dir);
    return rc;
}

static void ctf_dir_free(struct ctf_dir *cd)
{
    if (! cd) {
	return;
    }
    for (size_t i = 0; i < cd->decs_len; i++) {
	actf_decoder_free(cd->decs[i]);
    }
    free(cd->decs);
    for (size_t i = 0; i < cd->mmaps_len; i++) {
	munmap(cd->mmaps[i].pa, cd->mmaps[i].len);
    }
    free(cd->mmaps);
    actf_metadata_free(cd->metadata);
    free(cd->path);
}

int actf_freader_open_folders(actf_freader *rd, char **paths, size_t len)
{
    if (! len) {
	return ACTF_OK;
    }
    int rc;
    size_t i;
    struct error *e = &rd->err;

    struct ctf_dir *dirs = malloc(len * sizeof(*dirs));
    if (! dirs) {
	eprintf(e, "malloc: %s", strerror(errno));
	return ACTF_OOM;
    }
    /* Initialize all data stream files */
    size_t total_decs = 0;
    for (i = 0; i < len; i++) {
	rc = open_ctf_dir(paths[i], &rd->cfg, dirs + i, e);
	if (rc < 0) {
	    goto err;
	}
	total_decs += dirs[i].decs_len;
    }

    /* Put all data stream files in a muxer if there are more than one
     * data stream file in total, otherwise use the decoder directly
     * to avoid a redundant layer of buffering. */
    struct muxer_s mux = {0};
    struct actf_event_generator active_gen = {0};
    if (total_decs == 1) {
	actf_decoder *active_dec = NULL;
	for (i = 0; i < len; i++) {
	    if (dirs[i].decs_len == 1) {
		active_dec = dirs[i].decs[0];
	    }
	}
	active_gen = actf_decoder_to_generator(active_dec);
    } else if (total_decs > 0) {
	rc = init_muxer(dirs, len, rd->cfg.muxer_evs_cap,
			&mux, e);
	if (rc < 0) {
	    goto err;
	}
	active_gen = actf_muxer_to_generator(mux.muxer);
    }

    rd->dirs = dirs;
    rd->dirs_len = len;
    rd->mux = mux;
    rd->active_gen = active_gen;

    return ACTF_OK;

err:
    for (size_t j = 0; j < i; j++) {
	ctf_dir_free(&dirs[j]);
    }
    free(dirs);
    return rc;
}

int actf_freader_open_folder(actf_freader *rd, char *path)
{
    char *paths[] = {path};
    return actf_freader_open_folders(rd, paths, 1);
}

int actf_freader_read(actf_freader *rd, actf_event ***evs, size_t *evs_len)
{
    if (! rd->active_gen.generate) {
	*evs_len = 0;
	return ACTF_OK;
    }
    int rc = rd->active_gen.generate(rd->active_gen.self, evs, evs_len);
    if (rc < 0) {
	const char *msg = rd->active_gen.last_error(rd->active_gen.self);
	eprintf(&rd->err, msg);
    }
    return rc;
}

static int freader_read(void *self, actf_event ***evs, size_t *evs_len)
{
    actf_freader *rd = self;
    return actf_freader_read(rd, evs, evs_len);
}

int actf_freader_seek_ns_from_origin(actf_freader *rd, int64_t tstamp)
{
    if (! rd->active_gen.generate) {
	return ACTF_OK;
    }
    int rc = rd->active_gen.seek_ns_from_origin(rd->active_gen.self, tstamp);
    if (rc < 0) {
	const char *msg = rd->active_gen.last_error(rd->active_gen.self);
	eprintf(&rd->err, msg ? msg : "unknown seek_ns_from_origin error");
    }
    return rc;
}

static int freader_seek_ns_from_origin(void *self, int64_t tstamp)
{
    actf_freader *rd = self;
    return actf_freader_seek_ns_from_origin(rd, tstamp);
}

const char *actf_freader_last_error(actf_freader *rd)
{
    if (! rd || rd->err.buf[0] == '\0') {
	return NULL;
    }
    return rd->err.buf;
}

/* a actf_last_error */
static const char *freader_last_error(void *self)
{
    actf_freader *rd = self;
    return actf_freader_last_error(rd);
}

void actf_freader_free(actf_freader *rd)
{
    if (! rd) {
	return;
    }
    actf_muxer_free(rd->mux.muxer);
    free(rd->mux.gens);
    for (size_t i = 0; i < rd->dirs_len; i++) {
	ctf_dir_free(&rd->dirs[i]);
    }
    free(rd->dirs);
    error_free(&rd->err);
    free(rd);
}

struct actf_event_generator actf_freader_to_generator(actf_freader *rd)
{
    return (struct actf_event_generator) {
	.generate = freader_read,
	.seek_ns_from_origin = freader_seek_ns_from_origin,
	.last_error = freader_last_error,
	.self = rd,
    };
}
