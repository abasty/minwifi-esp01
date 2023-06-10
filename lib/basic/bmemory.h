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

#ifndef __BMEMORY_H__
#define __BMEMORY_H__

#include <stdint.h>

#include "ds_btree.h"
#include "ds_lifo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ds_btree_item_t tree;
    ds_lifo_item_t list;
    uint16_t line_no;
    uint16_t len;
    uint8_t *line;
} prog_t;

typedef struct {
    ds_btree_item_t tree;
    ds_lifo_item_t list;
    char *name;
    union {
        void *value;
        float number;
        char *string;
    };
} var_t;


void bmem_init();

void bmem_prog_new();
void bmem_prog_line_free(prog_t *prog);
prog_t *bmem_prog_line_new(uint16_t line_no, uint8_t *line, uint16_t len);
prog_t *bmem_prog_first_line();
prog_t *bmem_prog_next_line(prog_t *prog);

void bmem_vars_clear();
var_t *bmem_var_string_set(char *name, char *value);
var_t *bmem_var_number_set(char *name, float value);
var_t *bmem_var_find(char *name);
var_t *bmem_var_first();
var_t *bmem_var_next(var_t *var);


#ifdef __cplusplus
}
#endif

#endif // __BMEMORY_H__
