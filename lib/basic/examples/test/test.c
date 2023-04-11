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
        uint8_t len = line.write_ptr - line.start;
        for (uint8_t i = 0; i < len; i++)
        {
            char c = line.start[i];
            if (c & TOKEN_KEYWORD)
            {
                c &= ~TOKEN_KEYWORD;
                printf("[keyword id: %u]\n", c);
            }
            else if (c == TOKEN_INTEGER)
            {
                uint16_t value = line.start[i + 1] + (line.start[i + 2] << 8);
                printf("[uint: %u]\n", value);
                i += 2;
            }
            else
            {
                printf("%u\n", c);
            }
        }
        printf("\nOK, len: %u\n", len);
    }
}
