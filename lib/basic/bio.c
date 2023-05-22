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

extern uint8_t token_buffer[TOKEN_LINE_SIZE];

void bastos_init(bastos_io_t *_io)
{
    bio = _io;
    *io_buffer = 0;
    bmem_init();
}

size_t bastos_send_keys(char *keys, size_t n)
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
            // if (*src == '\n')
            // {
            //     src++;
            // }
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

int8_t bastos_input()
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

    bio->echo_newline();

    // Mark first command end with 0 and point to next one
    *next++ = 0;

    // Prepare move of the next commands to buffer start
    uint8_t *src = next;
    uint8_t *dst = io_buffer;

    // Manage INPUT command
    if (eval_inputting())
    {
        err = eval_input_store((char *) io_buffer);
        goto finalize;
    }

    // Tokenize command and handle tokenize error case
    tokenizer_state_t line;
    err = tokenize(&line, io_buffer_char);
    if (err < 0)
        goto finalize;

    // Allocate memory for the prog line
    uint16_t len = line.write_ptr - line.read_ptr;
    prog_t *prog = bmem_prog_line_new(line.line_no, line.read_ptr, len);
    if (prog == 0)
    {
        if (len != 0)
        {
            err = BERROR_MEMORY;
        }
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

    // Handle error
    if (err != BERROR_NONE)
    {
        bio->print_integer("Error %d\r\n", (int)-err);
    }

    return err;
}

bool bastos_running()
{
    return eval_running();
}

int8_t bastos_save(char *name)
{
    int fd = bio->bopen(name, B_CREAT | B_RDWR);
    prog_t *line = bmem_prog_first_line();
    while (line)
    {
        bio->bwrite(fd, &line->line_no, sizeof(line->line_no));
        bio->bwrite(fd, &line->len, sizeof(line->len));
        bio->bwrite(fd, line->line, line->len);
        line = bmem_prog_next_line(line);
    }
    bio->bclose(fd);
    return 0;
}

int8_t bastos_load(char *name)
{
    int8_t err = BERROR_NONE;
    int fd = bio->bopen(name, B_RDONLY);

    if (fd < 0)
        return BERROR_IO;

    bmem_prog_new();

    while (err == BERROR_NONE)
    {
        uint16_t line_no;
        uint16_t len;

        int bread = bio->bread(fd, &line_no, sizeof(line_no));
        if (bread != sizeof(line_no) && bread != 0)
        {
            err = BERROR_IO;
            break;
        }
        if (bread == 0)
            break;

        bread = bio->bread(fd, &len, sizeof(len));
        if (bread != sizeof(line_no))
        {
            err = BERROR_IO;
            break;
        }
        if (line_no == 0 || len == 0 || len >= TOKEN_LINE_SIZE)
        {
            err = BERROR_IO;
            break;
        }
        if (bio->bread(fd, token_buffer, len) != len)
        {
            err = BERROR_IO;
            break;
        }

        prog_t *prog = bmem_prog_line_new(line_no, token_buffer, len);
        if (prog == 0)
        {
            err = BERROR_MEMORY;
            break;
        }

        err = eval_prog(prog, false);
        if (err != BERROR_NONE)
        {
            bmem_prog_line_free(prog);
            break;
        }
    }

    bio->bclose(fd);
    return err;
}

void bastos_loop()
{

    if (eval_running() && !eval_inputting())
    {
        eval_prog_next();
        return;
    }

    bastos_input();
}
