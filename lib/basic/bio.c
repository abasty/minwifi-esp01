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
#include <string.h>
#include <stdlib.h>

#ifdef MINITEL
#include "tty-minitel.h"
#else
#include "tty-vt100.h"
#endif

#include "berror.h"
#include "bmemory.h"
#include "token.h"
#include "eval.h"
#include "bio.h"
#include "os.h"

#include "keywords.c-static"
#include "token.c-static"
#include "bmemory.c-static"
#include "string.c-static"
#include "eval.c-static"
#include "os.c-static"

bastos_io_t *bio = 0;

void bastos_init(bastos_io_t *_io)
{
    bio = _io;
    bmem_init(malloc(BASTOS_MEMORY_SIZE), BASTOS_MEMORY_SIZE);
}

void bastos_done()
{
    free(bmem);
    bmem = 0;
    bio = 0;
}

bool bastos_is_reset()
{
    return bmem == 0 || bmem->bstate.reset;
}

static void bastos_handle_ctrl_c()
{
    bastos_stop();
    *bmem->io_buffer = 0;
    bio->print_string("**Break**\r\n");
}

size_t bastos_send_keys(const char *keys, size_t n)
{
    size_t m = 0;
    uint8_t *src = (uint8_t *)keys;
    uint8_t *dst = bmem->io_buffer;

    // If no keys, do nothing
    if (n == 0 || src == 0 || *src == 0)
    {
        return 0;
    }

    // If key is Ctrl+C, stop the program
    if (*src == 3)
    {
        bastos_handle_ctrl_c();
        return 1;
    }

    // If running and not inputting, store the key in inkey state
    if (eval_running() && !eval_inputting())
    {
        bmem->bstate.inkey = (char ) *src;
        return 1;
    }

    // Find the terminal 0 in io buffer
    for (; *dst; dst++)
        ;

    // Copy keys at the end of io buffer
    size_t size = dst - bmem->io_buffer;
    while (size < IO_BUFFER_SIZE - 1 && *src && n > 0)
    {
        if (*src == 3)
        {
            bastos_stop();
            *bmem->io_buffer = 0;
            return 1;
        }
        else if (*src == '\r')
        {
            *dst++ = '\n';
            src++;
            bio->print_string("\r\n");
        }
        else if (*src == 127)
        {
            if (dst - bmem->io_buffer >= 1 && *(dst - 1) != '\n')
            {
                dst--;
                *dst = 0;
                bio->print_string(DEL);
            }
        }
        else
        {
            uint8_t *c = dst;
            *dst++ = *src++;
            *dst = 0;
            size++;
            bio->print_string((char *) c);
        }
        size = dst - bmem->io_buffer;
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
    uint8_t *next = bmem->io_buffer;
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
    uint8_t *dst = bmem->io_buffer;

    // Manage INPUT command
    if (eval_inputting())
    {
        err = eval_input_store((char *) bmem->io_buffer);
        goto finalize;
    }

    // Tokenize command and handle tokenize error case
    tokenizer_state_t line;
    err = tokenize(&line, (char *) bmem->io_buffer);
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
        bool is_load = prog->line[0] == TOKEN_KEYWORD_LOAD;
        err = eval_prog(prog, true);
        if (!is_load)
        {
            bmem_prog_line_free(prog);
        }
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

int8_t bastos_save(const char *name)
{
    int fd = bio->bopen(name, B_CREAT | B_RDWR);
    if (fd < 0) {
        goto err;
    }

    // save prog
    // Write prog total size
    uint16_t prog_size = bmem->prog_end - bmem->prog_start;
    if (bio->bwrite(fd, &prog_size, sizeof(prog_size)) < 0) {
        goto err;
    }

    // Write prog
    if (bio->bwrite(fd, bmem->prog_start, prog_size) < 0) {
        goto err;
    }

    // save vars
    // Write vars total size
    uint16_t vars_size = bmem->vars_end - bmem->vars_start;
    if (bio->bwrite(fd, &vars_size, sizeof(vars_size)) < 0) {
        goto err;
    }

    // Write vars
    if (bio->bwrite(fd, bmem->vars_start, vars_size) < 0) {
        goto err;
    }

    bio->bclose(fd);
    return 0;

err:
    if (fd >= 0) {
        bio->bclose(fd);
    }
    return -1;
}

int8_t bastos_load(const char *name)
{
    int8_t err = BERROR_NONE;
    int fd = bio->bopen(name, B_RDONLY);

    if (fd < 0)
        return BERROR_IO;

    bastos_prog_new();

    int bread;

    // load prog
    uint16_t prog_size;
    bread = bio->bread(fd, &prog_size, sizeof(prog_size));
    if (bread != sizeof(prog_size))
    {
        err = BERROR_IO;
        goto finalize;
    }
    if (prog_size >= bmem->vars_start - bmem->prog_start)
    {
        err = BERROR_IO;
        goto finalize;
    }
    if (bio->bread(fd, bmem->prog_start, prog_size) != prog_size)
    {
        err = BERROR_IO;
        goto finalize;
    }
    bmem->prog_end = bmem->prog_start + prog_size;
    bmem_strings_clear();

    // load vars
    uint16_t vars_size;
    bread = bio->bread(fd, &vars_size, sizeof(vars_size));
    if (bread != sizeof(vars_size))
    {
        err = BERROR_IO;
        goto finalize;
    }
    if (vars_size >= bmem->vars_end - bmem->prog_end)
    {
        err = BERROR_IO;
        goto finalize;
    }
    bmem->vars_start = bmem->vars_end - vars_size;
    if (bio->bread(fd, bmem->vars_start, vars_size) != vars_size)
    {
        err = BERROR_IO;
        bmem_vars_clear();
        goto finalize;
    }

finalize:
    bio->bclose(fd);
    bmem->bstate.read_ptr = 0;
    return err;
}

void bastos_stop()
{
    eval_stop();
}

void bastos_loop()
{
    if (bmem->bstate.reset)
        return;

    if (eval_running() && !eval_inputting())
    {
        eval_prog_next();
        if (bmem->bstate.reset)
            return;

        if (!eval_running())
        {
            bio->print_string("Ready\r\n");
        }
        return;
    }
    bastos_input();
}
