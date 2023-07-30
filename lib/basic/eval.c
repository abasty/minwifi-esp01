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
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef MINITEL
#include "tty-minitel.h"
#else
#include "tty-vt100.h"
#endif

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

static inline void eval_input_mode(bool mode);
static bool eval_string_tty();

static void string_set(string_t *string, char *chars, bool allocated)
{
    if (string->allocated && string->chars != chars)
        free(string->chars);
    string->chars = chars;
    string->allocated = allocated;
}

static void string_normalize(string_t *string)
{
    if (string->chars && *string->chars)
        return;

    if (string->allocated)
    {
        free(string->chars);
    }

    string->allocated = false;
    string->chars = 0;
}

static void string_move(string_t *from, string_t *to)
{
    *to = *from;
    from->chars = 0;
    from->allocated = false;
    string_normalize(to);
}

static void string_slice(string_t *string, uint16_t start, uint16_t end)
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

static void string_concat(string_t *string1, string_t *string2)
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
    float limit;
    float step;
    prog_t *for_line;
} loop_t;

typedef struct
{
    uint16_t line_no;
} return_t;

typedef struct
{
    prog_t *pc;
    prog_t *prog;
    char *var_ref;
    var_t *input_var;
    float number;
    uint8_t *read_ptr;
    uint8_t token;
    uint8_t input_var_token;
    int8_t error;
    bool do_eval;
    bool running;
    bool inputting;
    int sp;
    string_t string;
} eval_state_t;

loop_t loops[26] = {0};

#define EVAL_RETURNS_SIZE 32
return_t returns[EVAL_RETURNS_SIZE] = {0};

eval_state_t bstate;

extern bastos_io_t *bio;

extern ds_btree_t prog_tree;
extern ds_btree_t var_tree;

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
    TOKEN_KEYWORD_NOT,
    0,
};

uint8_t variables[] = {
    TOKEN_VARIABLE_NUMBER,
    TOKEN_VARIABLE_STRING,
    0,
};

char tty_codes[] = {
    TOKEN_KEYWORD_AT,
    TOKEN_KEYWORD_INK,
    TOKEN_KEYWORD_PAPER,
    TOKEN_KEYWORD_CURSOR,
    TOKEN_KEYWORD_CLS,
    0,
};

const char compare_tokens[] = {
    TOKEN_COMPARE_EQ,
    TOKEN_COMPARE_GT,
    TOKEN_COMPARE_LT,
    TOKEN_COMPARE_NE,
    TOKEN_COMPARE_GE,
    TOKEN_COMPARE_LE,
    0,
};

uint8_t instr0[] = {
    TOKEN_KEYWORD_CLEAR,
    TOKEN_KEYWORD_NEW,
    TOKEN_KEYWORD_CAT,
    TOKEN_KEYWORD_RESET,
    TOKEN_KEYWORD_STOP,
    TOKEN_KEYWORD_CONT,
    TOKEN_KEYWORD_RUN,
    TOKEN_KEYWORD_RETURN,
    0,
};

uint8_t instr1s[] = {
    TOKEN_KEYWORD_ERASE,
    TOKEN_KEYWORD_SAVE,
    TOKEN_KEYWORD_LOAD,
    0,
};

uint8_t instr1n[] = {
    TOKEN_KEYWORD_GOSUB,
    TOKEN_KEYWORD_GOTO,
    0,
};

static void running_state_clear()
{
    bstate.pc = 0;
    bstate.sp = 0;
    memset(loops, 0, sizeof(loops));
}

static bool eval_token(uint8_t c)
{
    if (*bstate.read_ptr != c)
    {
        return false;
    }
    bstate.token = *bstate.read_ptr++;
    return true;
}

static uint8_t eval_token_one_of(const char *set)
{
    char c = *bstate.read_ptr;

    while (*set && c != *set)
    {
        set++;
    }
    if (!*set)
    {
        return 0;
    }
    bstate.token = c;
    bstate.read_ptr++;
    return c;
}

static bool eval_string_expr();
static bool eval_factor();
static bool eval_expr(uint8_t type_token);

uint8_t len_code_functions[] = {
    TOKEN_KEYWORD_CODE,
    TOKEN_KEYWORD_LEN,
    0,
};

static bool eval_len_code()
{
    uint8_t token;

    if ((token = eval_token_one_of((char*) len_code_functions)) == 0 || !eval_string_expr())
        return false;

    if (!bstate.do_eval)
        return true;

    if (!bstate.string.chars)
    {
        bstate.number = 0;
        return true;
    }

    if (token == TOKEN_KEYWORD_LEN)
    {
        bstate.number = strlen(bstate.string.chars);
        return true;
    }

    bstate.number = *bstate.string.chars;
    return true;
}

static bool eval_number()
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
    bstate.token = TOKEN_NUMBER;
    if (minus)
    {
        bstate.number = -bstate.number;
    }
    return true;
}

static bool eval_function()
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
    case TOKEN_KEYWORD_NOT:
        bstate.number = truncf(bstate.number) == 0 ? 1 : 0;
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

static bool eval_factor()
{
    bool result =
        eval_number() ||
        eval_function() ||
        eval_len_code() ||
        (eval_token('(') && eval_expr(TOKEN_NUMBER) && eval_token(')'));
    return result;
}

static bool eval_term()
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
    bstate.token = TOKEN_NUMBER;
    return result;
}

static bool eval_float_expr()
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
        bstate.token = TOKEN_NUMBER;
    }
    return result;
}

static bool eval_compare_expr()
{
    if (!eval_float_expr() && !eval_string_expr())
        return false;

    uint8_t type_token = bstate.token;

    if (eval_token_one_of(compare_tokens))
    {
        uint8_t op = bstate.token;
        int result = 0;

        if (type_token == TOKEN_NUMBER)
        {
            float arg1 = bstate.number;

            if (!eval_float_expr())
                return false;

            result = arg1 - bstate.number;
        }
        else // type_token == TOKEN_STRING
        {
            string_t string1;
            string_move(&bstate.string, &string1);

            if (!eval_string_expr())
                return false;

            result = strcmp(string1.chars ? string1.chars : "", bstate.string.chars ? bstate.string.chars : "");

        }
        switch (op)
        {
        case '=':
            bstate.number = result == 0;
            break;
        case '<':
            bstate.number = result < 0;
            break;
        case '>':
            bstate.number = result > 0;
            break;
        case TOKEN_COMPARE_NE:
            bstate.number = result != 0;
            break;
        case TOKEN_COMPARE_LE:
            bstate.number = result <= 0;
            break;
        case TOKEN_COMPARE_GE:
            bstate.number = result >= 0;
            break;
        }
        bstate.token = TOKEN_NUMBER;
    }

    return true;
}

static bool eval_expr(uint8_t type_token)
{
    if (!eval_compare_expr())
        return false;

    if (bstate.token == TOKEN_STRING)
        return (type_token & TOKEN_STRING) != 0;

    float acc = bstate.number == 0 ? 0 : 1;
    bool is_bool_expr = false;

    while (eval_token(TOKEN_KEYWORD_AND) || eval_token(TOKEN_KEYWORD_OR))
    {
        uint8_t op = bstate.token;

        if (!eval_compare_expr())
            return false;

        float arg = bstate.number == 0 ? 0 : 1;

        if (op == TOKEN_KEYWORD_AND)
        {
            acc = acc && arg;
        }
        else
        {
            acc = acc || arg;
        }
        is_bool_expr = true;
    }
    if (is_bool_expr)
    {
        bstate.number = acc;
    }

    return (type_token & TOKEN_NUMBER) != 0;
}

static bool eval_string_chr()
{
    if (!eval_token(TOKEN_KEYWORD_CHR))
        return false;

    if (!eval_term())
        return false;

    if (bstate.do_eval)
    {
        char *str = (char *)malloc(2);
        if (!str)
            return false;

        str[0] = (char)((uint8_t)(truncf(bstate.number)));
        str[1] = 0;

        string_set(&bstate.string, str, true);
    }

    return true;
}

static bool eval_string_str()
{
    if (!eval_token(TOKEN_KEYWORD_STR))
        return false;

    if (!eval_term())
        return false;

    if (bstate.do_eval)
    {
        char dummy[2];
        int n = snprintf(dummy, 2, "%g", bstate.number);
        char *str = (char *)malloc(n + 1);
        if (!str)
            return false;

        sprintf(str, "%g", bstate.number);
        string_set(&bstate.string, str, true);
    }

    return true;
}

static bool eval_string_const()
{
    if (!eval_token(TOKEN_STRING))
        return false;

    string_set(&bstate.string, (char *)(bstate.read_ptr), false);
    while(*bstate.read_ptr++);
    return true;
}

static bool eval_string_var()
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

static bool eval_string_term()
{
    bool result =
        eval_string_const() ||
        eval_string_var() ||
        eval_string_chr() ||
        eval_string_tty() ||
        eval_string_str() ||
        (eval_token('(') && eval_string_expr() && eval_token(')'));

    if (!result)
        return false;

    // Optional range
    if (!eval_token('('))
        return true;

    uint16_t start = 1;
    uint16_t end = 0;

    if (eval_expr(TOKEN_NUMBER))
    {
        start = bstate.number;
        if (start < 1)
        {
            start = 1;
        }
        end = start;
    }

    if (eval_token(TOKEN_KEYWORD_TO))
    {
        if (eval_expr(TOKEN_NUMBER))
        {
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

static bool eval_string_expr()
{
    bool result = true;
    if ((result = eval_string_term()))
    {
        string_t string1;
        string_move(&bstate.string, &string1);

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
        bstate.token = TOKEN_STRING;
    }
    return result;
}

static bool eval_variable_ref()
{
    bstate.var_ref = (char *)bstate.read_ptr;

    if (!eval_token_one_of((char *)variables))
        return false;

    // Pass variable chars until zero
    while (*bstate.read_ptr++)
        ;

    return true;
}

static bool eval_let()
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
        if (!eval_expr(TOKEN_NUMBER))
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
        if (!eval_expr(TOKEN_STRING))
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

static bool eval_input()
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

static bool eval_print(bool implicit)
{
    if (!implicit && !eval_token(TOKEN_KEYWORD_PRINT))
        return false;

    if (implicit)
    {
        // rewind to tty / string token
        bstate.read_ptr--;
    }

    bool result = true;
    bool ln = true;

    while (result && *bstate.read_ptr != 0)
    {
        ln = true;
        if (eval_expr(TOKEN_NUMBER | TOKEN_STRING))
        {
            if (bstate.do_eval)
            {
                if (bstate.token == TOKEN_NUMBER)
                {
                    bio->print_float(bstate.number);
                }
                else // TOKEN_STRING
                {
                    bio->print_string(bstate.string.chars ? bstate.string.chars : "");
                }
            }
        }
        else if (eval_string_tty())
        {
            if (bstate.do_eval)
            {
                bio->print_string(bstate.string.chars ? bstate.string.chars : "");
            }
        }
        else if (eval_token(','))
        {
            if (bstate.do_eval)
            {
                bio->print_string(" ");
            }

        }
        else if (eval_token(';'))
        {
            ln = false;
        }
        else
        {
            result = *bstate.read_ptr == 0;
        }
    }

    if (result && bstate.do_eval)
    {
        if (ln && !implicit)
        {
            bio->print_string("\r\n");
        }
    }

    return result;
}

static bool eval_list()
{
    if (!eval_token(TOKEN_KEYWORD_LIST))
        return false;

    uint16_t start = 0;
    uint16_t n = 20;

    if (eval_number())
    {
        start = bstate.number;
        if (eval_token(','))
        {
            if (!eval_number())
                return false;

            n = bstate.number;
        }
    }

    if (bstate.do_eval)
    {
        bio->print_integer("lines/vars: %d/", prog_tree.count);
        bio->print_integer("%d\r\n", var_tree.count);
        prog_t *prog = bmem_prog_first_line();
        while (prog && n >= 1)
        {
            if (prog->line_no >= start)
            {
                bio->print_integer("%4d", (int) prog->line_no);
                char c[2];
                c[0] = bstate.pc == prog ? '>' : ' ';
                c[1] = 0;
                bio->print_string(c);
                bio->print_string(untokenize(prog->line));
                bio->print_string("\r\n");
                n--;
            }
            prog = bmem_prog_next_line(prog);
        }
    }

    return true;
}

static inline void eval_input_mode(bool mode)
{
    bstate.inputting = mode;
}

int8_t eval_prog_next()
{
    int8_t err = BERROR_NONE;
    prog_t *pc = bstate.pc;

    if (pc)
    {
        err = eval_prog(pc, true);
        if (err == BERROR_NONE)
        {
            if (bstate.pc == pc)
            {
                // If executed line did not change PC then move PC to next line
                bstate.pc = bmem_prog_next_line(bstate.pc);
            }
            return BERROR_NONE;
        }
    }

    if (err != BERROR_NONE && bstate.pc != 0)
    {
        bio->print_integer("On line %d: ", bstate.pc->line_no);
    }

    bstate.pc = 0;
    bstate.running = false;

    return err;
}

void eval_stop()
{
    bstate.running = false;
}

void eval_cont()
{
    bstate.running = bstate.pc != 0;
}

bool eval_running()
{
    return bstate.running;
}

bool eval_inputting()
{
    return bstate.inputting;
}

static void eval_run()
{
    running_state_clear();
    bmem_vars_clear();
    bstate.pc = bmem_prog_first_line();
    bstate.running = true;
}

static void eval_goto()
{
    bstate.pc = bmem_prog_get_line(bstate.number);
    bstate.running = bstate.pc != 0;

    if (bstate.pc)
        return;

    bstate.error = BERROR_RUN;
}

static void eval_gosub()
{
    if (bstate.sp >= EVAL_RETURNS_SIZE)
    {
        bstate.error = BERROR_RUN;
        return;
    }

    prog_t *next = bmem_prog_next_line(bstate.pc);
    bstate.pc = bmem_prog_get_line(bstate.number);
    bstate.running = bstate.pc != 0;

    if (!bstate.pc)
    {
        bstate.error = BERROR_RUN;
        return;
    }

    returns[bstate.sp++].line_no = next ? next->line_no : 0;
}

static void eval_return()
{
    if (bstate.sp < 1)
    {
        bstate.error = BERROR_RUN;
        return;
    }

    uint16_t line_no = returns[--bstate.sp].line_no;
    bstate.pc = bmem_prog_get_line(line_no);
    bstate.running = bstate.pc != 0;
}

static void eval_save()
{
    if (bstate.string.chars == 0)
        return;

    bastos_save(bstate.string.chars);
}

static void eval_load()
{
    if (bstate.string.chars == 0)
        return;

    bstate.running = false;
    bstate.pc = 0;
    running_state_clear();
    bstate.error = bastos_load(bstate.string.chars);
}

static void eval_erase()
{
    if (bio->erase(bstate.string.chars) != 0)
    {
        bstate.error = BERROR_IO;
    }
}

static void eval_clear()
{
    running_state_clear();
    bmem_vars_clear();
}

static void eval_new()
{
    running_state_clear();
    bmem_prog_new();
}

static bool eval_string_tty()
{
    if (!eval_token_one_of(tty_codes))
        return false;

    uint8_t fn = bstate.token;
    char codes[CODE_SEQUENCE_MAX_SIZE];
    *codes = 0;

    // 0 arg
    if (fn == TOKEN_KEYWORD_CLS)
    {
        snprintf(codes, CODE_SEQUENCE_MAX_SIZE, CLS);
        goto EVAL;
    }

    if (!eval_term())
        return false;

    // 1 arg
    uint8_t arg1 = bstate.number;
    if (fn == TOKEN_KEYWORD_INK)
    {
        snprintf(codes, CODE_SEQUENCE_MAX_SIZE, INK, arg1 + INK_DELTA);
        goto EVAL;
    }
    if (fn == TOKEN_KEYWORD_PAPER)
    {
        snprintf(codes, CODE_SEQUENCE_MAX_SIZE, PAPER, arg1 + PAPER_DELTA);
        goto EVAL;
    }
    if (fn == TOKEN_KEYWORD_CURSOR)
    {
        snprintf(codes, CODE_SEQUENCE_MAX_SIZE, "%s", arg1 ? CON : COFF);
        goto EVAL;
    }

    // 2 args
    if (!eval_token(','))
        return false;

    if (!eval_term())
        return false;

    uint8_t arg2 = bstate.number;
    snprintf(codes, CODE_SEQUENCE_MAX_SIZE, CUR, arg1 + CUR_DELTA_V, arg2 + CUR_DELTA_H);

EVAL:
    if (!bstate.do_eval)
        return true;

    string_set(&bstate.string, strdup(codes), true);
    return true;
}

static bool eval_simple_instruction()
{
    uint8_t instr;

    // 0 arg instructions
    if ((instr =  eval_token_one_of((char *)instr0)))
        goto EVAL;

    // 1 string instructions
    if ((instr =  eval_token_one_of((char *)instr1s)) && eval_string_expr())
        goto EVAL;

    // 1 number instructions
    if ((instr =  eval_token_one_of((char *)instr1n)) && eval_expr(TOKEN_NUMBER))
        goto EVAL;

    if (eval_token_one_of(tty_codes))
        return eval_print(true);

    return false;

EVAL:
    if (!bstate.do_eval)
        return true;

    if (instr == TOKEN_KEYWORD_GOTO)
    {
        eval_goto();
        return true;
    }
    if (instr == TOKEN_KEYWORD_GOSUB)
    {
        eval_gosub();
        return true;
    }
    if (instr == TOKEN_KEYWORD_ERASE)
    {
        eval_erase();
        return true;
    }
    if (instr == TOKEN_KEYWORD_SAVE)
    {
        eval_save();
        return true;
    }
    if (instr == TOKEN_KEYWORD_LOAD)
    {
        eval_load();
        return true;
    }
    if (instr == TOKEN_KEYWORD_RETURN)
    {
        eval_return();
        return true;
    }
    if (instr == TOKEN_KEYWORD_CLEAR)
    {
        eval_clear();
        return true;
    }
    if (instr == TOKEN_KEYWORD_RUN)
    {
        eval_run();
        return true;
    }
    if (instr == TOKEN_KEYWORD_NEW)
    {
        eval_new();
        return true;
    }
    if (instr == TOKEN_KEYWORD_RESET || instr == TOKEN_KEYWORD_CAT)
    {
        bio->function0(instr);
        return true;
    }
    if (instr == TOKEN_KEYWORD_STOP)
    {
        eval_stop();
        return true;
    }
    if (instr == TOKEN_KEYWORD_CONT)
    {
        eval_cont();
        return true;
    }
    return false;
}

static bool eval_rem()
{
    if (!eval_token(TOKEN_KEYWORD_REM))
        return false;

    bstate.read_ptr = bstate.prog->line + bstate.prog->len;

    return true;
}

static bool eval_instruction()
{
    return
        eval_print(false) ||
        eval_input() ||
        eval_simple_instruction()
#ifndef OTA_ONLY
        ||
        eval_rem() ||
        eval_let() ||
        eval_list()
#endif
        ;
}

static bool eval_if()
{
    if (!eval_token(TOKEN_KEYWORD_IF))
        return false;

    if (!eval_expr(TOKEN_NUMBER))
        return false;

    if (bstate.do_eval)
    {
        if (bstate.number == 0)
        {
            // If test is negative, ignore end of line
            bstate.read_ptr = bstate.prog->line + bstate.prog->len;
            return true;
        }
    }

    if (!eval_token(TOKEN_KEYWORD_THEN))
        return false;

    if (!eval_instruction())
        return false;

    return true;
}

static bool eval_for()
{
    if (!eval_token(TOKEN_KEYWORD_FOR))
        return false;

    if (!eval_variable_ref() || bstate.var_ref[0] != TOKEN_VARIABLE_NUMBER || bstate.var_ref[2] != 0)
        return false;

    if (!eval_token('='))
        return false;

    if (!eval_expr(TOKEN_NUMBER))
        return false;

    float init = bstate.number;

    if (!eval_token(TOKEN_KEYWORD_TO))
        return false;

    if (!eval_expr(TOKEN_NUMBER))
        return false;

    float limit = bstate.number;
    float step = 1;

    if (eval_token(TOKEN_KEYWORD_STEP))
    {
        if (!eval_expr(TOKEN_NUMBER))
            return false;

        step = bstate.number;
    }

    if (!bstate.do_eval)
        return true;

    int loop_index = bstate.var_ref[1] - 'A';
    if (loop_index < 0 || loop_index > 25)
    {
        bstate.error = BERROR_RANGE;
        return true;
    }

    loop_t *loop = loops + loop_index;
    if (loop->for_line != 0)
        return true;

    if (bmem_var_number_set(bstate.var_ref, init) == 0)
    {
        bstate.error = BERROR_MEMORY;
        return true;
    }

    loop->for_line = bstate.prog;
    loop->limit = limit;
    loop->step = step;
    // TODO: Search for next and run next? Check in emulator

    return true;
}

static bool eval_next()
{
    if (!eval_token(TOKEN_KEYWORD_NEXT))
        return false;

    if (!eval_variable_ref() || *bstate.var_ref != TOKEN_VARIABLE_NUMBER)
        return false;

    if (!bstate.do_eval)
        return true;

    int loop_index = bstate.var_ref[1] - 'A';
    if (loop_index < 0 || loop_index > 25)
    {
        bstate.error = BERROR_RANGE;
        return true;
    }

    loop_t *loop = loops + loop_index;
    if (loop->for_line == 0)
    {
        bstate.error = BERROR_RUN;
        return true;
    }

    var_t *var = bmem_var_find(bstate.var_ref);
    if (var == 0)
    {
        bstate.error = BERROR_RUN;
        return true;
    }

    var->number += loop->step;
    float cmp = loop->step >= 0 ? loop->limit - var->number : var->number - loop->limit;
    bstate.pc = cmp >= 0 ? loop->for_line : bmem_prog_next_line(bstate.prog);
    if (cmp < 0)
    {
        loop->for_line = 0;
    }

    return true;
}

int8_t eval_prog(prog_t *prog, bool do_eval)
{
    // Init evaluator state
    bstate.do_eval = do_eval;
    bstate.read_ptr = prog->line;
    bstate.token = 0;
    bstate.string.allocated = 0;
    bstate.string.chars = 0;
    bstate.error = BERROR_NONE;
    bstate.prog = prog;

    // Do syntax check or eval
    bool eval = eval_instruction() || eval_if() || eval_for() || eval_next();

    // Syntax check end of line.
    eval = eval && *bstate.read_ptr == 0;

    // Free evaluator state
    string_set(&bstate.string, 0, false);

    // Handle syntax error
    if (!eval && bstate.error == BERROR_NONE)
    {
        bstate.error = BERROR_SYNTAX;
    }

    // TODO: We can support multiple intructions on the same line here

    return bstate.error;
}
