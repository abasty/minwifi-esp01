#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "basic.h"

extern char *keywords;

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
        uint8_t len = line.write_ptr - command;
        for (uint8_t i = 0; i < len; i++)
        {
            char c = command[i] & ~TOKEN_KEYWORD;
            printf("%u ", c);
        }
        printf("\nOK, len: %u\n", len);
    }
}
