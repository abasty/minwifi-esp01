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

#ifndef __EVAL_H__
#define __EVAL_H__

#include <stdint.h>

#include "bmemory.h"
#include "token.h"

typedef struct
{
    prog_t *pc;
    prog_t *prog;
    char *var_ref;
    var_t *input_var;
    float number;
    uint8_t *read_ptr;
    uint8_t token;
    uint8_t input_var_token;
    int8_t error;
    bool do_eval;
    bool running;
    bool inputting;
    int sp;
    string_t string;
} eval_state_t;

static int8_t eval_prog(prog_t *prog, bool do_eval);
static bool eval_running();
static bool eval_inputting();
static void eval_stop();
static int8_t eval_input_store(char *io_string);
static int8_t eval_prog_next();

static bool eval_string_expr();
static bool eval_factor();
static bool eval_expr(uint8_t type_token);

extern eval_state_t bstate;

#endif // __EVAL_H__
