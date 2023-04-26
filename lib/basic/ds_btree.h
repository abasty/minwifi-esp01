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

#ifndef __DS_BTREE_H__
#define __DS_BTREE_H__

#include "ds_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ds_btree_item_s ds_btree_item_t;
struct ds_btree_item_s
{
    ds_btree_item_t *left;
    ds_btree_item_t *right;
    int height;
};

/**
 * @brief Items comparison function prototype
 *
 */
typedef int (*bs_btree_cmp_f)(void *, void *);

typedef struct ds_btree_s ds_btree_t;
struct ds_btree_s
{
    size_t count;
    ds_btree_item_t *root;
    size_t _offset_in_object;
    bs_btree_cmp_f cmp;
    ds_btree_item_t *_cmp_node;
    void *_cmp_object;
    ds_btree_item_t *_equal_node;
};

/**
 * @brief Initialize a binary tree
 *
 * @param btree The btree
 * @param cmp Comparison function between items
 */
void ds_btree_init(ds_btree_t *btree, size_t offset_in_object, bs_btree_cmp_f cmp);

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
void *ds_btree_insert(ds_btree_t *btree, void *object);

/**
 * @brief Remove an item from a btree. The comparison function is used.
 *
 * @param btree The btree
 * @param item The item in the btree to remove
 */
void *ds_btree_remove(ds_btree_t *btree, ds_btree_item_t *item);

/**
 * @brief Remove an object from a btree. The comparison function is used.
 *
 * @param btree The btree
 * @param item The object in the btree to remove
 */
static inline void *ds_btree_remove_object(ds_btree_t *btree, void *object)
{
    ds_btree_item_t *item = DS_ITEM_OF(btree, object);
    return ds_btree_remove(btree, item);
}

#ifdef __cplusplus
}
#endif

#endif // __DS_BTREE_H__
