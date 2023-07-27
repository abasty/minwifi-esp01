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

#include "keywords.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define IO_BUFFER_SIZE (256)

#define B_RDONLY  00
#define B_WRONLY  01
#define B_RDWR    02
#define B_CREAT   0100
#define B_TRUNC   01000
#define B_APPEND  02000

#define B_IO_CAT            0
#define B_IO_CLS            1
#define B_IO_RESET          2
#define B_IO_DEL            3
#define B_IO_AT             4
#define B_IO_INK            5
#define B_IO_PAPER          6
#define B_IO_PRINT_STRING   7
#define B_IO_PRINT_FLOAT    8
#define B_IO_PRINT_INTEGER  9
#define B_IO_ERASE          10
#define B_IO_OPEN           11
#define B_IO_CLOSE          12
#define B_IO_READ           13
#define B_IO_WRITE          14


typedef union {
    uint8_t as_uint8;
    int as_int;
    uint32_t as_uint32;
    float as_float;
    char* as_string;
    void* as_ptr;
} bst_io_argv_t;

typedef int bst_io_f(void);

extern bst_io_argv_t bastos_io_argv[];
extern bst_io_f *bastos_io;

static inline void bio_fn(int fn)
{
    bastos_io_argv[0].as_int = fn;
    bastos_io();
}

static inline int bio_open(const char *pathname, int flags)
{
    bastos_io_argv[0].as_int = B_IO_OPEN;
    bastos_io_argv[1].as_string = (char *)pathname;
    bastos_io_argv[2].as_int = flags;
    return bastos_io();
}

static inline int bio_close(int fd)
{
    bastos_io_argv[0].as_int = B_IO_CLOSE;
    bastos_io_argv[1].as_int = fd;
    return bastos_io();
}

static inline int bio_read(int fd, void *buf, int count)
{
    bastos_io_argv[0].as_int = B_IO_READ;
    bastos_io_argv[1].as_int = fd;
    bastos_io_argv[2].as_ptr = buf;
    bastos_io_argv[3].as_int = count;
    return bastos_io();
}

static inline int bio_write(int fd, void *buf, int count)
{
    bastos_io_argv[0].as_int = B_IO_WRITE;
    bastos_io_argv[1].as_int = fd;
    bastos_io_argv[2].as_ptr = buf;
    bastos_io_argv[3].as_int = count;
    return bastos_io();
}

static inline int bio_print_string(char *string)
{
    bastos_io_argv[0].as_int = B_IO_PRINT_STRING;
    bastos_io_argv[1].as_string = string;
    return bastos_io();
}

static inline int bio_print_float(float f)
{
    bastos_io_argv[0].as_int = B_IO_PRINT_FLOAT;
    bastos_io_argv[1].as_float = f;
    return bastos_io();
}

static inline int bio_print_integer(char *format, int i)
{
    bastos_io_argv[0].as_int = B_IO_PRINT_INTEGER;
    bastos_io_argv[1].as_string = format;
    bastos_io_argv[2].as_int = i;
    return bastos_io();
}

static inline int bio_erase(const char *pathname)
{
    bastos_io_argv[0].as_int = B_IO_ERASE;
    bastos_io_argv[1].as_string = (char*) pathname;
    return bastos_io();
}

void bastos_init(bst_io_f *_basto_io);

size_t bastos_send_keys(const char *keys, size_t n);
void bastos_loop();
bool bastos_running();
void bastos_stop();

int8_t bastos_save(const char *name);
int8_t bastos_load(const char *name);

#ifdef __cplusplus
}
#endif

#endif // __BIO_H__
