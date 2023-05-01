#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ds_common.h"
#include "ds_btree.h"

#include "token.h"
#include "keywords.h"
#include "bio.h"

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

bastos_io_t bio = {
    .printf = 0,
    .strtof = 0,
};

int main(int argc, char *argv[])
{
    bastos_init(&bio);
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
