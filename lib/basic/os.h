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

typedef struct network_s
{
    int32_t dBm;        // rssi in dBm
    uint8_t encryption; // Encryption
    uint8_t count;      // Count of sucessful connections
    char ssid[36];      // ssid
    char secret[64];    // password / passphrase
} network_t;

void os_bootstrap(void);

void os_wifi_add_network(const char *ssid, uint8_t encryption, int32_t rssi);
network_t *os_wifi_get_network_by_ssid(const char *ssid);
network_t *os_wifi_get_network_by_id(int16_t id);
void os_wifi_remove_unknown_networks(void);
void os_wifi_list_networks(void);
int os_wifi_connect(network_t *net);
uint8_t os_get_key(void);
int os_get_string(char *buf, int size, char secret_char);

uint8_t hal_get_key(void);
int hal_print_string(const char *s);
int hal_print_float(float f);
int hal_print_integer(const char *format, int i);
int hal_open(const char *pathname, int flags);
int hal_close(int fd);
int hal_write(int fd, const void *buf, int count);
int hal_read(int fd, void *buf, int count);
void hal_cat(void);
void hal_speed(uint8_t fn);
int hal_erase(const char *pathname);
int hal_wifi_scan(void);
int hal_wifi_connect(const char* ssid, const char* secret);

#ifdef __cplusplus
}
#endif

#endif // __OS_H__
