#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "basic.h"

#include "keywords.inc"

int char_is_sep(char test_char)
{
    return test_char <= ' ' || test_char == ':' || test_char == ';' || test_char == ',' || test_char == '"';
}

int char_of_keyword(char test_char)
{
    // TODO: test other implementation for size
    return (test_char >= 'A' && test_char <= 'Z') || (test_char >= 'a' && test_char <= 'z')  || (test_char >= '0' && test_char <= '9') || (test_char == '_');
}

int char_is_digit(char test_char)
{
    return test_char >= '0' && test_char <= '9';
}

// TODO: pase a string, split to spaces, and give pointer to words, then
// tokenize : interger, string, keyword
// Ou plutôt un truc qui fait "nextToken, nextTokenAsString"

int tokenize_keyword(t_tokenizer_state *state, const char *keyword_char)
{
    char *word = state->read_ptr;
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
        value = value * 10 + c - '0';
        c = *++state->read_ptr;
    }
    *state->write_ptr++ = TOKEN_INTEGER;
    *state->write_ptr++ = value & 0xFF;
    *state->write_ptr++ = value >> 8;
    return 0;
}

int tokenize(t_tokenizer_state *state, char *line_str)
{
    state->start = line_str;
    state->read_ptr = line_str;
    state->write_ptr = line_str;

    char current_char;
    while (current_char = *state->read_ptr)
    {
        // first char => type
        if (current_char == ' ')
        {
            state->read_ptr++;
        }
        else if (current_char == '"')
        {
            // string
        }
        else if (current_char >= '0' && current_char <= '9')
        {
            // integer
            if (tokenize_integer(state) != 0)
            {
                return -1;
            }
        }
        else if ((current_char >= 'A' && current_char <= 'Z') || (current_char >= 'a' && current_char <= 'z'))
        {
            // keyword
            if (tokenize_keyword(state, keywords) != 0)
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
    return 0;
}
