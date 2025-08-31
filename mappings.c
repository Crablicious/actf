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
#include <stdlib.h>
#include <stddef.h>

#include "mappings_int.h"
#include "types.h"
#include "crust/uival.h"
#include "crust/sival.h"
#include "crust/common.h"


/* Slightly copy-pasty, but this is loopy so want to avoid branches if
 * necessary. Maybe can macro-hack it into a single definition.
 */

size_t mappings_len(const struct mappings *maps)
{
    return maps->sign == RNG_SINT ? maps->d.smaps.len : maps->d.umaps.len;
}

const char *mappings_find_first_uint(const struct mappings *maps, uint64_t val)
{
    if (maps->sign == RNG_SINT) {
	if (val > (uint64_t)INT64_MAX) {
	    return NULL;
	} else {
	    return smappings_find_first(&maps->d.smaps, (int64_t)val);
	}
    } else {
	return umappings_find_first(&maps->d.umaps, val);
    }
}

const char *mappings_find_uint(const struct mappings *maps, uint64_t val, size_t *it)
{
    if (maps->sign == RNG_SINT) {
	if (val > (uint64_t)INT64_MAX) {
	    return NULL;
	} else {
	    return smappings_find(&maps->d.smaps, (int64_t)val, it);
	}
    } else {
	return umappings_find(&maps->d.umaps, val, it);
    }
}

const char *mappings_find_first_sint(const struct mappings *maps, int64_t val)
{
    if (maps->sign == RNG_SINT) {
	return smappings_find_first(&maps->d.smaps, val);
    } else {
	if (val < 0) {
	    return NULL;
	} else {
	    return umappings_find_first(&maps->d.umaps, (uint64_t)val);
	}
    }
}

const char *mappings_find_sint(const struct mappings *maps, int64_t val, size_t *it)
{
    if (maps->sign == RNG_SINT) {
	return smappings_find(&maps->d.smaps, val, it);
    } else {
	if (val < 0) {
	    return NULL;
	} else {
	    return umappings_find(&maps->d.umaps, (uint64_t)val, it);
	}
    }
}

const char *umappings_find_first(const struct umappings *maps, uint64_t val)
{
    size_t it = MAPPINGS_IT_BEGIN;
    return umappings_find(maps, val, &it);
}

const char *umappings_find(const struct umappings *maps, uint64_t val, size_t *it)
{
    for (size_t i = *it; i < maps->len; i++) {
	struct urng_set *urs = &maps->rng_sets[i];
	if (urng_set_intersect(urs, val)) {
	    *it = i + 1;
	    return maps->names[i];
	}
    }
    return NULL;
}

const char *smappings_find_first(const struct smappings *maps, int64_t val)
{
    size_t it = MAPPINGS_IT_BEGIN;
    return smappings_find(maps, val, &it);
}

const char *smappings_find(const struct smappings *maps, int64_t val, size_t *it)
{
    for (size_t i = *it; i < maps->len; i++) {
	struct srng_set *srs = &maps->rng_sets[i];
	if (srng_set_intersect(srs, val)) {
	    *it = i + 1;
	    return maps->names[i];
	}
    }
    return NULL;
}

void umappings_free(struct umappings *maps)
{
    if (! maps) {
	return;
    }
    for (size_t i = 0; i < maps->len; i++) {
	free(maps->names[i]);
	urng_set_free(&maps->rng_sets[i]);
    }
    free(maps->rng_sets);
    free(maps->names);
}

void smappings_free(struct smappings *maps)
{
    if (! maps) {
	return;
    }
    for (size_t i = 0; i < maps->len; i++) {
	free(maps->names[i]);
	srng_set_free(&maps->rng_sets[i]);
    }
    free(maps->rng_sets);
    free(maps->names);
}

void mappings_free(struct mappings *maps)
{
    if (! maps) {
	return;
    }
    if (maps->sign == RNG_SINT) {
	smappings_free(&maps->d.smaps);
    } else {
	umappings_free(&maps->d.umaps);
    }
}

size_t actf_mappings_len(const actf_mappings *maps)
{
    return maps->names_len;
}

const char *actf_mappings_find_first_uint(const actf_mappings *maps, uint64_t val)
{
    actf_it it = {0};
    return actf_mappings_find_uint(maps, val, &it);
}
const char *actf_mappings_find_uint(const actf_mappings *maps, uint64_t val, actf_it *it)
{
    struct uival *match = uivalt_intersect_pt(&maps->ivt, val, it->data);
    it->data = match;
    return match ? match->value : NULL;
}
const char *actf_mappings_find_first_sint(const actf_mappings *maps, int64_t val)
{
    actf_it it = {0};
    return actf_mappings_find_sint(maps, val, &it);
}
const char *actf_mappings_find_sint(const actf_mappings *maps, int64_t val, actf_it *it)
{
    struct sival *match = sivalt_intersect_pt(&maps->ivt, val, it->data);
    it->data = match;
    return match ? match->value : NULL;
}

static int urng_cmp(const void *p1, const void *p2)
{
    const struct urng *r1 = p1;
    const struct urng *r2 = p2;
    return r1->lower >= r2->lower ? (int)(r1->lower - r2->lower) : -1;
}

static int srng_cmp(const void *p1, const void *p2)
{
    const struct srng *r1 = p1;
    const struct srng *r2 = p2;
    return r1->lower - r2->lower;
}

/* Mutates maps (sorts ranges in range sets) */
int init_uivalt(struct umappings *maps, struct uival **ivs, size_t *ivs_len,
		struct rb_tree *ivt)
{
    size_t ivals_cap = 0;
    for (size_t i = 0; i < maps->len; i++) {
	ivals_cap += maps->rng_sets[i].len;
    }
    struct uival *ivals = malloc(ivals_cap * sizeof(*ivals));
    if (! ivals) {
	return ACTF_OOM;
    }

    size_t ivals_len = 0;
    for (size_t i = 0; i < maps->len; i++) {
	struct urng_set *rs = &maps->rng_sets[i];
	qsort(rs->rngs, rs->len, sizeof(*rs->rngs), urng_cmp);
	struct urng top;
	for (size_t j = 0; j < rs->len; j++) {
	    if (j == 0) {
		top = rs->rngs[j];
		continue;
	    }
	    if (urng_intersect_urng(&rs->rngs[j], &top)) {
		top.upper = MAX(rs->rngs[j].upper, top.upper);
	    } else {
		ivals[ivals_len++] = (struct uival){
		    .lower = top.lower, .upper = top.upper, .value = maps->names[i]};
		top = rs->rngs[j];
	    }
	}
	if (rs->len > 0) {
	    ivals[ivals_len++] = (struct uival){
		.lower = top.lower, .upper = top.upper, .value = maps->names[i]};
	}
    }

    int rc = rb_tree_init(ivt);
    if (rc < 0) {
	free(ivals);
	return rc == -ENOMEM ? ACTF_OOM : ACTF_ERROR;
    }
    for (size_t i = 0; i < ivals_len; i++) {
	uivalt_insert(ivt, &ivals[i]);
    }
    *ivs = ivals;
    *ivs_len = ivals_len;
    return ACTF_OK;
}

/* Mutates maps (sorts ranges in range sets) */
int init_sivalt(struct smappings *maps, struct sival **ivs, size_t *ivs_len,
		struct rb_tree *ivt)
{
    size_t ivals_cap = 0;
    for (size_t i = 0; i < maps->len; i++) {
	ivals_cap += maps->rng_sets[i].len;
    }
    struct sival *ivals = malloc(ivals_cap * sizeof(*ivals));
    if (! ivals) {
	return ACTF_OOM;
    }

    size_t ivals_len = 0;
    for (size_t i = 0; i < maps->len; i++) {
	struct srng_set *rs = &maps->rng_sets[i];
	qsort(rs->rngs, rs->len, sizeof(*rs->rngs), srng_cmp);
	struct srng top;
	for (size_t j = 0; j < rs->len; j++) {
	    if (j == 0) {
		top = rs->rngs[j];
		continue;
	    }
	    if (srng_intersect_srng(&rs->rngs[j], &top)) {
		top.upper = MAX(rs->rngs[j].upper, top.upper);
	    } else {
		ivals[ivals_len++] = (struct sival){
		    .lower = top.lower, .upper = top.upper, .value = maps->names[i]};
		top = rs->rngs[j];
	    }
	}
	if (rs->len > 0) {
	    ivals[ivals_len++] = (struct sival){
		.lower = top.lower, .upper = top.upper, .value = maps->names[i]};
	}
    }

    int rc = rb_tree_init(ivt);
    if (rc < 0) {
	free(ivals);
	return rc == -ENOMEM ? ACTF_OOM : ACTF_ERROR;
    }
    for (size_t i = 0; i < ivals_len; i++) {
	sivalt_insert(ivt, &ivals[i]);
    }
    *ivs = ivals;
    *ivs_len = ivals_len;
    return ACTF_OK;
}

int actf_mappings_init(struct mappings *raw_maps, struct actf_mappings *maps)
{
    int rc;
    maps->sign = raw_maps->sign;
    if (maps->sign == RNG_SINT) {
	rc = init_sivalt(&raw_maps->d.smaps, &maps->ivals.s,
			 &maps->ivals_len, &maps->ivt);
	if (rc < 0) {
	    return rc;
	}
	maps->names = raw_maps->d.smaps.names;
	maps->names_len = raw_maps->d.smaps.len;

	for (size_t i = 0; i < raw_maps->d.smaps.len; i++) {
	    srng_set_free(&raw_maps->d.smaps.rng_sets[i]);
	}
	free(raw_maps->d.smaps.rng_sets);
    } else {
	rc = init_uivalt(&raw_maps->d.umaps, &maps->ivals.u,
			 &maps->ivals_len, &maps->ivt);
	if (rc < 0) {
	    return rc;
	}
	maps->names = raw_maps->d.umaps.names;
	maps->names_len = raw_maps->d.umaps.len;

	for (size_t i = 0; i < raw_maps->d.umaps.len; i++) {
	    urng_set_free(&raw_maps->d.umaps.rng_sets[i]);
	}
	free(raw_maps->d.umaps.rng_sets);
    }
    return ACTF_OK;
}

void actf_mappings_free(struct actf_mappings *maps)
{
    if (! maps) {
	return;
    }
    if (maps->sign == RNG_SINT) {
	free(maps->ivals.s);
    } else {
	free(maps->ivals.u);
    }
    rb_tree_free(&maps->ivt);
    for (size_t i = 0; i < maps->names_len; i++) {
	free(maps->names[i]);
    }
    free(maps->names);
}
