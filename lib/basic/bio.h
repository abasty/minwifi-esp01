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

#ifndef __BIO_H__
#define __BIO_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "keywords.h"
#include "berror.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define B_RDONLY  00
#define B_WRONLY  01
#define B_RDWR    02
#define B_CREAT   0100
#define B_TRUNC   01000
#define B_APPEND  02000

typedef struct {
    uint16_t line_no;
    uint16_t len;
    uint8_t line[0];
} prog_t;

typedef struct {
    uint8_t token;
    uint8_t dim_count;    // 0 for simple vars
    uint16_t name_ofs;    // Offset of the var name in var.bytes array
    union {
        uint32_t dims[0]; // size of each dimension. Do not exists in simple vars
        float numbers[0]; // 1st element at numbers[dim_count], sizeof(float) == sizeof(uint32_t)
        uint8_t bytes[0]; // 1st element at bytes[dim_count * size_of(uint32_t)]
        char string[0];   // Single string for simple vars
    };
} var_t;

static inline char *name_of_var(var_t *var)
{
    return (char *)var->bytes + var->name_ofs;
}

void bastos_init(void);
void bastos_done(void);
bool bastos_is_reset(void);

size_t bastos_send_keys(const char *keys, size_t n, bool echo);
void bastos_loop(void);
bool bastos_running(void);
void bastos_stop(void);

int8_t bastos_save(const char *name);
int8_t bastos_load(const char *name);

void bastos_prog_new(void);
var_t *bastos_var_get(const char *name);

// void bmem_test();

#ifdef __cplusplus
}
#endif

#endif // __BIO_H__
