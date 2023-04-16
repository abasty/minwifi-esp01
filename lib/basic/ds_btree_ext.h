/*
 * Copyright © 2021 Alain Basty
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __DS_BTREE_EXT_H__
#define __DS_BTREE_EXT_H__

#include <stddef.h>

#include "ds_btree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ds_btree_ext_item_s ds_btree_ext_item_t;
struct ds_btree_ext_item_s
{
    ds_btree_ext_item_t *left;
    ds_btree_ext_item_t *right;
    int height;
    void *object;
};

typedef ds_btree_t ds_btree_ext_t;

/**
 * @brief Initialize a binary tree
 *
 * @param btree The btree
 * @param cmp Comparison function between items
 */
void ds_btree_ext_init(ds_btree_t *btree, bs_btree_cmp_f cmp);

/**
 \* @brief Insert an item into a btree and associate the related object
 *
 \* @param btree The btree
 \* @param item The item to insert
 \* @param object The associated object
 *
 \* @return If `object` has no equal object in the btree, the object is inserted
 * in the btree and the function returns `object`. If `object` has an equal
 * object in the btree, it is not inserted and the function returns the equal
 * object. "Equal" stands for : The comparison function returns 0.
 */
void *ds_btree_ext_insert(ds_btree_t *btree, ds_btree_ext_item_t *item, void *object);

/**
 * @brief Remove an item from a btree. The comparison function is used.
 *
 * @param btree The btree
 * @param item The item in the btree to remove
 */
void *ds_btree_ext_remove(ds_btree_t *btree, ds_btree_ext_item_t *item);

#ifdef __cplusplus
}
#endif

#endif // __DS_BTREE_EXT_H__
