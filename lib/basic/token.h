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

#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYWORD_END_TAG         ((uint8_t)  0b10000000)
#define VARIABLE_STRING_TAG     ((uint32_t) 0x80000000)

#define TOKEN_KEYWORD           ((uint8_t) 0b10000000)
#define TOKEN_NUMBER            ((uint8_t) 0b01000000)
#define TOKEN_STRING            ((uint8_t) 0b00100000)
#define TOKEN_VARIABLE_NUMBER   ((uint8_t) 0b00010000)
#define TOKEN_VARIABLE_STRING   ((uint8_t) 0b00010001)

typedef struct {
    uint16_t line_no;
    uint8_t *read_ptr;
    uint8_t *write_ptr;
} t_tokenizer_state;

int8_t tokenize(t_tokenizer_state *state, char *line);
char *untokenize(uint8_t *input);

uint8_t token_get_next(t_tokenizer_state *state);
float token_number_get_value(t_tokenizer_state *state);
char* token_string_get_value(t_tokenizer_state *state);

#ifdef __cplusplus
}
#endif

#endif // __TOKEN_H__
