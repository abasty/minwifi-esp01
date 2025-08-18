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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef MINITEL
#include "tty-minitel.h"
#else
#include "tty-vt100.h"
#endif

#include "os-private.h"

// External interface (extern functions)
#include "bio.h"
#include "os.h"

// Internal interface (static functions)
#include "berror.h"
#include "bmemory.h"
#include "bdb.h"
#include "token.h"
#include "eval.h"

#include "keywords.c-static"
#include "token.c-static"
#include "bmemory.c-static"
#include "string.c-static"
#include "eval.c-static"
#include "os.c-static"
#include "bdb.c-static"

void bastos_init(void)
{
    bmem_init(malloc(BASTOS_MEMORY_SIZE), BASTOS_MEMORY_SIZE);
}

void bastos_done()
{
    hal_net_disconnect(DB_MIN_SET, bmem->bstate.sock);
    hal_net_disconnect(DB_FTP_SET, -1);
    hal_wifi_disconnect();
    hal_speed(TOKEN_KEYWORD_SLOW);
    free(bmem);
    bmem = 0;
}

bool bastos_is_reset()
{
    return bmem == 0 || bmem->bstate.reset;
}

static void bastos_handle_ctrl_c()
{
    bastos_stop();
    *bmem->io_buffer = 0;
    os_redir_print_string("**Break**\r\n");
}

void bastos_send_keys(const char *keys, size_t n, bool echo)
{
    uint8_t *src = (uint8_t *)keys;
    uint8_t *dst = bmem->io_buffer;

    // If no keys, do nothing
    if (n == 0 || src == 0 || *src == 0)
    {
        return;
    }

    // If key is Ctrl+C, stop the program
    if (*src == 3)
    {
        bastos_handle_ctrl_c();
        return;
    }

    // If running and not inputting, store the key in inkey state
    if (eval_running() && !eval_inputting())
    {
        bmem->bstate.inkey = (char ) *src;
        return;
    }

    // Find the terminal 0 in io buffer
    for (; *dst; dst++)
        ;

    // Copy keys at the end of io buffer
    size_t size = dst - bmem->io_buffer;

    // If buffer is full, remove the last char
    if (size == IO_BUFFER_SIZE - 1 && *src == 127)
    {
        size--;
        dst--;
        *dst = 0;
        if (echo) os_redir_print_string(DEL);
        return;
    }

    while (size < IO_BUFFER_SIZE - 1 && *src && n > 0)
    {
        if (*src == 3)
        {
            bastos_stop();
            *bmem->io_buffer = 0;
            return;
        }
        else if (*src == '\r')
        {
            *dst++ = '\n';
            src++;
            if (echo) os_redir_print_string("\r\n");
        }
        else if (*src == 127)
        {
            if (dst - bmem->io_buffer >= 1 && *(dst - 1) != '\n')
            {
                dst--;
                *dst = 0;
                if (echo) os_redir_print_string(DEL);
            }
        }
        else
        {
            uint8_t *c = dst;
            *dst++ = *src++;
            *dst = 0;
            size++;
            if (echo) os_redir_print_string((char *) c);
        }
        size = dst - bmem->io_buffer;
        n--;
    }
    *dst = 0;
}

static int8_t bastos_input()
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
        os_redir_print_integer("Error %d\r\n", (int)-err);
    }

    return err;
}

static int32_t os_save_ascii(int fd)
{
    os_set_redirect(fd);
    prog_t *prog = bmem_prog_first_line();
    while (prog)
    {
        os_redir_print_integer("%d ", prog->line_no);
        untokenize(prog->line);
        os_redir_print_string("\n");
        prog = bmem_prog_next_line(prog);
    }
    os_set_redirect(-1);
    return 0;
}

static int32_t os_save_bin(int fd, int16_t type)
{
    if (type == FILE_TYPE_BST) {
        // save prog
        // Write prog total size
        uint16_t prog_size = bmem->prog_end - bmem->prog_start;
        if (hal_write(fd, &prog_size, sizeof(prog_size)) < 0) {
            return -1;
        }

        // Write prog
        if (hal_write(fd, bmem->prog_start, prog_size) < 0) {
            return -1;
        }
    } else {
        // Save empty prog
        uint16_t prog_size = 0;
        if (hal_write(fd, &prog_size, sizeof(prog_size)) < 0) {
            return -1;
        }
    }

    // save vars
    // Write vars total size
    uint16_t vars_size = bmem->vars_end - bmem->vars_start;
    if (hal_write(fd, &vars_size, sizeof(vars_size)) < 0) {
        return -1;
    }

    // Write vars
    if (hal_write(fd, bmem->vars_start, vars_size) < 0) {
        return -1;
    }

    return 0;
}

static int os_eval_string(char *str)
{
    // Tokenize command and handle tokenize error case
    tokenizer_state_t line;
    int err = tokenize(&line, str);
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
    // Handle error
    if (err != BERROR_NONE && line.line_no != 0)
        os_redir_print_integer("Line %d: ", (int)line.line_no);

    return err;

}

static int8_t os_load_ascii(int fd)
{
    // Read lines from file
    char line[IO_BUFFER_SIZE + 1] = {0};
    char *read_ptr = line;
    int32_t remains = hal_read(fd, line, IO_BUFFER_SIZE);
    while (remains > 0)
    {
        // Find LF in line
        char *lf = strchr(line, '\n');
        // If no LF in line, return BERROR_IO
        if (lf == 0)
            return BERROR_IO;
        // replace LF with 0
        *lf = 0;
        // If CR just before LF, replace CR with 0
        if (lf > line && *(lf - 1) == '\r')
            *(lf - 1) = 0;
        // If line is empty, continue
        int err = os_eval_string(read_ptr);
        if (err < 0)
            return err;

        remains -= (lf + 1 - line);
        memmove(line, lf + 1, remains + 1);
        int n = hal_read(fd, line + remains, IO_BUFFER_SIZE - remains);
        if (n < 0)
            return BERROR_IO;
        remains += n;
    }
    if (remains < 0)
        return BERROR_IO;

    return BERROR_NONE;
}

static int8_t os_load_bin(int fd, int16_t type)
{
    int8_t err = BERROR_NONE;

    // load prog
    uint16_t prog_size;
    int bread = hal_read(fd, &prog_size, sizeof(prog_size));
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
    if (type == FILE_TYPE_VAR && prog_size != 0)
    {
        err = BERROR_IO;
        goto finalize;
    }
    if (type == FILE_TYPE_BST)
    {
        if (hal_read(fd, bmem->prog_start, prog_size) != prog_size)
        {
            err = BERROR_IO;
            goto finalize;
        }
        bmem->prog_end = bmem->prog_start + prog_size;
        bmem_strings_clear();
    }

    // load vars
    uint16_t vars_size;
    bread = hal_read(fd, &vars_size, sizeof(vars_size));
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
    if (hal_read(fd, bmem->vars_start, vars_size) != vars_size)
    {
        err = BERROR_IO;
        bmem_vars_clear();
        goto finalize;
    }

finalize:
    return err;
}

int8_t bastos_save(const char *i_name)
{
    int16_t type = -1;
    const char *name = os_filename(i_name, &type);
    if (name == 0 || type < 0)
        return -1;

    int fd = hal_open(name, B_CREAT | B_RDWR);
    if (fd < 0)
        goto err;

    if (type == FILE_TYPE_BST || type == FILE_TYPE_VAR)
        if (os_save_bin(fd, type) < 0)
            goto err;

    if (type == FILE_TYPE_BAS)
        if (os_save_ascii(fd) < 0)
            goto err;

    hal_close(fd);
    return 0;

err:
    if (fd >= 0) {
        hal_close(fd);
        hal_erase(name);
    }
    return -1;
}

int8_t bastos_load(const char *i_name)
{
    int16_t type = -1;
    const char *name = os_filename(i_name, &type);
    if (name == 0 || type < 0)
        return BERROR_RANGE;

    int8_t err = BERROR_NONE;
    int fd = hal_open(name, B_RDONLY);

    if (fd < 0)
        return BERROR_IO;

    // Remove existing blocks
    if (type == FILE_TYPE_BAS || type == FILE_TYPE_BST)
        bastos_prog_new();

    if (type == FILE_TYPE_VAR)
        bastos_vars_clear();

    // Load file
    if (type == FILE_TYPE_VAR || type == FILE_TYPE_BST)
        err = os_load_bin(fd, type);

    if (type == FILE_TYPE_BAS)
        err = os_load_ascii(fd);

    hal_close(fd);
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
            os_redir_print_string("Ready\r\n");
        }
        return;
    }
    bastos_input();
}
