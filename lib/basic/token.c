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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "berror.h"

extern const char *keywords;

#define TOKENS_MAX_LINE_SIZE (256)
uint8_t token_buffer[TOKENS_MAX_LINE_SIZE];

bool char_of_keyword(char test_char)
{
    // TODO: test other implementation for size
    return (test_char >= 'A' && test_char <= 'Z') ||
           (test_char >= 'a' && test_char <= 'z') ||
           (test_char >= '0' && test_char <= '9') ||
           (test_char == '$');
}

const char *untokenize_keyword(t_tokenizer_state *state, const char *keyword)
{
    uint8_t token = (*(state->read_ptr - 1)) & ~TOKEN_KEYWORD;
    const char *keyword_char = keyword;
    char c;
    while (*keyword != 0)
    {
        c = *keyword_char++;
        if ((c & KEYWORD_END_TAG) != 0)
        {
            if (token != 0)
            {
                keyword = keyword_char;
                token--;
            }
            else
            {
                break;
            }
        }
    }

    if (*keyword == 0)
    {
        return 0;
    }
    keyword_char = keyword;
    do
    {
        c = *keyword_char++;
        putchar(c & ~KEYWORD_END_TAG);
    } while ((c & KEYWORD_END_TAG) == 0);

    return keyword;
}

int8_t tokenize_keyword(t_tokenizer_state *state, const char *keywords)
{
    uint8_t *keyword_char = (uint8_t *) keywords;
    uint8_t *word = state->read_ptr;
    uint8_t *word_char = word;

    // Search end of keyword and transform to uppercase
    uint8_t c = *state->read_ptr;
    while (char_of_keyword(c))
    {
        *state->read_ptr++ = c >= 'a' && c <= 'z' ? c - 32 : c;
        c = *state->read_ptr;
    }
    *(state->read_ptr - 1) |= KEYWORD_END_TAG;

    uint8_t index = 0;
    while (*keyword_char != 0)
    {
        c = *word_char;
        while (c == *keyword_char)
        {
            if ((c & KEYWORD_END_TAG) != 0)
            {
                *state->write_ptr++ = index | TOKEN_KEYWORD;
                return BERROR_NONE;
            }
            word_char++;
            keyword_char++;
            c = *word_char;
        }
        index++;
        while (*keyword_char && (*keyword_char & KEYWORD_END_TAG) == 0)
        {
            keyword_char++;
        }
        if (*keyword_char)
        {
            keyword_char++;
            word_char = word;
        }
    }

    // The word is not a keyword but a variable name

    // Get the last char of the variable name and test for string vs number
    word_char = state->read_ptr - 1;
    uint8_t token = *word_char == ('$' | KEYWORD_END_TAG) ? TOKEN_VARIABLE_STRING : TOKEN_VARIABLE_NUMBER;
    *state->write_ptr++ = token;

    // Copy all variable chars
    for (word_char = word; word_char != state->read_ptr; word_char++)
    {
        *state->write_ptr++ = *word_char & ~KEYWORD_END_TAG;
    }
    // Do not take last '$'
    if (token == TOKEN_VARIABLE_STRING)
    {
        state->write_ptr--;
    }
    // Zero terminate variable name
    *state->write_ptr++ = 0;

    return BERROR_NONE;
}

int8_t tokenize_number(t_tokenizer_state *state)
{
    float value = 0;
    int n = 0;
    if (sscanf((char *)state->read_ptr, "%f%n", &value, &n) != 1)
    {
        return BERROR_SYNTAX;
    }
    state->read_ptr += n; // +1 ?

    uint8_t *read_value_ptr = (uint8_t *)(&value);
    *state->write_ptr++ = TOKEN_NUMBER;
    *state->write_ptr++ = *read_value_ptr++;
    *state->write_ptr++ = *read_value_ptr++;
    *state->write_ptr++ = *read_value_ptr++;
    *state->write_ptr++ = *read_value_ptr;
    return BERROR_NONE;
}

int8_t tokenize_string(t_tokenizer_state *state)
{
    *state->write_ptr++ = TOKEN_STRING;
    char c = *++state->read_ptr;
    while (c != 0 && c != '"')
    {
        *state->write_ptr++ = c;
        c = *++state->read_ptr;
    }
    if (c == 0)
    {
        return BERROR_SYNTAX;
    }
    *state->write_ptr++ = 0;
    state->read_ptr++;
    return BERROR_NONE;
}

char *untokenize(uint8_t *input)
{
    t_tokenizer_state state;

    *token_buffer = 0;
    state.write_ptr = token_buffer;
    state.read_ptr = (uint8_t *)input;
    state.line_no = 0;

    uint8_t token;
    while ((token = token_get_next(&state)))
    {
        putchar(' ');
        if ((token & TOKEN_KEYWORD) != 0)
        {
            token &= ~TOKEN_KEYWORD;
            untokenize_keyword(&state, keywords);
        }
        else if (token == TOKEN_NUMBER)
        {
            float value = token_number_get_value(&state);
            printf("%g", value);
        }
        else if (token == TOKEN_STRING)
        {
            char *value = token_string_get_value(&state);
            printf("\"%s\"", value);
        }
        else if (token == TOKEN_VARIABLE_NUMBER || token == TOKEN_VARIABLE_STRING)
        {
            int n = printf("%s", state.read_ptr);
            state.read_ptr += n + 1;
            if (token == TOKEN_VARIABLE_STRING)
            {
                putchar('$');
            }
        }
        else
        {
            putchar(token);
        }
    }

    return (char *)token_buffer;
}

int8_t tokenize(t_tokenizer_state *state, char *input)
{
    state->read_ptr = (uint8_t *)input;
    state->write_ptr = token_buffer;
    state->line_no = 0;

    uint8_t c;
    uint8_t token_count = 0;
    int8_t err = 0;

    while ((c = *state->read_ptr))
    {
        // first char => type
        if (c == ' ')
        {
            state->read_ptr++;
            continue;
        }
        if (c == '"')
        {
            // string
            err = tokenize_string(state);
        }
        else if (c >= '0' && c <= '9')
        {
            // integer
            err = tokenize_number(state);
            if (err == 0 && token_count == 0)
            {
                uint8_t *read_ptr = state->read_ptr;
                state->read_ptr = token_buffer;
                token_get_next(state);
                state->line_no = token_number_get_value(state);
                state->read_ptr = read_ptr;
                state->write_ptr = token_buffer;
            }
        }
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        {
            err = tokenize_keyword(state, keywords);
        }
        else if (
            c == ';' || c == ',' ||
            c == '+' || c == '-' || c == '|' || c == '&' ||
            c == '*' || c == '/' || c == '%' ||
            c == '=' || c == '<' || c == '>' ||
            c == '(' || c == ')')
        {
            *state->write_ptr++ = *state->read_ptr++;
        }
        else
        {
            // syntax error
            err = BERROR_SYNTAX;
        }
        if (err < 0)
        {
            return err;
        }
        token_count++;
    }
    state->read_ptr = token_buffer;
    *state->write_ptr = 0;
    return 0;
}

uint8_t token_get_next(t_tokenizer_state *state)
{
    return *state->read_ptr != 0 ? *state->read_ptr++ : 0;
}

float token_number_get_value(t_tokenizer_state *state)
{
    float value = 0;
    uint8_t *write_value_ptr = (uint8_t *)&value;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    *write_value_ptr++ = *state->read_ptr++;
    return value;
}

char *token_string_get_value(t_tokenizer_state *state)
{
    char *value = (char *)(state->read_ptr);
    while (*state->read_ptr)
    {
        state->read_ptr++;
    }
    state->read_ptr++;
    return value;
}
