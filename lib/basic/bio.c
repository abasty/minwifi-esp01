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

#include "berror.h"
#include "bmemory.h"
#include "token.h"
#include "eval.h"
#include "bio.h"

bastos_io_t *bio = 0;

uint8_t io_buffer[IO_BUFFER_SIZE];
char *io_buffer_char = (char *)io_buffer;

void bastos_init(bastos_io_t *_io)
{
    bio = _io;
    *io_buffer = 0;
    bmem_init();
}

size_t bastos_handle_keys(char *keys, size_t n)
{
    size_t m = 0;
    uint8_t *src = (uint8_t *)keys;
    uint8_t *dst = io_buffer;

    // Find the terminal 0 in io buffer
    for (; *dst; dst++)
        ;

    // Copy keys at the end of io buffer
    size_t size = dst - io_buffer;
    while (size < IO_BUFFER_SIZE - 1 && *src && n > 0)
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
        n--;
        m++;
    }
    *dst = 0;

    return m;
}

int8_t io_run_command()
{
    int8_t err = BERROR_NONE;

    // Find first command end
    uint8_t *next = io_buffer;
    while (*next && *next != '\n')
    {
        next++;
    }
    // If no command: do nothing
    if (*next == 0)
        return BERROR_NONE;
    // Mark first command end with 0 and point to next one
    *next++ = 0;
    // Prepare move of the next commands to buffer start
    uint8_t *src = next;
    uint8_t *dst = io_buffer;
    // Tokenize command
    tokenizer_state_t line;
    err = tokenize(&line, io_buffer_char);
    uint16_t len = line.write_ptr - line.read_ptr;
    if (err < 0 || len == 0)
        goto finalize;
    // Allocate memory for the prog line
    prog_t *prog = bmem_prog_line_new(line.line_no, line.read_ptr, len);
    if (prog == 0)
    {
        err = BERROR_MEMORY;
        goto finalize;
    }
    // Check syntax
    err = eval_prog(prog, false);
    if (err != BERROR_NONE)
    {
        bmem_prog_line_free(prog);
        goto finalize;
    }
    // If line number is 0, evaluate and remove
    if (prog->line_no == 0)
    {
        err = eval_prog(prog, true);
        bmem_prog_line_free(prog);
    }

finalize:
    // remove first command
    while (*src)
    {
        *dst++ = *src++;
    }
    *dst = 0;

    if (err != BERROR_NONE)
    {
        bio->print_integer("Syntax error: %d.\n", (int) -err);
    }

    return err;
}

void bastos_loop()
{
    io_run_command();
    // run a prog step if "running"
    // else get a command and run it
    if (true)
    {
    }
}
