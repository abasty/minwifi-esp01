#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>

#include "ds_common.h"
#include "ds_btree.h"

#include "berror.h"
#include "bmemory.h"
#include "token.h"
#include "keywords.h"
#include "bio.h"

#define BASTOS_DISK_PATH "/home/alain/Projects/minwifi-esp01/lib/basic/examples/disk"


int print_float(float f)
{
    return printf("%g", f);
}

static int print_string(const char *s)
{
    int n = printf("%s", s);
    fflush(stdout);
    return n;
}

static int print_integer(const char *format, int32_t i)
{
    return printf(format, i);
}

void cls()
{
    printf("%s", "\033[2J"
                 "\033[H");
    fflush(stdout);
}

int bopen(const char *pathname, int flags)
{
    if ((flags & O_CREAT) != 0)
        return creat(pathname, 0644);

    return open(pathname, flags);
}

int bclose(int fd)
{
    return close(fd);
}

int bwrite(int fd, const void *buf, int count)
{
    return write(fd, buf, count);
}

int bread(int fd, void *buf, int count)
{
    return read(fd, buf, count);
}

#include <dirent.h>

void bcat()
{
    struct dirent **namelist;
    int n = scandir(BASTOS_DISK_PATH, &namelist, NULL, NULL);
    while (n--)
    {
        if (strcmp(".", namelist[n]->d_name) && strcmp("..", namelist[n]->d_name))
        {
            printf("%s\n", namelist[n]->d_name);
        }
        free(namelist[n]);
    }
    free(namelist);
}

int berase(const char *pathname)
{
    return unlink(pathname);
}

void del()
{
    print_string("\x08 \x08");
}

static inline void GotoXY(uint16_t c, uint16_t l)
{
    printf("\x1B" "[%d;%dH", l, c);
}

static void color(uint8_t color, uint8_t foreground)
{
    printf("\033" "[%dm", (foreground ? 30 : 40) + color);
}

bastos_io_t io = {
    .bclose = bclose,
    .bwrite = bwrite,
    .bread = bread,
};

int biocop(void)
{
    int fn = bastos_io_argv[0].as_int;
    int y = bastos_io_argv[1].as_int;
    int x = bastos_io_argv[2].as_int;

    switch (fn)
    {
    case B_IO_CAT:
        bcat();
        return 0;

    case B_IO_CLS:
        cls();
        return 0;

    case B_IO_DEL:
        del();
        return 0;

    case B_IO_RESET:
        //breset();
        return 0;

    case B_IO_AT:
        GotoXY(x, y);
        return 0;

    case B_IO_INK:
        color(y, 1);
        return 0;

    case B_IO_PAPER:
        color(y, 0);
        return 0;

    case B_IO_PRINT_STRING:
        return print_string(bastos_io_argv[1].as_string);

    case B_IO_PRINT_FLOAT:
        return print_float(bastos_io_argv[1].as_float);

    case B_IO_PRINT_INTEGER:
        return print_integer(bastos_io_argv[1].as_string, bastos_io_argv[2].as_int);

    case B_IO_ERASE:
        return berase(bastos_io_argv[1].as_string);

    case B_IO_OPEN:
        return bopen(bastos_io_argv[1].as_string, bastos_io_argv[2].as_int);
    }
    return 0;
}

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
    new.c_lflag &= false ? ECHO : ~ECHO;
    tcsetattr(0, TCSANOW, &new);
    struct pollfd input[1] = {{fd: 0, events: POLLIN}};
    int ch = 0;
    if (poll(input, 1, 1) == 1)
        ch = getchar();
    tcsetattr(0, TCSANOW, &old);
    return ch;
}

#define config_prog \
    "1CLS\n" \
    "2PRINT\"* WiFi parameters *\"\n" \
    "4INPUT\"SSID: \",WSSID$\n" \
    "5INPUT\"PASS: \",WSECRET$\n" \
    "6SAVE\"config$$$\"\n"

int main(int argc, char *argv[])
{
    bool cont = true;
    var_t *var = 0;

    chdir(BASTOS_DISK_PATH);

    bastos_init(&io, biocop);

    int err = bastos_load("config$$$");
    if (err != BERROR_NONE)
        goto finalize;

    print_string("Config file loaded.\n");

    var = bmem_var_find("\021WSSID");
    if (!var)
        goto finalize;

    var = bmem_var_find("\021WSECRET");
    if (!var)
        goto finalize;

    print_string("Config vars OK.\n");

finalize:

    if (var == 0)
    {
        bmem_prog_new();
        bastos_send_keys(config_prog, strlen(config_prog));
        bastos_send_keys("RUN\n", 4);
    }

    while (cont)
    {
        int c = getch();
        if (c != 0 && c != 24 && c != 1)
        {
            char *keys = (char *) &c;
            bastos_send_keys(keys, 1);
        }
        else
        {
            if (c == 1)
            {
                bastos_stop();
            }
        }
        bastos_loop();
        cont = c != 24;
    }

    bmem_prog_new();
}
