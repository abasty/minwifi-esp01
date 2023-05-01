#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ds_common.h"
#include "ds_btree.h"

#include "token.h"
#include "keywords.h"
#include "bio.h"


int print_float(float f)
{
    return printf("%g", f);
}

int print_string(char *s)
{
    return printf("%s", s);
}

int print_integer(char *format, int32_t i)
{
    return printf(format, i);
}

bastos_io_t io = {
    .print_string = print_string,
    .print_float = print_float,
    .print_integer = print_integer,
};

extern char *keywords;

#define KEYWORD_MAX_LENGHT ((uint8_t)16)
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

extern ds_btree_t prog_tree;
extern ds_btree_t vars;

#include <math.h>

int main(int argc, char *argv[])
{
    bastos_init(&io);
    bastos_handle_keys("print", 5);
    bastos_loop();
    bastos_handle_keys("\"Hello from Catlabs\"", 42);
    bastos_loop();
    bastos_handle_keys("\n", 42);
    bastos_loop();

    bool cont = true;
    while (cont)
    {
        size_t size = 0;
        char *command = 0;
        ssize_t len = getline(&command, &size, stdin);
        bastos_handle_keys(command, len);
        free(command);
        bastos_loop();
    }
}
