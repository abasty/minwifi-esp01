#ifdef _WIN32
#include <conio.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define sleep(s) Sleep((s) * 1000)
#define chdir _chdir
#define mkdir(path, mode) _mkdir(path)
#define open _open
#define creat _creat
#define close _close
#define write _write
#define read _read
#define lseek _lseek
#define unlink _unlink
#define rmdir _rmdir
#define getcwd _getcwd
#else
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
#include <sys/time.h>
#endif

#include "tty-minitel.h"

#include "bio.h"
#include "os.h"

/* Low level management */
#ifndef _WIN32
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
#endif

void hal_print_oem_string(void)
{
#ifdef _WIN32
    hal_print_string("Windows");
#else
    hal_print_string("Linux");
#endif
}

#ifdef _WIN32
// Piped stdin (normal deployment via websocat exec:) needs a non-blocking
// read. PeekNamedPipe is the textbook way to do that on Windows, but it
// fails with ERROR_NOT_SUPPORTED on the handle types some spawners (e.g.
// Wine, when it hands us a wrapped Unix pipe fd) give us for redirected
// stdin. A background thread doing plain blocking reads has no such
// dependency and works uniformly everywhere.
static CRITICAL_SECTION g_stdin_cs;
static uint8_t g_stdin_buf[256];
static volatile int g_stdin_head = 0, g_stdin_tail = 0;
static volatile bool g_stdin_closed = false;
static bool g_stdin_thread_started = false;

static DWORD WINAPI stdin_reader_thread(LPVOID unused)
{
    (void)unused;
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    for (;;) {
        uint8_t ch;
        DWORD n = 0;
        if (!ReadFile(h, &ch, 1, &n, NULL) || n == 0)
            break;

        EnterCriticalSection(&g_stdin_cs);
        int next = (g_stdin_head + 1) % (int)sizeof(g_stdin_buf);
        if (next != g_stdin_tail) {
            g_stdin_buf[g_stdin_head] = ch;
            g_stdin_head = next;
        }
        LeaveCriticalSection(&g_stdin_cs);
    }
    g_stdin_closed = true;
    return 0;
}

uint8_t hal_get_key()
{
    // Interactive console (local testing): use conio, never blocks.
    if (_isatty(_fileno(stdin))) {
        if (!_kbhit())
            return 0;
        int ch = _getch();
        if (ch == 0 || ch == 0xE0) {
            // extended key (arrows, function keys): swallow the second byte
            _getch();
            return 0;
        }
        if (ch == 0x08)
            ch = 0x7f;
        return (uint8_t)ch;
    }

    if (!g_stdin_thread_started) {
        g_stdin_thread_started = true;
        InitializeCriticalSection(&g_stdin_cs);
        CreateThread(NULL, 0, stdin_reader_thread, NULL, 0, NULL);
    }

    EnterCriticalSection(&g_stdin_cs);
    uint8_t ch = 0;
    bool got = false;
    if (g_stdin_tail != g_stdin_head) {
        ch = g_stdin_buf[g_stdin_tail];
        g_stdin_tail = (g_stdin_tail + 1) % (int)sizeof(g_stdin_buf);
        got = true;
    }
    LeaveCriticalSection(&g_stdin_cs);

    if (got) {
        if (ch == 0x08)
            ch = 0x7f;
        return ch;
    }

    if (g_stdin_closed) {
        fprintf(stderr, "Error reading key\n");
        exit(0);
    }
    return 0;
}
#else
uint8_t hal_get_key()
{
    struct pollfd input[1] = {{fd : 0, events : POLLIN}};
    usleep(100);
    int ret = poll(input, 1, 0);

    if (ret < 0)
        goto err;

    if (ret == 0)
        return 0;

    uint8_t ch = 0;
    int n = read(0, (char *)&ch, 1);
    if (n <= 0)
        goto err;

    // Conversion from PC key codes to BASTOS ones
    if (ch == 0x08) {
        ch = 0x7f;
    }
    return ch;

err:
    fprintf(stderr, "Error reading key\n");
    term_done();
    exit(0);
}
#endif

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
    // flags is BASTOS's own B_* bitmask (bio.h), not a real O_* flag set —
    // checking it against O_CREAT only ever worked on Linux by coincidence
    // (glibc's O_CREAT is also 0100). On Windows, MinGW's O_CREAT is 0400
    // (256), a disjoint bit, so this always took the open() branch instead
    // of creat() and SAVE silently never created a file.
    if ((flags & B_CREAT) != 0)
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

#ifdef _WIN32
int hal_read(int fd, void *buf, int count)
{
    // Regular files never block on Windows: no poll() equivalent needed.
    return read(fd, buf, count);
}
#else
int hal_read(int fd, void *buf, int count)
{
    struct pollfd input[1] = {{.fd = fd, .events = POLLIN}};
    int ret = poll(input, 1, 1);

    if (ret > 0)
        return read(fd, buf, count);

    return 0;
}
#endif

int hal_get_file_size(const char* pathname)
{
    int fd = hal_open(pathname, O_RDONLY);
    if (fd < 0)
        return 0;

    int fsize = lseek(fd, 0, SEEK_END);
    hal_close(fd);
    return fsize;
}

int hal_file(const char* pathname, char *buffer, uint16_t offset, uint16_t size)
{
    int r = 0;
    int fd = hal_open(pathname, O_RDONLY);
    if (fd < 0)
        return 0;

    if (offset == (uint16_t) lseek(fd, offset, SEEK_SET)) {
        r = read(fd, buffer, size);
    }

    hal_close(fd);
    return r;
}

// Simulated disk size for the PC test binary: the real BASTOS-S hardware
// has its own flash-based quota (see os_cat()'s callers), but on PC we just
// pretend the "disk" directory sits on a disk of this size.
#define HAL_CAT_DISK_SIZE (2 * 1024 * 1024)

// Absolute path of the disk/ directory chdir()'d into at startup (see
// main()), captured once via getcwd() so free space can always be computed
// from the true root regardless of where CD has taken us since.
static char g_disk_root[512] = "";

#ifdef _WIN32
// Recursively sums the size of every file under path (directories
// contribute nothing themselves, only what's inside them, matching the
// non-Windows st_blocks-based version closely enough for a 2 MB toy quota).
static off_t disk_usage(const char *path)
{
    off_t total = 0;
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 0;

    do {
        if (strcmp(".", fd.cFileName) == 0 || strcmp("..", fd.cFileName) == 0)
            continue;

        char child[MAX_PATH];
        snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            total += disk_usage(child);
            continue;
        }

        LARGE_INTEGER size;
        size.HighPart = fd.nFileSizeHigh;
        size.LowPart = fd.nFileSizeLow;
        total += size.QuadPart;
    } while (FindNextFile(h, &fd));

    FindClose(h);
    return total;
}

size_t hal_cat()
{
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile("*", &fd);
    if (h == INVALID_HANDLE_VALUE)
        return HAL_CAT_DISK_SIZE;

    do {
        if (strcmp(".", fd.cFileName) == 0 || strcmp("..", fd.cFileName) == 0)
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            os_cat_dir(fd.cFileName);
            continue;
        }

        LARGE_INTEGER size;
        size.HighPart = fd.nFileSizeHigh;
        size.LowPart = fd.nFileSizeLow;
        os_cat_file(fd.cFileName, (int)size.QuadPart);
    } while (FindNextFile(h, &fd));

    FindClose(h);

    off_t total = disk_usage(g_disk_root);
    return total < HAL_CAT_DISK_SIZE ? HAL_CAT_DISK_SIZE - total : 0;
}
#else
// Recursively sums st_blocks (allocated space, including for directories
// themselves) under path.
static off_t disk_usage(const char *path)
{
    off_t total = 0;
    DIR *dir = opendir(path);
    if (!dir)
        return 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
            continue;

        char child[1024];
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(child, &st) == -1)
            continue;
        total += st.st_blocks * 512; // st_blocks is in 512-byte blocks

        if (S_ISDIR(st.st_mode))
            total += disk_usage(child);
    }
    closedir(dir);
    return total;
}

size_t hal_cat()
{
    struct dirent **entry;
    int n = scandir(".", &entry, NULL, NULL);
    while (n--)
    {
        if (strcmp(".", entry[n]->d_name) && strcmp("..", entry[n]->d_name))
        {
            char *path = entry[n]->d_name;
            struct stat st;
            if (stat(path, &st) == 0) {
                if (S_ISDIR(st.st_mode))
                    os_cat_dir(path);
                else
                    os_cat_file(path, st.st_size);
            }
        }
        free(entry[n]);
    }
    free(entry);

    off_t total = disk_usage(g_disk_root);
    return total < HAL_CAT_DISK_SIZE ? HAL_CAT_DISK_SIZE - total : 0;
}
#endif

int hal_erase(const char *pathname)
{
    return unlink(pathname);
}

int hal_mkdir(const char *pathname)
{
    return mkdir(pathname, 0755);
}

// Depth below the initial disk/ directory chdir()'d into at startup (see
// main()). Real chdir() has no notion of "top of the sandbox" — without
// this, "CD .." at the root would walk straight out of disk/ onto the real
// host filesystem, same escape as an unvalidated absolute path.
static int g_cd_depth = 0;

int hal_chdir(const char *pathname)
{
    if (strcmp(pathname, "..") == 0) {
        if (g_cd_depth == 0)
            return -1; // already at the initial disk root
        if (chdir("..") != 0)
            return -1;
        g_cd_depth--;
        return 0;
    }

    if (chdir(pathname) != 0)
        return -1;
    g_cd_depth++;
    return 0;
}

int hal_rmdir(const char *pathname)
{
    return rmdir(pathname);
}

int hal_is_dir(const char *pathname)
{
    struct stat st;
    if (stat(pathname, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int hal_rename(const char *oldpath, const char *newpath)
{
    return rename(oldpath, newpath);
}

int hal_at_root(void)
{
    return g_cd_depth == 0;
}

void hal_reset()
{
}

void hal_speed(uint8_t fn)
{
    const char *p_speed = fn == TOKEN_KEYWORD_FAST2  ? P_PRISE_9600
                          : fn == TOKEN_KEYWORD_FAST ? P_PRISE_4800
                                                     : P_PRISE_1200;
    hal_print_string(p_speed);
}

int hal_wifi_scan()
{
    // Simulate scanning networks
    sleep(2);
    int n = 0;
    // Register dummy networks
    os_wifi_print_network(++n, "Host network", ENC_NONE, 0);
    os_wifi_print_network(++n, "Maison fake", ENC_NONE, 0);
    os_wifi_print_network(++n, "Reseau 3", ENC_NONE, 0);
    return n;
}

bool wifi_connected = false;
char wifi_ssid[36] = "";
char wifi_ip[16] = "";

int hal_wifi_connect(const char* ssid, const char* secret)
{
    sleep(1);
    if (strcmp(secret, "changeme") == 0)
    {
        // Simulate successful connection
        strncpy(wifi_ssid, ssid, sizeof(wifi_ssid) - 1);
        wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
        wifi_connected = true;
        // Get IP address
        // hal_net_disconnect(hal_net_connect(0, "google.com", 80, ""));
        return 0;
    }
    wifi_connected = false;
    return -1;
}

void hal_wifi_disconnect()
{
    if (wifi_connected)
    {
        wifi_connected = false;
    }
}

bool hal_wifi_is_connected()
{
    if (wifi_connected)
    {
        os_wifi_set_info(wifi_ssid, wifi_ip);
    }
    else
    {
        os_wifi_set_info("", "");
    }
    return wifi_connected;
}

bool g_ftp_connected = false;

#ifdef _WIN32
// Winsock SOCKET is UINT_PTR (64-bit on x64), but every hal_net_* signature
// carries the handle as a plain int (shared with the POSIX build, where fds
// and sockets share the same int space). Windows hands out small sequential
// handle values in practice, so the narrowing cast is safe here, but it is
// not guaranteed by the Winsock API contract.
int hal_net_connect(split_t *urn)
{
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(urn->parts[URN_PART_HOST], NULL, &hints, &res) != 0 || res == NULL)
        return -1;

    struct sockaddr_in addr = *(struct sockaddr_in *)res->ai_addr;
    addr.sin_port = htons(urn->port);
    freeaddrinfo(res);

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        return -1;

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        closesocket(s);
        return -1;
    }

    struct sockaddr_in addr_local;
    int addr_len = sizeof(addr_local);
    getsockname(s, (struct sockaddr *)&addr_local, &addr_len);
    inet_ntop(AF_INET, &addr_local.sin_addr, wifi_ip, sizeof(wifi_ip));

    return (int)s;
}

void hal_net_disconnect(uint8_t set, int n)
{
    closesocket((SOCKET)n);
}

int hal_net_send(int fd, const uint8_t *buffer, int n)
{
    return send((SOCKET)fd, (const char *)buffer, n, 0);
}

int hal_net_recv(int fd, uint8_t *buffer, int n)
{
    WSAPOLLFD input[1] = {{.fd = (SOCKET)fd, .events = POLLIN}};
    int ret = WSAPoll(input, 1, 1);

    if (ret > 0)
        return recv((SOCKET)fd, (char *)buffer, n, 0);

    return 0;
}
#else
int hal_net_connect(split_t *urn)
{
    struct sockaddr_in addr;
    struct sockaddr_in addr_local;

    struct hostent *hp = gethostbyname(urn->parts[URN_PART_HOST]);
    if (hp == 0)
        return -1;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    addr.sin_port = htons(urn->port);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        close(fd);
        return -1;
    }
    // Get local address
    socklen_t addr_len = sizeof(addr_local);
    getsockname(fd, (struct sockaddr *)&addr_local, &addr_len);
    // Set wifi IP address
    inet_ntop(AF_INET, &addr_local.sin_addr, wifi_ip, sizeof(wifi_ip));

    return fd;
}

void hal_net_disconnect(uint8_t set, int n)
{
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
#endif

#ifdef _WIN32
uint64_t hal_get_ms(void)
{
    return GetTickCount64();
}
#else
uint64_t hal_get_ms(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_usec / 1000 + t.tv_sec * 1000;
}
#endif

#ifdef _WIN32
int hal_get_function_key(void)
{
    uint8_t fn = 0;
    char path[MAX_PATH];
    const char *tmp = getenv("TEMP");
    if (tmp == NULL)
        tmp = ".";
    snprintf(path, sizeof(path), "%s\\fkey", tmp);

    int fkey = open(path, O_RDONLY);
    if (fkey >= 0) {
        read(fkey, &fn, 1);
        close(fkey);
        unlink(path);
    }
    return fn;
}
#else
int hal_get_function_key(void)
{
    uint8_t fn = 0;
    int fkey = open("/tmp/fkey", O_RDONLY);
    if (fkey >= 0) {
        read(fkey, &fn, 1);
        close(fkey);
        unlink("/tmp/fkey");
    }
    return fn;
}
#endif

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
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    struct sigaction action = {0};
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    term_init();
#endif

    mkdir("disk", 0755);
    if (chdir("disk") != 0) {
        fprintf(stderr, "Impossible d'accéder au répertoire 'disk'\n");
        return 1;
    }
    if (!getcwd(g_disk_root, sizeof(g_disk_root))) {
        fprintf(stderr, "Impossible de résoudre le chemin de 'disk'\n");
        return 1;
    }

    while (true)
    {
        setup();
        while (!bastos_is_reset())
        {
            loop();
        }
    }
}
