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

typedef int print_string_t(char *s);
typedef int print_float_t(float f);
typedef int print_format_integer_t(char *format, int i);
typedef void echo_newline_t();
typedef void cls_t();
typedef int bopen_t(const char *pathname, int flags);
typedef int bclose_t(int fd);
typedef int bwrite_t(int fd, const void *buf, int count);
typedef int bread_t(int fd, void *buf, int count);

typedef struct
{
    bopen_t *bopen;
    bclose_t *bclose;
    bwrite_t *bwrite;
    bread_t *bread;
    print_string_t *print_string;
    print_float_t *print_float;
    print_format_integer_t *print_integer;
    echo_newline_t *echo_newline;
    cls_t *cls;
} bastos_io_t;

void bastos_init(bastos_io_t *_io);

size_t bastos_send_keys(char *keys, size_t n);
void bastos_loop();
bool bastos_running();

int8_t bastos_save(char *name);
int8_t bastos_load(char *name);

#ifdef __cplusplus
}
#endif

#endif // __BIO_H__
