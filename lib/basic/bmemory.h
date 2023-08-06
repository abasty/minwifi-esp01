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

#include "bio.h"

static void bmem_init();
static void bmem_prog_line_free(prog_t *prog);
static prog_t *bmem_prog_line_new(uint16_t line_no, uint8_t *line, uint16_t len);
static prog_t *bmem_prog_first_line();
static prog_t *bmem_prog_next_line(prog_t *prog);
static prog_t *bmem_prog_get_line(uint16_t line_no);

static void bmem_vars_clear();
static var_t *bmem_var_string_set(const char *name, char *value);
static var_t *bmem_var_number_set(const char *name, float value);
static var_t *bmem_var_first();
static var_t *bmem_var_next(var_t *var);

#endif // __BMEMORY_H__
