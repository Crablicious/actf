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

#ifndef RB_TREE_H
#define RB_TREE_H

#include <stdint.h>


enum rb_color {
    RB_BLACK,
    RB_RED,
};

enum rb_dir {
    RB_LEFT,
    RB_RIGHT,
};

/* RB_P returns the color of node. */
#define RB_COLOR(node) ((uintptr_t)((node)->p) & 1)
/* RB_P returns the parent of node. */
#define RB_P(node) ((struct rb_node *)((uintptr_t)((node)->p) & ~(uintptr_t)1))

struct rb_node {
    /* Children, indexed by enum rb_dir */
    struct rb_node *c[2];
    /* Parent, bit 0 contains the color, so accessing p should be done
     * using the RB_P macro. */
    struct rb_node *p;
};

struct rb_tree {
    struct rb_node *root;
    /* nil sentinel node, mallocd to allow rb_tree to be copied. */
    struct rb_node *nil;
    /* Callback that will be called every time the subtrees of x
     * changes.
     */
    void (*reorder_cb)(struct rb_tree *rbt, struct rb_node *x);
};

#define RB_EMPTY_NODE (struct rb_node){0}

/* A user needs to implement the following:
   - Insert
   - Search
   - Delete

   rb_tree.h only provides the necessary primitives to do so.
*/
int rb_tree_init(struct rb_tree *rbt);
void rb_tree_insert_root(struct rb_tree *rbt, struct rb_node *new,
			 void (*reorder_cb)(struct rb_tree *rbt, struct rb_node *x));

void rb_tree_insert(struct rb_tree *rbt, struct rb_node *par,
		    struct rb_node *new, struct rb_node **side);
void rb_tree_insert_rebalance(struct rb_tree *rbt, struct rb_node *new);

void rb_tree_delete(struct rb_tree *rbt, struct rb_node *x);
void rb_tree_free(struct rb_tree *rbt);

#endif /* RB_TREE_H */
