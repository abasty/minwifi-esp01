#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <poll.h>

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
    int n = printf("%s", s);
    fflush(stdout);
    return n;
}

int print_integer(char *format, int32_t i)
{
    return printf(format, i);
}

void echo_newline()
{
}

void cls()
{
    printf("%s", "\033[2J"
                 "\033[H");
    fflush(stdout);
}

bastos_io_t io = {
    .print_string = print_string,
    .print_float = print_float,
    .print_integer = print_integer,
    .echo_newline = echo_newline,
    .cls = cls,
};

#if 0
extern char *keywords;

#define KEYWORD_MAX_LENGTH ((uint8_t)16)
void keyword_print_all(const char *keyword_char)
{
    char keyword[KEYWORD_MAX_LENGTH + 1];

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

#endif

int getch()
{
    struct termios old, new;
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= true ? ECHO : ~ECHO;
    tcsetattr(0, TCSANOW, &new);
    struct pollfd input[1] = {{fd: 0, events: POLLIN}};
    int ch = 0;
    if (poll(input, 1, 10) == 1)
        ch = getchar();
    tcsetattr(0, TCSANOW, &old);
    return ch;
}

int main(int argc, char *argv[])
{
    bastos_init(&io);

    bool cont = true;
    while (cont)
    {
        int c = getch();
        if (c != 0 && c != 3)
        {
            char *keys = (char *) &c;
            bastos_send_keys(keys, 1);
        }
        bastos_loop();
        cont = c != 3;
    }
}
