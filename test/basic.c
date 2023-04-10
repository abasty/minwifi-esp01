#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define KEYWORD_END_TAG (0b10000000)
#define TOKEN_KEYWORD ((uint8_t) 0b10000000)
#define TOKEN_INTEGER ((uint8_t) 0b10000001)
#define TOKEN_STRING  ((uint8_t) 0b10000010)

#include "keywords.c"

typedef struct {
    char *read_ptr;
    char *write_ptr;
} t_line;

int char_is_sep(char test_char)
{
    return test_char <= ' ' || test_char == ':' || test_char == ';' || test_char == ',' || test_char == '"';
}

int char_of_keyword(char test_char)
{
    // TODO: test other implementation for size
    return (test_char >= 'A' && test_char <= 'Z') || (test_char >= 'a' && test_char <= 'z')  || (test_char >= '0' && test_char <= '9') || (test_char == '_');
}

// TODO: pase a string, split to spaces, and give pointer to words, then
// tokenize : interger, string, keyword
// Ou plutôt un truc qui fait "nextToken, nextTokenAsString"

int tokenize_keyword(t_line *line, const char *keyword_char)
{
    char *word = line->read_ptr;
    char *word_char = word;

    // Search end of keyword and transform to uppercase
    char c = *line->read_ptr;
    while (char_of_keyword(c))
    {
        *line->read_ptr++ = c >= 'a' && c <= 'z' ? c - 32 : c;
        c = *line->read_ptr;
    }
    *(line->read_ptr - 1) |= KEYWORD_END_TAG;

    uint8_t index = 0;
    while (*keyword_char != 0)
    {
        c = *word_char;
        while (c == *keyword_char)
        {
            if ((c & KEYWORD_END_TAG) != 0)
            {
                *line->write_ptr++ = TOKEN_KEYWORD;
                *line->write_ptr++ = index;
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

int tokenize(t_line *line)
{
    char current_char;

    while (current_char = *line->read_ptr)
    {
        // first char => type
        if (current_char == ' ')
        {
            line->read_ptr++;
        }
        else if (current_char == '"')
        {
            // string
        }
        else if (current_char >= '0' && current_char <= '9')
        {
            // integer
        }
        else if ((current_char >= 'A' && current_char <= 'Z') || (current_char >= 'a' && current_char <= 'z'))
        {
            // keyword
            if (tokenize_keyword(line, keywords) != 0)
            {
                return -1;
            }
            printf("keyword: %d\n", *(line->write_ptr - 1));
        }
        else
        {
            // syntax error
            return -1;
        }
    }
    return 0;
}

#define KEYWORD_MAX_LENGHT ((uint8_t) 16)
void keyword_print_all(const char *keyword_char)
{
    char keyword[KEYWORD_MAX_LENGHT + 1];

    char *current_keyword_char = keyword;
    uint8_t index = 0;
    while (*keyword_char)
    {
        *current_keyword_char++ = *keyword_char & ~KEYWORD_END_TAG;
        if ((*keyword_char & KEYWORD_END_TAG) != 0)
        {
            *current_keyword_char = 0;
            current_keyword_char = keyword;
            printf("%d: %s\n", index, keyword);
            index++;
        }
        keyword_char++;
    }
}

int main()
{
    keyword_print_all(keywords);

    const char *test_input = "config      reset     free clear cats";
    char command[256];
    strcpy(command, test_input);

    t_line line = {
        .read_ptr = command,
        .write_ptr = command,
    };

    printf("\n%s\n", test_input);
    int err = tokenize(&line);
    if (err)
    {
        printf("Syntax error.\n");
    }
    else
    {
        printf("OK, len: %zd\n", line.write_ptr - command);
    }
}
