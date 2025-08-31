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

/* Interval RB tree for arbitrary integer type

   It has two required prerequisite defines:
   IVAL_TYPE :: The integer type of the interval
   IVAL_TYPE_FMT :: The formatting string of IVAL_TYPE
   IVAL_NAME :: The name of the interval struct. Interval related
                functions will be named IVAL_NAME_sth, and interval
                tree related functions will be named IVAL_NAMEt_sth.

   The functions that will be defined are the following
   (IVAL_TYPE=uint64_t, IVAL_NAME=uival):

   bool uival_intersects_pt(struct uival *iv, uint64_t val);
   bool uival_intersects(struct uival *iv, struct uival *val);
   bool uival_eq(struct uival *a, struct uival *b);

   void uivalt_insert(struct rb_tree *ivt, struct uival *iv);
   void uivalt_delete(struct rb_tree *ivt, struct uival *iv);
   struct uival *uivalt_intersect(struct rb_tree *ivt, struct uival *iv);
   struct uival *uivalt_intersect_pt(struct rb_tree *ivt, uint64_t pt);

   The print functions print the tree as a DOT graph.
   void uivalt_print(struct rb_tree *ivt);
   void uivalt_fprint(FILE *s, struct rb_tree *ivt);
   void uival_fprint(FILE *s, struct rb_tree *ivt, struct uival *iv);
*/

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "rb_tree.h"


#ifndef IVAL_MAX
#define IVAL_MAX(a,b) (((a) >= (b)) ? (a) : (b))
#endif

struct IVAL_NAME {
    IVAL_TYPE lower;
    IVAL_TYPE upper;  // inclusive
    void *value;

    // link_max holds the max upper limit by the current interval and
    // all of its children.
    IVAL_TYPE link_max;
    struct rb_node link;
};


static inline struct IVAL_NAME *CRUST_PREFIX(IVAL_NAME, _unpack)(struct rb_node *n)
{
    return (struct IVAL_NAME *)((uintptr_t)n - offsetof(struct IVAL_NAME, link));
}

static inline bool CRUST_PREFIX(IVAL_NAME, _intersects_pt)(const struct IVAL_NAME *iv, IVAL_TYPE val)
{
    return val >= iv->lower && val <= iv->upper;
}

static inline bool CRUST_PREFIX(IVAL_NAME, _intersects)(const struct IVAL_NAME *iv, const struct IVAL_NAME *val)
{
    return iv->lower <= val->upper && val->lower <= iv->upper;
}

static inline bool CRUST_PREFIX(IVAL_NAME, _eq)(const struct IVAL_NAME *a, const struct IVAL_NAME *b)
{
    return a->upper == b->upper && a->lower == b->lower;
}

static inline void CRUST_PREFIX(IVAL_NAME, t_update_link_max)(struct rb_tree *rbt, struct rb_node *x)
{
    struct IVAL_NAME *l_iv = x->c[RB_LEFT] != rbt->nil ? CRUST_PREFIX(IVAL_NAME, _unpack)(x->c[RB_LEFT]) : NULL;
    struct IVAL_NAME *r_iv = x->c[RB_RIGHT] != rbt->nil ? CRUST_PREFIX(IVAL_NAME, _unpack)(x->c[RB_RIGHT]) : NULL;
    IVAL_TYPE l_max = l_iv ? l_iv->link_max : 0;
    IVAL_TYPE r_max = r_iv ? r_iv->link_max : 0;
    struct IVAL_NAME *x_iv = CRUST_PREFIX(IVAL_NAME, _unpack)(x);
    x_iv->link_max = IVAL_MAX(x_iv->upper, IVAL_MAX(l_max, r_max));
}

static inline void CRUST_PREFIX(IVAL_NAME, t_insert)(struct rb_tree *ivt, struct IVAL_NAME *new_iv)
{
    new_iv->link_max = new_iv->upper;
    if (ivt->root == ivt->nil) {
	rb_tree_insert_root(ivt, &new_iv->link, CRUST_PREFIX(IVAL_NAME, t_update_link_max));
	return;
    }

    struct rb_node **cur = &ivt->root, *par = ivt->nil;
    while (*cur != ivt->nil) {
	struct IVAL_NAME *cur_iv = CRUST_PREFIX(IVAL_NAME, _unpack)(*cur);
	/* Update max as we go down */
	cur_iv->link_max = IVAL_MAX(new_iv->upper, cur_iv->link_max);
	par = *cur;
	if (new_iv->lower < cur_iv->lower) {
	    cur = &(*cur)->c[RB_LEFT];
	} else {
	    cur = &(*cur)->c[RB_RIGHT];
	}
    }
    rb_tree_insert(ivt, par, &new_iv->link, cur);
    rb_tree_insert_rebalance(ivt, &new_iv->link);
}

static inline struct IVAL_NAME *CRUST_PREFIX(IVAL_NAME, t_intersect)(const struct rb_tree *ivt,
								    const struct IVAL_NAME *iv,
								    struct IVAL_NAME *lastmatch)
{
    struct rb_node *cur = lastmatch ? &lastmatch->link : ivt->root;
    while (cur != ivt->nil) {
	struct IVAL_NAME *cur_iv = CRUST_PREFIX(IVAL_NAME, _unpack)(cur);
	if (cur_iv != lastmatch && CRUST_PREFIX(IVAL_NAME, _intersects)(cur_iv, iv)) {
	    return cur_iv;
	}
	struct rb_node *p = cur;
	if (cur->c[RB_LEFT] != ivt->nil && CRUST_PREFIX(IVAL_NAME, _unpack)(cur->c[RB_LEFT])->link_max >= iv->lower) {
	    cur = cur->c[RB_LEFT];
	} else {
	    /* Could early exit if the right link_max < iv->lower or
	     * cur->lower > iv->upper (similar to below), but we make
	     * the assumption that at least one interval will be found
	     * thus making the checks slow us down. */
	    cur = cur->c[RB_RIGHT];
	}
   	/* There's only a need to keep searching in a different
	 * subtree if we actually have found a match. */
	if (cur == ivt->nil && lastmatch) {
	    while (p != ivt->nil) {
		if (cur == p->c[RB_LEFT] &&
		    p->c[RB_RIGHT] != ivt->nil &&
		    CRUST_PREFIX(IVAL_NAME, _unpack)(p->c[RB_RIGHT])->link_max >= iv->lower &&
		    CRUST_PREFIX(IVAL_NAME, _unpack)(p)->lower <= iv->upper) {
		    cur = p->c[RB_RIGHT];
		    // lastmatch = NULL; ?? Does not seem to perform better with this.
		    break;
		}
		cur = p;
		p = RB_P(p);
	    };
	    if (cur == ivt->root) {
		break;
	    }
	}
    }
    return NULL;
}

static inline struct IVAL_NAME *CRUST_PREFIX(IVAL_NAME, t_intersect_pt)(const struct rb_tree *ivt,
								       IVAL_TYPE pt,
								       struct IVAL_NAME *lastmatch)
{
    struct IVAL_NAME iv = {.lower = pt, .upper = pt};
    return CRUST_PREFIX(IVAL_NAME, t_intersect)(ivt, &iv, lastmatch);
}

static inline void CRUST_PREFIX(IVAL_NAME, t_delete)(struct rb_tree *ivt, struct IVAL_NAME *iv)
{
    rb_tree_delete(ivt, &iv->link);
}

static inline void CRUST_PREFIX(IVAL_NAME, _fprint)(FILE *s, struct rb_tree *ivt, struct IVAL_NAME *iv)
{
    const char *color = RB_COLOR(&iv->link) == RB_RED ? "red" : "black";
    fprintf(s, "    \"%p\" [color=%s, label=\"<lower> %"IVAL_TYPE_FMT" |<upper> %"IVAL_TYPE_FMT" |<linkmax> %"IVAL_TYPE_FMT" |<left> %p |<right> %p\"]\n",
	    (void *)&iv->link, color, iv->lower, iv->upper, iv->link_max, (void *)iv->link.c[RB_LEFT], (void *)iv->link.c[RB_RIGHT]);
    if (RB_P(&iv->link) != ivt->nil) {
	const char *plink;
	if (RB_P(&iv->link)->c[RB_LEFT] == &iv->link) {
	    plink = "left";
	} else {
	    plink = "right";
	}
	fprintf(s, "    \"%p\":%s -- \"%p\"\n", (void *)RB_P(&iv->link), plink, (void *)&iv->link);
    }
    if (iv->link.c[RB_LEFT] != ivt->nil) {
	CRUST_PREFIX(IVAL_NAME, _fprint)(s, ivt, CRUST_PREFIX(IVAL_NAME, _unpack)(iv->link.c[RB_LEFT]));
    }
    if (iv->link.c[RB_RIGHT] != ivt->nil) {
	CRUST_PREFIX(IVAL_NAME, _fprint)(s, ivt, CRUST_PREFIX(IVAL_NAME, _unpack)(iv->link.c[RB_RIGHT]));
    }
}

static inline void CRUST_PREFIX(IVAL_NAME, _print)(struct rb_tree *ivt, struct IVAL_NAME *iv)
{
    CRUST_PREFIX(IVAL_NAME, _fprint)(stdout, ivt, iv);
}

static inline void CRUST_PREFIX(IVAL_NAME, t_fprint)(FILE *s, struct rb_tree *ivt)
{
    fprintf(s, "graph {\n");
    fprintf(s, "    label=\"%ld: <lower> | <upper> | <linkmax> | <left> | <right>\"\n", time(NULL));
    fprintf(s, "    node [shape=record];\n");
    if (ivt->root != ivt->nil) {
	CRUST_PREFIX(IVAL_NAME, _fprint)(s, ivt, CRUST_PREFIX(IVAL_NAME, _unpack)(ivt->root));
    }
    fprintf(s, "}\n");
}

static inline void CRUST_PREFIX(IVAL_NAME, t_print)(struct rb_tree *ivt)
{
    CRUST_PREFIX(IVAL_NAME, t_fprint)(stdout, ivt);
}

#undef IVAL_TYPE
#undef IVAL_NAME
#undef IVAL_TYPE_FMT
