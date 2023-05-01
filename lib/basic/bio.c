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

#include <stdbool.h>

#include "bmemory.h"
#include "token.h"
#include "bio.h"

t_bastos_io *io = 0;

uint8_t io_buffer[IO_BUFFER_SIZE];
char *io_buffer_char = (char *)io_buffer;

void bastos_init(t_bastos_io *_io)
{
    io = _io;
    *io_buffer = 0;
    bmem_init();
}

size_t bastos_handle_keys(char *keys, size_t n)
{
    size_t m = 0;
    uint8_t *src = (uint8_t *)keys;

    // Find the terminal 0 in io buffer
    uint8_t *dst = io_buffer;
    while (*dst)
        dst++;

    // Copy keys at the end of io buffer
    size_t size = dst - io_buffer;
    while (size < IO_BUFFER_SIZE - 1 && *src)
    {
        if (*src == '\r')
        {
            *dst++ = '\n';
            src++;
        }
        else
        {
            *dst++ = *src++;
            size++;
        }
        m++;
    }
    *dst = 0;

    return m;
}

void bastos_loop()
{
    // run a prog step if "running"
    // else get a command and run it
    if (true)
    {
        // Find first command
        uint8_t *command_end = io_buffer;
        while(*command_end && *command_end != '\n')
        {
            command_end++;
        }
        // Mark first command
        *command_end++ = 0;

        // Tokenize
        t_tokenizer_state line;
        int8_t err = tokenize(&line, io_buffer_char);

        // Check syntax

        // If line# is 0, evaluate and remove
    }
}
