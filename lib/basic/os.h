/*
 * Copyright © 2023 Alain Basty
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

#include "bio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENC_TKIP (2)
#define ENC_WEP  (5)
#define ENC_CCMP (4)
#define ENC_NONE (7)
#define ENC_AUTO (8)

#define URN_PROTO_TCP  (0)
#define URN_PROTO_WS   (1)
#define URN_PROTO_WSS  (2)

#define NET_BUFFER_MAX (128)

// OS main functions
void os_setup(void);
void os_loop(void);

// OS Wi-Fi functions
void os_wifi_list(void);
void os_wifi_add_network(const char *ssid, uint8_t encryption, int32_t rssi);
int16_t os_wifi_get_id_from_ssid(const char *ssid);
int os_wifi_connect(int16_t net_id);
int os_wifi_erase(int16_t net_id);
void os_wifi_status(void);
void os_wifi_set_info(const char *ssid, const char *ip);

// OS network functions
int os_connect(const char* urn);

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

int hal_net_connect(uint16_t proto, const char* host, uint16_t port, const char* path);
void hal_net_disconnect(int n);
int hal_net_send(int fd, const uint8_t *buffer, int n);
int hal_net_recv(int fd, uint8_t *buffer, int n);

void hal_speed(uint8_t fn);
void hal_reset(void);

#ifdef __cplusplus
}
#endif

#endif // __OS_H__
