#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "basic.h"
#include "keywords.h"

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
    printf("\n");
}

int main(int argc, char *argv[])
{
    const char *test_input = "0 9 15 16 255 256 65535 connect ws \"3615co.de/ws\" 80";
    char command[256];

    if (argc >= 2)
    {
        strncpy(command, argv[1], 255);
        command[255] = 0;
    }
    else
    {
        strcpy(command, test_input);
    }

    t_tokenizer_state line;

    printf("\n%s\n", command);
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
            if ((token & TOKEN_KEYWORD) != 0)
            {
                if (token == TOKEN_KEYWORD_HELP)
                {
                    keyword_print_all(keywords);
                }
                token &= ~TOKEN_KEYWORD;
                printf("[keyword id: %u]\n", token);
            }
            else if ((token & TOKEN_INTEGER_TYPE_MASK) == TOKEN_INTEGER)
            {
                uint16_t value = token_integer_get_value(&line);
                printf("[uint: %u]\n", value);
            }
            else if (token == TOKEN_STRING)
            {
                char *value = token_string_get_value(&line);
                printf("[string: %s]\n", value);
            }
            else
            {
                printf("%u\n", token);
            }
        }
        printf("\nOK, len: %zu\n", line.write_ptr - line.start);
    }
}
