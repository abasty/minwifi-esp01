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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t line_no;
    uint8_t *read_ptr;
    uint8_t *write_ptr;
} t_interpreter_state;

int8_t eval_prog(prog_t *prog, bool do_eval);
bool eval_running();
bool eval_inputting();
void eval_input_mode(bool mode);
int8_t eval_input_store(char *io_string);
int8_t eval_prog_next();

#ifdef __cplusplus
}
#endif

#endif // __EVAL_H__
