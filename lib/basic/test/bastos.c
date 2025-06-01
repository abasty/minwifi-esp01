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
#include <signal.h>

#ifdef MINITEL
#include "tty-minitel.h"
#endif

#include "bio.h"

struct sigaction old_action;
struct termios old, new;

void term_init()
{
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &new);

#ifdef MINITEL
    printf("%s", CON P_ACK_OFF_PRISE P_LOCAL_ECHO_OFF P_ROULEAU CLS);
#endif

}

void term_done()
{
    tcsetattr(0, TCSANOW, &old);
}

void sigint_handler(int sig_no)
{
    term_done();
    sigaction(SIGINT, &old_action, NULL);
    kill(0, SIGINT);
}

char getch()
{
    struct pollfd input[1] = {{fd: 0, events: POLLIN}};
    char ch = 0;
    if (poll(input, 1, 1) == 1)
    {
        read(0, &ch, 1);
    }
    return ch;
}

int print_float(float f)
{
    return printf("%g", f);
}

int print_string(const char *s)
{
    int n = printf("%s", s);
    fflush(stdout);
    return n;
}

int print_integer(const char *format, int32_t i)
{
    return printf(format, i);
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

static void bcat()
{
    struct dirent **namelist;
    int n = scandir(".", &namelist, NULL, NULL);
    while (n--)
    {
        if (strcmp(".", namelist[n]->d_name) && strcmp("..", namelist[n]->d_name))
        {
            printf("%s\r\n", namelist[n]->d_name);
        }
        free(namelist[n]);
    }
    free(namelist);
}

int berase(const char *pathname)
{
    return unlink(pathname);
}

static inline void bio_f0(uint8_t fn)
{
    if (fn == TOKEN_KEYWORD_CAT)
    {
        bcat();
        return;
    }
    if (fn == TOKEN_KEYWORD_RESET)
    {
        //breset();
        return;
    }
}

bastos_io_t io = {
    .print_string = print_string,
    .print_float = print_float,

    .print_integer = print_integer,
    .bopen = bopen,
    .erase = berase,

    .bclose = bclose,

    .bwrite = bwrite,
    .bread = bread,

    .function0 = bio_f0,
};

#define config_prog \
    "1CLS\n" \
    "2PRINT\"* WiFi parameters *\"\n" \
    "4INPUT\"SSID: \",WSSID$\n" \
    "5INPUT\"PASS: \",WSECRET$\n" \
    "6SAVE\"config$$$\"\n"

int basic_main(int argc, char *argv[])
{
    bool cont = true;
    var_t *var = 0;
    struct sigaction action = {0};

    term_init();

    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    chdir("disk");

    bastos_init(&io);

    int err = bastos_load("config$$$");
    if (err != BERROR_NONE)
        goto after_config;

    print_string("Config file loaded.\r\n");

    var = bastos_var_get("\021WSSID");
    if (!var)
        goto after_config;

    var = bastos_var_get("\021WSECRET");
    if (!var)
        goto after_config;

    print_string("Config vars OK.\r\n");

after_config:

    if (var == 0)
    {
        // No config file or vars, run the config program
        bastos_prog_new();
        bastos_send_keys(config_prog, strlen(config_prog));
        bastos_send_keys("RUN\n", 4);
    }


#ifdef MINITEL
    bool fkey = false;
#endif

    while (cont)
    {
        int key = getch();

#ifdef MINITEL
        if (key == 0x13) {
            fkey = true;
        } else {
            if (fkey) {
                if (key == 0x47) { // CORRECTION
                    key = 0x7F;
                } else if (key == 0x45) { // ANNULATION
                    key = 3;
                } else { // ENVOI
                    key = '\r';
                }
                fkey = false;
            }
            bastos_send_keys((char *)&key, 1);
        }
#else
        if (key == 0x08) {
            key = 0x7F;
        } else if (key == 0x1b) {
            key = 3;
        }
        bastos_send_keys((char *)&key, 1);
#endif

        // if (c != 0 && c != 24 && c != 1)
        // {
        //     char *keys = (char *) &c;
        //     bastos_send_keys(keys, 1);
        // }
        // else
        // {
        //     if (c == 1)
        //     {
        //         bastos_stop();
        //     }
        // }
        bastos_loop();
        cont = key != 24;
    }

    bastos_prog_new();
    term_done();

    return 0;
}

int main()
{
    // bmem_test();
    basic_main(0, 0);
    return 0;
}
