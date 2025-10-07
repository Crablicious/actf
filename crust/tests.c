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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gvec.h"
#include "int_vec.h"
#include "uival.h"
#include "sival.h"
#include "u64tosize.h"
#include "strtoidx.h"
#include "arena.h"


static int init_suite_crust(void)
{
	return 0;
}

static int clean_suite_crust(void)
{
	return 0;
}

static void test_gvec(void)
{
	struct gvec v = gvec_init(12, sizeof(int));
	int a = 13;
	int b = 37;
	int c = 42;
	/* Test basic pushing and popping */
	gvec_push(v, a);
	gvec_push(v, b);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 0), a);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 1), b);
	CU_ASSERT_EQUAL(v.len, 2);

	gvec_pop(v);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 0), a);
	CU_ASSERT_EQUAL(v.len, 1);

	gvec_pop(v);
	CU_ASSERT_EQUAL(v.len, 0);

	/* Test index popping */
	gvec_push(v, a);
	gvec_push(v, b);
	gvec_push(v, c);
	CU_ASSERT_EQUAL(v.len, 3);

	gvec_pop_idx(v, 1);
	CU_ASSERT_EQUAL(v.len, 2);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 0), a);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 1), c);

	gvec_pop_idx(v, 1);
	CU_ASSERT_EQUAL(v.len, 1);
	CU_ASSERT_EQUAL(gvec_idx(v, int, 0), a);

	gvec_free(v);
}

static void test_int_vec(void)
{
	struct int_vec v = int_vec_init(12);
	int a = 13;
	int b = 37;
	int c = 42;
	/* Test basic pushing and popping */
	int_vec_push(&v, a);
	int_vec_push(&v, b);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 0), a);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 1), b);
	CU_ASSERT_EQUAL(v.len, 2);

	int_vec_pop(&v);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 0), a);
	CU_ASSERT_EQUAL(v.len, 1);

	int_vec_pop(&v);
	CU_ASSERT_EQUAL(v.len, 0);

	/* Test index popping */
	int_vec_push(&v, a);
	int_vec_push(&v, b);
	int_vec_push(&v, c);
	CU_ASSERT_EQUAL(v.len, 3);

	int_vec_pop_idx(&v, 1);
	CU_ASSERT_EQUAL(v.len, 2);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 0), a);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 1), c);

	int_vec_pop_idx(&v, 1);
	CU_ASSERT_EQUAL(v.len, 1);
	CU_ASSERT_EQUAL(*int_vec_idx(&v, 0), a);

	int_vec_free(&v);
}

/* verify that a node's children have it set as the parent. */
static void verify_sane_parenthood(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return;
	}
	if (n->c[RB_LEFT] != ivt->nil) {
		CU_ASSERT_FATAL(RB_P(n->c[RB_LEFT]) == n);
		verify_sane_parenthood(ivt, n->c[RB_LEFT]);
	}
	if (n->c[RB_RIGHT] != ivt->nil) {
		CU_ASSERT_FATAL(RB_P(n->c[RB_RIGHT]) == n);
		verify_sane_parenthood(ivt, n->c[RB_RIGHT]);
	}
}

/* verify that we do not have two adjacent red nodes. */
static void verify_no_red_with_red_parent(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return;
	}
	if (RB_COLOR(n) == RB_RED) {
		if (n->c[RB_LEFT] == ivt->nil) {
			CU_ASSERT_FATAL(RB_COLOR(n->c[RB_LEFT]) != RB_RED);
		}
		if (n->c[RB_RIGHT] == ivt->nil) {
			CU_ASSERT_FATAL(RB_COLOR(n->c[RB_RIGHT]) != RB_RED);
		}
	}
	verify_no_red_with_red_parent(ivt, n->c[RB_LEFT]);
	verify_no_red_with_red_parent(ivt, n->c[RB_RIGHT]);
}

/* verify that the distance from n to the leafs have the same amount
 * of black nodes for all children. */
static int verify_equal_black_children(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return 1;
	}
	int l = verify_equal_black_children(ivt, n->c[RB_LEFT]);
	int r = verify_equal_black_children(ivt, n->c[RB_RIGHT]);
	CU_ASSERT_FATAL(l == r);
	return l + (RB_COLOR(n) == RB_BLACK);
}

/* verify that the tree is sorted by lower of uival. */
static void verify_sort_invariant(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return;
	}
	if (n->c[RB_LEFT] != ivt->nil) {
		struct uival *n_ival = uival_unpack(n);
		struct uival *l_ival = uival_unpack(n->c[RB_LEFT]);
		CU_ASSERT_FATAL(n_ival->lower >= l_ival->lower);
		verify_sort_invariant(ivt, n->c[RB_LEFT]);
	}
	if (n->c[RB_RIGHT] != ivt->nil) {
		struct uival *n_ival = uival_unpack(n);
		struct uival *r_ival = uival_unpack(n->c[RB_RIGHT]);
		CU_ASSERT_FATAL(n_ival->lower <= r_ival->lower);
		verify_sort_invariant(ivt, n->c[RB_RIGHT]);
	}
}

static uint64_t calc_max_upper(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return 0;
	}
	struct uival *n_ival = uival_unpack(n);
	return IVAL_MAX(n_ival->upper, IVAL_MAX(calc_max_upper(ivt, n->c[RB_LEFT]), calc_max_upper(ivt, n->c[RB_RIGHT])));
}

/* verify that link max is consistent across the tree. */
static void verify_link_max(struct rb_tree *ivt, struct rb_node *n)
{
	if (n == ivt->nil) {
		return;
	}
	uint64_t expected = calc_max_upper(ivt, n);
	struct uival *n_ival = uival_unpack(n);
	CU_ASSERT_FATAL(n_ival->link_max == expected);
	verify_link_max(ivt, n->c[RB_LEFT]);
	verify_link_max(ivt, n->c[RB_RIGHT]);
}

static void verify_uivalt(struct rb_tree *ivt)
{
	if (ivt->root == ivt->nil) {
		return;
	}
	CU_ASSERT_FATAL(RB_COLOR(ivt->root) == RB_BLACK);
	verify_sane_parenthood(ivt, ivt->root);
	verify_no_red_with_red_parent(ivt, ivt->root);
	verify_equal_black_children(ivt, ivt->root);
	verify_sort_invariant(ivt, ivt->root);
	verify_link_max(ivt, ivt->root);
}

static void test_uival(void)
{
	struct rb_tree ivt;
	rb_tree_init(&ivt);
	struct uival uivals[] = {
		{.lower = 3, .upper = 5},
		{.lower = 21, .upper = 21},
		{.lower = 32, .upper = 40},
	};
	for (size_t i = 0; i < sizeof(uivals)/sizeof(*uivals); i++) {
		uivalt_insert(&ivt, &uivals[i]);
		verify_uivalt(&ivt);
	}

	/* Delete middle interval */
	uivalt_delete(&ivt, &uivals[1]);
	verify_uivalt(&ivt);
	struct uival tar = {.lower = 4, .upper = 4};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) != NULL);
	tar = (struct uival) {.lower = 21, .upper = 21};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct uival) {.lower = 33, .upper = 33};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) != NULL);
	/* Delete first interval */
	uivalt_delete(&ivt, &uivals[0]);
	verify_uivalt(&ivt);
	tar = (struct uival) {.lower = 4, .upper = 4};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct uival) {.lower = 21, .upper = 21};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct uival) {.lower = 33, .upper = 33};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) != NULL);
	/* Delete last interval */
	uivalt_delete(&ivt, &uivals[2]);
	verify_uivalt(&ivt);
	/* Tree should be empty now */
	tar = (struct uival) {.lower = 4, .upper = 4};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct uival) {.lower = 21, .upper = 21};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct uival) {.lower = 33, .upper = 33};
	CU_ASSERT(uivalt_intersect(&ivt, &tar, NULL) == NULL);
	rb_tree_free(&ivt);
}

static void shuffle(unsigned *arr, size_t len)
{
	// Fisher-yates shuffle
	for (size_t i = 0; i < len - 1; i++) {
		size_t j = i + (rand() % (len - i));
		unsigned tmp = arr[i];
		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

int uival_cmp(const void *p1, const void *p2)
{
	const struct uival *i1 = p1;
	const struct uival *i2 = p2;
	if (i1->lower == i2->lower) {
		if (i1->upper == i2->lower) {
			return 0;
		} else if (i1->upper < i2->upper) {
			return -1;
		} else {
			return 1;
		}
	} else if (i1->lower < i2->lower) {
		return -1;
	} else {
		return 1;
	}
}

static void test_uival_fuzz_int(bool verbose)
{
	/* Generate a random set of intervals which are inserted into an
	 * uivalt and then deleted in random order.
	 *
	 * After every operation the consistency of the tree is checked.
	 */
#define N_IVALS 32
	/* Generate a random set of intervals */
	struct uival ivals[N_IVALS];
	for (size_t i = 0; i < N_IVALS; i++) {
		ivals[i].lower = 1024 + rand();
		ivals[i].upper = ivals[i].lower + (rand() % (RAND_MAX - ivals[i].lower));
		ivals[i].link = RB_EMPTY_NODE;
	}

	/* Generate a random order of deletion. */
	unsigned delete_order[N_IVALS];
	for (size_t i = 0; i < N_IVALS; i++) {
		delete_order[i] = i;
	}
	shuffle(delete_order, N_IVALS);

	struct rb_tree ivt;
	rb_tree_init(&ivt);

	/* Insert all intervals and make sure the tree is kept
	 * conistent.
	 */
	for (size_t i = 0; i < N_IVALS; i++) {
		if (verbose) printf("Inserting interval: [%"PRIu64", %"PRIu64"]\n", ivals[i].lower, ivals[i].upper);
		uivalt_insert(&ivt, &ivals[i]);
		if (verbose) {
			char filename[20];
			sprintf(filename, "tree_ins%zu.dot", i);
			FILE *f = fopen(filename, "w");
			uivalt_fprint(f, &ivt);
			fclose(f);
		}
		verify_uivalt(&ivt);
	}

#define N_QUERIES 1000
	struct uival act_matches[N_IVALS];
	struct uival exp_matches[N_IVALS];
	size_t n_act_matches;
	size_t n_exp_matches;
	for (size_t i = 0; i < N_QUERIES; i++) {
		struct uival query = {0};
		query.lower = rand();
		query.upper = query.lower + (rand() % (RAND_MAX - query.lower));
		n_act_matches = 0;
		struct uival *match = NULL;
		while ((match = uivalt_intersect(&ivt, &query, match))) {
			act_matches[n_act_matches++] = *match;
		}
		n_exp_matches = 0;
		for (size_t j = 0; j < N_IVALS; j++) {
			if (uival_intersects(&ivals[j], &query)) {
				exp_matches[n_exp_matches++] = ivals[j];
			}
		}
		CU_ASSERT_FATAL(n_act_matches == n_exp_matches);
		qsort(exp_matches, n_exp_matches, sizeof(*exp_matches), uival_cmp);
		qsort(act_matches, n_act_matches, sizeof(*act_matches), uival_cmp);
		for (size_t j = 0; j < n_exp_matches; j++) {
			CU_ASSERT_FATAL(exp_matches[j].lower == act_matches[j].lower &&
					exp_matches[j].upper == act_matches[j].upper);
		}
	}

	/* Delete all intervals in a random order and make sure the tree
	 * is kept consistent. */
	for (size_t i = 0; i < N_IVALS; i++) {
		if (verbose) printf("Deleting interval: [%"PRIu64", %"PRIu64"]\n", ivals[delete_order[i]].lower, ivals[delete_order[i]].upper);
		uivalt_delete(&ivt, &ivals[delete_order[i]]);
		if (verbose) {
			char filename[20];
			sprintf(filename, "tree_del%zu.dot", i);
			FILE *f = fopen(filename, "w");
			uivalt_fprint(f, &ivt);
			fclose(f);
		}
		verify_uivalt(&ivt);
	}

	rb_tree_free(&ivt);
}

static void test_uival_fuzz(void)
{
	bool verbose = false;
	for (int i = 0; i < 64; i++) {
		if (verbose) printf("===== SEED: %d ======\n", i);
		srand(i);
		test_uival_fuzz_int(verbose);
	}
}

static void test_uival_multi_intersect(void)
{
	struct rb_tree ivt;
	rb_tree_init(&ivt);
	struct uival uivals[] = {
		{.lower = 20, .upper = 45},
		{.lower = 12, .upper = 34},
		{.lower = 70, .upper = 80},
		{.lower = 30, .upper = 30},
		{.lower = 5, .upper = 10},
	};
	for (size_t i = 0; i < sizeof(uivals)/sizeof(*uivals); i++) {
		uivalt_insert(&ivt, &uivals[i]);
		verify_uivalt(&ivt);
	}
	/* FILE *f = fopen("tree_multi_intersect.dot", "w"); */
	/* uivalt_fprint(f, &ivt); */
	/* fclose(f); */

	struct uival *match = NULL;
	match = uivalt_intersect_pt(&ivt, 30, match);
	CU_ASSERT_PTR_NOT_NULL_FATAL(match);
	match = uivalt_intersect_pt(&ivt, 30, match);
	CU_ASSERT_PTR_NOT_NULL_FATAL(match);
	match = uivalt_intersect_pt(&ivt, 30, match);
	CU_ASSERT_PTR_NOT_NULL_FATAL(match);
	match = uivalt_intersect_pt(&ivt, 30, match);
	CU_ASSERT_PTR_NULL_FATAL(match);

	match = NULL;
	match = uivalt_intersect_pt(&ivt, 70, match);
	CU_ASSERT_PTR_NOT_NULL_FATAL(match);
	match = uivalt_intersect_pt(&ivt, 70, match);
	CU_ASSERT_PTR_NULL_FATAL(match);
	rb_tree_free(&ivt);
}

static void test_sival(void)
{
	struct rb_tree ivt;
	rb_tree_init(&ivt);
	struct sival sivals[] = {
		{.lower = 3, .upper = 5},
		{.lower = 21, .upper = 21},
		{.lower = 32, .upper = 40},
	};
	CU_ASSERT_PTR_NOT_NULL_FATAL(sivals);

	sivalt_insert(&ivt, &sivals[0]);
	sivalt_insert(&ivt, &sivals[1]);
	sivalt_insert(&ivt, &sivals[2]);
	/* Delete middle interval */
	sivalt_delete(&ivt, &sivals[1]);
	struct sival tar = {.lower = 4, .upper = 4};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) != NULL);
	tar = (struct sival) {.lower = 21, .upper = 21};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct sival) {.lower = 33, .upper = 33};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) != NULL);
	/* Delete first interval */
	sivalt_delete(&ivt, &sivals[0]);
	tar = (struct sival) {.lower = 4, .upper = 4};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct sival) {.lower = 21, .upper = 21};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct sival) {.lower = 33, .upper = 33};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) != NULL);
	/* Delete last interval */
	sivalt_delete(&ivt, &sivals[2]);
	/* Tree should be empty now */
	tar = (struct sival) {.lower = 4, .upper = 4};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct sival) {.lower = 21, .upper = 21};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	tar = (struct sival) {.lower = 33, .upper = 33};
	CU_ASSERT(sivalt_intersect(&ivt, &tar, NULL) == NULL);
	rb_tree_free(&ivt);
}

static void test_u64tosize(void)
{
	int rc;
	struct u64tosize m;
	uint64_t keys[] = {2, 6, 7, 10};
	size_t vals[] = {4, 12, 14, 20};
	rc = u64tosize_init_cap(&m, 4);
	CU_ASSERT(rc == 0);
	for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); i++) {
		rc = u64tosize_insert(&m, keys[i], vals[i]);
		CU_ASSERT(rc == 0);
	}

	size_t *vp;
	for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); i++) {
		vp = u64tosize_find(&m, keys[i]);
		CU_ASSERT(vp && *vp == vals[i]);
	}

	vp = u64tosize_find(&m, 8);
	CU_ASSERT(vp == NULL);
	vp = u64tosize_find(&m, 0);
	CU_ASSERT(vp == NULL);
	vp = u64tosize_find(&m, 3);
	CU_ASSERT(vp == NULL);

	uint64_t k;
	size_t v;
	size_t cnt = 0;
	struct u64tosize_it it = {0};
	while (u64tosize_foreach(&m, &k, &v, &it) == 0) {
		bool k_found = false, v_found = false;
		for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); i++) {
			k_found |= keys[i] == k;
			v_found |= vals[i] == v;
		}
		CU_ASSERT(k_found && v_found);
		cnt++;
	}
	CU_ASSERT(cnt == sizeof(keys)/sizeof(*keys));

	rc = u64tosize_delete(&m, 4);
	CU_ASSERT(rc != 0);
	rc = u64tosize_delete(&m, 1);
	CU_ASSERT(rc != 0);

	for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); i++) {
		rc = u64tosize_delete(&m, keys[i]);
		CU_ASSERT(rc == 0);
	}

	u64tosize_free(&m);
}

static void test_strtoidx(void)
{
	int rc;
	struct strtoidx m;
	rc = strtoidx_init_cap(&m, 2);
	CU_ASSERT(rc == 0);
	char *s;
	s = malloc(20);
	strcpy(s, "asdf0");
	rc = strtoidx_insert(&m, s, 0);
	CU_ASSERT(rc == 0);
	s = malloc(20);
	strcpy(s, "asdf1");
	rc = strtoidx_insert(&m, s, 1);
	CU_ASSERT(rc == 0);

	size_t *v;
	v = strtoidx_find(&m, "asdf1");
	CU_ASSERT(v && *v == 1);
	v = strtoidx_find(&m, "asdf0");
	CU_ASSERT(v && *v == 0);

	v = strtoidx_find(&m, "asdf2");
	CU_ASSERT(v == NULL);
	v = strtoidx_find(&m, "");
	CU_ASSERT(v == NULL);

	rc = strtoidx_delete(&m, "asdf2");
	CU_ASSERT(rc != 0);
	rc = strtoidx_delete(&m, "");
	CU_ASSERT(rc != 0);

	rc = strtoidx_delete(&m, "asdf1");
	CU_ASSERT(rc == 0);

	strtoidx_free(&m);
}

static void test_arena(void)
{
	struct arena a = ARENA_EMPTY;
	a.default_cap = 8;

	char *s = arena_alloc(&a, 3);
	strcpy(s, "ab");
	s = arena_alloc(&a, 2);
	strcpy(s, "c");
	uint16_t *i = arena_alloc(&a, sizeof(*i));
	*i = 5;
	i = arena_alloc(&a, sizeof(*i));
	*i = 6;

	arena_free(&a);
}

struct dummy {
	char d[40];
};

static void test_arena_basic(void)
{
	struct arena a = ARENA_EMPTY;
	a.default_cap = 8;

	/* Do a bunch of allocations. */
	struct dummy *vals = NULL;
	for (size_t i = 0; i < 3; i++) {
		vals = arena_allocn(&a, struct dummy, 10);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 40);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 63);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 1);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 65);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 1000);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 80);
		CU_ASSERT_PTR_NOT_NULL(vals);
		vals = arena_allocn(&a, struct dummy, 1);
		CU_ASSERT_PTR_NOT_NULL(vals);
		/* Free all regions */
		arena_clear(&a);
	}

	arena_free(&a);
}

int main(void)
{
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("crust", init_suite_crust, clean_suite_crust);
	if (pSuite == NULL) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ((CU_ADD_TEST(pSuite, test_gvec) == NULL)
	    || (CU_ADD_TEST(pSuite, test_int_vec) == NULL)
	    || (CU_ADD_TEST(pSuite, test_uival) == NULL)
	    || (CU_ADD_TEST(pSuite, test_uival_fuzz) == NULL)
	    || (CU_ADD_TEST(pSuite, test_uival_multi_intersect) == NULL)
	    || (CU_ADD_TEST(pSuite, test_sival) == NULL)
	    || (CU_ADD_TEST(pSuite, test_u64tosize) == NULL)
	    || (CU_ADD_TEST(pSuite, test_strtoidx) == NULL)
	    || (CU_ADD_TEST(pSuite, test_arena) == NULL)
	    || (CU_ADD_TEST(pSuite, test_arena_basic) == NULL)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	int n_fails = CU_get_number_of_tests_failed();
	CU_cleanup_registry();
	if (CU_get_error() != CUE_SUCCESS) {
		fprintf(stderr, "Cunit framework error after cleanup: %s\n", CU_get_error_msg());
	}
	return n_fails;
}
