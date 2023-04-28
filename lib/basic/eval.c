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
#include <stdlib.h>
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

bool eval_string(t_eval_state *state);
bool eval_factor(t_eval_state *state);
bool eval_expr(t_eval_state *state);

bool eval_code(t_eval_state *state)
{
    if (eval_token(state, TOKEN_KEYWORD_CODE) && eval_string(state))
    {
        state->number = state->string[0];
        return true;
    }
    return false;
}

bool eval_number(t_eval_state *state)
{
    bool minus = eval_token(state, '-');
    float value = 0;

    if (eval_token(state, TOKEN_KEYWORD_PI))
    {
        value = 3.1415926536;
    }
    else if (eval_token(state, TOKEN_KEYWORD_RND))
    {
        value = (float)((double)rand() / (double)RAND_MAX);
    }
    else if (eval_token(state, TOKEN_NUMBER))
    {
        uint8_t *write_value_ptr = (uint8_t *)&value;
        *write_value_ptr++ = *state->read_ptr++;
        *write_value_ptr++ = *state->read_ptr++;
        *write_value_ptr++ = *state->read_ptr++;
        *write_value_ptr++ = *state->read_ptr++;
    }
    else
    {
        return false;
    }

    state->number = value;
    if (minus)
    {
        state->number = -state->number;
    }
    return true;
}

bool eval_function(t_eval_state *state)
{
    if (!eval_token_one_of(state, (char *)functions))
    {
        return false;
    }
    uint8_t f = state->token;
    if (!eval_factor(state))
    {
        return false;
    }
    switch (f)
    {
    case TOKEN_KEYWORD_ABS:
        state->number = fabsf(state->number);
        break;
    case TOKEN_KEYWORD_ACS:
        state->number = acosf(state->number);
        break;
    case TOKEN_KEYWORD_ASN:
        state->number = asinf(state->number);
        break;
    case TOKEN_KEYWORD_ATN:
        state->number = atanf(state->number);
        break;
    case TOKEN_KEYWORD_BIN:
        // FIXME
        break;
    case TOKEN_KEYWORD_COS:
        state->number = cosf(state->number);
        break;
    case TOKEN_KEYWORD_EXP:
        state->number = expf(state->number);
        break;
    case TOKEN_KEYWORD_INT:
        state->number = truncf(state->number);
        break;
    case TOKEN_KEYWORD_LN:
        state->number = logf(state->number);
        break;
    case TOKEN_KEYWORD_SGN:
        state->number = (state->number > 0) - (state->number < 0);
        break;
    case TOKEN_KEYWORD_SIN:
        state->number = sinf(state->number);
        break;
    case TOKEN_KEYWORD_SQR:
        state->number = sqrtf(state->number);
        break;
    case TOKEN_KEYWORD_TAN:
        state->number = tanf(state->number);
        break;
    default:
        return false;
    }
    return true;
}

bool eval_factor(t_eval_state *state)
{
    bool result =
        eval_number(state) ||
        eval_function(state) ||
        eval_code(state) ||
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
        // TODO: Manage / and % by zero (inf)
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

// TODO: Add eval_condition (< > <> = <= >=)
// TODO: Add eval_boolean (AND OR)

bool eval_expr(t_eval_state *state)
{
    bool result = true;
    float acc = 0;
    if ((result = eval_term(state)))
    {
        acc = state->number;
        while (eval_token_one_of(state, "+-|&"))
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
                printf("%g", state->number);
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
