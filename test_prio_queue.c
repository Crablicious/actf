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
#include <CUnit/TestDB.h>
#include <inttypes.h>
#include <time.h>

#include "crust/common.h"
#include "test_prio_queue.h"
#include "prio_queue.h"


static int test_prio_queue_suite_init(void)
{
	return 0;
}

static int test_prio_queue_suite_clean(void)
{
	return 0;
}

static void test_prio_queue_test_setup(void)
{
	return;
}

static void test_prio_queue_test_teardown(void)
{
	return;
}

static int cmpnode(const void *p1, const void *p2)
{
	const struct node *n1 = p1;
	const struct node *n2 = p2;
	return (n1->key < n2->key) ? -1 : (int) (n1->key - n2->key);
}

static void rand_test()
{
#define N_NODES 256
	int rc;
	struct prio_queue pq = { 0 };
	rc = prio_queue_init(&pq, N_NODES);
	CU_ASSERT_FATAL(rc == ACTF_OK);
	struct node nodes[N_NODES];
	for (int i = 0; i < N_NODES; i++) {
		nodes[i] = (struct node) {
			.value = i,
			.key = rand(),
		};
		prio_queue_push_unchecked(&pq, nodes[i]);
	}
	qsort(nodes, N_NODES, sizeof(*nodes), cmpnode);
	for (int i = 0; i < N_NODES; i++) {
		struct node expect = nodes[i];
		struct node actual = prio_queue_pop_unchecked(&pq);
		CU_ASSERT(memcmp(&expect, &actual, sizeof(expect)) == 0);
	}

	prio_queue_free(&pq);
}

/* useful to enable if implementation changes and you want to flush it
 * out properly. */
__maybe_unused static void test_prio_queue_fuzz(void)
{
	time_t t;
	unsigned seed = (unsigned) time(&t);
	printf("(seed: %u)", seed);
	srand(seed);
	for (int i = 0; i < 256; i++) {
		rand_test();
	}
}

static void test_prio_queue_fuzz_stable(void)
{
	srand(1);
	rand_test();
}

static void test_prio_queue_basic(void)
{
	int rc;
	struct prio_queue pq = { 0 };
	rc = prio_queue_init(&pq, 4);
	CU_ASSERT_FATAL(rc == ACTF_OK);
	struct node nodes[] = {
		{.value = 0,.key = 0 },
		{.value = 1,.key = 1 },
		{.value = 2,.key = 2 },
		{.value = 3,.key = 3 },
	};

	/* Insert smallest to largest */
	for (size_t i = 0; i < sizeof(nodes) / sizeof(*nodes); i++) {
		prio_queue_push_unchecked(&pq, nodes[i]);
		CU_ASSERT(prio_queue_peek_unchecked(&pq).value == 0);
	}
	for (size_t i = 0; i < sizeof(nodes) / sizeof(*nodes); i++) {
		struct node n = prio_queue_pop_unchecked(&pq);
		CU_ASSERT(n.value == i);
	}

	/* Insert largest to smallest */
	for (size_t i = 0; i < sizeof(nodes) / sizeof(*nodes); i++) {
		size_t oppo_i = sizeof(nodes) / sizeof(*nodes) - 1 - i;
		prio_queue_push_unchecked(&pq, nodes[oppo_i]);
		CU_ASSERT(prio_queue_peek_unchecked(&pq).value == oppo_i);
	}
	for (size_t i = 0; i < sizeof(nodes) / sizeof(*nodes); i++) {
		struct node n = prio_queue_pop_unchecked(&pq);
		CU_ASSERT(n.value == i);
	}

	prio_queue_free(&pq);
}

static CU_TestInfo test_prio_queue_tests[] = {
	{ "basic", test_prio_queue_basic },
	// {"fuzz", test_prio_queue_fuzz},
	{ "fuzz stable", test_prio_queue_fuzz_stable },
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_prio_queue_suite = {
	"Prio Queue", test_prio_queue_suite_init, test_prio_queue_suite_clean,
	test_prio_queue_test_setup, test_prio_queue_test_teardown, test_prio_queue_tests
};
