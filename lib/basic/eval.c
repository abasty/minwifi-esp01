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
    char *chars;
    bool allocated;
} string_t;

void string_set(string_t *string, char *chars, bool allocated)
{
    if (string->allocated && string->chars != chars)
        free(string->chars);
    string->chars = chars;
    string->allocated = allocated;
}

void string_normalize(string_t *string)
{
    if (!string->chars)
    {
        string->allocated = false;
    }
    else if (*string->chars == 0)
    {
        if (string->allocated)
        {
            free(string->chars);
            string->allocated = false;
        }
        string->chars = 0;
    }
}

void string_slice(string_t *string, uint16_t start, uint16_t end)
{
    string_normalize(string);

    if (!string->chars)
        return;

    uint16_t len = strlen(string->chars);

    if (end == 0)
    {
        end = len;
    }

    if (start > end || start > len)
    {
        string_set(string, 0, false);
        return;
    }

    if (end > len || end == 0)
    {
        end = len;
    }

    uint16_t slice_len = end - start + 1;
    char *slice = (char *)malloc(slice_len + 1);

    if (!slice)
    {
        string_set(string, 0, false);
        return;
    }

    char *src = string->chars + start - 1;
    char *dst = slice;

    while (slice_len > 0)
    {
        *dst++ = *src++;
        slice_len--;
    }
    *dst = 0;

    string_set(string, slice, true);
}

void string_concat(string_t *string1, string_t *string2)
{
    string_normalize(string2);

    if (!string2->chars)
        return;

    if (!string1->chars)
    {
        string_set(string1, string2->chars, string2->allocated);
        return;
    }

    char *concat = (char *)malloc(strlen(string1->chars) + strlen(string2->chars) + 1);
    char *dst = concat;
    char *src = string1->chars;
    while (*src) *dst++ = *src++;
    src = string2->chars;
    while (*src) *dst++ = *src++;
    *dst = 0;
    string_set(string1, concat, true);
}

typedef struct
{
    prog_t *pc;
    bool do_eval;
    bool running;
    bool inputting;
    uint8_t *read_ptr;
    uint8_t token;
    float number;
    string_t string;
    char *var_ref;
    var_t *input_var;
    uint8_t input_var_token;
} eval_state_t;

eval_state_t bstate;

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
    if (*bstate.read_ptr != c)
    {
        return false;
    }
    bstate.token = *bstate.read_ptr++;
    return true;
}

bool eval_token_one_of(const char *set)
{
    char c = *bstate.read_ptr;

    while (*set && c != *set)
    {
        set++;
    }
    if (!*set)
    {
        return false;
    }
    bstate.token = c;
    bstate.read_ptr++;
    return true;
}

bool eval_string_expr();
bool eval_factor();
bool eval_expr();

bool eval_code()
{
    if (eval_token(TOKEN_KEYWORD_CODE) && eval_string_expr())
    {
        if (bstate.do_eval)
        {
            bstate.number = bstate.string.chars ? *bstate.string.chars : 0;
        }
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
        *write_value_ptr++ = *bstate.read_ptr++;
        *write_value_ptr++ = *bstate.read_ptr++;
        *write_value_ptr++ = *bstate.read_ptr++;
        *write_value_ptr++ = *bstate.read_ptr++;
    }
    else if (eval_token(TOKEN_VARIABLE_NUMBER))
    {
        if (bstate.do_eval)
        {
            var_t *var = bmem_var_find((char *)bstate.read_ptr - 1);
            if (var)
            {
                value = var->number;
            }
            else
            {
                value = 0;
            }
        }
        bstate.read_ptr += strlen((char *)bstate.read_ptr) + 1;
    }
    else
    {
        return false;
    }

    bstate.number = value;
    if (minus)
    {
        bstate.number = -bstate.number;
    }
    return true;
}

bool eval_function()
{
    if (!eval_token_one_of((char *)functions))
    {
        return false;
    }
    uint8_t f = bstate.token;
    if (!eval_factor())
    {
        return false;
    }
    switch (f)
    {
    case TOKEN_KEYWORD_ABS:
        bstate.number = fabsf(bstate.number);
        break;
    case TOKEN_KEYWORD_ACS:
        bstate.number = acosf(bstate.number);
        break;
    case TOKEN_KEYWORD_ASN:
        bstate.number = asinf(bstate.number);
        break;
    case TOKEN_KEYWORD_ATN:
        bstate.number = atanf(bstate.number);
        break;
    case TOKEN_KEYWORD_BIN:
        // FIXME
        break;
    case TOKEN_KEYWORD_COS:
        bstate.number = cosf(bstate.number);
        break;
    case TOKEN_KEYWORD_EXP:
        bstate.number = expf(bstate.number);
        break;
    case TOKEN_KEYWORD_INT:
        bstate.number = truncf(bstate.number);
        break;
    case TOKEN_KEYWORD_LN:
        bstate.number = logf(bstate.number);
        break;
    case TOKEN_KEYWORD_SGN:
        bstate.number = (bstate.number > 0) - (bstate.number < 0);
        break;
    case TOKEN_KEYWORD_SIN:
        bstate.number = sinf(bstate.number);
        break;
    case TOKEN_KEYWORD_SQR:
        bstate.number = sqrtf(bstate.number);
        break;
    case TOKEN_KEYWORD_TAN:
        bstate.number = tanf(bstate.number);
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
        acc = bstate.number;
        // TODO: Manage / and % by zero (inf)
        while (eval_token_one_of("*/%"))
        {
            uint8_t op = bstate.token;
            result = eval_factor();
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '*':
                acc *= bstate.number;
                break;
            case '/':
                acc /= bstate.number;
                break;
            case '%':
                if ((int)(truncf(bstate.number)) != 0)
                {
                   acc = (int)(truncf(acc)) % (int)(truncf(bstate.number));
                }
                else
                {
                    acc = INFINITY;
                }
                break;
            }
            bstate.number = acc;
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
        acc = bstate.number;
        while (eval_token_one_of("+-|&"))
        {
            uint8_t op = bstate.token;
            result = eval_term();
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '+':
                acc += bstate.number;
                break;
            case '-':
                acc -= bstate.number;
                break;
            case '&':
                acc = (int)(truncf(acc)) & (int)(truncf(bstate.number));
                break;
            case '|':
                acc = (int)(truncf(acc)) | (int)(truncf(bstate.number));
                break;
            }
            bstate.number = acc;
        }
    }
    return result;
}

bool eval_string_const()
{
    if (!eval_token(TOKEN_STRING))
        return false;

    string_set(&bstate.string, (char *)(bstate.read_ptr), false);
    while(*bstate.read_ptr++);
    return true;
}

bool eval_string_var()
{
    if (!eval_token(TOKEN_VARIABLE_STRING))
        return false;

    if (bstate.do_eval)
    {
        var_t *var = bmem_var_find((char *)bstate.read_ptr - 1);
        if (var)
        {
            string_set(&bstate.string, var->string, false);
        }
        else
        {
            string_set(&bstate.string, 0, false);
        }
    }
    while(*bstate.read_ptr++);
    return true;
}

bool eval_string_term()
{
    bool result =
        eval_string_const() ||
        eval_string_var() ||
        (eval_token('(') && eval_string_expr() && eval_token(')'));

    if (!result)
        return false;

    // Optional range
    if (!eval_token('('))
        return true;

    uint16_t start = 1;
    uint16_t end = 0;

    if (eval_expr())
    {
        if (bstate.number < 1)
            return false;

        start = bstate.number;
        end = start;
    }

    if (eval_token(TOKEN_KEYWORD_TO))
    {
        if (eval_expr())
        {
            if (bstate.number < 1)
                return false;

            end = bstate.number;
        }
        else
        {
            end = 0;
        }
    }

    if (!eval_token(')'))
        return false;

    if (bstate.do_eval)
    {
        string_slice(&bstate.string, start, end);
    }

    return result;
}

bool eval_string_expr()
{
    bool result = true;
    if ((result = eval_string_term()))
    {
        string_t string1 = bstate.string;
        string_normalize(&string1);

        while(eval_token('+'))
        {
            result = eval_string_term();
            if (!result)
                break;

            if (bstate.do_eval)
            {
                string_concat(&string1, &bstate.string);
            }
        }
        if (bstate.do_eval)
        {
            string_set(&bstate.string, string1.chars, string1.allocated);
        }
    }
    return result;
}

bool eval_variable_ref()
{
    bstate.var_ref = (char *)bstate.read_ptr;

    if (!eval_token_one_of((char *)variables))
        return false;

    // Pass variable chars until zero
    while (*bstate.read_ptr++)
        ;

    return true;
}

bool eval_let()
{
    if (!eval_token(TOKEN_KEYWORD_LET))
        return false;

    if (!eval_variable_ref())
        return false;

    if (!eval_token('='))
        return false;

    char *name = bstate.var_ref;
    uint8_t token = *((uint8_t *)name);

    if (token == TOKEN_VARIABLE_NUMBER)
    {
        if (!eval_expr())
            return false;

        if (bstate.do_eval)
        {
            if (bmem_var_number_set(name, bstate.number) == 0)
                return false;
        }
        return true;
    }
    else if (token == TOKEN_VARIABLE_STRING)
    {
        if (!eval_string_expr())
            return false;

        if (bstate.do_eval)
        {
            if (bmem_var_string_set(name, bstate.string.chars) == 0)
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

int8_t eval_input_store(char *io_string)
{
    if (strlen(io_string) == 0)
        return BERROR_NONE;

    eval_input_mode(false);

    if (bstate.input_var_token == TOKEN_VARIABLE_NUMBER)
    {
        char *end_ptr = 0;
        float value = strtof(io_string, &end_ptr);
        if (end_ptr - io_string != strlen(io_string))
            return BERROR_SYNTAX;

        if (bmem_var_number_set(bstate.input_var->name, value) == 0)
            return BERROR_MEMORY;

        return BERROR_NONE;
    }
    else if (bstate.input_var_token == TOKEN_VARIABLE_STRING)
    {
        if (bmem_var_string_set(bstate.input_var->name, io_string) == 0)
            return BERROR_MEMORY;

        return BERROR_NONE;
    }
    return BERROR_SYNTAX;
}

bool eval_input()
{
    if (!eval_token(TOKEN_KEYWORD_INPUT))
        return false;

    if (eval_string_const())
    {
        if (bstate.do_eval)
        {
            bio->print_string(bstate.string.chars);
        }

        if (!eval_token(','))
            return false;
    }

    if (!eval_variable_ref())
        return false;

    if (bstate.do_eval)
    {
        bstate.input_var = 0;
        bstate.input_var_token = bstate.token;

        // Create input variable
        if (bstate.input_var_token == TOKEN_VARIABLE_NUMBER)
        {
            // Create variable with default value of 0
            bstate.input_var = bmem_var_number_set(bstate.var_ref, 0);
        }
        else if (bstate.input_var_token == TOKEN_VARIABLE_STRING)
        {
            bstate.input_var = bmem_var_string_set(bstate.var_ref, 0);
        }
        if (bstate.input_var == 0)
            return false;

        // Go to input mode
        eval_input_mode(true);
    }
    return true;
}

bool eval_print()
{
    if (!eval_token(TOKEN_KEYWORD_PRINT))
        return false;

    bool result = true;
    while (true)
    {
        if (eval_string_expr())
        {
            if (bstate.do_eval)
            {
                bio->print_string(bstate.string.chars ? bstate.string.chars : "");
            }
        }
        else if (eval_expr())
        {
            if (bstate.do_eval)
            {
                bio->print_float(bstate.number);
            }
        }
        else
        {
            result = false;
        }
        if (result && eval_token_one_of(";,"))
        {
            if (bstate.token == ',' && bstate.do_eval)
            {
                bio->print_string(" ");
            }
        }
        else
        {
            break;
        }
    }

    if (bstate.do_eval)
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

    if (bstate.do_eval)
    {
        bio->print_integer("Program: %d line(s).\r\n", prog_tree.count);
        bio->print_integer("Variables: %d symbols(s).\r\n", vars.count);
        btree_node_print(&prog_tree, prog_tree.root);
    }

    return true;
}

bool eval_running()
{
    return bstate.running;
}

bool eval_inputting()
{
    return bstate.inputting;
}

void eval_input_mode(bool mode)
{
    bstate.inputting = mode;
}

int8_t eval_prog_next()
{
    int8_t err = BERROR_NONE;

    if (bstate.pc)
    {
        err = eval_prog(bstate.pc, true);
        if (err == BERROR_NONE)
        {
            bstate.pc = bmem_prog_next_line(bstate.pc);
        }
        else
        {
            bio->print_integer("Error %d", -err);
            bio->print_integer(" on line %d", bstate.pc->line_no);
        }
    }
    else
    {
        bstate.pc = 0;
        bstate.running = false;
        err = BERROR_NONE;
    }
    return err;
}

bool eval_run()
{
    if (!eval_token(TOKEN_KEYWORD_RUN))
        return false;

    if (!bstate.do_eval)
        return true;

    bmem_vars_clear();

    bstate.pc = bmem_prog_first_line();
    bstate.running = true;

    return eval_prog_next() == BERROR_NONE;
}

bool eval_clear()
{
    if (!eval_token(TOKEN_KEYWORD_CLEAR))
        return false;

    if (bstate.do_eval)
    {
        bmem_vars_clear();
    }

    return true;
}

bool eval_new()
{
    if (!eval_token(TOKEN_KEYWORD_NEW))
        return false;

    if (bstate.do_eval)
    {
        bmem_prog_new();
    }

    return true;
}

int8_t eval_prog(prog_t *prog, bool do_eval)
{
    bstate.do_eval = do_eval;
    bstate.read_ptr = prog->line;
    bstate.token = 0;
    bstate.string.allocated = 0;
    bstate.string.chars = 0;

    bool eval =
        eval_cls() ||
        eval_print() ||
        eval_list() ||
        eval_run() ||
        eval_new() ||
        eval_clear() ||
        eval_let() ||
        eval_input();

    // Free memory if needed
    string_set(&bstate.string, 0, false);

    if (!eval)
        return BERROR_SYNTAX;

    return BERROR_NONE;
}
