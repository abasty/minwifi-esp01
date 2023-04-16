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

#include <stdlib.h>

#include "ds_btree.h"
#include "ds_btree_ext.h"

// A utility function to get height of the tree
static inline int height(ds_btree_item_t *node)
{
    if (node == 0)
        return 0;
    return node->height;
}

// A utility function to get maximum of two integers
static inline int max(int a, int b)
{
    return (a > b) ? a : b;
}

// Get Balance factor of a node
static inline int BF(ds_btree_item_t *node)
{
    if (node == 0)
        return 0;
    return height(node->left) - height(node->right);
}

// A utility function to right rotate subtree rooted with y
static ds_btree_item_t *ds_btree_right_rotate(ds_btree_item_t *y)
{
    ds_btree_item_t *x = y->left;
    ds_btree_item_t *tmp = x->right;

    // Perform rotation
    x->right = y;
    y->left = tmp;

    // Update heights
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    // Return new root
    return x;
}

// A utility function to left rotate subtree rooted with x
static ds_btree_item_t *ds_btree_left_rotate(ds_btree_item_t *x)
{
    ds_btree_item_t *y = x->right;
    ds_btree_item_t *tmp = y->left;

    // Perform rotation
    y->left = x;
    x->right = tmp;

    // Update heights
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    // Return new root
    return y;
}

// Given a non-empty binary search tree, return the father son of the node with
// minimum key value found in that tree. Note that the entire tree does not need
// to be searched
static ds_btree_item_t **ds_btree_min_value_node_father_son(ds_btree_item_t **father_son)
{
    ds_btree_item_t *current = *father_son;

    /* loop down to find the leftmost leaf */
    while (current->left != 0)
    {
        father_son = &current->left;
        current = current->left;
    }
    return father_son;
}

// Get the allocated node
static inline ds_btree_item_t *ds_btree_node_alloc(ds_btree_t *btree)
{
    ds_btree_item_t *node = btree->_cmp_node;
    btree->count++;
    // new node is initially added at leaf
    node->left = 0;
    node->right = 0;
    node->height = 1;
    return node;
}

static inline void *ds_btree_object_of(ds_btree_t *btree, ds_btree_item_t *node)
{
    return btree->_offset_in_object == -1 ? ((ds_btree_ext_item_t *)node)->object : DS_OBJECT_OF(btree, node);
}

static inline int ds_btree_cmp_object_to(ds_btree_t *btree, ds_btree_item_t *node)
{
    return btree->cmp(btree->_cmp_object, ds_btree_object_of(btree, node));
}

// Recursive function to insert a node with given key into subtree with given
// root. It returns root of the modified subtree.
static ds_btree_item_t *ds_btree_node_insert(ds_btree_t *btree, ds_btree_item_t *node)
{
    // 1. Perform the normal BST rotation
    if (node == 0)
        return ds_btree_node_alloc(btree);

    int cmp = ds_btree_cmp_object_to(btree, node);
    if (cmp <= -1)
        node->left = ds_btree_node_insert(btree, node->left);
    else if (cmp >= 1)
        node->right = ds_btree_node_insert(btree, node->right);
    else
    {
        // Equal keys not allowed
        btree->_equal_node = node;
        return node;
    }

    // 2. Update height of this ancestor node
    node->height = 1 + max(height(node->left), height(node->right));

    // 3. Get the balance factor of this ancestor node to check whether this
    // node became unbalanced
    int balance = BF(node);

    // If this node becomes unbalanced, then there are 4 cases
    if (balance > 1)
    {
        int cmp_left = ds_btree_cmp_object_to(btree, node->left);
        // Left Left Case
        if (cmp_left <= -1)
            return ds_btree_right_rotate(node);
        // Left Right Case
        if (cmp_left >= 1)
        {
            node->left = ds_btree_left_rotate(node->left);
            return ds_btree_right_rotate(node);
        }
    }
    else if (balance < -1)
    {
        int cmp_right = ds_btree_cmp_object_to(btree, node->right);
        // Right Right Case
        if (cmp_right >= 1)
            return ds_btree_left_rotate(node);
        // Right Left Case
        if (cmp_right <= -1)
        {
            node->right = ds_btree_right_rotate(node->right);
            return ds_btree_left_rotate(node);
        }
    }

    // return the (unchanged) node
    return node;
}

// Recursive function to delete a node with given key from subtree with given
// root. It returns root of the modified subtree.
static ds_btree_item_t *ds_btree_node_remove(ds_btree_t *btree, ds_btree_item_t **father_son)
{
    ds_btree_item_t *node = *father_son;
    // 1. Perform standard BST delete
    if (node == 0)
        return node;

    int cmp = ds_btree_cmp_object_to(btree, node);
    // If the key to be deleted is smaller than the root's key, then it lies in
    // left subtree
    if (cmp <= -1)
        node->left = ds_btree_node_remove(btree, &node->left);

    // If the key to be deleted is greater than the root's key, then it lies in
    // right subtree
    else if (cmp >= 1)
        node->right = ds_btree_node_remove(btree, &node->right);

    // If key is same as root's key, then This is the node to be deleted
    else
    {
        // Case 1: node with only one child or no child
        if ((node->left == 0) || (node->right == 0))
        {
            ds_btree_item_t *son = node->left ? node->left : node->right;
            *father_son = son;
            node->left = 0;
            node->right = 0;
            btree->_equal_node = node;
            btree->count--;
            node = son;
        }
        else
        {
            // Case 2: node with two children: Get the inorder successor
            // (smallest in the right subtree). This successor has no left son.
            // We exchange the successor and the node in the global tree. The
            // node will then become a node with no left son in the successor
            // right subtree. Then we delete the node in the right subtree : we
            // will fall in case 1.

            // Get successor
            ds_btree_item_t **successor_father_son = ds_btree_min_value_node_father_son(&node->right);
            ds_btree_item_t *successor = *successor_father_son;

            // node <-> successor
            // Exchanging node and successor sons (the successor has no left son)
            successor->left = node->left;
            node->left = 0;
            ds_btree_item_t *successor_right_copy = successor->right;
            if (node->right == successor)
            {
                // node is father of successor
                successor->right = node;
            }
            else
            {
                successor->right = node->right;
                *successor_father_son = node;
            }
            node->right = successor_right_copy;

            // Exchanging height
            int successor_height_copy = successor->height;
            successor->height = node->height;
            node->height = successor_height_copy;

            // Update node father
            *father_son = successor;

            // Delete the node in its new subtree
            successor->right = ds_btree_node_remove(btree, &successor->right);
            node = successor;
        }
    }

    // If the tree had only one node then return
    if (node == 0)
        return node;

    // 2. Update height of the current node
    node->height = 1 + max(height(node->left), height(node->right));

    // 3. Get the balance factor of this node (to check whether this node
    // became unbalanced)
    int balance = BF(node);

    // If this node becomes unbalanced, then there are 4 cases

    // Left Left Case
    if (balance > 1 && BF(node->left) >= 0)
        return ds_btree_right_rotate(node);

    // Left Right Case
    if (balance > 1 && BF(node->left) < 0)
    {
        node->left = ds_btree_left_rotate(node->left);
        return ds_btree_right_rotate(node);
    }

    // Right Right Case
    if (balance < -1 && BF(node->right) <= 0)
        return ds_btree_left_rotate(node);

    // Right Left Case
    if (balance < -1 && BF(node->right) > 0)
    {
        node->right = ds_btree_right_rotate(node->right);
        return ds_btree_left_rotate(node);
    }

    return node;
}

void ds_btree_init(ds_btree_t *btree, size_t offset_in_object, bs_btree_cmp_f cmp)
{
    btree->count = 0;
    btree->root = 0;
    btree->_offset_in_object = offset_in_object;
    btree->cmp = cmp;
}

void *ds_btree_insert(ds_btree_t *btree, void *object)
{
    ds_btree_item_t *item = DS_ITEM_OF(btree, object);
    btree->_cmp_node = item;
    btree->_cmp_object = object;
    btree->_equal_node = 0;
    btree->root = ds_btree_node_insert(btree, btree->root);
    return btree->_equal_node ? DS_OBJECT_OF(btree, btree->_equal_node) : object;
}

void *ds_btree_remove(ds_btree_t *btree, ds_btree_item_t *item)
{
    btree->_cmp_node = item;
    btree->_cmp_object = DS_OBJECT_OF(btree, item);
    btree->_equal_node = 0;
    btree->root = ds_btree_node_remove(btree, &btree->root);
    return btree->_equal_node ? DS_OBJECT_OF(btree, btree->_equal_node) : 0;
}

void ds_btree_ext_init(ds_btree_ext_t *btree, bs_btree_cmp_f cmp)
{
    btree->count = 0;
    btree->root = 0;
    btree->_offset_in_object = -1;
    btree->cmp = cmp;
}

void *ds_btree_ext_insert(ds_btree_ext_t *btree, ds_btree_ext_item_t *item, void *object)
{
    item->object = object;
    btree->_cmp_node = (ds_btree_item_t *)item;
    btree->_cmp_object = object;
    btree->_equal_node = 0;
    btree->root = ds_btree_node_insert(btree, btree->root);
    return btree->_equal_node ? ((ds_btree_ext_item_t *)btree->_equal_node)->object : object;
}

void *ds_btree_ext_remove(ds_btree_ext_t *btree, ds_btree_ext_item_t *item)
{
    btree->_cmp_node = (ds_btree_item_t *)item;
    btree->_cmp_object = item->object;
    btree->_equal_node = 0;
    btree->root = ds_btree_node_remove(btree, &btree->root);
    return btree->_equal_node ? ((ds_btree_ext_item_t *)btree->_equal_node)->object : 0;
}
