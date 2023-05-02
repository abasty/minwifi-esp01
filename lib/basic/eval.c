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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "berror.h"
#include "bmemory.h"
#include "token.h"
#include "keywords.h"
#include "eval.h"
#include "bio.h"

typedef struct
{
    prog_t *pc;
    bool do_eval;
    bool running;
    uint8_t *read_ptr;
    uint8_t token;
    float number;
    char *string;
} eval_state_t;

eval_state_t eval_state;

extern bastos_io_t *bio;

extern ds_btree_t prog_tree;
extern ds_btree_t vars;

uint8_t functions[] = {
    TOKEN_KEYWORD_ABS,
    TOKEN_KEYWORD_ACS,
    TOKEN_KEYWORD_ASN,
    TOKEN_KEYWORD_ATN,
    TOKEN_KEYWORD_BIN,
    TOKEN_KEYWORD_COS,
    TOKEN_KEYWORD_EXP,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_LN,
    TOKEN_KEYWORD_SGN,
    TOKEN_KEYWORD_SIN,
    TOKEN_KEYWORD_SQR,
    TOKEN_KEYWORD_TAN,
    0,
};

uint8_t variables[] = {
    TOKEN_VARIABLE_NUMBER,
    TOKEN_VARIABLE_STRING,
    0,
};

bool eval_token(uint8_t c)
{
    if (*eval_state.read_ptr != c)
    {
        return false;
    }
    eval_state.token = *eval_state.read_ptr++;
    return true;
}

bool eval_token_one_of(const char *set)
{
    char c = *eval_state.read_ptr;

    while (*set && c != *set)
    {
        set++;
    }
    if (!*set)
    {
        return false;
    }
    eval_state.token = c;
    eval_state.read_ptr++;
    return true;
}

bool eval_string();
bool eval_factor();
bool eval_expr();

bool eval_code()
{
    if (eval_token(TOKEN_KEYWORD_CODE) && eval_string())
    {
        eval_state.number = eval_state.string[0];
        return true;
    }
    return false;
}

bool eval_number()
{
    bool minus = eval_token('-');
    float value = 0;

    if (eval_token(TOKEN_KEYWORD_PI))
    {
        value = 3.1415926536;
    }
    else if (eval_token(TOKEN_KEYWORD_RND))
    {
        value = (float)((double)rand() / (double)RAND_MAX);
    }
    else if (eval_token(TOKEN_NUMBER))
    {
        uint8_t *write_value_ptr = (uint8_t *)&value;
        *write_value_ptr++ = *eval_state.read_ptr++;
        *write_value_ptr++ = *eval_state.read_ptr++;
        *write_value_ptr++ = *eval_state.read_ptr++;
        *write_value_ptr++ = *eval_state.read_ptr++;
    }
    else if (eval_token(TOKEN_VARIABLE_NUMBER))
    {
        eval_state.read_ptr--;
        if (eval_state.do_eval)
        {
            var_t *var = bmem_var_get((char *)eval_state.read_ptr);
            if (var)
            {
                value = var->number;
            }
            else
            {
                value = 0;
            }
        }
        eval_state.read_ptr += strlen((char *)eval_state.read_ptr) + 1;
    }
    else
    {
        return false;
    }

    eval_state.number = value;
    if (minus)
    {
        eval_state.number = -eval_state.number;
    }
    return true;
}

bool eval_function()
{
    if (!eval_token_one_of((char *)functions))
    {
        return false;
    }
    uint8_t f = eval_state.token;
    if (!eval_factor())
    {
        return false;
    }
    switch (f)
    {
    case TOKEN_KEYWORD_ABS:
        eval_state.number = fabsf(eval_state.number);
        break;
    case TOKEN_KEYWORD_ACS:
        eval_state.number = acosf(eval_state.number);
        break;
    case TOKEN_KEYWORD_ASN:
        eval_state.number = asinf(eval_state.number);
        break;
    case TOKEN_KEYWORD_ATN:
        eval_state.number = atanf(eval_state.number);
        break;
    case TOKEN_KEYWORD_BIN:
        // FIXME
        break;
    case TOKEN_KEYWORD_COS:
        eval_state.number = cosf(eval_state.number);
        break;
    case TOKEN_KEYWORD_EXP:
        eval_state.number = expf(eval_state.number);
        break;
    case TOKEN_KEYWORD_INT:
        eval_state.number = truncf(eval_state.number);
        break;
    case TOKEN_KEYWORD_LN:
        eval_state.number = logf(eval_state.number);
        break;
    case TOKEN_KEYWORD_SGN:
        eval_state.number = (eval_state.number > 0) - (eval_state.number < 0);
        break;
    case TOKEN_KEYWORD_SIN:
        eval_state.number = sinf(eval_state.number);
        break;
    case TOKEN_KEYWORD_SQR:
        eval_state.number = sqrtf(eval_state.number);
        break;
    case TOKEN_KEYWORD_TAN:
        eval_state.number = tanf(eval_state.number);
        break;
    default:
        return false;
    }
    return true;
}

bool eval_factor()
{
    bool result =
        eval_number() ||
        eval_function() ||
        eval_code() ||
        (eval_token('(') && eval_expr() && eval_token(')'));
    return result;
}

bool eval_term()
{
    bool result = true;
    float acc = 0;
    if ((result = eval_factor()))
    {
        acc = eval_state.number;
        // TODO: Manage / and % by zero (inf)
        while (eval_token_one_of("*/%"))
        {
            uint8_t op = eval_state.token;
            result = eval_factor();
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '*':
                acc *= eval_state.number;
                break;
            case '/':
                acc /= eval_state.number;
                break;
            case '%':
                if ((int)(truncf(eval_state.number)) != 0)
                {
                   acc = (int)(truncf(acc)) % (int)(truncf(eval_state.number));
                }
                else
                {
                    acc = INFINITY;
                }
                break;
            }
            eval_state.number = acc;
        }
    }
    return result;
}

// TODO: Add eval_condition (< > <> = <= >=)
// TODO: Add eval_boolean (AND OR)

bool eval_expr()
{
    bool result = true;
    float acc = 0;
    if ((result = eval_term()))
    {
        acc = eval_state.number;
        while (eval_token_one_of("+-|&"))
        {
            uint8_t op = eval_state.token;
            result = eval_term();
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '+':
                acc += eval_state.number;
                break;
            case '-':
                acc -= eval_state.number;
                break;
            case '&':
                acc = (int)(truncf(acc)) & (int)(truncf(eval_state.number));
                break;
            case '|':
                acc = (int)(truncf(acc)) | (int)(truncf(eval_state.number));
                break;
            }
            eval_state.number = acc;
        }
    }
    return result;
}

bool eval_string()
{
    if (!eval_token(TOKEN_STRING))
        return false;

    eval_state.string = (char *)(eval_state.read_ptr);
    while(*eval_state.read_ptr++);
    return true;
}

bool eval_let()
{
    if (!eval_token(TOKEN_KEYWORD_LET))
        return false;

    char *name = (char *)eval_state.read_ptr;
    if (!eval_token_one_of((char *)variables))
        return false;
    uint8_t token = eval_state.token;

    // Pass variable chars until zero
    while (*eval_state.read_ptr++)
        ;

    if (!eval_token('='))
        return false;

    if (token == TOKEN_VARIABLE_NUMBER)
    {
        if (!eval_expr(eval_state))
            return false;

        if (eval_state.do_eval)
        {
            if (bmem_var_number_new(name, eval_state.number) == 0)
                return false;
        }
        return true;
    }

    return false;
}

bool eval_cls()
{
    if (!eval_token(TOKEN_KEYWORD_CLS))
        return false;

    bio->cls();
    return true;
}

bool eval_print()
{
    if (!eval_token(TOKEN_KEYWORD_PRINT))
        return false;

    bool result = true;
    while (true)
    {
        if (eval_string(eval_state))
        {
            if (eval_state.do_eval)
            {
                bio->print_string(eval_state.string);
            }
        }
        else if (eval_expr(eval_state))
        {
            if (eval_state.do_eval)
            {
                bio->print_float(eval_state.number);
            }
        }
        else
        {
            result = false;
        }
        if (result && eval_token_one_of(";,"))
        {
            if (eval_state.token == ',' && eval_state.do_eval)
            {
                bio->print_string(" ");
            }
        }
        else
        {
            break;
        }
    }

    if (eval_state.do_eval)
    {
        bio->print_string("\r\n");
    }

    return result;
}

#define progof(_ds, _item) ((prog_t *)DS_OBJECT_OF(_ds, _item))

void btree_node_print(ds_btree_t *btree, ds_btree_item_t *node)
{
    if (node)
    {
        btree_node_print(btree, node->left);
        bio->print_integer("%d", (int) progof(btree, node)->line_no);
        bio->print_string(untokenize(progof(btree, node)->line));
        bio->print_string("\r\n");
        btree_node_print(btree, node->right);
    }
}

bool eval_list()
{
    if (!eval_token(TOKEN_KEYWORD_LIST))
        return false;

    if (eval_state.do_eval)
    {
        bio->print_integer("Program: %d line(s).\r\n", prog_tree.count);
        bio->print_integer("Variables: %d symbols(s).\r\n", vars.count);
        btree_node_print(&prog_tree, prog_tree.root);
    }

    return true;
}

bool eval_running()
{
    return eval_state.running;
}

int8_t eval_prog_next()
{
    int8_t err = BERROR_NONE;

    if (eval_state.pc)
    {
        err = eval_prog(eval_state.pc, true);
        if (err == BERROR_NONE)
        {
            eval_state.pc = bmem_prog_next_line(eval_state.pc);
        }
        else
        {
            bio->print_integer("Error %d", -err);
            bio->print_integer(" on line %d", eval_state.pc->line_no);
        }
    }
    else
    {
        eval_state.pc = 0;
        eval_state.running = false;
        err = BERROR_NONE;
    }
    return err;
}

bool eval_run()
{
    if (!eval_token(TOKEN_KEYWORD_RUN))
        return false;

    if (!eval_state.do_eval)
        return true;

    bmem_var_new();

    eval_state.pc = bmem_prog_first_line();
    eval_state.running = true;

    return eval_prog_next() == BERROR_NONE;
}

bool eval_clear()
{
    if (!eval_token(TOKEN_KEYWORD_CLEAR))
        return false;

    if (eval_state.do_eval)
    {
        bmem_var_new();
    }

    return true;
}

bool eval_new()
{
    if (!eval_token(TOKEN_KEYWORD_NEW))
        return false;

    if (eval_state.do_eval)
    {
        bmem_prog_new();
    }

    return true;
}

int8_t eval_prog(prog_t *prog, bool do_eval)
{
    eval_state.do_eval = do_eval;
    eval_state.read_ptr = prog->line;
    eval_state.token = 0;

    bool eval =
        eval_cls() ||
        eval_print() ||
        eval_list() ||
        eval_run() ||
        eval_new() ||
        eval_clear() ||
        eval_let();
    // || eval_input() ...

    if (!eval)
    {
        return BERROR_SYNTAX;
    }

    return BERROR_NONE;
}
