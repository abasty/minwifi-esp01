/*
 * Minserve build only: client of the Minserve program socket, used for the
 * application data (GET, PUT, DB LIST, DB ERASE). The protocol is described
 * in minserve/documents/protocole-exec.md: one text request, one answer, over
 * an AF_UNIX socket; every request acts on behalf of the user logged in on
 * the Minserve session that started the program.
 *
 * Every failure (no socket, nobody logged in, server gone, timeout) is
 * reported to the caller as -1: BASTOS then behaves as if there was no data.
 *
 * This whole file is empty in the other builds.
 */

#ifdef BASTOS_MINSERVE

#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "os.h"

#define KV_TIMEOUT_S (2)
#define KV_HEADER_MAX (1024)
// Longest value or key list we are ready to receive.
#define KV_BODY_MAX (16384)
// Longest percent-encoded key (200 characters on the server side).
#define KV_KEY_MAX (200)

static int g_fd = -1;

static void kv_close(void)
{
    if (g_fd >= 0)
        close(g_fd);
    g_fd = -1;
}

static bool kv_send_all(const void *buf, size_t n)
{
    const uint8_t *p = buf;
    while (n > 0) {
        ssize_t w = send(g_fd, p, n, MSG_NOSIGNAL);
        if (w <= 0)
            return false;
        p += w;
        n -= (size_t)w;
    }
    return true;
}

static bool kv_recv_all(void *buf, size_t n)
{
    uint8_t *p = buf;
    while (n > 0) {
        ssize_t r = recv(g_fd, p, n, 0);
        if (r <= 0)
            return false;
        p += r;
        n -= (size_t)r;
    }
    return true;
}

// Reads one answer line (without the newline).
static bool kv_recv_line(char *line, size_t size)
{
    size_t n = 0;
    while (n < size - 1) {
        char c;
        if (!kv_recv_all(&c, 1))
            return false;
        if (c == '\n') {
            if (n > 0 && line[n - 1] == '\r')
                n--;
            line[n] = 0;
            return true;
        }
        line[n++] = c;
    }
    return false;
}

// "taille=N" of an answer line, 0 when absent.
static long kv_size_of(const char *line)
{
    const char *p = strstr(line, "taille=");
    return p ? strtol(p + 7, NULL, 10) : 0;
}

// Sends a request, reads the answer. On success (answer "ok ...") returns the
// body length (0 for no body) and fills body (cap bytes at most); -1 on an
// "err" answer or when the exchange fails.
static int kv_exchange(const char *header, const void *req_body, size_t req_len,
                       uint8_t *body, size_t cap)
{
    char line[KV_HEADER_MAX];

    if (!kv_send_all(header, strlen(header)) ||
        (req_len > 0 && !kv_send_all(req_body, req_len)) ||
        !kv_recv_line(line, sizeof(line))) {
        kv_close();
        return -2; // transport error: the caller may reconnect
    }

    long size = kv_size_of(line);
    if (size < 0 || size > KV_BODY_MAX) {
        kv_close();
        return -1;
    }

    // Always read the whole body so that the connection stays in sync.
    uint8_t *tmp = size > 0 ? malloc((size_t)size) : NULL;
    if (size > 0 && (tmp == NULL || !kv_recv_all(tmp, (size_t)size))) {
        free(tmp);
        kv_close();
        return -2;
    }

    int ret = -1;
    if (strncmp(line, "ok", 2) == 0 && (line[2] == 0 || line[2] == ' ')) {
        size_t n = (size_t)size < cap ? (size_t)size : cap;
        if (n > 0)
            memcpy(body, tmp, n);
        ret = (int)n;
    }
    free(tmp);
    return ret;
}

static bool kv_connect(void)
{
    const char *path = getenv("MINSERVE_SOCKET");
    const char *token = getenv("MINSERVE_TOKEN");
    if (!path || !token)
        return false;

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    if (strlen(path) >= sizeof(addr.sun_path))
        return false;
    strcpy(addr.sun_path, path);

    g_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_fd < 0)
        return false;

    struct timeval tv = {.tv_sec = KV_TIMEOUT_S};
    setsockopt(g_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(g_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(g_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        kv_close();
        return false;
    }

    char header[KV_HEADER_MAX];
    snprintf(header, sizeof(header), "auth jeton=%s\n", token);
    uint8_t reply[64];
    if (kv_exchange(header, NULL, 0, reply, sizeof(reply)) < 0) {
        kv_close();
        return false;
    }
    return true;
}

// One request, with one reconnection if the connection was lost.
static int kv_request(const char *header, const void *req_body, size_t req_len,
                      uint8_t *body, size_t cap)
{
    for (int attempt = 0; attempt < 2; attempt++) {
        if (g_fd < 0 && !kv_connect())
            return -1;
        int ret = kv_exchange(header, req_body, req_len, body, cap);
        if (ret != -2)
            return ret;
    }
    return -1;
}

// Percent-encodes src into dst (see "Format" in protocole-exec.md).
static bool kv_encode(const char *src, char *dst, size_t size)
{
    size_t n = 0;
    for (; *src; src++) {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || strchr("._~-:,@/", c)) {
            if (n + 1 >= size)
                return false;
            dst[n++] = (char)c;
        } else {
            if (n + 3 >= size)
                return false;
            n += (size_t)snprintf(dst + n, 4, "%%%02X", c);
        }
    }
    dst[n] = 0;
    return true;
}

// Decodes a percent-encoded value in place.
static void kv_decode(char *s)
{
    char *dst = s;
    while (*s) {
        unsigned int c;
        if (s[0] == '%' && s[1] && s[2] && sscanf(s + 1, "%2x", &c) == 1) {
            *dst++ = (char)c;
            s += 3;
        } else {
            *dst++ = *s++;
        }
    }
    *dst = 0;
}

bool hal_kv_active(void)
{
    return getenv("MINSERVE_SOCKET") != NULL && getenv("MINSERVE_TOKEN") != NULL;
}

int hal_kv_get(const char *key, uint8_t *buf, int size)
{
    char enc[3 * KV_KEY_MAX + 1], header[KV_HEADER_MAX];
    if (!kv_encode(key, enc, sizeof(enc)))
        return -1;
    snprintf(header, sizeof(header), "kv.get cle=%s\n", enc);
    return kv_request(header, NULL, 0, buf, (size_t)size);
}

int hal_kv_set(const char *key, const uint8_t *value, int len)
{
    char enc[3 * KV_KEY_MAX + 1], header[KV_HEADER_MAX];
    if (!kv_encode(key, enc, sizeof(enc)))
        return -1;
    snprintf(header, sizeof(header), "kv.set cle=%s type=octets taille=%d\n", enc, len);
    return kv_request(header, value, (size_t)len, NULL, 0) < 0 ? -1 : 0;
}

int hal_kv_delete(const char *key)
{
    char enc[3 * KV_KEY_MAX + 1], header[KV_HEADER_MAX];
    if (!kv_encode(key, enc, sizeof(enc)))
        return -1;
    snprintf(header, sizeof(header), "kv.delete cle=%s\n", enc);
    return kv_request(header, NULL, 0, NULL, 0) < 0 ? -1 : 0;
}

int hal_kv_keys(const char *prefix, char *buf, int size)
{
    char enc[3 * KV_KEY_MAX + 1], header[KV_HEADER_MAX];
    if (!kv_encode(prefix, enc, sizeof(enc)))
        return -1;
    snprintf(header, sizeof(header), "kv.keys prefixe=%s limite=1000\n", enc);

    uint8_t *body = malloc(KV_BODY_MAX + 1);
    if (!body)
        return -1;
    int n = kv_request(header, NULL, 0, body, KV_BODY_MAX);
    if (n < 0) {
        free(body);
        return -1;
    }
    body[n] = 0;

    // One encoded key per line: decode, drop the prefix, keep "key\n".
    size_t plen = strlen(prefix), out = 0;
    char *line = strtok((char *)body, "\n");
    while (line) {
        kv_decode(line);
        if (strncmp(line, prefix, plen) == 0) {
            size_t len = strlen(line + plen);
            if (out + len + 1 < (size_t)size) {
                memcpy(buf + out, line + plen, len);
                out += len;
                buf[out++] = '\n';
            }
        }
        line = strtok(NULL, "\n");
    }
    free(body);
    return (int)out;
}

#endif // BASTOS_MINSERVE
