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

/*
 * Unit tests for lib/basic bugs.
 * Each test demonstrates a bug (FAIL before fix, PASS after fix).
 *
 * Build with:  make test_bugs  (in lib/basic/test/)
 * Run with:    ./bin/test_bugs
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "bio.h"
#include "os.h"

/* ---- Output capture ---------------------------------------------------- */

#define OUTPUT_BUF_SIZE 16384
static char g_output[OUTPUT_BUF_SIZE];
static int  g_output_len = 0;

static void capture_clear(void) {
    g_output_len = 0;
    g_output[0] = '\0';
}

static void capture_append(const char *s, int len) {
    if (g_output_len + len < OUTPUT_BUF_SIZE - 1) {
        memcpy(g_output + g_output_len, s, len);
        g_output_len += len;
        g_output[g_output_len] = '\0';
    }
}

/* ---- HAL stubs ---------------------------------------------------------- */

void hal_print_oem_string(void) {}

uint8_t hal_get_key(void) { return 0; }

int hal_print_float(float f) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%g", (double)f);
    capture_append(buf, n);
    return n;
}

int hal_print_string(const char *s) {
    int n = (int)strlen(s);
    capture_append(s, n);
    return n;
}

int hal_print_integer(const char *format, int i) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), format, i);
    capture_append(buf, n);
    return n;
}

int hal_print_buffer(uint8_t *buffer, int n) {
    capture_append((const char *)buffer, n);
    return n;
}

int hal_open(const char *pathname, int flags) {
    (void)pathname;
    (void)flags;
    return -1;
}

int hal_close(int fd) {
    (void)fd;
    return 0;
}

int hal_write(int fd, const void *buf, int count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

int hal_read(int fd, void *buf, int count) {
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

int hal_get_file_size(const char *pathname) {
    (void)pathname;
    return 0;
}

int hal_file(const char *pathname, char *buffer, uint16_t offset, uint16_t size) {
    (void)pathname;
    (void)buffer;
    (void)offset;
    (void)size;
    return 0;
}

size_t hal_cat(void) { return 0; }

int hal_erase(const char *pathname) {
    (void)pathname;
    return 0;
}

int hal_wifi_scan(void) { return 0; }

int hal_wifi_connect(const char *ssid, const char *secret) {
    (void)ssid;
    (void)secret;
    return -1;
}

void hal_wifi_disconnect(void) {}

bool hal_wifi_is_connected(void) { return false; }

int hal_net_connect(split_t *urn) {
    (void)urn;
    return -1;
}

void hal_net_disconnect(uint8_t set, int n) {
    (void)set;
    (void)n;
}

int hal_net_send(int fd, const uint8_t *buffer, int n) {
    (void)fd;
    (void)buffer;
    (void)n;
    return -1;
}

int hal_net_recv(int fd, uint8_t *buffer, int n) {
    (void)fd;
    (void)buffer;
    (void)n;
    return 0;
}

void hal_speed(uint8_t fn) { (void)fn; }

void hal_reset(void) {}

uint64_t hal_get_ms(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (uint64_t)(t.tv_usec / 1000) + (uint64_t)(t.tv_sec * 1000);
}

int hal_get_function_key(void) { return 0; }

/* ---- Test helpers ------------------------------------------------------- */

/*
 * Run a NULL-terminated array of BASIC program lines then RUN the program.
 * Waits until "Ready" appears in the captured output (program finished).
 * Returns the captured output (static buffer — valid until next call).
 */
static const char *run_program(const char **lines) {
    bastos_init();

    /* Enter each program line (line_no + statement) */
    for (const char **l = lines; *l != NULL; l++) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\r", *l);
        bastos_send_keys(buf, strlen(buf), false);
        bastos_loop();
    }

    /* Clear output, then send RUN */
    capture_clear();
    bastos_send_keys("RUN\r", 4, false);

    /* Pump the interpreter until "Ready" (normal or error exit) */
    for (int i = 0; i < 500000 && strstr(g_output, "Ready") == NULL; i++) {
        bastos_loop();
    }

    bastos_done();
    return g_output;
}

/* ---- Test result tracking ---------------------------------------------- */

static int g_tests_run    = 0;
static int g_tests_failed = 0;

static void check(const char *test_name, bool condition) {
    g_tests_run++;
    if (condition) {
        printf("  [PASS] %s\n", test_name);
    } else {
        printf("  [FAIL] %s\n", test_name);
        g_tests_failed++;
    }
}

/* ======================================================================== */
/* Bug 1 — eval.c-static:382  UNPLOT uses logical NOT (!) instead of        */
/*         bitwise NOT (~), zeroing the whole screen cell instead of         */
/*         clearing only the target pixel bit.                               */
/* ======================================================================== */
static void test_bug1_unplot(void) {
    printf("Bug 1: UNPLOT logical NOT (!) vs bitwise NOT (~)\n");

    /*
     * PLOT (0,0) and PLOT (1,0) share the same screen cell (same column C,
     * same row L).  After UNPLOT (0,0), pixel (1,0) must still be set.
     *
     * Before fix: *addr &= !cbit → *addr &= 0 → entire cell zeroed.
     *             TEST(1,0) == 0  →  BASIC prints "FAIL"
     * After fix:  *addr &= ~cbit → only bit for (0,0) cleared.
     *             TEST(1,0) != 0  →  BASIC prints "PASS"
     */
    const char *lines[] = {
        "10 PLOT 0,0",
        "20 PLOT 1,0",
        "30 UNPLOT 0,0",
        "40 IF TEST(1,0) > 0 THEN PRINT \"PASS\" ELSE PRINT \"FAIL\"",
        NULL
    };
    const char *out = run_program(lines);
    check("UNPLOT clears only target pixel; neighbour pixel survives",
          strstr(out, "PASS") != NULL && strstr(out, "FAIL") == NULL);
}

/* ======================================================================== */
/* Bug 2 — eval.c-static:898  Unsigned underflow: `end - start < 0` is      */
/*         always false for uint16_t, so slice length wraps to 65535 when    */
/*         start > end, causing BERROR_MEMORY instead of an empty slice.     */
/* ======================================================================== */
static void test_bug2_string_slice(void) {
    printf("Bug 2: string_slice unsigned underflow (start > end)\n");

    /*
     * A$(4,2) on "HELLO" (len=5): start=4, end=2.
     * Both are in range so clamping leaves them unchanged.
     *
     * Before fix: end - start = (uint16_t)(2 - 4) = 65534, +1 = 65535.
     *             bmem_string_alloc(65535) "succeeds" with corrupted length.
     *             LEN(B$) returns 65535, not 0 → BASIC prints "FAIL".
     * After fix:  (end >= start) is false → slice_len = 0 → empty string.
     *             LEN(B$) returns 0 → BASIC prints "PASS".
     */
    const char *lines[] = {
        "10 LET A$ = \"HELLO\"",
        "20 LET B$ = A$(4,2)",
        "30 IF LEN(B$) = 0 THEN PRINT \"PASS\" ELSE PRINT \"FAIL\"",
        NULL
    };
    const char *out = run_program(lines);
    check("A$(start,end) with start > end yields empty string (no error)",
          strstr(out, "PASS") != NULL && strstr(out, "FAIL") == NULL);
}

/* ======================================================================== */
/* Bug 4 — bmemory.c-static:163  Off-by-one in bastos_var_get: strncpy       */
/*         copies (len-1) characters for *all* variable names, dropping the  */
/*         last character of numeric variable names.                          */
/* ======================================================================== */
static void test_bug4_var_get(void) {
    printf("Bug 4: bastos_var_get off-by-one (last char dropped)\n");

    bastos_init();
    /* Suppress init output */
    capture_clear();

    /* Set a single-letter numeric variable via BASIC */
    bastos_send_keys("LET A=42\r", 9, false);
    bastos_loop();

    /*
     * Before fix: strncpy(typed_name+1, "A", len-1=0) copies 0 chars.
     *             typed_name = {TOKEN_NUMBER, '\0'} → searches for "".
     *             bastos_var_get("A") returns NULL.
     * After fix:  strncpy copies 1 char → typed_name = {TOKEN_NUMBER,'A','\0'}.
     *             bastos_var_get("A") returns the correct var_t pointer.
     */
    var_t *v1 = bastos_var_get("A");
    check("bastos_var_get finds 1-char numeric variable 'A'", v1 != NULL);

    /* Set a two-letter numeric variable */
    bastos_send_keys("LET AB=99\r", 10, false);
    bastos_loop();

    /*
     * Before fix: strncpy copies len-1=1 char ('A') → searches for "A".
     *             bastos_var_get("AB") returns a non-NULL pointer to var "A"
     *             (value=42), not "AB" (value=99).
     * After fix:  copies 2 chars → finds "AB" correctly (value=99).
     */
    var_t *v2 = bastos_var_get("AB");
    check("bastos_var_get finds 2-char numeric variable 'AB'",
          v2 != NULL && v2->numbers[0] == 99.0f);

    bastos_done();
}

/* ======================================================================== */
/* Bug 6 — eval.c-static:668  STR$() allocates a 16-byte string but never   */
/*         updates the length header after sprintf, so LEN(STR$(x)) always   */
/*         returns 16 regardless of the actual string length.                */
/* ======================================================================== */
static void test_bug6_str_length(void) {
    printf("Bug 6: STR$() does not update string length header\n");

    /*
     * sprintf("%g", 3.14f) produces "3.14" → 4 characters.
     * LEN(STR$(3.14)) should therefore equal 4.
     *
     * Before fix: bmem_string_alloc(16) sets length header to 16; sprintf
     *             does not update it.  LEN reads the header → 16. FAIL.
     * After fix:  length header is updated to sprintf's return value (4).
     *             LEN reads the header → 4. PASS.
     */
    const char *lines[] = {
        "10 IF LEN(STR$(3.14)) = 4 THEN PRINT \"PASS\" ELSE PRINT \"FAIL\"",
        NULL
    };
    const char *out = run_program(lines);
    check("LEN(STR$(3.14)) == 4", strstr(out, "PASS") != NULL && strstr(out, "FAIL") == NULL);
}

/* ======================================================================== */
/* main                                                                       */
/* ======================================================================== */
int main(void) {
    printf("=== lib/basic Bug Tests ===\n\n");

    test_bug1_unplot();
    printf("\n");

    test_bug2_string_slice();
    printf("\n");

    test_bug4_var_get();
    printf("\n");

    test_bug6_str_length();
    printf("\n");

    printf("=== Results: %d/%d tests passed ===\n",
           g_tests_run - g_tests_failed, g_tests_run);

    return g_tests_failed > 0 ? 1 : 0;
}
