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
#include "eval.h"

#define BASTOS_MEMORY_SIZE (16 * 1024)
#define BASTOS_MEMORY_ALIGN (sizeof(uint32_t))

#define EVAL_RETURNS_SIZE 32

static inline int bmem_align4(int size)
{
    return (size + BASTOS_MEMORY_ALIGN - 1) & ~(BASTOS_MEMORY_ALIGN - 1);
}

typedef struct
{
    float limit;
    float step;
    prog_t *for_line;
} loop_t;

typedef struct
{
    uint16_t line_no;
} return_t;

// Bastos evaluation state
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
    char *string;
    prog_buffer_t token_buffer;
} eval_state_t;

// Bastos low memory system variables
typedef struct {
    uint8_t *prog_start;
    uint8_t *prog_end;
    uint8_t *strings_end;
    uint8_t *vars_start;
    uint8_t *vars_end;
    eval_state_t bstate;
    loop_t loops['Z' - 'A' + 1];
    return_t returns[EVAL_RETURNS_SIZE];
} bmem_t;

static void bmem_init(uint8_t *mem, uint16_t size);

// prog related functions
static void bmem_prog_line_free(prog_t *prog);
static prog_t *bmem_prog_line_new(uint16_t line_no, uint8_t *line, uint16_t len);
static prog_t *bmem_prog_first_line();
static prog_t *bmem_prog_next_line(prog_t *prog);
static prog_t *bmem_prog_get_line_or_next(uint16_t line_no);

// var related functions
static void bmem_vars_clear();
static var_t *bmem_var_string_set(const char *name, char *value);
static var_t *bmem_var_number_set(const char *name, float value);
static var_t *bmem_var_first();
static var_t *bmem_var_next(var_t *var);

// string related functions
static char *bmem_string_alloc(uint16_t size);
static void bmem_strings_clear();

static void string_set(char **string, char *chars);
static void string_slice(char **string, uint16_t start, uint16_t end);
static void string_concat(char **string1, char *string2);


#endif // __BMEMORY_H__
