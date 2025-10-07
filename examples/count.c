/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2025  Adam Wendelin <adwe live se>
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

/* count is a simple program that counts the number of events in one
 * or more CTF traces by name. */

#include <stdio.h>

/* Include actf, this is the only header that must be included. */
#include <actf/actf.h>

/* Use a header-only hash map from the project. */
#include "../crust/strtoidx.h"


static void count_event(actf_event *ev, struct strtoidx *nametocnt)
{
	const actf_event_cls *evc = actf_event_event_cls(ev);
	const char *name = actf_event_cls_name(evc);
	if (!name) {
		name = "UNNAMED EVENT";
	}
	size_t *cnt = strtoidx_find(nametocnt, (char *) name);
	if (cnt) {
		*cnt += 1;
	} else {
		strtoidx_insert(nametocnt, strdup(name), 1);
	}
}

int main(int argc, char *argv[])
{
	int rc = 1;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [CTF_PATH(s)]\n", argv[0]);
		return rc;
	}

	/* Initialize a CTF-FS reader and open the provided folders with
	 * it. */
	struct actf_freader_cfg cfg = { 0 };
	actf_freader *rd = actf_freader_init(cfg);
	if (!rd) {
		perror("actf_freader_init");
		return rc;
	}
	if (actf_freader_open_folders(rd, argv + 1, argc - 1) < 0) {
		fprintf(stderr, "actf_freader_open_folder: %s\n", actf_freader_last_error(rd));
		goto err;
	}

	/* Initialize a char * to size_t hash map. */
	struct strtoidx nametocnt;
	rc = strtoidx_init_cap(&nametocnt, 16);
	if (rc < 0) {
		goto err;
	}

	/* Read all events and count them. */
	size_t evs_len = 0;
	actf_event **evs = NULL;
	while ((rc = actf_freader_read(rd, &evs, &evs_len)) == 0 && evs_len) {
		for (size_t i = 0; i < evs_len; i++) {
			count_event(evs[i], &nametocnt);
		}
	}
	if (rc < 0) {
		fprintf(stderr, "actf_freader_read: %s\n", actf_freader_last_error(rd));
	}

	/* Loop through all keys and values in the hash map and print
	 * them. */
	char *name;
	size_t cnt;
	struct strtoidx_it it = { 0 };
	while (strtoidx_foreach(&nametocnt, &name, &cnt, &it) == 0) {
		printf("%s: %zu\n", name, cnt);
	}

	strtoidx_free(&nametocnt);
err:
	actf_freader_free(rd);
	return rc;
}
