#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "keywords.h"
#include "bmemory.h"

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

extern ds_btree_t progs;
extern ds_btree_t vars;

void prog_list()
{
    printf("Program: %zd line(s).\n", progs.count);
    printf("Variables: %zd symbols(s).\n", vars.count);
}

int main(int argc, char *argv[])
{
    bool cont = true;
    t_tokenizer_state line;

    bmem_init();

    while (cont)
    {
        size_t size = 0;
        char *command = 0;
        ssize_t len = getline(&command, &size, stdin);

        if (len == 1)
        {
            break;
        }

        if (len > 0)
        {
            command[len - 1] = 0;
        }

        int err = tokenize(&line, command);
        if (err)
        {
            printf("Syntax error.\n");
        }
        else
        {
            if (line.line_no != 0)
            {
                printf("line no: %u, len: %zu\n\n", line.line_no, line.write_ptr - line.read_ptr);
                bmem_prog_new(line.line_no, line.read_ptr, line.write_ptr - line.read_ptr);
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
                        else if (token == TOKEN_KEYWORD_LIST)
                        {
                            prog_list();
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
            }
        }
        free(command);
    }
}
