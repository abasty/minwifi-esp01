/*
 * Copyright © 2023-2025 Alain Basty
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __OS_H__
#define __OS_H__

#include <sys/types.h>

#include "bio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENC_TKIP (2)
#define ENC_WEP  (5)
#define ENC_CCMP (4)
#define ENC_NONE (7)
#define ENC_AUTO (8)

#define NET_BUFFER_MAX (128)

#define WIFI_SSID_ARRAY_SIZE (10)
#define WIFI_SSID_MAX_SIZE (36)
#define WIFI_SECRET_MAX_SIZE (64)

#define URN_PROTO_NONE (0)
#define URN_PROTO_TCP  (1)
#define URN_PROTO_WS   (2)
#define URN_PROTO_WSS  (3)
#define URN_PROTO_FTP  (4)

#define URN_SIZE_MAX   (192)

#define URN_PART_PROTO (0)
#define URN_PART_HOST  (1)
#define URN_PART_PORT  (2)
#define URN_PART_PATH  (3)
#define URN_PART_LOGIN (4)
#define URN_PART_PASS  (5)
#define URN_PARTS_MAX  (8)

// Init entry
#define DB_WIFI_SET ((uint8_t) 255)
#define DB_MIN_SET ((uint8_t) 254)
#define DB_FTP_SET ((uint8_t) 253)

typedef struct split_s {
    uint8_t n;
    uint8_t proto;
    uint16_t port;
    char *parts[URN_PARTS_MAX];
    char urn[URN_SIZE_MAX];
} split_t;

// OS main functions
void os_setup(void);
void os_loop(void);

// OS config function
void os_db_load(void);
void os_db_save(void);

// OS Wi-Fi functions
void os_wifi_scan(void);
void os_wifi_print_network(int i, const char *ssid, uint8_t encryption, int32_t rssi);
int os_wifi_connect(const char* ssid);
int os_wifi_erase(const char* ssid);
void os_wifi_status(void);
void os_wifi_set_info(const char *ssid, const char *ip);

// OS DB functions
void os_db_list(uint8_t set);
void os_db_erase(uint8_t set, const char *name);

// OS network functions
int os_connect(uint8_t set, const char *name, const char* urn);
void os_disconnect(uint8_t set);
void os_ftp_status(void);
bool os_ftp_cat(void);
void os_ftp_cat_file(const char* line);

// OS keyboard functions
uint8_t os_get_key(void);
int os_get_string(char *buf, int size, char secret_char);

// OS file functions
void os_cat_file(const char *filename, size_t size);
void os_cat(void);

// HAL functions
void hal_print_oem_string(void);

uint8_t hal_get_key(void);

int hal_print_string(const char *s);
int hal_print_float(float f);
int hal_print_integer(const char *format, int i);
int hal_print_buffer(uint8_t *buffer, int n);

int hal_open(const char *pathname, int flags);
int hal_close(int fd);
int hal_write(int fd, const void *buf, int count);
int hal_read(int fd, void *buf, int count);

size_t hal_cat(void);
int hal_erase(const char *pathname);

int hal_wifi_scan(void);
int hal_wifi_connect(const char* ssid, const char* secret);
void hal_wifi_disconnect(void);
bool hal_wifi_is_connected(void);

int hal_net_connect(split_t *urn_split);
void hal_net_disconnect(uint8_t set, int fd);
int hal_net_send(int fd, const uint8_t *buffer, int n);
int hal_net_recv(int fd, uint8_t *buffer, int n);

ssize_t hal_ftp_cat(void);
bool hal_ftp_is_connected(void);
bool hal_ftp_files(uint8_t func, const char *filename);

void hal_speed(uint8_t fn);
void hal_reset(void);

#ifdef __cplusplus
}
#endif

#endif // __OS_H__
