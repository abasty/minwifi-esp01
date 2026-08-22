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
#include <fcntl.h>
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

/* Large enough to hold any realistic BASIC program output in tests.        */
/* The BASIC interpreter's own io_buffer is 4096 bytes; 16384 gives us      */
/* comfortable headroom for multi-line output without arbitrary truncation. */
#define OUTPUT_BUF_SIZE 16384
static char g_output[OUTPUT_BUF_SIZE];
static int  g_output_len = 0;

static void capture_clear(void) {
    g_output_len = 0;
    g_output[0] = '\0';
}

static void capture_append(const char *s, int len) {
    /* Reserve one byte for the null terminator. */
    if (g_output_len + len <= OUTPUT_BUF_SIZE - 1) {
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

/*
 * Real file I/O (not a stub): needed so LOAD/SAVE regression tests (e.g.
 * test_bug_load_immediate_null_read_ptr) can exercise the actual code path,
 * matching how bastos.c's own HAL implements these.
 */
int hal_open(const char *pathname, int flags) {
    if ((flags & O_CREAT) != 0)
        return creat(pathname, 0644);
    return open(pathname, flags);
}

int hal_close(int fd) { return close(fd); }

int hal_write(int fd, const void *buf, int count) {
    return write(fd, buf, count);
}

int hal_read(int fd, void *buf, int count) {
    struct pollfd input[1] = {{.fd = fd, .events = POLLIN}};
    int ret = poll(input, 1, 1);
    if (ret > 0)
        return read(fd, buf, count);
    return 0;
}

int hal_get_file_size(const char *pathname) {
    int fd = hal_open(pathname, O_RDONLY);
    if (fd < 0)
        return 0;
    int fsize = lseek(fd, 0, SEEK_END);
    hal_close(fd);
    return fsize;
}

int hal_file(const char *pathname, char *buffer, uint16_t offset, uint16_t size) {
    (void)pathname;
    (void)buffer;
    (void)offset;
    (void)size;
    return 0;
}

size_t hal_cat(void) { return 0; }

int hal_erase(const char *pathname) { return unlink(pathname); }

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

    /*
     * bastos_init() places an initial command in io_buffer (the "bastos"
     * splash).  Drain it — and any other queued init commands — before we
     * start entering program lines.  bastos_loop() is a no-op when
     * io_buffer is empty, so extra calls are harmless.
     */
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Enter each program line (line_no + statement) */
    for (const char **l = lines; *l != NULL; l++) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\r", *l);
        bastos_send_keys(buf, strlen(buf), false);
        /* Two calls: one may be needed for a previous residual, one for ours */
        bastos_loop();
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
     *             TEST 1,0 == 0  →  IF condition is false, "PASS" never printed.
     * After fix:  *addr &= ~cbit → only bit for (0,0) cleared.
     *             TEST 1,0 != 0  →  IF condition is true → "PASS" is printed.
     *
     */
    const char *lines[] = {
        "10 PLOT 0,0",
        "20 PLOT 1,0",
        "30 UNPLOT 0,0",
        "40 IF TEST 1,0 > 0 THEN PRINT \"PASS\"",
        NULL
    };
    const char *out = run_program(lines);
    check("UNPLOT clears only target pixel; neighbour pixel survives",
          strstr(out, "PASS") != NULL);
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
        "30 IF LEN(B$) = 0 THEN PRINT \"PASS\"",
        NULL
    };
    const char *out = run_program(lines);
    check("A$(start,end) with start > end yields empty string (no error)",
          strstr(out, "PASS") != NULL);
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
        "10 IF LEN(STR$(3.14)) = 4 THEN PRINT \"PASS\"",
        NULL
    };
    const char *out = run_program(lines);
    check("LEN(STR$(3.14)) == 4", strstr(out, "PASS") != NULL);
}

/* ======================================================================== */
/* Bug 7 — eval.c-static eval_prog()  LOAD as an immediate command crashes  */
/*         the interpreter. bastos_load() (bio.c) resets bstate.read_ptr to */
/*         NULL after a successful load, since the temporary immediate-mode */
/*         line's own storage (at the old prog_end) is no longer valid once */
/*         the whole program has been replaced. The ':'-statement loop in   */
/*         eval_prog() dereferenced read_ptr unconditionally right after    */
/*         LOAD returned (via eval_token(':')), segfaulting on NULL.        */
/* ======================================================================== */
static void test_bug7_load_immediate_null_read_ptr(void) {
    printf("Bug 7: LOAD as an immediate command crashes on NULL read_ptr\n");

    const char *path = "regress_bug7.bas";
    FILE *fp = fopen(path, "w");
    if (fp) {
        fputs("10 PRINT \"LOADED\"\n", fp);
        fclose(fp);
    }

    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /*
     * Before fix: this call segfaults the whole process (a crash is not a
     * regular test failure, but if this line is ever removed and the bug
     * comes back, the test binary will die here instead of completing).
     * After fix: LOAD succeeds silently and execution continues normally.
     */
    capture_clear();
    const char *cmd = "LOAD \"regress_bug7\"\r";
    bastos_send_keys(cmd, strlen(cmd), false);
    for (int i = 0; i < 64; i++)
        bastos_loop();
    check("LOAD as an immediate command does not crash the interpreter", true);

    capture_clear();
    bastos_send_keys("RUN\r", 4, false);
    for (int i = 0; i < 500000 && strstr(g_output, "Ready") == NULL; i++)
        bastos_loop();
    check("the loaded program actually runs", strstr(g_output, "LOADED") != NULL);

    bastos_done();
    remove(path);
}

/* ======================================================================== */
/* Feature — line editing (bio.c, bastos_send_keys() and helpers)            */
/*           Left/right arrow keys move an edit cursor over the current      */
/*           line; typed characters insert at the cursor instead of always   */
/*           being appended; Correction (Backspace) deletes before the       */
/*           cursor instead of always the last character of the buffer.      */
/* ======================================================================== */
static void type_raw(const char *s) {
    bastos_send_keys(s, strlen(s), false);
    bastos_loop();
    bastos_loop();
}

static void test_line_edit_insert_at_cursor(void) {
    printf("Line edit: typed characters insert at the cursor, not always at the end\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 13");
    type_raw("\x08");   /* cursor between '1' and '3' */
    type_raw("2");
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("character inserted mid-line, not appended at the end",
          strstr(g_output, "10 PRINT 123") != NULL);

    bastos_done();
}

static void test_line_edit_consecutive_inserts(void) {
    printf("Line edit: several inserts in a row keep landing at the cursor\n");
    /*
     * Regression for a cursor desync: after inserting mid-line, the terminal
     * cursor must be walked back to right after the just-typed character,
     * not back to where the insert started. Getting that wrong doesn't
     * break the buffer content (this test would still pass on that front
     * alone) — it only shows up visually, as a second insert overwriting
     * instead of shifting the tail right. Assert on the exact echoed bytes
     * of each insert, not just the final LIST text, so a regression here
     * is actually caught.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    const char *line = "10 PRINT 9";
    for (const char *p = line; *p; p++) {
        bastos_send_keys(p, 1, true);
        bastos_loop();
    }
    bastos_send_keys("\x08", 1, true); /* cursor right before the '9' */
    bastos_loop();

    capture_clear();
    bastos_send_keys("1", 1, true);
    bastos_loop();
    check("first insert prints the new char + tail, cursor back after it",
          g_output_len == 3 && memcmp(g_output, "19\x08", 3) == 0);

    capture_clear();
    bastos_send_keys("2", 1, true);
    bastos_loop();
    check("second insert lands right after the first, not overwriting it",
          g_output_len == 3 && memcmp(g_output, "29\x08", 3) == 0);

    capture_clear();
    bastos_send_keys("3", 1, true);
    bastos_loop();
    check("third insert also lands correctly",
          g_output_len == 3 && memcmp(g_output, "39\x08", 3) == 0);

    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("all three inserts landed in typed order before the '9'",
          strstr(g_output, "10 PRINT 1239") != NULL);

    bastos_done();
}

static void test_line_edit_correction_at_cursor(void) {
    printf("Line edit: Correction deletes before the cursor, not always the last char\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 123");
    type_raw("\x08\x08"); /* cursor between '1' and '2' */
    type_raw("\x7f");     /* delete the '1' before the cursor */
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("Correction removed the char before the cursor",
          strstr(g_output, "10 PRINT 23") != NULL);
    check("Correction did not just chop the last char off the end",
          strstr(g_output, "10 PRINT 12") == NULL);

    bastos_done();
}

static void test_line_edit_right_arrow_returns_to_append(void) {
    printf("Line edit: right-arrow moves back toward the end\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Two lefts then one right must net to "one left" (cursor between '2'
     * and '4'), proving right-arrow actually moves forward rather than
     * being a no-op that happens to land in the right place by luck. All
     * digits, so the line round-trips through LIST unchanged (unlike a
     * letter, which would parse as a separate, reformatted variable name). */
    type_raw("10 PRINT 124");
    type_raw("\x08\x08");
    type_raw("\x09");
    type_raw("3");
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("right-arrow moved the cursor forward by exactly one",
          strstr(g_output, "10 PRINT 1234") != NULL);

    bastos_done();
}

static void test_line_edit_ctrl_a_clears_both_sides(void) {
    printf("Line edit: Ctrl+A clears the whole line regardless of cursor position\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\x08");        /* cursor before the '1' */
    type_raw("\x01");        /* Ctrl+A: Annulation */
    type_raw("20 PRINT 2\r");

    capture_clear();
    type_raw("LIST\r");
    check("only the second line survives", strstr(g_output, "20 PRINT 2") != NULL);
    check("no remnant of the cancelled first line",
          strstr(g_output, "10 PRINT") == NULL);

    bastos_done();
}

static void test_line_edit_multibyte_atomic(void) {
    printf("Line edit: left-arrow/Correction move over a multi-byte accent as one unit\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Build "e-acute" (SS2 'B' 'e') at the end of the line, one keystroke at
     * a time, matching how a real keyboard delivers it. */
    char ss2[2] = {(char)0x19, 0};
    bastos_send_keys(ss2, 1, false);
    bastos_loop();
    bastos_send_keys("B", 1, false);
    bastos_loop();
    bastos_send_keys("e", 1, false);
    bastos_loop();

    /* One left-arrow press, echoed, must send exactly one backspace even
     * though the accent is 3 raw bytes: it is one screen column. Move back
     * right afterwards so the cursor sits after the accent again. */
    capture_clear();
    bastos_send_keys("\x08", 1, true);
    bastos_loop();
    check("left-arrow over a 3-byte accent sends exactly one backspace",
          g_output_len == 1 && g_output[0] == '\x08');
    bastos_send_keys("\x09", 1, false);
    bastos_loop();

    /* Correction now removes the whole accent in one press; the line must
     * not retain any orphaned SS2/diacritic bytes. */
    bastos_send_keys("\x7f", 1, false);
    bastos_loop();
    type_raw("10 PRINT \"X\"\r");

    capture_clear();
    type_raw("LIST\r");
    check("no orphaned SS2/diacritic bytes remain",
          strstr(g_output, "10 PRINT \"X\"") != NULL);

    bastos_done();
}

static void test_line_edit_ss2_mid_line_before_diacritic_byte(void) {
    printf("Line edit: inserting an accent mid-line, right before a diacritic-class byte\n");
    /*
     * Regression: inserting SS2 mid-line, immediately before existing tail
     * content whose first byte happens to be a diacritic code (A/B/C/H/K —
     * 'C' here), used to print the dangling, still-incomplete SS2 directly
     * next to that unrelated byte in the same redraw. The Minitel would
     * then try (and fail) to combine them, advancing the cursor by fewer
     * columns than the naive per-byte backspace count assumed, drifting
     * the terminal cursor out of sync with our model.
     *
     * The fix defers the echo of an in-progress SS2 sequence until it
     * resolves, then reprints the whole completed group + tail as one
     * atomic redraw — so nothing is ever shown that a real Minitel could
     * misinterpret. This test asserts on exact echoed bytes at each step,
     * not just the final LIST text, so a regression here is caught even
     * though the buffer content alone was already correct before the fix.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    const char *line = "10 PRINT \"AC\"";
    for (const char *p = line; *p; p++) {
        bastos_send_keys(p, 1, true);
        bastos_loop();
    }
    bastos_send_keys("\x08\x08", 2, true); /* cursor between A and C */
    bastos_loop();
    bastos_loop();

    char ss2[2] = {(char)0x19, 0};
    capture_clear();
    bastos_send_keys(ss2, 1, true);
    bastos_loop();
    check("SS2 alone produces no output (deferred, not yet resolved)",
          g_output_len == 0);

    capture_clear();
    bastos_send_keys("B", 1, true);
    bastos_loop();
    check("SS2+diacritic still produces no output (always needs one more byte)",
          g_output_len == 0);

    capture_clear();
    bastos_send_keys("e", 1, true);
    bastos_loop();
    check("completing the accent prints the whole group + tail atomically",
          g_output_len == 7 && memcmp(g_output, "\x19"
                                                  "BeC\"\x08\x08",
                                       7) == 0);

    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("the accent landed correctly between A and C",
          strstr(g_output, "A\x19"
                            "BeC\"") != NULL);

    bastos_done();
}

static void test_line_edit_ss2_abandoned_by_other_keys(void) {
    printf("Line edit: an in-progress accent is abandoned by any other key\n");
    char ss2[2] = {(char)0x19, 0};

    /* Left-arrow while SS2 is pending: silently discarded, no echo. */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();
    const char *line = "10 PRINT \"AC\"";
    for (const char *p = line; *p; p++) {
        bastos_send_keys(p, 1, true);
        bastos_loop();
    }
    bastos_send_keys("\x08\x08", 2, true);
    bastos_loop();
    bastos_loop();
    bastos_send_keys(ss2, 1, true);
    bastos_loop();

    capture_clear();
    bastos_send_keys("\x08", 1, true); /* left-arrow abandons the pending SS2 */
    bastos_loop();
    check("left-arrow while SS2 is pending produces no output",
          g_output_len == 0);

    bastos_send_keys("X", 1, true);
    bastos_loop();
    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("abandoned SS2 is gone, X landed where it was",
          strstr(g_output, "10 PRINT \"AXC\"") != NULL);
    bastos_done();

    /* Correction while SS2 is pending: also silently discarded. */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();
    for (const char *p = line; *p; p++) {
        bastos_send_keys(p, 1, true);
        bastos_loop();
    }
    bastos_send_keys("\x08\x08", 2, true);
    bastos_loop();
    bastos_loop();
    bastos_send_keys(ss2, 1, true);
    bastos_loop();

    capture_clear();
    bastos_send_keys("\x7f", 1, true); /* Correction abandons the pending SS2 */
    bastos_loop();
    check("Correction while SS2 is pending produces no output",
          g_output_len == 0);

    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("abandoned SS2 leaves the original content untouched",
          strstr(g_output, "10 PRINT \"AC\"") != NULL);
    bastos_done();

    /* Enter (validation) while SS2+diacritic is pending: discarded before
     * the line is submitted, no dangling SS2 ends up stored. */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();
    for (const char *p = line; *p; p++) {
        bastos_send_keys(p, 1, true);
        bastos_loop();
    }
    bastos_send_keys("\x08\x08", 2, true);
    bastos_loop();
    bastos_loop();
    bastos_send_keys(ss2, 1, true);
    bastos_loop();
    bastos_send_keys("B", 1, true);
    bastos_loop();

    capture_clear();
    bastos_send_keys("\r", 1, true);
    bastos_loop();
    check("Enter while SS2+diacritic is pending only echoes the normal "
          "validation sequence",
          g_output_len == 3 && memcmp(g_output, "\x0f\x0d\x0a", 3) == 0);

    capture_clear();
    type_raw("LIST\r");
    check("no dangling SS2 was stored", strstr(g_output, "10 PRINT \"AC\"") != NULL);
    bastos_done();
}

static void test_line_edit_up_down_absorbed_when_typing(void) {
    printf("Line edit: Up/Down are absorbed once something is typed, untouched when empty\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Buffer empty: Up/Down are not BASTOS input keys, so they are echoed
     * raw to the terminal exactly as before this change. */
    capture_clear();
    bastos_send_keys("\x0b", 1, true); /* Up */
    bastos_loop();
    check("Up is passed through to the terminal when the buffer is empty",
          g_output_len == 1 && g_output[0] == '\x0b');

    capture_clear();
    bastos_send_keys("\x0a", 1, true); /* Down */
    bastos_loop();
    check("Down is passed through to the terminal when the buffer is empty",
          g_output_len == 1 && g_output[0] == '\x0a');

    /* Once something has been typed, both keys must be silently absorbed:
     * no echo, no effect on the buffer content. */
    type_raw("10 PRINT 1");

    capture_clear();
    bastos_send_keys("\x0b", 1, true);
    bastos_loop();
    check("Up produces no output once the buffer has content", g_output_len == 0);

    capture_clear();
    bastos_send_keys("\x0a", 1, true);
    bastos_loop();
    check("Down produces no output once the buffer has content", g_output_len == 0);

    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("the line is unaffected by the absorbed keys",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

static void test_line_edit_works_when_buffer_full(void) {
    printf("Line edit: Correction and arrow keys still work when the buffer is full\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* "10 '" + lots of 'A's: a numbered comment is valid BASIC regardless of
     * how long/what its text is, so this is guaranteed to tokenize and get
     * stored once entered, however much of the filler actually made it in. */
    char filler[200];
    memset(filler, 'A', sizeof(filler) - 1);
    filler[sizeof(filler) - 1] = 0;
    type_raw("10 '");
    type_raw(filler); /* overflows the io_buffer; extra bytes are silently dropped */

    type_raw("\x08"); /* movement must still work when full */
    type_raw("\x7f"); /* deletion must still work when full */
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("interpreter survives and responds after a full-buffer edit",
          strstr(g_output, "'A") != NULL);

    bastos_done();
}

static void test_line_edit_input_statement(void) {
    printf("Line edit: cursor editing also works while an INPUT statement is reading\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    const char *prog[] = {"10 INPUT A$", "20 PRINT A$", NULL};
    for (const char **l = prog; *l; l++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s\r", *l);
        bastos_send_keys(buf, strlen(buf), false);
        bastos_loop();
        bastos_loop();
    }

    capture_clear();
    bastos_send_keys("RUN\r", 4, false);
    for (int i = 0; i < 2000; i++)
        bastos_loop(); /* let RUN reach the INPUT prompt */

    bastos_send_keys("WRLD", 4, false);
    bastos_loop();
    bastos_send_keys("\x08\x08\x08", 3, false); /* cursor before 'R' */
    bastos_loop();
    bastos_send_keys("O", 1, false);
    bastos_loop();
    bastos_send_keys("\r", 1, false);

    for (int i = 0; i < 500000 && strstr(g_output, "Ready") == NULL; i++)
        bastos_loop();

    check("edited INPUT value used by the program", strstr(g_output, "WORLD") != NULL);

    bastos_done();
}

static void test_line_edit_finalize_shift(void) {
    printf("Line edit: cursor stays correct after a queued line shifts the buffer\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* One complete line + a second, partially-typed line, in a single batch. */
    const char *batch = "10 PRINT 1\r20 PRINT 2";
    bastos_send_keys(batch, strlen(batch), false);
    bastos_loop(); /* drains line 10, shifting line 20's partial content down */
    bastos_loop();

    bastos_send_keys("\x08", 1, false); /* cursor before the trailing '2' */
    bastos_loop();
    bastos_send_keys("9", 1, false); /* insert into the now-shifted line */
    bastos_loop();
    bastos_send_keys("\r", 1, false);
    bastos_loop();
    bastos_loop();

    capture_clear();
    type_raw("LIST\r");
    check("first (already-committed) line intact",
          strstr(g_output, "10 PRINT 1") != NULL);
    check("second line's mid-batch edit landed at the right (shifted) position",
          strstr(g_output, "20 PRINT 92") != NULL);

    bastos_done();
}

static void test_line_edit_correction_erases_with_spaces_in_mode80(void) {
    printf("Line edit: Correction erases the stale character with spaces in 80-column mode (MODE 2)\n");
    /*
     * Regression: an "erase to end of line" escape (Videotex CLEOL 0x18, or
     * its ANSI ESC [ K equivalent) is not reliably supported by real
     * hardware in 80-column mode — it left stray characters on screen and
     * lost cursor sync. Correction must instead use the same plain
     * backspace/space/backspace technique already relied on elsewhere
     * (os_get_string()), which only needs a bare backspace and a printable
     * space and so behaves identically in both screen modes.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");

    type_raw("AB");
    capture_clear();
    bastos_send_keys("\x7f", 1, true); /* Correction */
    bastos_loop();
    check("Correction uses plain backspace/space/backspace, not an erase escape",
          g_output_len == 3 && memcmp(g_output, "\x08 \x08", 3) == 0);

    bastos_done();
}

static void test_line_edit_insert_redraws_without_erase_escape_in_mode80(void) {
    printf("Line edit: mid-line insert redraws without any erase escape in 80-column mode (MODE 2)\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");

    type_raw("AC");
    bastos_send_keys("\x08", 1, false); /* cursor between A and C */
    bastos_loop();

    capture_clear();
    bastos_send_keys("B", 1, true); /* insert B: AC -> ABC, mid-line redraw */
    bastos_loop();
    check("mid-line insert just reprints the tail and backs up, no erase escape",
          g_output_len == 3 && memcmp(g_output, "BC\x08", 3) == 0);

    bastos_done();
}

static void test_line_edit_annulation_erases_with_spaces_in_mode80(void) {
    printf("Line edit: Annulation (Ctrl+A) erases the whole line with spaces in 80-column mode (MODE 2)\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");

    type_raw("AB");
    capture_clear();
    bastos_send_keys("\x01", 1, true); /* Annulation */
    bastos_loop();
    check("Annulation uses plain backspace/space/backspace, not an erase escape",
          g_output_len == 6 && memcmp(g_output, "\x08\x08  \x08\x08", 6) == 0);

    bastos_done();
}

static void test_ctrl_enter_clears_screen_in_mode80(void) {
    printf("Line edit: Ctrl+Enter (byte 12) clears the screen in 80-column mode (MODE 2)\n");
    /*
     * On a real Minitel keyboard, Ctrl+Enter sends the same byte 12 in
     * both 40- and 80-column mode. In 40-column/Videotex mode, 0x0C is
     * itself the native CLS code, so echoing it raw already clears the
     * screen with no BASTOS-side help. In 80-column/ANSI mode the
     * terminal doesn't understand a raw 0x0C as "clear screen", so
     * BASTOS must translate it to the ANSI clear+home sequence instead.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");

    capture_clear();
    bastos_send_keys("\x0c", 1, true); /* Ctrl+Enter */
    bastos_loop();
    check("byte 12 is translated to the ANSI clear+home sequence in mode 80",
          g_output_len == 7 && memcmp(g_output, "\x1b[2J\x1b[H", 7) == 0);

    bastos_done();
}

static void test_ctrl_enter_passes_through_raw_in_mode40(void) {
    printf("Line edit: Ctrl+Enter (byte 12) is echoed raw in 40-column mode (unchanged)\n");
    /* Default mode (40 columns): the real Minitel already interprets a
     * raw 0x0C as CLS natively, so BASTOS must not translate it here. */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    bastos_send_keys("\x0c", 1, true); /* Ctrl+Enter */
    bastos_loop();
    check("byte 12 is echoed raw, unmodified, in mode 40",
          g_output_len == 1 && g_output[0] == '\x0c');

    bastos_done();
}

static void test_mode80_survives_new(void) {
    printf("MODE: MODE 2 (80 columns) is not reset by NEW\n");
    /*
     * Regression: running_state_clear() (shared by RUN/CLEAR/NEW) used to
     * unconditionally reset bstate.screen_mode to 0 (40 columns), even
     * though it is a screen/terminal setting, not program execution state.
     * NEW must leave the current screen mode untouched.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");
    type_raw("NEW");
    type_raw("\r");

    capture_clear();
    type_raw("CLS");
    type_raw("\r");
    check("CLS still emits the 80-column (ANSI) clear-screen sequence after NEW",
          strstr(g_output, "\x1b[2J\x1b[H") != NULL);
    check("CLS does not fall back to the Videotex clear-screen code",
          strchr(g_output, '\x0c') == NULL);

    bastos_done();
}

static void test_mode80_survives_run_with_no_program(void) {
    printf("MODE: MODE 2 (80 columns) is not reset by RUN, even with no program loaded\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("MODE 2");
    type_raw("\r");
    type_raw("RUN"); /* no program in memory: this is a no-op run */
    type_raw("\r");

    capture_clear();
    type_raw("CLS");
    type_raw("\r");
    check("CLS still emits the 80-column (ANSI) clear-screen sequence after RUN",
          strstr(g_output, "\x1b[2J\x1b[H") != NULL);
    check("CLS does not fall back to the Videotex clear-screen code",
          strchr(g_output, '\x0c') == NULL);

    bastos_done();
}

/* ======================================================================== */
/* Feature — EDIT <line_no> (eval.c-static eval_edit(), bio.c              */
/*           bastos_input()'s finalize step)                                */
/*           Loads the given line's own text directly into io_buffer,       */
/*           shows it on screen, and places the cursor at its end — ready   */
/*           to be edited instead of retyped from scratch.                  */
/* ======================================================================== */
static void test_edit_shows_the_staged_line(void) {
    printf("EDIT: the staged line is echoed to the screen when it loads\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");

    capture_clear();
    type_raw("EDIT 10");
    type_raw("\r");
    check("the line's own text is shown right when EDIT resolves",
          strstr(g_output, "10 PRINT \"HELLO\"") != NULL);

    bastos_done();
}

static void test_edit_prefills_at_end(void) {
    printf("EDIT: stages the line's text with the cursor at the end\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");

    type_raw("EDIT 10");
    type_raw("\r");

    /* A character typed right after EDIT must land at the true end of the
     * staged text (proving the cursor was placed there, not left at 0). */
    bastos_send_keys("9", 1, false);
    bastos_loop();
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("typed char landed after the staged line's own text",
          strstr(g_output, "10 PRINT \"HELLO\"9") != NULL);

    bastos_done();
}

static void test_edit_unmodified_round_trips(void) {
    printf("EDIT: pressing Enter with no changes leaves the line untouched\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");
    type_raw("EDIT 10");
    type_raw("\r");
    type_raw("\r"); /* submit the staged line as-is, no edits made */

    capture_clear();
    type_raw("LIST\r");
    check("line is unchanged after an EDIT immediately followed by Enter",
          strstr(g_output, "10 PRINT \"HELLO\"") != NULL);

    bastos_done();
}

static void test_edit_allows_modification(void) {
    printf("EDIT: the staged text can be edited with Correction/insert before submitting\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");
    type_raw("EDIT 10");
    type_raw("\r");

    /* Replace the trailing O" with X" using Correction then insert. */
    bastos_send_keys("\x7f", 1, false); /* remove closing quote */
    bastos_loop();
    bastos_send_keys("\x7f", 1, false); /* remove O */
    bastos_loop();
    bastos_send_keys("X\"", 2, false);
    bastos_loop();
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("edited line stored the modification",
          strstr(g_output, "10 PRINT \"HELLX\"") != NULL);
    check("old text is gone", strstr(g_output, "HELLO") == NULL);

    bastos_done();
}

static void test_edit_escape_after_syntax_error_keeps_original_line(void) {
    printf("EDIT: cancelling with ESC after a syntax error does not lose the original line\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");
    type_raw("EDIT 10");
    type_raw("\r");

    /* Break the syntax: remove the closing quote and add a dangling '+'. */
    bastos_send_keys("\x7f", 1, false);
    bastos_loop();
    bastos_send_keys("+", 1, false);
    bastos_loop();

    capture_clear();
    type_raw("\r");
    check("the broken submission beeps and reports an error",
          g_output_len > 0 && g_output[0] == '\x07' &&
          strstr(g_output, "Error") != NULL);

    /* Give up on the edit instead of fixing it. */
    bastos_send_keys("\x1b", 1, false);
    bastos_loop();

    capture_clear();
    type_raw("LIST\r");
    check("the original line 10 is still there",
          strstr(g_output, "10 PRINT \"HELLO\"") != NULL);

    bastos_done();
}

static void test_edit_left_arrow_echoes_one_backspace(void) {
    printf("EDIT: normal line-editing keys work on the staged text\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HELLO\"");
    type_raw("\r");
    type_raw("EDIT 10");
    type_raw("\r");

    capture_clear();
    bastos_send_keys("\x08", 1, true); /* left-arrow over the closing quote */
    bastos_loop();
    check("left-arrow on the staged line echoes exactly one backspace",
          g_output_len == 1 && g_output[0] == '\x08');

    bastos_done();
}

static void test_edit_past_last_line_is_a_noop(void) {
    printf("EDIT: a line number past every existing line is a silent no-op (like GOTO)\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");

    capture_clear();
    type_raw("EDIT 99");
    type_raw("\r");
    check("no error and nothing staged for editing",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

static void test_edit_missing_line_number_picks_next_line(void) {
    printf("EDIT: a missing line number stages the next existing line, like GOTO\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");
    type_raw("30 PRINT 3");
    type_raw("\r");

    capture_clear();
    type_raw("EDIT 20");
    type_raw("\r");
    check("line 30 (the next one after 20) was staged and shown",
          strstr(g_output, "30 PRINT 3") != NULL);

    bastos_done();
}

static void test_edit_no_argument_targets_first_line(void) {
    printf("EDIT: no argument targets the first line of the program (like EDIT 0)\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("20 PRINT 2");
    type_raw("\r");
    type_raw("10 PRINT 1");
    type_raw("\r");

    capture_clear();
    type_raw("EDIT");
    type_raw("\r");
    check("bare EDIT staged the first (lowest-numbered) line",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

static void test_edit_zero_targets_first_line(void) {
    printf("EDIT: EDIT 0 targets the first line of the program, same as bare EDIT\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("20 PRINT 2");
    type_raw("\r");
    type_raw("10 PRINT 1");
    type_raw("\r");

    capture_clear();
    type_raw("EDIT 0");
    type_raw("\r");
    check("EDIT 0 staged the first (lowest-numbered) line",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

static void test_edit_no_argument_with_empty_program_is_a_noop(void) {
    printf("EDIT: no argument with no program loaded does nothing\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    type_raw("EDIT");
    type_raw("\r");
    check("no error and nothing staged for editing",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

static void test_suite_auto_stages_next_line(void) {
    printf("SUITE (VKEY 4): after validating a line, the next program line is auto-staged for editing\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");
    type_raw("20 PRINT 2");
    type_raw("\r");

    /* Re-submit line 10 unchanged, but validate with SUITE instead of Enter. */
    capture_clear();
    type_raw("10 PRINT 1");
    bastos_send_keys("\x04", 1, true); /* SUITE */
    bastos_loop();
    check("line 20 is automatically staged and shown after SUITE",
          strstr(g_output, "20 PRINT 2") != NULL);

    bastos_done();
}

static void test_suite_with_no_next_line_behaves_like_enter(void) {
    printf("SUITE (VKEY 4): validating the last line behaves like Enter when there's no next line\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");

    capture_clear();
    type_raw("10 PRINT 1");
    bastos_send_keys("\x04", 1, true); /* SUITE, no line after 10 */
    bastos_loop();
    check("no error and nothing extra staged",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    /* Confirm a fresh line can still be typed normally afterward. */
    type_raw("30 PRINT 3");
    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("both lines present, no corruption from the SUITE validation",
          strstr(g_output, "10 PRINT 1") != NULL &&
          strstr(g_output, "30 PRINT 3") != NULL);

    bastos_done();
}

static void test_retour_auto_stages_previous_line(void) {
    printf("RETOUR (VKEY 5): after validating a line, the previous program line is auto-staged for editing\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");
    type_raw("20 PRINT 2");
    type_raw("\r");

    /* Re-submit line 20 unchanged, but validate with RETOUR instead of Enter. */
    capture_clear();
    type_raw("20 PRINT 2");
    bastos_send_keys("\x05", 1, true); /* RETOUR */
    bastos_loop();
    check("line 10 is automatically staged and shown after RETOUR",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

static void test_retour_with_no_previous_line_behaves_like_enter(void) {
    printf("RETOUR (VKEY 5): validating the first line behaves like Enter when there's no previous line\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");

    capture_clear();
    type_raw("10 PRINT 1");
    bastos_send_keys("\x05", 1, true); /* RETOUR, no line before 10 */
    bastos_loop();
    check("no error and nothing extra staged",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    /* Confirm a fresh line can still be typed normally afterward. */
    type_raw("30 PRINT 3");
    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("both lines present, no corruption from the RETOUR validation",
          strstr(g_output, "10 PRINT 1") != NULL &&
          strstr(g_output, "30 PRINT 3") != NULL);

    bastos_done();
}

/* ======================================================================== */
/* Feature — Up-arrow recall of the last stored numbered line or immediate  */
/*           command (bio.c bastos_load_edit_line() / io_last_line /        */
/*           io_recall_len), complementing EDIT <line_no> for the common    */
/*           case of re-editing whatever was just entered without having    */
/*           to know or retype its line number.                             */
/* ======================================================================== */
static void test_recall_noop_if_line_since_deleted(void) {
    printf("Recall: Up-arrow silently does nothing if the recalled line no longer exists\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT 1");
    type_raw("\r");
    type_raw("10"); /* empty body for an existing line number: deletes it */
    type_raw("\r");

    capture_clear();
    bastos_send_keys("\x0b", 1, true); /* Up: line 10 no longer exists */
    bastos_loop();
    check("nothing is echoed for a since-deleted line", g_output_len == 0);

    bastos_done();
}

static void test_recall_numbered_line_via_up_arrow(void) {
    printf("Recall: Up-arrow reloads the last stored numbered line\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT \"HI\"");
    type_raw("\r");

    capture_clear();
    bastos_send_keys("\x0b", 1, true); /* Up: buffer is empty, nothing typed since */
    bastos_loop();
    check("Up-arrow echoes the untokenized line",
          strstr(g_output, "10 PRINT \"HI\"") != NULL);

    /* Resubmitting the recalled text unmodified must not duplicate/corrupt it. */
    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("the line is still stored exactly once",
          strstr(g_output, "10 PRINT \"HI\"") != NULL);

    bastos_done();
}

static void test_recall_immediate_command_via_up_arrow(void) {
    printf("Recall: Up-arrow reloads the last immediate command's raw text\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("print 1");
    capture_clear();
    type_raw("\r");
    check("the immediate command ran", strstr(g_output, "1") != NULL);

    capture_clear();
    bastos_send_keys("\x0b", 1, true); /* Up: recall it for editing */
    bastos_loop();
    /* tokenize() case-folds identifier-like words in place; clearing bit 7
     * recovers that normalized (uppercase) form, same as LIST would show. */
    check("Up-arrow echoes the last immediate command back",
          strstr(g_output, "PRINT 1") != NULL);

    /* It must still be a valid, resubmittable line. */
    capture_clear();
    type_raw("\r");
    check("resubmitting the recalled command runs it again",
          strstr(g_output, "1") != NULL);

    bastos_done();
}

static void test_recall_discarded_by_new_typing(void) {
    printf("Recall: typing something new discards the pending recall instead of mixing with it\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("print 1");
    type_raw("\r");

    /* Don't press Up: type a fresh line directly. */
    type_raw("10 PRINT 2");
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("only the freshly typed numbered line was stored",
          strstr(g_output, "10 PRINT 2") != NULL);
    check("no leftover from the earlier immediate command leaked in",
          strstr(g_output, "PRINT 1") == NULL);

    bastos_done();
}

static void test_recall_survives_a_blank_enter(void) {
    printf("Recall: pressing Enter with nothing typed doesn't lose the pending recall\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("print 1");
    capture_clear();
    type_raw("\r");
    check("the immediate command ran", strstr(g_output, "1") != NULL);

    /* Press Enter again with nothing typed: must behave as a no-op (CRLF
     * only, same as an empty buffer) and not resubmit or discard the
     * pending recall. */
    capture_clear();
    bastos_send_keys("\r", 1, true);
    bastos_loop();
    check("a blank Enter only echoes CRLF, no duplicate execution and no error",
          strstr(g_output, "Error") == NULL && strstr(g_output, "1") == NULL);

    capture_clear();
    bastos_send_keys("\x0b", 1, true); /* Up: still recalls "PRINT 1" */
    bastos_loop();
    check("Up-arrow still recalls the previous entry after the blank Enter",
          strstr(g_output, "PRINT 1") != NULL);

    bastos_done();
}

static void test_recall_not_offered_for_autoexec_banner(void) {
    printf("Recall: the autoexec fallback banner is not offered for Up-arrow recall\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* No autoexec.bas exists in the test disk, so bmem_init() already
     * queued and ran the internal "bastos" banner fallback during the
     * warmup loops above. A fresh prompt must behave exactly as if nothing
     * had run yet: Up passes through untouched. */
    capture_clear();
    bastos_send_keys("\x0b", 1, true);
    bastos_loop();
    check("Up is passed through, not a recall of the internal banner command",
          g_output_len == 1 && g_output[0] == '\x0b');

    bastos_done();
}

/* ======================================================================== */
/* Feature — stay in edit mode on a syntax error (bio.c bastos_input())      */
/*           Validating a line that isn't valid BASIC beeps (BEL, char 7)    */
/*           and leaves the typed text in the input buffer instead of       */
/*           discarding it, so it can be fixed and resubmitted — reusing    */
/*           the existing validation key handling, not a separate command.  */
/* ======================================================================== */
static void test_syntax_error_beeps_and_stays_editable(void) {
    printf("Syntax error: beeps and keeps the invalid line editable\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* "PRINT +" is invalid: a dangling operator with no right-hand side. */
    capture_clear();
    type_raw("10 PRINT +");
    type_raw("\r");
    check("invalid line beeps (BEL) and reports the error",
          g_output_len > 0 && g_output[0] == '\x07' &&
          strstr(g_output, "Error") != NULL);

    /* "Error N" moved the terminal cursor to a fresh row below the line
     * being edited, so the line must be re-echoed after it or the screen
     * and the internal cursor/buffer state no longer match. */
    {
        char *err_pos = strstr(g_output, "Error");
        check("the invalid line is re-echoed on screen after the error message",
              err_pos != NULL && strstr(err_pos, "10 PRINT +") != NULL);
    }

    /* Fix it in place: remove the '+' and supply a valid operand. */
    bastos_send_keys("\x7f", 1, false);
    bastos_loop();
    bastos_send_keys("1", 1, false);
    bastos_loop();
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("the fixed line was stored correctly",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

static void test_syntax_error_never_stores_invalid_line(void) {
    printf("Syntax error: the invalid text is never stored as a program line\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 PRINT +");
    type_raw("\r");

    /* Cancel the still-pending invalid line instead of fixing it. */
    type_raw("\x01");

    capture_clear();
    type_raw("LIST\r");
    check("nothing was stored", g_output_len == 0);

    bastos_done();
}

static void test_valid_line_does_not_beep(void) {
    printf("Syntax error: a valid line does not beep\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    type_raw("10 PRINT 1");
    type_raw("\r");
    check("no BEL and no error for valid syntax",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

static void test_runtime_error_does_not_stay_in_edit_mode(void) {
    printf("Syntax error: a runtime error (valid syntax) does not stay in edit mode\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Valid syntax, but the target file does not exist: a runtime error,
     * not a syntax error, so the prompt must move on normally afterwards. */
    capture_clear();
    type_raw("LOAD \"does_not_exist\"");
    type_raw("\r");
    check("runtime error does not beep",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    /* A fresh command right after must run on its own, not get appended to
     * anything left over from the failed LOAD. */
    capture_clear();
    type_raw("10 PRINT 1");
    type_raw("\r");
    capture_clear();
    type_raw("LIST\r");
    check("the interpreter is back to normal, ready for a fresh command",
          strstr(g_output, "10 PRINT 1") != NULL);

    bastos_done();
}

/* ======================================================================== */
/* Feature — variable-less NEXT (eval.c-static, eval_next()/eval_for())      */
/*           FOR/NEXT nesting is a stack (bstate.for_sp / bmem->for_stack):  */
/*           NEXT always closes the innermost currently active loop. A      */
/*           bare NEXT just omits the (optional) consistency-check name;    */
/*           naming any loop other than the innermost one is a mismatch     */
/*           error, since it can never be closed out of order.              */
/* ======================================================================== */
static void test_next_bare_nested(void) {
    printf("NEXT: bare NEXT closes the innermost loop first\n");

    const char *lines[] = {
        "10 FOR I=1 TO 2",
        "20 FOR J=1 TO 2",
        "30 PRINT I;J",
        "40 NEXT",
        "50 NEXT",
        "60 PRINT \"DONE\"",
        NULL
    };
    const char *out = run_program(lines);
    check("all four (I,J) pairs printed in nested order",
          strstr(out, "11") && strstr(out, "12") && strstr(out, "21") &&
          strstr(out, "22"));
    check("execution continues after both loops exit",
          strstr(out, "DONE") != NULL);
}

static void test_next_bare_mixed_with_explicit(void) {
    printf("NEXT: an explicit NEXT and a bare NEXT can be mixed\n");

    const char *lines[] = {
        "10 FOR I=1 TO 2",
        "20 FOR J=1 TO 2",
        "30 PRINT I;J",
        "40 NEXT J",
        "50 NEXT",
        "60 PRINT \"DONE\"",
        NULL
    };
    const char *out = run_program(lines);
    check("all four (I,J) pairs printed in nested order",
          strstr(out, "11") && strstr(out, "12") && strstr(out, "21") &&
          strstr(out, "22"));
    check("bare NEXT correctly picks up the outer loop afterwards",
          strstr(out, "DONE") != NULL);
}

static void test_next_bare_no_active_loop(void) {
    printf("NEXT: bare NEXT with no active loop is a runtime error\n");

    const char *lines[] = {
        "10 NEXT",
        NULL
    };
    const char *out = run_program(lines);
    check("running a bare NEXT outside any loop reports an error",
          strstr(out, "Error") != NULL);
}

static void test_next_wrong_variable_is_error(void) {
    printf("NEXT: naming a loop other than the innermost one is an error\n");

    /* J is the innermost active loop here: "NEXT I" must be rejected. */
    const char *lines[] = {
        "10 FOR I=1 TO 2",
        "20 FOR J=1 TO 2",
        "30 NEXT I",
        NULL
    };
    const char *out = run_program(lines);
    check("NEXT naming a non-innermost loop reports an error",
          strstr(out, "Error") != NULL);
}

/* ======================================================================== */
/* Feature — ':' multi-statement lines (eval.c-static, eval_prog() et al.)   */
/*           Several statements can now be chained on one physical line,     */
/*           separated by ':'.                                               */
/* ======================================================================== */
static void test_colon_basic_sequence(void) {
    printf("Colon: basic multi-statement sequencing\n");

    /* PRINT "A": PRINT "B": PRINT "C" must run all three in order. */
    const char *lines[] = {
        "10 PRINT \"A\": PRINT \"B\": PRINT \"C\"",
        NULL
    };
    const char *out = run_program(lines);
    check("A/B/C all printed in order",
          strstr(out, "A") && strstr(out, "B") && strstr(out, "C"));
}

static void test_colon_if_then(void) {
    printf("Colon: IF true THEN executes rest of line, false skips all\n");

    /*
     * Everything after THEN, up to the end of the physical line, is part
     * of the conditional block: it all runs if the test is true, and is
     * all skipped (including further ':'-statements) if it is false.
     */
    const char *lines[] = {
        "10 IF 1 THEN PRINT \"T1\": PRINT \"T2\"",
        "20 IF 0 THEN PRINT \"F1\": PRINT \"F2\"",
        "30 PRINT \"END\"",
        NULL
    };
    const char *out = run_program(lines);
    check("true branch runs both statements", strstr(out, "T1") && strstr(out, "T2"));
    check("false branch skips both statements", !strstr(out, "F1") && !strstr(out, "F2"));
    check("execution continues after the IF line", strstr(out, "END") != NULL);
}

static void test_colon_single_line_for_next(void) {
    printf("Colon: single-line FOR/NEXT loop\n");

    /* FOR and NEXT can now live on the very same physical line. */
    const char *lines[] = {
        "10 FOR I=1 TO 3: PRINT I: NEXT I",
        "20 PRINT \"DONE\"",
        NULL
    };
    const char *out = run_program(lines);
    check("loop body ran for each iteration (1,2,3 present)",
          strstr(out, "1") && strstr(out, "2") && strstr(out, "3"));
    check("execution continues after loop exits", strstr(out, "DONE") != NULL);
}

static void test_colon_gosub_return(void) {
    printf("Colon: GOSUB followed by more statements resumes after RETURN\n");

    /*
     * RETURN must resume right after the GOSUB on its own line, not just
     * at the start of the next physical line.
     */
    const char *lines[] = {
        "10 GOSUB 100: PRINT \"AFTER\"",
        "20 PRINT \"END\"",
        "30 STOP",
        "100 PRINT \"SUB\"",
        "110 RETURN",
        NULL
    };
    const char *out = run_program(lines);
    check("subroutine ran", strstr(out, "SUB") != NULL);
    check("statement after GOSUB on same line ran after RETURN",
          strstr(out, "AFTER") != NULL);
    check("execution continues to next line", strstr(out, "END") != NULL);
}

/* ======================================================================== */
/* Feature — ELSE for IF/THEN (eval.c-static: eval_if(), eval_else(),        */
/*           eval_skip_token()). A false condition scans forward (without    */
/*           evaluating) for a matching ELSE at the same IF-nesting depth;   */
/*           a true condition falls through the then-clause's ':'-chain as   */
/*           before and, on reaching an unconsumed ELSE, skips its clause.   */
/* ======================================================================== */
static void test_else_true_false_branches(void) {
    printf("ELSE: true runs THEN only, false runs ELSE only\n");

    const char *lines[] = {
        "10 IF 1 THEN PRINT \"A\" ELSE PRINT \"B\"",
        "20 IF 0 THEN PRINT \"C\" ELSE PRINT \"D\"",
        NULL
    };
    const char *out = run_program(lines);
    check("true condition prints the THEN branch", strstr(out, "A") != NULL);
    check("true condition does not print the ELSE branch", strstr(out, "B") == NULL);
    check("false condition prints the ELSE branch", strstr(out, "D") != NULL);
    check("false condition does not print the THEN branch", strstr(out, "C") == NULL);
}

static void test_else_absent_no_regression(void) {
    printf("ELSE: a false IF with no ELSE present still skips to end of line\n");

    const char *lines[] = {
        "10 IF 0 THEN PRINT \"A\"",
        "20 PRINT \"END\"",
        NULL
    };
    const char *out = run_program(lines);
    check("nothing printed for the false, else-less THEN", strstr(out, "A") == NULL);
    check("execution continues to the next line", strstr(out, "END") != NULL);
}

static void test_else_multi_statement_clauses(void) {
    printf("ELSE: THEN and ELSE clauses each span multiple ':'-statements\n");

    const char *lines[] = {
        "10 LET a=0: LET b=0",
        "20 IF 1 THEN LET a=1: LET b=2 ELSE LET a=9: LET b=9",
        "30 PRINT a; b",
        NULL
    };
    const char *out = run_program(lines);
    check("both THEN statements ran", strstr(out, "12") != NULL);
    check("the ELSE clause never ran", strstr(out, "9") == NULL);
}

static void test_else_nested_if_outer_false_skips_whole_then(void) {
    printf("ELSE: outer false with no outer ELSE skips a whole nested IF/ELSE unit\n");

    /*
     * The critical depth-tracking case: the outer IF's forward scan must
     * skip over the entire inner "IF 1 THEN LET b=2 ELSE LET b=9" —
     * including the inner ELSE — without mistaking it for the outer's own
     * (absent) ELSE.
     */
    const char *lines[] = {
        "10 LET b=0",
        "20 IF 0 THEN LET a=1: IF 1 THEN LET b=2 ELSE LET b=9",
        "30 PRINT b",
        NULL
    };
    const char *out = run_program(lines);
    check("b is untouched: neither branch of the nested IF ran",
          strstr(out, "0") != NULL && strstr(out, "2") == NULL &&
          strstr(out, "9") == NULL);
}

static void test_else_nested_if_outer_true_inner_false_with_else(void) {
    printf("ELSE: outer true runs a nested IF, which takes its own ELSE\n");

    const char *lines[] = {
        "10 LET a=0: LET b=0",
        "20 IF 1 THEN LET a=1: IF 0 THEN LET b=2 ELSE LET b=9",
        "30 PRINT a; b",
        NULL
    };
    const char *out = run_program(lines);
    check("outer THEN ran and inner ELSE ran", strstr(out, "19") != NULL);
}

static void test_else_nested_if_inside_else_clause(void) {
    printf("ELSE: a nested IF inside an ELSE clause is skipped when the outer is true\n");

    const char *lines[] = {
        "10 LET a=0",
        "20 IF 1 THEN LET a=1 ELSE LET a=2: IF 1 THEN LET a=3 ELSE LET a=9",
        "30 PRINT a",
        NULL
    };
    const char *out = run_program(lines);
    check("only the outer THEN ran; the whole ELSE (with its nested IF) was skipped",
          strstr(out, "1") != NULL && strstr(out, "2") == NULL &&
          strstr(out, "3") == NULL && strstr(out, "9") == NULL);
}

static void test_else_doubly_nested_depth(void) {
    printf("ELSE: the nesting-depth counter tracks two levels, not just one\n");

    /*
     * Outer condition is false with no outer ELSE, and its then-clause
     * contains a nested IF whose own then-clause contains a further
     * nested IF — the scan must walk depth 0->1->2->1->0 in one pass
     * before correctly concluding there is no ELSE for the outer IF.
     */
    const char *lines[] = {
        "10 LET p=0: LET q=0: LET r=0",
        "20 IF 0 THEN LET p=1: IF 1 THEN LET q=1: IF 1 THEN LET r=99 ELSE LET r=1 ELSE LET r=2",
        "30 PRINT p; q; r",
        NULL
    };
    const char *out = run_program(lines);
    check("the entire outer then-clause (both nested IFs) was skipped",
          strstr(out, "000") != NULL);
}

static void test_else_save_load_roundtrip(void) {
    printf("ELSE: a program containing ELSE survives a SAVE/LOAD round trip\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 IF 1 THEN PRINT \"A\" ELSE PRINT \"B\"");
    type_raw("\r");
    type_raw("SAVE \"else_test\"");
    type_raw("\r");
    for (int i = 0; i < 1000; i++)
        bastos_loop();

    type_raw("NEW");
    type_raw("\r");
    type_raw("LOAD \"else_test\"");
    type_raw("\r");
    for (int i = 0; i < 1000; i++)
        bastos_loop();

    capture_clear();
    type_raw("LIST\r");
    check("the loaded program still contains the ELSE clause",
          strstr(g_output, "ELSE") != NULL);

    capture_clear();
    type_raw("RUN\r");
    for (int i = 0; i < 500000 && strstr(g_output, "Ready") == NULL; i++)
        bastos_loop();
    check("the loaded program still runs correctly", strstr(g_output, "A") != NULL);

    hal_erase("else_test.bas");
    bastos_done();
}

static void test_else_bare_linenum_goto(void) {
    printf("ELSE: bare line numbers on THEN and ELSE act as GOTO targets\n");

    const char *lines_true[] = {
        "10 IF 1 THEN 40 ELSE 60",
        "20 PRINT \"X\"",
        "30 END",
        "40 PRINT \"THEN\"",
        "50 GOTO 30",
        "60 PRINT \"ELSE\"",
        NULL
    };
    const char *out_true = run_program(lines_true);
    check("true condition jumps to the THEN line number",
          strstr(out_true, "THEN") != NULL && strstr(out_true, "ELSE") == NULL);

    const char *lines_false[] = {
        "10 IF 0 THEN 40 ELSE 60",
        "20 PRINT \"X\"",
        "30 END",
        "40 PRINT \"THEN\"",
        "50 GOTO 30",
        "60 PRINT \"ELSE\"",
        NULL
    };
    const char *out_false = run_program(lines_false);
    check("false condition jumps to the ELSE line number",
          strstr(out_false, "ELSE") != NULL && strstr(out_false, "THEN") == NULL);
}

static void test_else_gosub_in_then_with_else_present(void) {
    printf("ELSE: GOSUB in a THEN-clause with an ELSE present resumes past it, not into it\n");

    /*
     * Regression for the trickiest interaction: GOSUB's recorded resume
     * offset lands exactly on the unconsumed ELSE token. On RETURN,
     * eval_prog() starts a fresh call with else_pending reset to 0, so
     * eval_else() must correctly treat this as "skip the else-clause"
     * (the original condition was true), not re-run it or error.
     */
    const char *lines[] = {
        "10 IF 1 THEN GOSUB 1000 ELSE PRINT \"NO\"",
        "20 PRINT \"AFTER\"",
        "30 END",
        "1000 PRINT \"SUB\"",
        "1010 RETURN",
        NULL
    };
    const char *out = run_program(lines);
    check("the subroutine ran once", strstr(out, "SUB") != NULL);
    check("the else-clause never ran", strstr(out, "NO") == NULL);
    check("execution continues normally after the RETURN",
          strstr(out, "AFTER") != NULL);
}

static void test_else_gosub_in_else_clause(void) {
    printf("ELSE: GOSUB inside an ELSE clause runs and RETURN resumes correctly\n");

    const char *lines[] = {
        "10 IF 0 THEN PRINT \"NO\" ELSE GOSUB 1000",
        "20 PRINT \"AFTER\"",
        "30 END",
        "1000 PRINT \"SUB\"",
        "1010 RETURN",
        NULL
    };
    const char *out = run_program(lines);
    check("the then-clause never ran", strstr(out, "NO") == NULL);
    check("the subroutine in the else-clause ran", strstr(out, "SUB") != NULL);
    check("execution continues normally after the RETURN",
          strstr(out, "AFTER") != NULL);
}

static void test_else_syntax_check_does_not_beep(void) {
    printf("ELSE: typing a well-formed IF/THEN/ELSE line does not beep or error\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    type_raw("10 IF 1 THEN PRINT \"A\" ELSE PRINT \"B\"");
    type_raw("\r");
    check("no BEL and no error for a well-formed ELSE line",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

static void test_else_list_roundtrip(void) {
    printf("ELSE: LIST shows correct spacing around ELSE and re-tokenizes identically\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("10 IF 1 THEN PRINT \"A\" ELSE PRINT \"B\"");
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("ELSE has a leading space, not glued to the previous token",
          strstr(g_output, "\" ELSE") != NULL);
    check("ELSE has a trailing space before PRINT",
          strstr(g_output, "ELSE PRINT") != NULL);

    bastos_done();
}

static void test_else_skips_string_with_embedded_nul(void) {
    printf("ELSE: the false-branch scanner skips a string with an embedded NUL correctly\n");

    /*
     * tokenize_string() allows an escaped \x00, so a skipped string's
     * payload can contain a raw NUL byte before its real end. The skip
     * primitive must use the string's length prefix, not scan for a NUL,
     * or it would stop early and never find the ELSE below.
     */
    const char *lines[] = {
        "10 IF 0 THEN PRINT \"a\\x00b\" ELSE PRINT \"OK\"",
        NULL
    };
    const char *out = run_program(lines);
    check("the ELSE branch was found and ran despite the embedded NUL",
          strstr(out, "OK") != NULL);
}

static void test_else_skips_string_with_apostrophe(void) {
    printf("ELSE: the false-branch scanner skips a string containing ' correctly\n");

    const char *lines[] = {
        "10 IF 0 THEN PRINT \"it's\" ELSE PRINT \"OK\"",
        NULL
    };
    const char *out = run_program(lines);
    check("the apostrophe inside the string was not mistaken for a comment",
          strstr(out, "OK") != NULL);
}

static void test_else_orphan_is_a_noop(void) {
    printf("ELSE: an ELSE with no preceding IF on the line is a silent no-op\n");

    const char *lines[] = {
        "10 PRINT \"BEFORE\"",
        "20 ELSE PRINT \"ORPHAN\"",
        "30 PRINT \"AFTER\"",
        NULL
    };
    const char *out = run_program(lines);
    check("the orphan ELSE's clause never ran", strstr(out, "ORPHAN") == NULL);
    check("execution continues normally past it",
          strstr(out, "BEFORE") != NULL && strstr(out, "AFTER") != NULL);
}

static void test_else_inherits_equals_ambiguity(void) {
    printf("ELSE: a bare 'a=1' after THEN/ELSE is a GOTO attempt, not an assignment (pre-existing)\n");

    /*
     * eval_if() tries eval_expr(TOKEN_NUMBER) before eval_instruction()
     * after THEN, so a bare "a=1" parses as the comparison (a==1) used as
     * a computed GOTO target, not an assignment — every real .bas file
     * already uses "THEN LET var=..." for this reason. eval_else() mirrors
     * THEN's grammar exactly, so it inherits the same trap. This is a
     * pre-existing, out-of-scope quirk being pinned down, not fixed.
     *
     * Not run to completion: any resulting GOTO 0/1 resolves to this
     * program's first line, looping forever. Only the syntax-check is
     * exercised, confirming ELSE's grammar doesn't change this behavior.
     */
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    type_raw("10 IF 1 THEN a=1 ELSE a=2");
    type_raw("\r");
    check("the ambiguous line is still accepted as valid syntax",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

/* ======================================================================== */
/* Feature — LABEL "name" (eval.c-static: eval_label_set(),                  */
/*           bmemory.c-static: bmem_var_label_set()). Executing LABEL       */
/*           "name" caches the current line's own number under a TOKEN_LABEL */
/*           variable (a namespace separate from ordinary variables), for    */
/*           GOTO/GOSUB "name" to look up. This first commit only covers the */
/*           statement itself; GOTO/GOSUB "name" lands in a later commit.    */
/* ======================================================================== */
static void test_label_syntax_check_does_not_beep(void) {
    printf("LABEL: a well-formed LABEL \"name\" line does not beep or error\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    capture_clear();
    type_raw("1000 LABEL \"decadix\"");
    type_raw("\r");
    check("no BEL and no error for a well-formed LABEL line",
          g_output_len == 0 || strchr(g_output, '\x07') == NULL);

    bastos_done();
}

static void test_label_runs_without_error(void) {
    printf("LABEL: executing LABEL \"name\" runs without error\n");

    const char *lines[] = {
        "1000 LABEL \"decadix\"",
        "1010 PRINT \"AFTER\"",
        NULL
    };
    const char *out = run_program(lines);
    check("execution reaches the line after LABEL",
          strstr(out, "AFTER") != NULL);
    check("no error was reported", strstr(out, "Error") == NULL);
}

static void test_label_list_roundtrip(void) {
    printf("LABEL: LIST shows the label text back correctly\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("1000 LABEL \"decadix\"");
    type_raw("\r");

    capture_clear();
    type_raw("LIST\r");
    check("LIST reproduces the LABEL statement verbatim",
          strstr(g_output, "1000 LABEL \"decadix\"") != NULL);

    bastos_done();
}

static void test_label_save_load_roundtrip(void) {
    printf("LABEL: a program containing LABEL survives a SAVE/LOAD round trip\n");
    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    type_raw("1000 LABEL \"decadix\"");
    type_raw("\r");
    type_raw("SAVE \"label_test\"");
    type_raw("\r");
    for (int i = 0; i < 1000; i++)
        bastos_loop();

    type_raw("NEW");
    type_raw("\r");
    type_raw("LOAD \"label_test\"");
    type_raw("\r");
    for (int i = 0; i < 1000; i++)
        bastos_loop();

    capture_clear();
    type_raw("LIST\r");
    check("the loaded program still contains the LABEL statement",
          strstr(g_output, "LABEL \"decadix\"") != NULL);

    capture_clear();
    type_raw("RUN\r");
    for (int i = 0; i < 500000 && strstr(g_output, "Ready") == NULL; i++)
        bastos_loop();
    check("the loaded program still runs without error",
          strstr(g_output, "Error") == NULL);

    hal_erase("label_test.bas");
    bastos_done();
}

/* ======================================================================== */
/* Feature — ''' end-of-line comment (token.c-static, tokenize())            */
/*           Everything from a ''' up to the end of the physical line is     */
/*           kept verbatim in the stored program (so LIST shows it back),    */
/*           but has no effect: it is never interpreted.                     */
/* ======================================================================== */
static void test_comment_trailing(void) {
    printf("Comment: trailing ''' after a statement is ignored\n");

    const char *lines[] = {
        "10 PRINT \"A\" ' this text must never run",
        NULL
    };
    const char *out = run_program(lines);
    check("statement before the comment still runs", strstr(out, "A") != NULL);
    check("comment text is not printed", strstr(out, "never") == NULL);
}

static void test_comment_after_colon_statements(void) {
    printf("Comment: ''' after ':'-separated statements ignores the rest\n");

    /*
     * The comment must swallow everything after it on the line, including
     * any ':' inside the comment text (it must not be mistaken for another
     * statement separator).
     */
    const char *lines[] = {
        "10 PRINT \"A\": PRINT \"B\" ' comment: with a colon inside it",
        NULL
    };
    const char *out = run_program(lines);
    check("statements before the comment still run",
          strstr(out, "A") && strstr(out, "B"));
    check("comment text (and the ':' inside it) is not interpreted",
          strstr(out, "comment") == NULL && strstr(out, "with") == NULL);
}

static void test_comment_only_line(void) {
    printf("Comment: a line that is only a comment does not break the program\n");

    const char *lines[] = {
        "5 ' just a note, nothing to run here",
        "10 PRINT \"OK\"",
        NULL
    };
    const char *out = run_program(lines);
    check("program still runs normally", strstr(out, "OK") != NULL);
}

static void test_comment_preserved_in_list(void) {
    printf("Comment: text is kept in the stored line and shown by LIST\n");

    bastos_init();
    for (int i = 0; i < 64; i++)
        bastos_loop();

    /* Enter a line whose comment even contains a ':' and quote-like text. */
    const char *entry = "10 PRINT \"''toto''\" ' mon commentaire\r";
    bastos_send_keys(entry, strlen(entry), false);
    bastos_loop();
    bastos_loop();

    capture_clear();
    bastos_send_keys("LIST\r", 5, false);
    bastos_loop();
    bastos_loop();

    check("LIST reproduces the comment text verbatim",
          strstr(g_output, "mon commentaire") != NULL);
    check("LIST still shows the statement that precedes the comment",
          strstr(g_output, "PRINT") != NULL && strstr(g_output, "toto") != NULL);

    bastos_done();
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

    test_bug7_load_immediate_null_read_ptr();
    printf("\n");

    test_line_edit_insert_at_cursor();
    printf("\n");

    test_line_edit_consecutive_inserts();
    printf("\n");

    test_line_edit_correction_at_cursor();
    printf("\n");

    test_line_edit_right_arrow_returns_to_append();
    printf("\n");

    test_line_edit_ctrl_a_clears_both_sides();
    printf("\n");

    test_line_edit_multibyte_atomic();
    printf("\n");

    test_line_edit_ss2_mid_line_before_diacritic_byte();
    printf("\n");

    test_line_edit_ss2_abandoned_by_other_keys();
    printf("\n");

    test_line_edit_up_down_absorbed_when_typing();
    printf("\n");

    test_line_edit_works_when_buffer_full();
    printf("\n");

    test_line_edit_input_statement();
    printf("\n");

    test_line_edit_finalize_shift();
    printf("\n");

    test_line_edit_correction_erases_with_spaces_in_mode80();
    printf("\n");

    test_line_edit_insert_redraws_without_erase_escape_in_mode80();
    printf("\n");

    test_line_edit_annulation_erases_with_spaces_in_mode80();
    printf("\n");

    test_ctrl_enter_clears_screen_in_mode80();
    printf("\n");

    test_ctrl_enter_passes_through_raw_in_mode40();
    printf("\n");

    test_mode80_survives_new();
    printf("\n");

    test_mode80_survives_run_with_no_program();
    printf("\n");

    test_edit_shows_the_staged_line();
    printf("\n");

    test_edit_prefills_at_end();
    printf("\n");

    test_edit_unmodified_round_trips();
    printf("\n");

    test_edit_allows_modification();
    printf("\n");

    test_edit_escape_after_syntax_error_keeps_original_line();
    printf("\n");

    test_edit_left_arrow_echoes_one_backspace();
    printf("\n");

    test_edit_past_last_line_is_a_noop();
    printf("\n");

    test_edit_missing_line_number_picks_next_line();
    printf("\n");

    test_edit_no_argument_targets_first_line();
    printf("\n");

    test_edit_zero_targets_first_line();
    printf("\n");

    test_edit_no_argument_with_empty_program_is_a_noop();
    printf("\n");

    test_suite_auto_stages_next_line();
    printf("\n");

    test_suite_with_no_next_line_behaves_like_enter();
    printf("\n");

    test_retour_auto_stages_previous_line();
    printf("\n");

    test_retour_with_no_previous_line_behaves_like_enter();
    printf("\n");

    test_recall_noop_if_line_since_deleted();
    printf("\n");

    test_recall_numbered_line_via_up_arrow();
    printf("\n");

    test_recall_immediate_command_via_up_arrow();
    printf("\n");

    test_recall_discarded_by_new_typing();
    printf("\n");

    test_recall_survives_a_blank_enter();
    printf("\n");

    test_recall_not_offered_for_autoexec_banner();
    printf("\n");

    test_syntax_error_beeps_and_stays_editable();
    printf("\n");

    test_syntax_error_never_stores_invalid_line();
    printf("\n");

    test_valid_line_does_not_beep();
    printf("\n");

    test_runtime_error_does_not_stay_in_edit_mode();
    printf("\n");

    test_next_bare_nested();
    printf("\n");

    test_next_bare_mixed_with_explicit();
    printf("\n");

    test_next_bare_no_active_loop();
    printf("\n");

    test_next_wrong_variable_is_error();
    printf("\n");

    test_colon_basic_sequence();
    printf("\n");

    test_colon_if_then();
    printf("\n");

    test_colon_single_line_for_next();
    printf("\n");

    test_colon_gosub_return();
    printf("\n");

    test_else_true_false_branches();
    printf("\n");

    test_else_absent_no_regression();
    printf("\n");

    test_else_multi_statement_clauses();
    printf("\n");

    test_else_nested_if_outer_false_skips_whole_then();
    printf("\n");

    test_else_nested_if_outer_true_inner_false_with_else();
    printf("\n");

    test_else_nested_if_inside_else_clause();
    printf("\n");

    test_else_doubly_nested_depth();
    printf("\n");

    test_else_bare_linenum_goto();
    printf("\n");

    test_else_gosub_in_then_with_else_present();
    printf("\n");

    test_else_gosub_in_else_clause();
    printf("\n");

    test_else_syntax_check_does_not_beep();
    printf("\n");

    test_else_list_roundtrip();
    printf("\n");

    test_else_skips_string_with_embedded_nul();
    printf("\n");

    test_else_skips_string_with_apostrophe();
    printf("\n");

    test_else_orphan_is_a_noop();
    printf("\n");

    test_else_inherits_equals_ambiguity();
    printf("\n");

    test_else_save_load_roundtrip();
    printf("\n");

    test_label_syntax_check_does_not_beep();
    printf("\n");

    test_label_runs_without_error();
    printf("\n");

    test_label_list_roundtrip();
    printf("\n");

    test_label_save_load_roundtrip();
    printf("\n");

    test_comment_trailing();
    printf("\n");

    test_comment_after_colon_statements();
    printf("\n");

    test_comment_only_line();
    printf("\n");

    test_comment_preserved_in_list();
    printf("\n");

    printf("=== Results: %d/%d tests passed ===\n",
           g_tests_run - g_tests_failed, g_tests_run);

    return g_tests_failed > 0 ? 1 : 0;
}
