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
#include <stdio.h>
#include <math.h>

#include "berror.h"
#include "bmemory.h"
#include "token.h"
#include "keywords.h"
#include "eval.h"

typedef struct
{
    prog_t prog;
    bool do_eval;
    uint8_t *read_ptr;
    uint8_t token;
    float number;
    char *string;
} t_eval_state;

extern ds_btree_t progs;
extern ds_btree_t vars;

bool eval_expr(t_eval_state *state);

bool eval_token(t_eval_state *state, uint8_t c)
{
    if (*state->read_ptr != c)
    {
        return false;
    }
    state->token = *state->read_ptr++;
    return true;
}

bool eval_token_one_of(t_eval_state *state, const char *set)
{
    char c = *state->read_ptr;

    while (*set && c != *set)
    {
        set++;
    }
    if (!*set)
    {
        return false;
    }
    state->token = c;
    state->read_ptr++;
    return true;
}

bool eval_number(t_eval_state *state)
{
    bool minus = eval_token(state, '-');

    if (!eval_token(state, TOKEN_NUMBER))
    {
        return false;
    }

    float value = 0;
    uint8_t *write_value_ptr = (uint8_t *)&value;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    state->number = value;
    if (minus)
    {
        state->number = -state->number;
    }
    return true;
}

bool eval_factor(t_eval_state *state)
{
    bool result =
        eval_number(state) ||
        (eval_token(state, '(') && eval_expr(state) && eval_token(state, ')'));
    return result;
}

bool eval_term(t_eval_state *state)
{
    bool result = true;
    float acc = 0;
    if ((result = eval_factor(state)))
    {
        acc = state->number;
        while (eval_token_one_of(state, "*/%"))
        {
            uint8_t op = state->token;
            result = eval_factor(state);
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '*':
                acc *= state->number;
                break;
            case '/':
                acc /= state->number;
                break;
            case '%':
                acc = (int)(truncf(acc)) % (int)(truncf(state->number));
                break;
            }
            state->number = acc;
        }
    }
    return result;
}

bool eval_expr(t_eval_state *state)
{
    bool result = true;
    float acc = 0;
    if ((result = eval_term(state)))
    {
        acc = state->number;
        while (eval_token_one_of(state, "+/|&"))
        {
            uint8_t op = state->token;
            result = eval_term(state);
            if (!result)
            {
                break;
            }
            switch (op)
            {
            case '+':
                acc += state->number;
                break;
            case '-':
                acc -= state->number;
                break;
            case '&':
                acc = (int)(truncf(acc)) & (int)(truncf(state->number));
                break;
            case '|':
                acc = (int)(truncf(acc)) | (int)(truncf(state->number));
                break;
            }
            state->number = acc;
        }
    }
    return result;
}

bool eval_string(t_eval_state *state)
{
    if (!eval_token(state, TOKEN_STRING))
        return false;

    state->string = (char *)(state->read_ptr);
    state->read_ptr += snprintf(0, 0, "%s", state->read_ptr) + 1;
    return true;
}

bool eval_print(t_eval_state *state)
{
    if (!eval_token(state, TOKEN_KEYWORD_PRINT))
        return false;

    bool result = true;
    while (true)
    {
        if (eval_string(state))
        {
            if (state->do_eval)
            {
                printf("%s", state->string);
            }
        }
        else if (eval_expr(state))
        {
            if (state->do_eval)
            {
                printf("%f", state->number);
            }
        }
        else
        {
            result = false;
        }
        if (result && eval_token_one_of(state, ";,"))
        {
            if (state->token == ',' && state->do_eval)
            {
                printf(" ");
            }
        }
        else
        {
            break;
        }
    }

    if (state->do_eval)
    {
        printf("\n");
    }

    return result;
}

#define progof(_ds, _item) ((prog_t *)DS_OBJECT_OF(_ds, _item))

void btree_node_print(ds_btree_t *btree, ds_btree_item_t *node)
{
    if (node)
    {
        btree_node_print(btree, node->left);
        printf("%u", progof(btree, node)->line_no);
        printf("%s\n", untokenize(progof(btree, node)->line));
        btree_node_print(btree, node->right);
    }
}

bool eval_list(t_eval_state *state)
{
    if (!eval_token(state, TOKEN_KEYWORD_LIST))
        return false;

    if (state->do_eval)
    {
        printf("Program: %zd line(s).\n", progs.count);
        printf("Variables: %zd symbols(s).\n", vars.count);
        btree_node_print(&progs, progs.root);
    }

    return true;
}

int8_t eval_prog(prog_t *prog, bool do_eval)
{
    t_eval_state state = {
        .do_eval = do_eval,
        .prog = *prog,
        .read_ptr = prog->line,
        .token = 0,
    };

    bool eval = eval_print(&state) || eval_list(&state); // || eval_input(&state) ...

    if (eval)
        return BERROR_NONE;

    return BERROR_SYNTAX;
}
