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
} t_eval_state;

extern ds_btree_t progs;
extern ds_btree_t vars;

bool eval_next_token(t_eval_state *state)
{
    if (*state->read_ptr == 0)
    {
        state->token = 0;
        return false;
    }
    state->token = *state->read_ptr++;
    return true;
}

#ifdef __NO__
#define DEBUG_PRINTF printf

float term()
{
    return 1.0;
}

float expr(void)
{
    float t1, t2;
    int op;

    t1 = 1.0; // term();
    op = '+';
    DEBUG_PRINTF("expr: token %d\n", op);
    while (op == '+' ||
           op == '-' ||
           op == '&' ||
           op == '|')
    {
        // tokenizer_next();
        t2 = term();
        DEBUG_PRINTF("expr: %f %d %f\n", t1, op, t2);
        switch (op)
        {
        case '+':
            t1 = t1 + t2;
            break;
        case '-':
            t1 = t1 - t2;
            break;
        case '&':
            t1 = (int)(truncf(t1)) & (int)(truncf(t2));
            break;
        case '|':
            t1 = (int)(truncf(t1)) | (int)(truncf(t2));
            break;
        }
        // op = tokenizer_token();
    }
    DEBUG_PRINTF("expr: %f\n", t1);
    return t1;
}
#endif

bool eval_string(t_eval_state *state)
{
    if (state->token != TOKEN_STRING)
        return false;

    int n = 0;

    if (state->do_eval)
        n = printf("%s", state->read_ptr);
    else
        n = snprintf(0, 0, "%s", state->read_ptr);
    state->read_ptr += n + 1;
    return true;
}

bool eval_print(t_eval_state *state)
{
    if (state->token != TOKEN_KEYWORD_PRINT)
        return false;

    bool eval = true;
    while (eval && eval_next_token(state))
    {
        eval = eval_string(state);
        if (eval && eval_next_token(state))
        {
            if (state->token == ';')
            {
                continue;
            }
            if (state->token == ',')
            {
                if (state->do_eval)
                    printf(" ");
                continue;
            }
            eval = false;
            break;
        }
    }

    if (state->do_eval)
    {
        printf("\n");
    }
    return eval;
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
    if (state->token != TOKEN_KEYWORD_LIST)
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

    bool eval = true;
    while (eval && eval_next_token(&state))
    {
        eval = eval_print(&state) || eval_list(&state); // || eval_input(&state) ...
    }

    if (eval)
        return BERROR_NONE;

    return BERROR_SYNTAX;
}
