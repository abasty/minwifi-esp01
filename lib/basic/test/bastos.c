#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifdef MINITEL
#include "tty-minitel.h"
#endif

#include "bio.h"
#include "os.h"

/* Low level management */
struct sigaction old_action;
struct termios old, new;

void term_init()
{
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &new);
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

uint8_t hal_get_key()
{
    struct pollfd input[1] = {{fd : 0, events : POLLIN}};
    int ret = poll(input, 1, 1);

    if (ret < 0)
        goto err;

    if (ret == 0)
        return 0;

    uint8_t ch = 0;
    int n = read(0, (char *)&ch, 1);
    if (n <= 0)
        goto err;

    return ch;

err:
    fprintf(stderr, "Error reading key\n");
    term_done();
    exit(0);
}

int hal_print_float(float f)
{
    int n = printf("%g", f);
    fflush(stdout);
    return n;
}

int hal_print_string(const char *s)
{
    int n = printf("%s", s);
    fflush(stdout);
    return n;
}

int hal_print_integer(const char *format, int32_t i)
{
    int n = printf(format, i);
    fflush(stdout);
    return n;
}

int hal_print_buffer(uint8_t *buffer, int n)
{
    n = fwrite(buffer, 1, n, stdout);
    fflush(stdout);
    return n;
}

int hal_open(const char *pathname, int flags)
{
    if ((flags & O_CREAT) != 0)
        return creat(pathname, 0644);

    return open(pathname, flags);
}

int hal_close(int fd)
{
    return close(fd);
}

int hal_write(int fd, const void *buf, int count)
{
    return write(fd, buf, count);
}

int hal_read(int fd, void *buf, int count)
{
    struct pollfd input[1] = {{.fd = fd, .events = POLLIN}};
    int ret = poll(input, 1, 1);

    if (ret > 0)
        return read(fd, buf, count);

    return 0;
}

void hal_cat()
{
    off_t total = 0;
    hal_print_string("\r\nDrive: A\r\n\r\n");
    struct dirent **entry;
    int n = scandir(".", &entry, NULL, NULL);
    while (n--)
    {
        if (strcmp(".", entry[n]->d_name) && strcmp("..", entry[n]->d_name))
        {
            char *path = entry[n]->d_name;
            struct stat st;
            if (stat(path, &st) == -1)
                continue;
            total += st.st_blocks * 512; // st_blocks is in 512-byte blocks

            uint8_t len = strlen(path);
            hal_print_string(path);
            for (int i = 16 - len; i > 0; i--)
                hal_print_string(" ");
            hal_print_integer("%ju\r\n", st.st_size);
        }
        free(entry[n]);
    }
    free(entry);
    hal_print_integer("\r\n%3uK free\r\n\r\nReady\r\n", (524288 - total) / 1024);
    fflush(stdout);
}

int hal_erase(const char *pathname)
{
    return unlink(pathname);
}

void hal_reset()
{
}

void hal_speed(uint8_t fn)
{
    if (fn == TOKEN_KEYWORD_FAST || fn == TOKEN_KEYWORD_SLOW)
    {
#ifdef MINITEL
        hal_print_string(fn == TOKEN_KEYWORD_FAST ? P_PRISE_4800 : P_PRISE_1200);
#endif
    }
}

int hal_wifi_scan()
{
    // Simulate scanning networks
    sleep(2);
    // Register dummy networks
    os_wifi_add_network("Host network", ENC_NONE, 0);
    os_wifi_add_network("Maison fake", ENC_NONE, 0);
    os_wifi_add_network("Reseau 3", ENC_NONE, 0);
    return 1;
}

int hal_wifi_connect(const char* ssid, const char* secret)
{
    sleep(1);
    if (strcmp(secret, "changeme") == 0)
    {
        // Simulate successful connection
        return 0;
    }
    return -1;
}

struct sockaddr_in addr;

int hal_net_connect(uint16_t proto, const char* host, uint16_t port, const char* path)
{
    // Do the connection (TCP socket or WebSocket)
    // Disable nagle's algo

    struct hostent *hp = gethostbyname(host);
    if (hp == 0)
        return -1;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    addr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        close(fd);
        fd = -1;
    }

    return fd;
}

void hal_net_disconnect(int n)
{
    // If connected, disconnect and remove associated resources
    close(n);
}

int hal_net_send(int fd, const uint8_t *buffer, int n)
{
    return write(fd, buffer, n);
}

int hal_net_recv(int fd, uint8_t *buffer, int n)
{
    struct pollfd input[1] = {{fd : fd, events : POLLIN}};
    int ret = poll(input, 1, 1);

    if (ret > 0)
        return read(fd, buffer, n);

    return 0;
}

void setup()
{
    os_setup();
}

void loop(void)
{
    os_loop();
}

int main()
{
    struct sigaction action = {0};
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    term_init();

    chdir("disk");

    while (true)
    {
        setup();
        while (!bastos_is_reset())
        {
            loop();
        }
        bastos_done();
    }
}
