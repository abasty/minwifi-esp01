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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef MINITEL
#include "tty-minitel.h"
#else
#include "tty-vt100.h"
#endif

#include "os-private.h"

// External interface (extern functions)
#include "bio.h"
#include "os.h"

// Internal interface (static functions)
#include "bdb.h"
#include "berror.h"
#include "bmemory.h"
#include "eval.h"
#include "token.h"

#include "bdb.c-static"
#include "bmemory.c-static"
#include "eval.c-static"
#include "keywords.c-static"
#include "os.c-static"
#include "string.c-static"
#include "token.c-static"

void bastos_init(void) {
    void *mem = malloc(BASTOS_MEMORY_SIZE);
    if (mem == NULL)
        return;
    bmem_init(mem, BASTOS_MEMORY_SIZE);
}

void bastos_done() {
    if (bmem == NULL)
        return;
    hal_net_disconnect(DB_MIN_SET, bmem->sock);
    hal_net_disconnect(DB_FTP_SET, -1);
    hal_wifi_disconnect();
    hal_speed(TOKEN_KEYWORD_SLOW);
    free(bmem);
    bmem = 0;
}

bool bastos_is_reset() { return bmem == 0 || bmem->bstate.reset; }

static void bastos_handle_escape() {
    bastos_stop();
    *bmem->io_buffer = 0;
    bmem->io_cursor = 0;
    os_redir_print_string("*Break*\r\n");
}

static bool is_char_diacritic(char test_char) {
    if (test_char == 'A')
        return true;
    if (test_char == 'B')
        return true;
    if (test_char == 'C')
        return true;
    if (test_char == 'H')
        return true;
    if (test_char == 'K')
        return true;
    return false;
}

static bool is_char_input_key(char test_char) {
    if (test_char >= ' ') // Printable chars and DEL
        return true;
    if (test_char == '\r') // Enter key
        return true;
    if (test_char == SS2) // G2 chars prefix
        return true;
    if ((test_char >= 1 && test_char <= 7) || test_char == 14) // Function keys and Ctrl+G
        return true;
    if (test_char == '\x08' || test_char == '\x09') // Left / right arrow
        return true;
    return false;
}

// Byte-width of the logical character immediately before `pos` (never
// looking further back than `line_start`): 1 for a plain byte, 2 for an
// SS2- or SO-prefixed character, 3 for SS2+diacritic-code+combined char.
// The Minitel always consumes the byte right after a diacritic code —
// combining it into an accented glyph when possible, or just showing it
// plain (accent silently dropped) otherwise — so the group is always
// exactly 3 bytes / 1 column regardless of what that byte is. Returns 0 if
// there is nothing before `pos` on this line.
static uint8_t char_width_before(uint8_t *line_start, uint8_t *pos) {
    int32_t len = pos - line_start;
    if (len < 1)
        return 0;
    if (len >= 3 && *(pos - 3) == SS2 && is_char_diacritic(*(pos - 2)))
        return 3;
    if (len >= 2 && (*(pos - 2) == SS2 || *(pos - 2) == SO))
        return 2;
    return 1;
}

// Mirror of char_width_before, looking forward from `pos` instead. Returns 0
// if `pos` is at (or past) `end` or right before the line's own terminator.
static uint8_t char_width_at(uint8_t *pos, uint8_t *end) {
    if (pos >= end || *pos == '\n')
        return 0;
    if (*pos == SS2 && pos + 2 < end && is_char_diacritic(*(pos + 1)))
        return 3;
    if ((*pos == SS2 || *pos == SO) && pos + 1 < end)
        return 2;
    return 1;
}

// Number of logical characters (screen columns) in [from, end) — every unit
// char_width_at() recognizes renders as exactly one column, same as DEL
// already assumes by sending itself exactly once regardless of raw width.
static uint8_t visual_width(uint8_t *from, uint8_t *end) {
    uint8_t cols = 0;
    uint8_t *p = from;
    while (p < end) {
        uint8_t width = char_width_at(p, end);
        if (width == 0)
            break;
        p += width;
        cols++;
    }
    return cols;
}

// Byte-width (1 or 2) of a still-incomplete SS2 sequence ending at `pos`
// that hasn't been resolved (and thus not yet echoed) yet: either a lone
// SS2 (could still become a self-contained 2-byte G2 symbol, or the start
// of a 3-byte accent), or SS2+diacritic-code (always needs exactly one
// more byte to complete — combining it, or dropping the accent if it
// can't). Returns 0 once the unit is definitely complete.
static uint8_t pending_unit_width(uint8_t *line_start, uint8_t *pos) {
    int32_t len = pos - line_start;
    if (len >= 1 && *(pos - 1) == SS2)
        return 1;
    if (len >= 2 && *(pos - 2) == SS2 && is_char_diacritic(*(pos - 1)))
        return 2;
    return 0;
}

// Silently removes a still-incomplete, never-echoed SS2 sequence sitting
// right before *cur from the buffer — nothing needs to be erased on
// screen, since nothing was ever shown for it. Called before any operation
// other than an insert that continues the sequence, so we never strand
// un-echoed bytes (e.g. moving away, deleting, or validating the line
// mid-accent abandons it, the same way a dead key is cancelled by doing
// anything other than completing it).
static void discard_pending(uint8_t **end, uint8_t **cur, uint8_t *line_start) {
    uint8_t pending = pending_unit_width(line_start, *cur);
    if (pending == 0)
        return;
    uint8_t *unit_start = *cur - pending;
    size_t tail_len = *end - *cur;
    memmove(unit_start, *cur, tail_len + 1); // +1 also moves the NUL terminator
    *end -= pending;
    *cur = unit_start;
}

// Position right after the last '\n' still in the buffer before `end`, or
// bmem->io_buffer itself if there is none — i.e. the start of the line
// currently being edited.
static uint8_t *line_start_of(uint8_t *end) {
    uint8_t *p = end;
    while (p > bmem->io_buffer && *(p - 1) != '\n')
        p--;
    return p;
}

// Reprints [from, end) (terminal cursor assumed to already be at `from`),
// erases any stale trailing column with CLEOL, then walks the terminal
// cursor back left to `leave_at` (which must be within [from, end]) — the
// caller's new logical cursor position, not necessarily where the reprint
// started (e.g. after an insert, the cursor must end up one column past the
// character just typed, not back at its start). No-op if echo is off.
static void redraw_tail(uint8_t *from, uint8_t *end, uint8_t *leave_at, bool echo) {
    if (!echo)
        return;
    if (end > from)
        hal_print_buffer(from, end - from);
    hal_print_string(CLEOL);
    for (uint8_t cols = visual_width(leave_at, end); cols > 0; cols--)
        hal_print_string("\x08");
}

// Deletes the logical character immediately before *cur (the former
// del_last_key), shifting [*cur, *end) left to close the gap. No-op if *cur
// is already at line_start.
static void delete_before_cursor(uint8_t **end, uint8_t **cur, uint8_t *line_start, bool echo) {
    if (pending_unit_width(line_start, *cur) > 0) {
        discard_pending(end, cur, line_start);
        return;
    }

    uint8_t width = char_width_before(line_start, *cur);
    if (width == 0)
        return;

    uint8_t *unit_start = *cur - width;
    if (width == 2 && *unit_start == SO) {
        bmem->bstate.g_mode = 0;
        if (echo)
            hal_print_string(G0);
    }

    size_t tail_len = *end - *cur;
    memmove(unit_start, *cur, tail_len + 1); // +1 also moves the NUL terminator
    *end -= width;
    *cur = unit_start;

    if (echo) {
        hal_print_string("\x08");
        redraw_tail(unit_start, *end, unit_start, echo);
    }
}

// Moves *cur back one logical character. Buffer is untouched — nothing is
// erased, only the terminal cursor moves. A still-pending, never-echoed
// sequence right before the cursor is abandoned instead (see
// discard_pending): there is no valid "cursor mid-accent" position to move
// through, since nothing was ever shown for it.
static void move_cursor_left(uint8_t **end, uint8_t **cur, uint8_t *line_start, bool echo) {
    if (pending_unit_width(line_start, *cur) > 0) {
        discard_pending(end, cur, line_start);
        return;
    }

    uint8_t width = char_width_before(line_start, *cur);
    if (width == 0)
        return;
    *cur -= width;
    if (echo)
        hal_print_string("\x08");
}

// Moves *cur forward one logical character by re-printing it — printing
// already advances the terminal's own cursor, so no separate "cursor right"
// escape is needed (portable across Minitel and VT100 alike). Discards any
// still-pending sequence right before the cursor first (see
// discard_pending), then moves normally from the resulting position.
static void move_cursor_right(uint8_t **end, uint8_t **cur, uint8_t *line_start, bool echo) {
    if (pending_unit_width(line_start, *cur) > 0)
        discard_pending(end, cur, line_start);

    uint8_t width = char_width_at(*cur, *end);
    if (width == 0)
        return;
    if (echo)
        hal_print_buffer(*cur, width);
    *cur += width;
}

// Erases the whole current line on screen (both before and after the
// cursor) and truncates the buffer back to line_start.
static void clear_current_line(uint8_t **end, uint8_t **cur, uint8_t *line_start, bool echo) {
    if (echo) {
        for (uint8_t cols = visual_width(line_start, *cur); cols > 0; cols--)
            hal_print_string("\x08");
        hal_print_string(G0 CLEOL);
    }
    bmem->bstate.g_mode = 0;
    *cur = line_start;
    *end = line_start;
    **end = 0;
}

void bastos_send_keys(const char *keys, size_t n, bool echo) {
    uint8_t *src = (uint8_t *)keys;
    uint8_t *dst = bmem->io_buffer;

    // If no keys, do nothing
    if (n == 0 || src == 0 || *src == 0) {
        if (eval_paused()) {
            eval_check_pause();
        }
        return;
    }

    // If key is ESC, stop the program
    if (*src == '\e') {
        bastos_handle_escape();
        return;
    }

    // If running and not inputting or paused mode, store the key in inkey state
    if ((eval_running() && !eval_inputting()) || eval_paused()) {
        bmem->inkey = (char)*src;
        if (eval_paused()) {
            eval_check_pause();
        }
        return;
    }

    // If key is not a printable char nor an editing key, BASTOS ignores it but
    // the key is sent to terminal. Some filtered sequences does not come here
    // anyway (INS/ DEL sequences for example), see os_get_key().
    if (!is_char_input_key(*src)) {
        // Up/Down: once something has been typed, silently absorb them
        // instead of letting them leak through to the terminal — they have
        // no action yet (reserved for a possible future use, e.g. command
        // history). Left untouched while the buffer is empty.
        if ((*src == '\x0a' || *src == '\x0b') && *bmem->io_buffer != 0)
            return;
        if (echo && !eval_inputting())
            hal_print_buffer(src, 1);
        return;
    }

    // Find the terminal 0 in io buffer
    for (; *dst; dst++)
        ;

    size_t size = dst - bmem->io_buffer;
    uint8_t *line_start = line_start_of(dst);
    uint8_t *cur = bmem->io_buffer + bmem->io_cursor;
    if (cur < line_start || cur > dst) // defensive: stale/out-of-range cursor
        cur = dst;

    while (*src && n > 0) {
        bool room = size < IO_BUFFER_SIZE - 1;

        if (*src == 1) { // Annulation (ctrl+A): clears the rest of the batch too
            clear_current_line(&dst, &cur, line_start, echo);
            src++;
            n = 0;
            break;
        } else if (*src == '\r' || *src == 2 || *src == 4 || *src == 5 || *src == 6 || *src == 14) {
            // Validation key: always submits the whole line, regardless of
            // where the edit cursor currently sits.
            if (!room) {
                src++;
                n--;
                continue;
            }
            discard_pending(&dst, &cur, line_start);
            bmem->vkey = *src;
            *dst++ = '\n';
            src++;
            bmem->bstate.g_mode = 0;
            if (echo)
                hal_print_string(G0 "\r\n");
            line_start = dst;
            cur = dst;
        } else if (*src == 127) { // Correction (DEL): deletes before the cursor
            delete_before_cursor(&dst, &cur, line_start, echo);
            src++;
        } else if (*src == '\x08') { // Left arrow
            move_cursor_left(&dst, &cur, line_start, echo);
            src++;
        } else if (*src == '\x09') { // Right arrow
            move_cursor_right(&dst, &cur, line_start, echo);
            src++;
        } else {
            if (!room) {
                src++;
                n--;
                continue;
            }
            uint8_t byte_to_insert;
            if (*src == 7) { // Ctrl+G
                bmem->bstate.g_mode = !bmem->bstate.g_mode;
                byte_to_insert = bmem->bstate.g_mode ? SO : SI;
                src++;
            } else {
                byte_to_insert = *src++;
            }

            size_t tail_len = dst - cur;
            if (tail_len > 0)
                memmove(cur + 1, cur, tail_len + 1); // +1 also moves the NUL terminator
            *cur = byte_to_insert;
            dst++;
            *dst = 0;

            if (tail_len == 0) {
                // Appending at the true end is always safe to echo right
                // away: there is no unrelated content after it that a
                // still-incomplete SS2 sequence could wrongly combine with.
                if (echo) {
                    uint8_t one[2] = {byte_to_insert, 0};
                    hal_print_string((char *)one);
                }
                cur++;
            } else {
                // Mid-line insert: defer the echo while a still-incomplete
                // SS2 sequence is in progress, so we never print a dangling
                // SS2/diacritic code directly next to unrelated tail bytes
                // it could get wrongly combined with (see
                // pending_unit_width). Once it resolves — a self-contained
                // 2-byte G2 symbol, or a completed 3-byte accent — redraw
                // the whole group plus the tail in one atomic step.
                cur++;
                if (pending_unit_width(line_start, cur) == 0) {
                    uint8_t width = char_width_before(line_start, cur);
                    redraw_tail(cur - width, dst, cur, echo);
                }
            }
        }
        size = dst - bmem->io_buffer;
        n--;
    }
    *dst = 0;
    bmem->io_cursor = (uint8_t)(cur - bmem->io_buffer);
}

static int8_t bastos_input() {
    int8_t err = BERROR_NONE;

    // Find first command end
    uint8_t *next = bmem->io_buffer;
    while (*next && *next != '\n')
        next++;

    // If no command: do nothing
    if (*next == 0)
        return BERROR_NONE;

    // Mark first command end with 0 and point to next one
    *next++ = 0;

    // Prepare move of the next commands to buffer start
    uint8_t *src = next;
    uint8_t *dst = bmem->io_buffer;

    // Manage INPUT command
    if (eval_inputting()) {
        err = eval_input_store((char *)bmem->io_buffer);
        goto finalize;
    }

    // Tokenize command and handle tokenize error case
    tokenizer_state_t line;
    err = tokenize(&line, (char *)bmem->io_buffer, false);
    if (err < 0)
        goto finalize;

    // Allocate memory for the prog line
    uint16_t len = line.write_ptr - line.read_ptr;
    prog_t *prog = bmem_prog_line_new(line.line_no, line.read_ptr, len);
    if (prog == 0) {
        if (len != 0) {
            err = BERROR_MEMORY;
        }
        goto finalize;
    }

    // Check syntax
    err = eval_prog(prog, false);
    if (err != BERROR_NONE) {
        bmem_prog_line_free(prog);
        goto finalize;
    }

    // If line number is 0, evaluate and remove
    if (prog->line_no == 0) {
        bool is_load = prog->line[0] == TOKEN_KEYWORD_LOAD;
        err = eval_prog(prog, true);
        if (!is_load)
            bmem_prog_line_free(prog);
    }

finalize:
    // remove first command
    while (*src)
        *dst++ = *src++;
    *dst = 0;

    // The edit cursor for whatever the user already typed of the next
    // command shifts down by the same amount the buffer content just did.
    {
        uint16_t shift = (uint16_t)(next - bmem->io_buffer);
        bmem->io_cursor = bmem->io_cursor >= shift ? bmem->io_cursor - shift : 0;
    }

    // Handle error
    if (err != BERROR_NONE) {
        os_redir_print_integer("Error %d\r\n", (int)-err);
    }

    return err;
}

static int32_t os_save_ascii(int fd) {
    os_set_redirect(fd);
    prog_t *prog = bmem_prog_first_line();
    while (prog) {
        os_redir_print_integer("%d ", prog->line_no);
        untokenize(prog->line);
        os_redir_print_string("\n");
        prog = bmem_prog_next_line(prog);
    }
    os_set_redirect(-1);
    return 0;
}

static int32_t os_save_bin(int fd, int16_t type) {
    if (type == FILE_TYPE_BST) {
        // save prog
        // Write prog total size
        uint16_t prog_size = bmem->prog_end - bmem->prog_start;
        if (hal_write(fd, &prog_size, sizeof(prog_size)) < 0)
            return -1;

        // Write prog
        if (hal_write(fd, bmem->prog_start, prog_size) < 0)
            return -1;
    } else {
        // Save empty prog
        uint16_t prog_size = 0;
        if (hal_write(fd, &prog_size, sizeof(prog_size)) < 0)
            return -1;
    }

    // save vars
    // Write vars total size
    uint16_t vars_size = bmem->vars_end - bmem->vars_start;
    if (hal_write(fd, &vars_size, sizeof(vars_size)) < 0)
        return -1;

    // Write vars
    if (hal_write(fd, bmem->vars_start, vars_size) < 0)
        return -1;

    return 0;
}

static int os_eval_string(char *str) {
    // Tokenize command and handle tokenize error case
    tokenizer_state_t line;
    int err = tokenize(&line, str, false);
    if (err < 0)
        goto finalize;

    // Allocate memory for the prog line
    uint16_t len = line.write_ptr - line.read_ptr;
    prog_t *prog = bmem_prog_line_new(line.line_no, line.read_ptr, len);
    if (prog == 0) {
        if (len != 0)
            err = BERROR_MEMORY;
        goto finalize;
    }

    // Check syntax
    err = eval_prog(prog, false);
    if (err != BERROR_NONE) {
        bmem_prog_line_free(prog);
        goto finalize;
    }

    // If line number is 0, evaluate and remove
    if (prog->line_no == 0) {
        bool is_load = prog->line[0] == TOKEN_KEYWORD_LOAD;
        err = eval_prog(prog, true);
        if (!is_load)
            bmem_prog_line_free(prog);
    }

finalize:
    // Handle error
    if (err != BERROR_NONE && line.line_no != 0)
        os_redir_print_integer("Line %d: ", (int)line.line_no);

    return err;
}

static void parse_utf8_to_minitel(char *dst, size_t dst_size, const char *src) {
    while (*src && dst_size > 1) {
        uint8_t prefix = *src;
        if (prefix == 0xc3 || prefix == 0xc2 || prefix == 0xc5) {
            uint8_t code = *(src + 1);
            uint8_t seq[4] = {0};
            seq[0] = '\x19'; // SS2
            if (prefix == 0xc3) {
                switch (code) {
                case 0xa0: // à
                    seq[1] = 'A';
                    seq[2] = 'a';
                    break;
                case 0xa8: // è
                    seq[1] = 'A';
                    seq[2] = 'e';
                    break;
                case 0xb9: // ù
                    seq[1] = 'A';
                    seq[2] = 'u';
                    break;
                case 0xa9: // é
                    seq[1] = 'B';
                    seq[2] = 'e';
                    break;
                case 0xa2: // â
                    seq[1] = 'C';
                    seq[2] = 'a';
                    break;
                case 0xaa: // ê
                    seq[1] = 'C';
                    seq[2] = 'e';
                    break;
                case 0xae: // î
                    seq[1] = 'C';
                    seq[2] = 'i';
                    break;
                case 0xb4: // ô
                    seq[1] = 'C';
                    seq[2] = 'o';
                    break;
                case 0xbb: // û
                    seq[1] = 'C';
                    seq[2] = 'u';
                    break;
                case 0xa4: // ä
                    seq[1] = 'H';
                    seq[2] = 'a';
                    break;
                case 0xab: // ë
                    seq[1] = 'H';
                    seq[2] = 'e';
                    break;
                case 0xaf: // ï
                    seq[1] = 'H';
                    seq[2] = 'i';
                    break;
                case 0xb6: // ö
                    seq[1] = 'H';
                    seq[2] = 'o';
                    break;
                case 0xbc: // ü
                    seq[1] = 'H';
                    seq[2] = 'u';
                    break;
                case 0xa7: // ç
                    seq[1] = 'K';
                    seq[2] = 'c';
                    break;
                case 0x87: // Ç
                    seq[1] = 'K';
                    seq[2] = 'C';
                    break;
                case 0x9f: // ß
                    seq[1] = '\x7b';
                    break;
                default:
                    seq[0] = 0;
                    break;
                }
            } else if (prefix == 0xc2) {
                switch (code) {
                case 0xa3: // £
                    seq[1] = '\x23';
                    break;
                case 0xa7: // §
                    seq[1] = '\x27';
                    break;
                case 0xb0: // °
                    seq[1] = '\x30';
                    break;
                case 0xb1: // ±
                    seq[1] = '\x31';
                    break;
                case 0xb7: // ÷
                    seq[1] = '\x38';
                    break;
                case 0xbc: // ¼
                    seq[1] = '\x34';
                    break;
                case 0xbd: // ½
                    seq[1] = '\x35';
                    break;
                case 0xbe: // ¾
                    seq[1] = '\x36';
                    break;
                default:
                    seq[0] = 0;
                    break;
                }
            } else if (prefix == 0xc5) {
                switch (code) {
                case 0x92: // Œ
                    seq[1] = '\x6a';
                    break;
                case 0x93: // œ
                    seq[1] = '\x7a';
                    break;
                default:
                    seq[0] = 0;
                    break;
                }
            }
            size_t slen = strlen((char *)seq);
            if (slen < dst_size) {
                strcpy(dst, (char *)seq);
                dst += slen;
                dst_size -= slen;
            }
            src += 2;
        } else {
            *dst++ = *src++;
            dst_size--;
        }
    }
    *dst = 0;
}

static int8_t os_load_ascii(int fd) {
    // Read lines from file
    char line_utf8[IO_BUFFER_SIZE + 1] = {0};
    int32_t remains = hal_read(fd, line_utf8, IO_BUFFER_SIZE);
    while (remains > 0) {
        // Find LF in line
        char *lf = strchr(line_utf8, '\n');
        // If no LF in line, return BERROR_IO
        if (lf == 0)
            return BERROR_IO;
        // replace LF with 0
        *lf = 0;
        // If CR just before LF, replace CR with 0
        if (lf > line_utf8 && *(lf - 1) == '\r')
            *(lf - 1) = 0;
        char line[IO_BUFFER_SIZE + 1] = {0};

        // Convert utf8 chars into Minitel SS2 chars
        parse_utf8_to_minitel(line, sizeof(line), line_utf8);

        // If line is empty, continue
        int err = os_eval_string(line);
        if (err < 0)
            return err;

        remains -= (lf + 1 - line_utf8);
        memmove(line_utf8, lf + 1, remains + 1);
        int n = hal_read(fd, line_utf8 + remains, IO_BUFFER_SIZE - remains);
        if (n < 0)
            return BERROR_IO;
        remains += n;
    }
    if (remains < 0)
        return BERROR_IO;

    return BERROR_NONE;
}

static int8_t os_load_bin(int fd, int16_t type) {
    int8_t err = BERROR_NONE;

    // load prog
    uint16_t prog_size;
    int bread = hal_read(fd, &prog_size, sizeof(prog_size));
    if (bread != sizeof(prog_size)) {
        err = BERROR_IO;
        goto finalize;
    }
    if (prog_size >= bmem->vars_start - bmem->prog_start) {
        err = BERROR_IO;
        goto finalize;
    }
    if (type == FILE_TYPE_VAR && prog_size != 0) {
        err = BERROR_IO;
        goto finalize;
    }
    if (type == FILE_TYPE_BST) {
        if (hal_read(fd, bmem->prog_start, prog_size) != prog_size) {
            err = BERROR_IO;
            goto finalize;
        }
        bmem->prog_end = bmem->prog_start + prog_size;
        bmem_strings_clear();
    }

    // load vars
    uint16_t vars_size;
    bread = hal_read(fd, &vars_size, sizeof(vars_size));
    if (bread != sizeof(vars_size)) {
        err = BERROR_IO;
        goto finalize;
    }
    if (vars_size >= bmem->vars_end - bmem->prog_end) {
        err = BERROR_IO;
        goto finalize;
    }
    bmem->vars_start = bmem->vars_end - vars_size;
    if (hal_read(fd, bmem->vars_start, vars_size) != vars_size) {
        err = BERROR_IO;
        bmem_vars_clear();
        goto finalize;
    }

finalize:
    return err;
}

int8_t bastos_save(const char *i_name) {
    int16_t type = -1;
    const char *name = os_filename(i_name, &type);
    if (name == 0 || type < 0)
        return -1;

    int fd = hal_open(name, B_CREAT | B_RDWR);
    if (fd < 0)
        goto err;

    if (type == FILE_TYPE_BST || type == FILE_TYPE_VAR)
        if (os_save_bin(fd, type) < 0)
            goto err;

    if (type == FILE_TYPE_BAS)
        if (os_save_ascii(fd) < 0)
            goto err;

    hal_close(fd);
    return 0;

err:
    if (fd >= 0) {
        hal_close(fd);
        hal_erase(name);
    }
    return -1;
}

int8_t bastos_load(const char *i_name) {
    int16_t type = -1;
    const char *name = os_filename(i_name, &type);
    if (name == 0 || type < 0)
        return BERROR_RANGE;

    int8_t err = BERROR_NONE;
    int fd = hal_open(name, B_RDONLY);

    if (fd < 0)
        return BERROR_IO;

    // Remove existing blocks
    if (type == FILE_TYPE_BAS || type == FILE_TYPE_BST)
        bastos_prog_new();

    if (type == FILE_TYPE_VAR)
        bastos_vars_clear();

    // Load file
    if (type == FILE_TYPE_VAR || type == FILE_TYPE_BST)
        err = os_load_bin(fd, type);

    if (type == FILE_TYPE_BAS)
        err = os_load_ascii(fd);

    hal_close(fd);
    bmem->bstate.read_ptr = 0;
    return err;
}

void bastos_stop() { eval_stop(); }

void bastos_loop() {
    if (bmem->bstate.reset)
        return;

    if (eval_running() && !eval_inputting() && !eval_paused()) {
        eval_prog_next();
        if (bmem->bstate.reset)
            return;

        if (!eval_running()) {
            hal_print_string("Ready\r\n");
        }
        return;
    }
    bastos_input();
}
