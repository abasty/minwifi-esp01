#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "token.h"

#include "keywords.inc"

int char_is_sep(char test_char)
{
    return test_char <= ' ' || test_char == ':' || test_char == ';' || test_char == ',' || test_char == '"';
}

int char_of_keyword(char test_char)
{
    // TODO: test other implementation for size
    return (test_char >= 'A' && test_char <= 'Z') || (test_char >= 'a' && test_char <= 'z') || (test_char >= '0' && test_char <= '9') || (test_char == '_');
}

int char_is_digit(char test_char)
{
    return test_char >= '0' && test_char <= '9';
}

int tokenize_keyword(t_tokenizer_state *state, const char *keyword_char)
{
    char *word = (char *)(state->read_ptr);
    char *word_char = word;

    // Search end of keyword and transform to uppercase
    char c = *state->read_ptr;
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
                return 0;
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
    return -1;
}

int tokenize_integer(t_tokenizer_state *state)
{
    uint16_t value = 0;

    char c = *state->read_ptr;
    while (char_is_digit(c))
    {
        if (value > 6553)
        {
            return -1;
        }
        value *= 10;
        c -= '0';
        if (value == 65530 && c > 5)
        {
            return -1;
        }
        value += c;
        c = *++state->read_ptr;
    }
    if (value < 16)
    {
        *state->write_ptr++ = TOKEN_INTEGER_4 + value;
        return 0;
    }
    if (value < 256)
    {
        *state->write_ptr++ = TOKEN_INTEGER_8;
        *state->write_ptr++ = value;
        return 0;
    }
    *state->write_ptr++ = TOKEN_INTEGER_16;
    *state->write_ptr++ = value & 0xFF;
    *state->write_ptr++ = value >> 8;
    return 0;
}

int tokenize_string(t_tokenizer_state *state)
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
        return -1;
    }
    *state->write_ptr++ = 0;
    state->read_ptr++;
    return 0;
}

int tokenize(t_tokenizer_state *state, char *line_str)
{
    state->start = (uint8_t *)line_str;
    state->read_ptr = state->start;
    state->write_ptr = state->start;

    uint8_t c;
    while ((c = *state->read_ptr))
    {
        // first char => type
        if (c == ' ')
        {
            state->read_ptr++;
        }
        else if (c == '"')
        {
            // string
            if (tokenize_string(state) == -1)
            {
                return -1;
            }
        }
        else if (c >= '0' && c <= '9')
        {
            // integer
            if (tokenize_integer(state) == -1)
            {
                return -1;
            }
        }
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        {
            // keyword
            if (tokenize_keyword(state, keywords) == -1)
            {
                return -1;
            }
        }
        else
        {
            // syntax error
            return -1;
        }
    }
    state->read_ptr = state->start;
    *state->write_ptr = 0;
    return 0;
}

uint8_t token_get_next(t_tokenizer_state *state)
{
    return *state->read_ptr != 0 ? *state->read_ptr++ : 0;
}

uint16_t token_integer_get_value(t_tokenizer_state *state)
{
    uint8_t token = *(state->read_ptr - 1);
    if ((token & TOKEN_INTEGER_BITS_MASK) == TOKEN_INTEGER_4)
    {
        return token & 0b00001111;
    }
    uint16_t value = *state->read_ptr++;
    if ((token & TOKEN_INTEGER_BITS_MASK) == TOKEN_INTEGER_8)
    {
        return value;
    }
    value += *state->read_ptr << 8;
    state->read_ptr++;
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
