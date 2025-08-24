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

#ifndef __OS_PRIVATE_H__
#define __OS_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// OS DB functions
static void os_db_load(void);
static void os_db_save(void);
static void os_db_list(uint8_t set);
static void os_db_erase(uint8_t set, const char *name);

// OS Wi-Fi functions
static void os_wifi_scan(void);
static int os_wifi_connect(const char* ssid);
static int os_wifi_erase(const char* ssid);
static void os_wifi_status(void);

// OS network functions
static int os_connect(uint8_t set, const char *name, const char* urn);
static void os_disconnect(uint8_t set);
static void os_ftp_status(void);
static bool os_ftp_cat(void);

// OS keyboard functions
static uint8_t os_get_key(void);
static int os_get_string(char *buf, int size, char secret_char);

// OS file functions
static char *os_filename(const char *name, int16_t *type);
static void os_cat(void);

// OS printing and redirect functions
static void os_set_redirect(int fd);
static int os_redir_print_string(const char *s);
static int os_redir_print_integer(const char *format, int i);
static int os_redir_print_float(float f);
static int os_redir_print_buffer(uint8_t *buffer, int n);

// OS startup function
static void os_autoexec(void);

#ifdef __cplusplus
}
#endif

#endif // __OS_PRIVATE_H__
