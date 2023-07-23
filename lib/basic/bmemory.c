/*
 * Copyright © 2023 Alain Basty
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ds_common.h"
#include "ds_btree.h"
#include "ds_lifo.h"
#include "bmemory.h"
#include "token.h"

ds_btree_t prog_tree;
ds_lifo_t prog_list;
ds_btree_t var_tree;
ds_lifo_t var_list;

static int bmem_prog_cmp(void *_prog1, void *_prog2)
{
    prog_t *prog1 = (prog_t *)_prog1;
    prog_t *prog2 = (prog_t *)_prog2;
    return prog1->line_no - prog2->line_no;
}

static int bmem_vars_cmp(void *_var1, void *_var2)
{
    var_t *var1 = (var_t *)_var1;
    var_t *var2 = (var_t *)_var2;
    return strcmp(var1->name, var2->name);
}

var_t *bmem_var_find(const char *name)
{
    var_t search = {
        .name = (char *) name,
    };
    return ds_btree_find(&var_tree, &search);
}

static void bmem_var_unset(var_t *var)
{
    ds_btree_remove_object(&var_tree, var);
    if (var->name && *var->name == TOKEN_VARIABLE_STRING)
    {
        free(var->string);
    }
    free(var->name);
    free(var);
}

void bmem_vars_clear()
{
    while (var_tree.root)
    {
        bmem_var_unset((var_t *)DS_OBJECT_OF(&var_tree, var_tree.root));
    }
}

static var_t *bmem_var_get_or_new(const char *name)
{
    var_t *var = bmem_var_find(name);
    if (var == 0)
    {
        var = (var_t *)malloc(sizeof(var_t));
        if (var == 0)
            return 0;
        size_t n = strlen(name) + 1;
        var->name = (char *)malloc(n);
        if (var->name == 0)
        {
            free(var);
            return 0;
        }
        strcpy(var->name, name);
        var->value = 0;
        ds_btree_insert(&var_tree, var);
    }
    return var;
}

var_t *bmem_var_string_set(const char *name, char *value)
{
    var_t *var = bmem_var_get_or_new(name);
    if (var == 0)
        return 0;
    if (var->string != 0)
    {
        free(var->string);
        var->string = 0;
    }
    if (value != 0)
    {
        var->string = (char *)malloc(strlen(value) + 1);
        if (var->string != 0)
        {
            strcpy(var->string, value);
        }
    }
    return var;
}

var_t *bmem_var_number_set(const char *name, float value)
{
    var_t *var = bmem_var_get_or_new(name);
    if (var == 0)
        return 0;
    var->number = value;
    return var;
}

static inline void bmem_invalidate_prog_list()
{
    prog_list.root = 0;
}

void bmem_prog_new()
{
    bmem_vars_clear();
    while (prog_tree.root)
    {
        bmem_prog_line_free((prog_t *)DS_OBJECT_OF(&prog_tree, prog_tree.root));
    }
}

void bmem_prog_line_free(prog_t *prog)
{
    if (prog->line_no != 0)
    {
        bmem_invalidate_prog_list();
        ds_btree_remove_object(&prog_tree, prog);
    }
    free(prog->line);
    free(prog);
}

prog_t *bmem_prog_line_new(uint16_t line_no, uint8_t *line, uint16_t len)
{

    if (len == 0 && line_no == 0)
        return 0;

    bmem_invalidate_prog_list();

    prog_t search = {
        .line_no = line_no,
    };

    prog_t *prog = ds_btree_find(&prog_tree, &search);
    if (prog != 0)
    {
        bmem_prog_line_free(prog);
    }

    if (len == 0)
        return 0;

    prog = (prog_t *)malloc(sizeof(prog_t));
    if (prog == 0)
        return 0;

    prog->line = (uint8_t *)malloc(len + 1);
    if (prog->line == 0)
    {
        free(prog);
        return 0;
    }

    memcpy(prog->line, line, len);
    prog->line[len] = 0;
    prog->len = len;
    prog->line_no = line_no;

    if (line_no > 0)
    {
        ds_btree_insert(&prog_tree, prog);
    }

    return prog;
}

static void bmem_prog_node_push(ds_btree_item_t *node)
{
    if (node)
    {
        bmem_prog_node_push(node->right);
        ds_lifo_push(&prog_list, DS_OBJECT_OF(&prog_tree, node));
        bmem_prog_node_push(node->left);
    }
}

prog_t *bmem_prog_first_line()
{
    if (prog_list.root == 0)
    {
        bmem_prog_node_push(prog_tree.root);
    }

    if (prog_list.root == 0)
        return 0;

    return (prog_t *)(DS_OBJECT_OF(&prog_list, prog_list.root));
}

prog_t *bmem_prog_next_line(prog_t *prog)
{
    if (!prog)
        return 0;

    if (!(prog->list.next))
        return 0;

    return (prog_t *) (DS_OBJECT_OF(&prog_list, prog->list.next));
}

prog_t *bmem_prog_get_line(uint16_t line_no)
{
    prog_t search = {
        .line_no = line_no,
    };

    if (prog_list.root == 0)
    {
        bmem_prog_node_push(prog_tree.root);
    }

    return ds_btree_find(&prog_tree, &search);
}

static void bmem_var_node_push(ds_btree_item_t *node)
{
    if (node)
    {
        bmem_var_node_push(node->right);
        ds_lifo_push(&var_list, DS_OBJECT_OF(&var_tree, node));
        bmem_var_node_push(node->left);
    }
}

var_t *bmem_var_first()
{
    var_list.root = 0;
    bmem_var_node_push(var_tree.root);

    if (var_list.root == 0)
        return 0;

    return (var_t *)(DS_OBJECT_OF(&var_list, var_list.root));
}

var_t *bmem_var_next(var_t *var)
{
    if (!var)
        return 0;

    if (!(var->list.next))
        return 0;

    return (var_t *) (DS_OBJECT_OF(&var_list, var->list.next));
}

void bmem_init()
{
    ds_btree_init(&prog_tree, offsetof(prog_t, tree), bmem_prog_cmp);
    ds_btree_init(&var_tree, offsetof(var_t, tree), bmem_vars_cmp);
    ds_lifo_init(&prog_list, offsetof(prog_t, list));
    ds_lifo_init(&var_list, offsetof(var_t, list));
}
