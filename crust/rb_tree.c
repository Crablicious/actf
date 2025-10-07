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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "rb_tree.h"

/* Red-black tree with support for augmentation. RB-tree algorithm
 * based on pseudocode in "Introductions to Algorithms, Third Edition"
 * 2009.
 *
 * 1. Nodes are either black or red
 * 2. All NULL children are black
 * 3. A red node MUST not have a red child
 * 4. All paths from a given node to its NULL descendant goes through same number of black nodes
 * 5. If a node has one child, it must be a red child (a black child would violate 4)
 *
 * Useful links:
 * - https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#req3
 * - https://fekir.info/post/constexpr-tree/
 * - https://lwn.net/Articles/184495/ :: rb-trees
 * - https://lwn.net/Articles/388118/ :: Augmented rb-trees
 * - https://lwn.net/Articles/500355/ :: Generic rb-trees
 * - https://en.wikipedia.org/wiki/Red-black_tree
 * - https://www.geeksforgeeks.org/insertion-in-red-black-tree/
 * - http://jstimpfle.de/blah/rbtree/main.html
 * - https://www.cs.toronto.edu/~tabrown/csc263/2014W/week4.interval.pdf
 */

#define __maybe_unused __attribute__((unused))
#define FLIP(dir) (1 - (dir))
#define COLOR(node) RB_COLOR((node))
#define P(node) RB_P((node))
#define SET_COLOR_BLACK(node) ((node)->p = (struct rb_node *)((uintptr_t)((node)->p) & ~(uintptr_t)1))
#define SET_COLOR_RED(node) ((node)->p = (struct rb_node *)((uintptr_t)((node)->p) | (uintptr_t)1))
#define SET_COLOR(node, color) ((node)->p = (struct rb_node *)(((uintptr_t)((node)->p) & ~(uintptr_t)1) | (uintptr_t)color))
#define SET_P(node, newp) do {			\
	enum rb_color c_ = COLOR((node));	\
	(node)->p = (newp);			\
	SET_COLOR((node), c_);			\
    } while (0)

int rb_tree_init(struct rb_tree *rbt)
{
	rbt->nil = malloc(sizeof(*rbt->nil));
	if (!rbt->nil) {
		return -ENOMEM;
	}
	rbt->nil->p = RB_BLACK;
	rbt->root = rbt->nil;
	rbt->reorder_cb = NULL;
	return 0;
}

void rb_tree_insert(struct rb_tree *rbt, struct rb_node *par,
		    struct rb_node *new, struct rb_node **side)
{
	if (!par) {
		par = rbt->nil;
	}
	new->p = par;
	SET_COLOR_RED(new);
	new->c[RB_LEFT] = rbt->nil;
	new->c[RB_RIGHT] = rbt->nil;
	if (&par->c[RB_LEFT] == side) {
		par->c[RB_LEFT] = new;
	} else {
		par->c[RB_RIGHT] = new;
	}
}

void rb_tree_insert_root(struct rb_tree *rbt, struct rb_node *new,
			 void (*reorder_cb)(struct rb_tree *rbt, struct rb_node *x))
{
	new->p = rbt->nil;
	SET_COLOR_BLACK(new);
	new->c[RB_LEFT] = rbt->nil;
	new->c[RB_RIGHT] = rbt->nil;
	rbt->root = new;
	rbt->reorder_cb = reorder_cb;
	rbt->reorder_cb(rbt, rbt->root);
}

static void reorder_cb(struct rb_tree *rbt, struct rb_node *x)
{
	if (rbt->reorder_cb && x != rbt->nil) {
		rbt->reorder_cb(rbt, x);
	}
}

/*
  dir=RB_LEFT = left-rotate:
  x            y
   \    =>    /
    y        x

  dir=RB_RIGHT = right-rotate:
    x      y
   /   =>   \
  y          x
 */
static void rotate(struct rb_tree *rbt, struct rb_node *x, enum rb_dir dir)
{
	struct rb_node *y = x->c[FLIP(dir)];
	x->c[FLIP(dir)] = y->c[dir];
	if (x->c[FLIP(dir)] != rbt->nil) {
		SET_P(x->c[FLIP(dir)], x);
	}

	SET_P(y, x->p);
	if (P(x) != rbt->nil) {
		if (P(x)->c[dir] == x) {
			P(x)->c[dir] = y;
		} else {
			P(x)->c[FLIP(dir)] = y;
		}
	} else {
		rbt->root = y;
	}

	y->c[dir] = x;
	SET_P(x, y);

	reorder_cb(rbt, x);
	reorder_cb(rbt, y);
}

/* Make sure rb-tree invariants hold and update max. `x` in args is
 * the starting point of the rebalancing.
 *
 * u = uncle
 * p = parent
 */
void rb_tree_insert_rebalance(struct rb_tree *rbt, struct rb_node *x)
{
	while (COLOR(P(x)) == RB_RED) {
		enum rb_dir dir = P(x) == P(P(x))->c[RB_LEFT] ? RB_LEFT : RB_RIGHT;
		struct rb_node *u = P(P(x))->c[FLIP(dir)];
		if (COLOR(u) == RB_RED) {
			SET_COLOR_BLACK(P(x));
			SET_COLOR_BLACK(u);
			SET_COLOR_RED(P(P(x)));
			x = P(P(x));
		} else {
			if (x == P(x)->c[FLIP(dir)]) {
				x = P(x);
				rotate(rbt, x, dir);
			}
			SET_COLOR_BLACK(P(x));
			SET_COLOR_RED(P(P(x)));
			rotate(rbt, P(P(x)), FLIP(dir));
			// this will break the loop since the parent of x is
			// BLACK.
		}
	}
	SET_COLOR_BLACK(rbt->root);
}

static struct rb_node *node_min(struct rb_tree *rbt, struct rb_node *x)
{
	while (x->c[RB_LEFT] != rbt->nil) {
		x = x->c[RB_LEFT];
	}
	return x;
}

__maybe_unused static struct rb_node *node_max(struct rb_tree *rbt, struct rb_node *x)
{
	while (x->c[RB_RIGHT] != rbt->nil) {
		x = x->c[RB_RIGHT];
	}
	return x;
}

__maybe_unused static struct rb_node *node_succ(struct rb_tree *rbt, struct rb_node *x)
{
	if (x->c[RB_RIGHT] != rbt->nil) {
		return node_min(rbt, x->c[RB_RIGHT]);
	}
	struct rb_node *p = P(x);
	while (p != rbt->nil && x == p->c[RB_RIGHT]) {
		x = p;
		p = P(p);
	}
	return p;
}

/* Replace x with y, linking x's parent to y. */
static void replace_node(struct rb_tree *rbt, struct rb_node *x, struct rb_node *y)
{
	if (P(x) == rbt->nil) {
		rbt->root = y;
	} else if (P(x)->c[RB_RIGHT] == x) {
		P(x)->c[RB_RIGHT] = y;
	} else {
		P(x)->c[RB_LEFT] = y;
	}
	SET_P(y, x->p);
}

/* Returns the deepest non-max-updated node. */
static struct rb_node *rebalance_delete(struct rb_tree *rbt, struct rb_node *x)
{
	while (rbt->root != x && COLOR(x) == RB_BLACK) {
		enum rb_dir dir = x == P(x)->c[RB_LEFT] ? RB_LEFT : RB_RIGHT;
		struct rb_node *s = P(x)->c[FLIP(dir)];
		if (COLOR(s) == RB_RED) {	// case 1
			SET_COLOR_RED(P(x));
			SET_COLOR_BLACK(s);
			rotate(rbt, P(x), dir);	// we technically do not need to reorder_cb on y in here
			// since it is x's parent which we'll get to eventually.
			s = P(x)->c[FLIP(dir)];	// Only so we can fall through to case
			// 2, not needed if instead else if and
			// loop.
		}
		if (COLOR(s->c[RB_LEFT]) == RB_BLACK && COLOR(s->c[RB_RIGHT]) == RB_BLACK) {	// case 2
			SET_COLOR_RED(s);
			reorder_cb(rbt, x);
			x = P(x);
		} else {
			if (COLOR(s->c[FLIP(dir)]) == RB_BLACK) {	// case 3
				SET_COLOR_RED(s);
				SET_COLOR_BLACK(s->c[dir]);
				rotate(rbt, s, FLIP(dir));
				s = P(x)->c[FLIP(dir)];	// Only to be able to fall-through
			}
			// case 4
			SET_COLOR(s, COLOR(P(x)));
			SET_COLOR_BLACK(P(x));
			SET_COLOR_BLACK(s->c[FLIP(dir)]);
			rotate(rbt, P(x), dir);

			// x is supposed to be set to root and colored black,
			// but we need to return the deepest delete-affected
			// node who has not had reorder_cb called on it, so we
			// just color root unconditionally instead.
			SET_COLOR_BLACK(rbt->root);
			return x;
		}
	}
	SET_COLOR_BLACK(x);
	return x;
}

void rb_tree_delete(struct rb_tree *rbt, struct rb_node *x)
{
	/* `y` holds the deepest part of the tree that was modified. */
	struct rb_node *y = x;
	enum rb_color deleted_color = COLOR(x);
	if (x->c[RB_LEFT] == rbt->nil) {	// Single or no child, we know child must be red
		y = x->c[RB_RIGHT];
		replace_node(rbt, x, x->c[RB_RIGHT]);
	} else if (x->c[RB_RIGHT] == rbt->nil) {	// Single child, we know child must be red
		y = x->c[RB_LEFT];
		replace_node(rbt, x, x->c[RB_LEFT]);
	} else {		// Two children
		struct rb_node *min = node_min(rbt, x->c[RB_RIGHT]);
		deleted_color = COLOR(min);
		y = min->c[RB_RIGHT];
		if (P(min) == x) {
			SET_P(y, min);
		} else {
			replace_node(rbt, min, min->c[RB_RIGHT]);
			min->c[RB_RIGHT] = x->c[RB_RIGHT];
			SET_P(min->c[RB_RIGHT], min);
		}
		replace_node(rbt, x, min);
		min->c[RB_LEFT] = x->c[RB_LEFT];
		SET_P(min->c[RB_LEFT], min);
		SET_COLOR(min, COLOR(x));
	}
	if (deleted_color == RB_BLACK) {
		y = rebalance_delete(rbt, y);
	} else {
		/* The subtree of y is never modified, so we can start
		 * cb-fixup from its parent. */
		y = P(y);
	}
	/* Walk to the root and call reorder_cb on each node affected by
	 * the deletion. Could potentially add an early exit if a
	 * reorder_cb doesn't change anything. */
	do {
		reorder_cb(rbt, y);
		y = P(y);
	} while (y != rbt->nil);
}

void rb_tree_free(struct rb_tree *rbt)
{
	if (!rbt) {
		return;
	}
	free(rbt->nil);
}
