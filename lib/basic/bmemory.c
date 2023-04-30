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

#include "ds_btree.h"
#include "bmemory.h"

ds_btree_t progs;
ds_btree_t vars;

int bmem_prog_cmp(void *_prog1, void *_prog2)
{
    prog_t *prog1 = (prog_t *)_prog1;
    prog_t *prog2 = (prog_t *)_prog2;
    return prog1->line_no - prog2->line_no;
}

int bmem_vars_cmp(void *_var1, void *_var2)
{
    var_t *var1 = (var_t *)_var1;
    var_t *var2 = (var_t *)_var2;
    return strcmp(var1->name, var2->name);
}

int bmem_init()
{
    ds_btree_init(&progs, offsetof(prog_t, item), bmem_prog_cmp);
    ds_btree_init(&vars, offsetof(var_t, item), bmem_vars_cmp);
    return 0;
}

var_t *bmem_var_get(char *name)
{
    var_t search = {
        .name = name,
    };
    return ds_btree_find(&vars, &search);
}

void bmem_var_number_free(var_t *var)
{
    ds_btree_remove_object(&vars, var);
    free(var->name);
    free(var);
}

var_t *bmem_var_number_new(char *name, float value)
{
    var_t *var = (var_t *)malloc(sizeof(var_t));
    if (var == 0)
    {
        return 0;
    }
    var->number = value;
    var->name = name;

    var_t *exist = (var_t *)ds_btree_insert(&vars, var);
    if (exist != var)
    {
        exist->number = var->number;
        free(var);
        return exist;
    }

    size_t n = strlen(name);
    var->name = (char *)malloc(n);
    if (var->name == 0)
    {
        bmem_var_number_free(var);
        return 0;
    }
    strcpy(var->name, name);

    return var;
}

void bmem_prog_free(prog_t *prog)
{
    if (prog->line_no != 0)
    {
        ds_btree_remove_object(&progs, prog);
    }
    free(prog->line);
    free(prog);
}

prog_t *bmem_prog_new(uint16_t line_no, uint8_t *line, uint16_t len)
{
    prog_t *prog = (prog_t *)malloc(sizeof(prog_t));
    if (prog == 0)
    {
        return 0;
    }
    prog->line = (uint8_t *)malloc(len + 1);
    if (prog->line == 0)
    {
        return 0;
    }

    memcpy(prog->line, line, len + 1);
    prog->len = len;
    prog->line_no = line_no;

    if (line_no == 0)
    {
        return prog;
    }

    prog_t *exist = (prog_t *)ds_btree_insert(&progs, prog);
    if (exist != prog)
    {
        free(exist->line);
        exist->line = prog->line;
        exist->len = prog->len;
        free(prog);
        prog = exist;
    }

    if (prog->len == 0)
    {
        bmem_prog_free(prog);
        return 0;
    }

    return prog;
}
