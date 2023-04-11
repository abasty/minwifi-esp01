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

    const char *test_input = "config  65535    reset  3615   free 0 clear 2024 cats";
    char command[256];
    strcpy(command, test_input);

    t_tokenizer_state line;

    printf("\n%s\n\n", test_input);
    int err = tokenize(&line, command);
    if (err)
    {
        printf("Syntax error.\n");
    }
    else
    {
        uint8_t token;
        while ((token = token_get_next(&line)))
        {
            if (token & TOKEN_KEYWORD)
            {
                token &= ~TOKEN_KEYWORD;
                printf("[keyword id: %u]\n", token);
            }
            else if (token == TOKEN_INTEGER)
            {
                uint16_t value = token_integer_get_value(&line);
                printf("[uint: %u]\n", value);
            }
            else
            {
                printf("%u\n", token);
            }
        }
        printf("\nOK, len: %zu\n", line.write_ptr - line.start);
    }
}
