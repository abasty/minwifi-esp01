/*
 * Copyright © 2023-2025 Alain Basty
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

#ifdef ESP32
#define BASTOS_MEMORY_SIZE_KB (56U)
#define BASTOS_DB_SIZE_KB (6U)
#else
#define BASTOS_MEMORY_SIZE_KB (32U)
#define BASTOS_DB_SIZE_KB (4U)
#endif

#define BASTOS_DB_SIZE (BASTOS_DB_SIZE_KB * 1024U)
#define BASTOS_MEMORY_SIZE ((BASTOS_MEMORY_SIZE_KB + BASTOS_DB_SIZE_KB) * 1024U)
#define BASTOS_MEMORY_ALIGN (sizeof(uint32_t))
#define IO_BUFFER_SIZE  (128U)
#define TOKEN_LINE_SIZE (128U)
#define EVAL_RETURNS_SIZE (32U)
#define B_DIM_MAX (16U)
#define B_DIM_RANGE_FLAG (128U)
#define B_NAME_SIZE_MAX (16U)

typedef struct {
    uint16_t line_no;
    uint16_t len;
    uint8_t line[TOKEN_LINE_SIZE];
} prog_buffer_t;

typedef struct
{
    float limit;
    float step;
    prog_t *for_line;
} loop_t;

typedef struct
{
    uint16_t line_no;
    uint16_t offset;
} return_t;

// Bastos evaluation state
typedef struct eval_state_s eval_state_t;
struct eval_state_s {
    prog_t *pc;
    prog_t *prog;
    char *var_ref;
    var_t *input_var;
    uint8_t *read_ptr;
    char *string;
    float number;
    uint16_t pc_offset;

    uint8_t token;
    uint8_t input_var_token;
    int8_t error;
    int8_t sp;
    uint8_t for_sp;

    uint8_t in_goto: 1;
    uint8_t do_eval: 1;
    uint8_t running: 1;
    uint8_t inputting: 1;
    uint8_t reset: 1;
    uint8_t debug: 1;
    uint8_t g_mode: 1;
    uint8_t screen_mode: 1;
    uint8_t else_pending: 1; // set by eval_if()'s false branch when it found
                              // a matching ELSE, consumed by eval_else()

    prog_buffer_t token_buffer;
};

// Bastos low memory system variables
typedef struct {
    uint8_t *prog_start;
    uint8_t *prog_end;
    uint8_t *strings_end;
    uint8_t *vars_start;
    uint8_t *vars_end;
    uint8_t *db_start;
    uint8_t *db_end;
    loop_t loops['Z' - 'A' + 1];
    uint8_t for_stack['Z' - 'A' + 1]; // loop indices, in nesting order
    return_t returns[EVAL_RETURNS_SIZE];
    uint8_t io_buffer[IO_BUFFER_SIZE];
    uint8_t io_cursor; // offset from io_buffer of the edit cursor in the current line
    uint16_t io_edit_line; // set by EDIT, consumed by bastos_input() once the
                            // current command has been removed from io_buffer
    uint16_t io_last_line; // line number of the last line successfully stored,
                            // for Up-arrow recall (0 = none)
    uint8_t io_recall_len;  // > 0 while the last immediate command's raw text
                             // is kept resident at the start of io_buffer for
                             // Up-arrow recall (0 = none pending)
    uint8_t io_no_recall;   // set around a system-injected command (autoexec's
                             // banner fallback) so it isn't mistaken for
                             // something the user typed and offered for recall
    uint16_t list_start;
    char inkey;
    char vkey;
    int8_t ofs_c;
    int8_t ofs_l;
    uint32_t pause_delay;
    uint64_t pause_start_time;
    int sock;
    int output_fd;
    uint8_t output_prefill; // 1 while os_redir_print_* should write into io_buffer
    char output_var[B_NAME_SIZE_MAX];
    eval_state_t bstate;
    uint8_t paper;
    uint8_t ink;
    uint8_t screen[24][40];
} bmem_t;

static void bmem_init(uint8_t *mem, uint16_t size);
static void bmem_screen_clear();

// prog related functions
static void bmem_prog_line_free(prog_t *prog);
static prog_t *bmem_prog_line_new(uint16_t line_no, uint8_t *line, uint16_t len);
static prog_t *bmem_prog_first_line();
static prog_t *bmem_prog_next_line(prog_t *prog);
static prog_t *bmem_prog_prev_line(prog_t *prog);
static prog_t *bmem_prog_get_line_or_next(uint16_t line_no);
static uint16_t bmem_prog_find_label(const char *label_name);

// var related functions
static void bmem_vars_clear();
static var_t *bmem_var_string_set(const char *name, char *value, bool is_cstr);
static var_t *bmem_var_number_set(const char *name, float value);
static var_t *bmem_var_label_set(const char *name, float value);
static var_t *bmem_var_first();
static var_t *bmem_var_next(var_t *var);

// string related functions
static char *bmem_string_alloc(uint16_t size);
static void bmem_strings_clear();

static char *string_cstr(const char *string);
static int string_len(const char *string);
static char *string_dup(const char *string);
static void string_slice(char **string, uint16_t start, uint16_t end);
static void string_concat(char **string1, char *string2);
static uint16_t string_index(char *string1, char *string2, uint16_t index);

static inline int bmem_align4(int size)
{
    return (size + BASTOS_MEMORY_ALIGN - 1) & ~(BASTOS_MEMORY_ALIGN - 1);
}

extern bmem_t *bmem;

#endif // __BMEMORY_H__
