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

#include "ds_btree.c-static"
#include "token.c-static"
#include "bmemory.c-static"
#include "string.c-static"
#include "eval.c-static"

bastos_io_t *bio = 0;

// bst_io_argv_t bastos_io_argv[4] = {0};
// function0_t *function0_io = 0;

uint8_t io_buffer[IO_BUFFER_SIZE];
char *io_buffer_char = (char *)io_buffer;

extern uint8_t token_buffer[TOKEN_LINE_SIZE];

void bastos_init(bastos_io_t *_io)
{
    bio = _io;
    *io_buffer = 0;
    bmem_init(malloc(BASTOS_MEMORY_SIZE));
}

size_t bastos_send_keys(const char *keys, size_t n)
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
        if (*src == 3)
        {
            bastos_stop();
            *io_buffer = 0;
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
            if (dst - io_buffer >= 1 && *(dst - 1) != '\n')
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
        size = dst - io_buffer;
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

int8_t bastos_save(const char *name)
{
    int fd = bio->bopen(name, B_CREAT | B_RDWR);
    // save prog
    prog_t *line = bmem_prog_first_line();
    while (line)
    {
        bio->bwrite(fd, &line->line_no, sizeof(line->line_no));
        bio->bwrite(fd, &line->len, sizeof(line->len));
        bio->bwrite(fd, line->line, line->len);
        line = bmem_prog_next_line(line);
    }

    // write "end of prog"
    uint32_t zero = 0;
    bio->bwrite(fd, &zero, sizeof(zero));

    // save vars
    // var_t *var = bmem_var_first();
    // while (var)
    // {
    //     uint16_t len = strlen(var->name);
    //     bio->bwrite(fd, &len, sizeof(len));
    //     bio->bwrite(fd, var->name, len);
    //     uint8_t token = var->name[0];
    //     if (token == TOKEN_VARIABLE_STRING)
    //     {
    //         len = var->string ? strlen(var->string) : 0;
    //         bio->bwrite(fd, &len, sizeof(len));
    //         if (len > 0)
    //         {
    //             bio->bwrite(fd, var->string, len);
    //         }
    //     }
    //     else if (token == TOKEN_VARIABLE_NUMBER)
    //     {
    //         bio->bwrite(fd, &var->number, sizeof(var->number));
    //     }
    //     var = bmem_var_next(var);
    // }
    bio->bclose(fd);
    return 0;
}

int8_t bastos_load(const char *name)
{
    int8_t err = BERROR_NONE;
    int fd = bio->bopen(name, B_RDONLY);

    if (fd < 0)
        return BERROR_IO;

    bmem_prog_new();
    bmem_vars_clear();

    int bread;

    // load prog
    while (err == BERROR_NONE)
    {
        uint16_t line_no;
        uint16_t len;

        bread = bio->bread(fd, &line_no, sizeof(line_no));
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

        // End of prog
        if (line_no == 0 && len == 0)
            break;

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

    // load vars
    while (err == BERROR_NONE)
    {
        uint16_t len_name;
        char name[TOKEN_LINE_SIZE];
        var_t *var;

        bread = bio->bread(fd, &len_name, sizeof(len_name));
        if (bread == 0)
            break;

        if (bread != sizeof(len_name) || len_name < 1 || len_name > TOKEN_LINE_SIZE - 1)
        {
            err = BERROR_IO;
            break;
        }
        bread = bio->bread(fd, name, len_name);
        name[len_name] = 0;
        uint8_t token = name[0];
        if (token == TOKEN_VARIABLE_STRING)
        {
            uint16_t len;
            bread = bio->bread(fd, &len, sizeof(len));
            if (bread != sizeof(len))
            {
                err = BERROR_IO;
                break;
            }
            char *string = (char *) malloc(len + 1);
            if (!string)
            {
                err = BERROR_MEMORY;
                break;
            }
            bread = bio->bread(fd, string, len);
            if (bread != len)
            {
                err = BERROR_IO;
                break;
            }
            string[len] = 0;
            var = bmem_var_string_set(name, string);
            free(string);
            if (!var)
            {
                err = BERROR_MEMORY;
                break;
            }
        }
        else if (token == TOKEN_VARIABLE_NUMBER)
        {
            float number;
            bread = bio->bread(fd, &number, sizeof(number));
            if (bread != sizeof(number))
            {
                err = BERROR_IO;
                break;
            }
            var = bmem_var_number_set(name, number);
            if (!var)
            {
                err = BERROR_MEMORY;
                break;
            }
        }
        else
        {
            err = BERROR_IO;
            break;
        }
    }

    bio->bclose(fd);
    return err;
}

void bastos_stop()
{
    eval_stop();
}

void bastos_loop()
{

    if (eval_running() && !eval_inputting())
    {
        eval_prog_next();
        if (!eval_running())
        {
            bio->print_string("Ready\r\n");
        }
        return;
    }

    bastos_input();
}
