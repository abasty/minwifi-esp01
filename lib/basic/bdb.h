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

#ifndef __BDB_H__
#define __BDB_H__

#include <stdbool.h>
#include "token.h"
#include "bmemory.h"

typedef var_t entry_t;

    // Init entry
#define DB_WIFI_SET ((uint8_t) 32)
#define DB_MIN_SET ((uint8_t) 254)
#define DB_FTP_SET ((uint8_t) 253)

static uint16_t bdb_entry_count();
static entry_t *bdb_entry_set(uint8_t set, const char *name, char *value, bool is_cstr);
static entry_t *bdb_entry_get(uint8_t set, const char *name);
static void bdb_entry_unset(entry_t *entry);
static void bmem_db_clear();

#endif // __BDB_H__
