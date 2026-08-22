/* * Copyright © 2023-2025 Alain Basty
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

#include "tty-minitel.h"

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
    bmem->io_recall_len = 0; // any resident recall text is being wiped too
    os_redir_print_string("*Break*\r\n");
}

// G0 (tty-minitel.h's charset-reset macro) only means something while the
// terminal is actually in native Minitel Videotex mode. There is no G0/G1
// charset-shift concept in 80-column mode (MODE 80, bstate.screen_mode) —
// nothing needs to be sent there.
static const char *mode_g0() {
    return bmem->bstate.screen_mode ? "" : G0;
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

// Any of the keys that submit the current io_buffer line (ENVOI, REPETITION,
// SUITE, RETOUR, SOMMAIRE, GUIDE). This exact byte is what terminates the
// submitted line in io_buffer (see the validation branch below) instead of a
// generic '\n' — see is_validation_key()'s callers for why that matters.
static bool is_validation_key(uint8_t b) {
    return b == '\r' || b == 2 || b == 4 || b == 5 || b == 6 || b == 14;
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
    if (pos >= end || is_validation_key(*pos))
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

// Position right after the last queued line's terminator still in the
// buffer before `end`, or bmem->io_buffer itself if there is none — i.e.
// the start of the line currently being edited.
static uint8_t *line_start_of(uint8_t *end) {
    uint8_t *p = end;
    while (p > bmem->io_buffer && !is_validation_key(*(p - 1)))
        p--;
    return p;
}

// Reprints [from, end) (terminal cursor assumed to already be at `from`),
// overwrites `stale_cols` further columns with spaces to erase any leftover
// from a shorter previous line (0 when the line only grew, as with an
// insert; 1 after a single-character delete), then walks the terminal
// cursor back left to `leave_at` (which must be within [from, end]) — the
// caller's new logical cursor position, not necessarily where the reprint
// started (e.g. after an insert, the cursor must end up one column past the
// character just typed, not back at its start).
//
// Deliberately avoids any "erase to end of line" escape (Videotex CLEOL or
// its ANSI equivalent): on real hardware, 80-column mode does not reliably
// support it — Correction/insert were leaving stray characters on screen
// and losing cursor sync. Plain backspace/space/backspace is used instead,
// the same technique already relied on elsewhere (e.g. os_get_string()),
// since it only needs a bare C0 backspace and a printable space, which
// both modes handle the same way. No-op if echo is off.
static void redraw_tail(uint8_t *from, uint8_t *end, uint8_t *leave_at, uint8_t stale_cols, bool echo) {
    if (!echo)
        return;
    if (end > from)
        hal_print_buffer(from, end - from);
    for (uint8_t i = 0; i < stale_cols; i++)
        hal_print_string(" ");
    uint8_t back = stale_cols + visual_width(leave_at, end);
    for (; back > 0; back--)
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
            hal_print_string(mode_g0());
    }

    size_t tail_len = *end - *cur;
    memmove(unit_start, *cur, tail_len + 1); // +1 also moves the NUL terminator
    *end -= width;
    *cur = unit_start;

    if (echo) {
        hal_print_string("\x08");
        redraw_tail(unit_start, *end, unit_start, 1, echo);
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
// escape is needed. Discards any still-pending sequence right before the
// cursor first (see discard_pending), then moves normally from the
// resulting position.
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
// cursor) and truncates the buffer back to line_start. Backs up to
// line_start, resets the charset, then overwrites the entire former line
// with spaces and backs up again — see redraw_tail() for why this avoids
// an "erase to end of line" escape.
static void clear_current_line(uint8_t **end, uint8_t **cur, uint8_t *line_start, bool echo) {
    if (echo) {
        for (uint8_t cols = visual_width(line_start, *cur); cols > 0; cols--)
            hal_print_string("\x08");
        hal_print_string(mode_g0());
        uint8_t total_cols = visual_width(line_start, *end);
        for (uint8_t i = 0; i < total_cols; i++)
            hal_print_string(" ");
        for (uint8_t i = 0; i < total_cols; i++)
            hal_print_string("\x08");
    }
    bmem->bstate.g_mode = 0;
    *cur = line_start;
    *end = line_start;
    **end = 0;
}

// Loads line_no's own text into io_buffer for editing — used both by EDIT
// <line_no> and by the Up-arrow recall of the last stored numbered line:
// prepend its untokenized text ahead of whatever is already queued there,
// show it, and place the cursor at its end. No-op if line_no is 0 or no
// longer exists.
static void bastos_load_edit_line(uint16_t line_no) {
    if (line_no == 0)
        return;
    prog_t *edit_prog = bmem_prog_get_line_or_next(line_no);
    if (!edit_prog || edit_prog->line_no != line_no)
        return;

    uint8_t remainder[IO_BUFFER_SIZE];
    strcpy((char *)remainder, (char *)bmem->io_buffer);

    bmem->io_buffer[0] = 0;
    os_set_redirect_prefill(true);
    os_redir_print_integer("%d ", (int)edit_prog->line_no);
    untokenize(edit_prog->line);
    os_set_redirect_prefill(false);

    size_t edit_len = strlen((char *)bmem->io_buffer);
    hal_print_buffer(bmem->io_buffer, (int)edit_len);

    size_t room = sizeof(bmem->io_buffer) - 1 - edit_len;
    size_t rem_len = strlen((char *)remainder);
    if (rem_len > room)
        rem_len = room;
    memcpy(bmem->io_buffer + edit_len, remainder, rem_len);
    bmem->io_buffer[edit_len + rem_len] = 0;

    bmem->io_cursor = (uint8_t)edit_len;
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
        // Up: recall the last successfully-submitted line for further
        // editing, as long as nothing has been typed since. Two sources:
        // an immediate command's raw text, kept resident right here in
        // io_buffer (tokenize() only ever tags bit 7 on former keyword-end
        // bytes, so clearing it recovers exactly what was typed); or, for
        // a numbered line, its own text loaded from program memory.
        if (*src == '\x0b') {
            if (bmem->io_recall_len != 0) {
                uint8_t recall_len = bmem->io_recall_len;
                for (uint8_t i = 0; i < recall_len; i++)
                    bmem->io_buffer[i] &= (uint8_t)~KEYWORD_END_TAG;
                bmem->io_recall_len = 0;
                bmem->io_cursor = recall_len;
                if (echo)
                    hal_print_buffer(bmem->io_buffer, recall_len);
                return;
            }
            if (*bmem->io_buffer == 0 && bmem->io_last_line != 0) {
                bastos_load_edit_line(bmem->io_last_line);
                return;
            }
        }

        // Ctrl+Enter (byte 12): on a real Minitel keyboard this sends the
        // same code in both 40- and 80-column mode, but only means
        // "clear screen" natively to the terminal in 40-column/Videotex
        // mode, where 0x0C is itself the CLS code — echoing it raw there
        // already works. In 80-column/ANSI mode the terminal doesn't
        // recognize a raw 0x0C as clear-screen, so translate it to the
        // ANSI equivalent instead of echoing it raw (which would do
        // nothing there).
        if (*src == '\x0c' && bmem->bstate.screen_mode) {
            bmem_screen_clear();
            if (echo && !eval_inputting())
                hal_print_string(MODE80_CLS);
            return;
        }

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

    // Enter with a pending recall and nothing typed since: submit an empty
    // line (no-op, same as Enter on a truly empty buffer) instead of either
    // resubmitting the stale text as a duplicate command or discarding it —
    // Up-arrow must still bring back that same previous entry afterward.
    // This shortcut only makes sense for the interactive immediate-command
    // prompt: a running program's INPUT has no resident recall text of its
    // own, so a leftover recall from before RUN must not swallow its first
    // validation keypress (which would echo CRLF without ever terminating
    // the line, leaving the INPUT stuck). No line is queued here, so there
    // is nothing for VKEY to reflect — unlike the real validation branch
    // below, this must not touch bmem->vkey (see is_validation_key()'s
    // callers for why a global write here would be a race).
    if (bmem->io_recall_len != 0 && !eval_inputting() && is_validation_key(*src)) {
        if (echo) {
            hal_print_string(mode_g0());
            hal_print_string("\r\n");
        }
        return;
    }

    // A pending Up-arrow recall (the last immediate command, kept resident
    // in io_buffer) is only valid until the user starts a real edit; any
    // other accepted editing key here discards it first so it can't get
    // mixed into a new line.
    if (bmem->io_recall_len != 0) {
        bmem->io_buffer[0] = 0;
        bmem->io_recall_len = 0;
        bmem->io_cursor = 0;
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
        } else if (is_validation_key(*src)) {
            // Validation key: always submits the whole line, regardless of
            // where the edit cursor currently sits. The key's own byte is
            // stored as the line's terminator (instead of a generic '\n')
            // so that whichever key ends up validating THIS particular
            // queued line can still be recovered correctly at the moment
            // it's actually dequeued and processed by bastos_input() —
            // which may be several lines and several more keypresses later
            // if the interpreter is still busy (e.g. mid-redraw) when keys
            // arrive faster than they can be consumed. A single shared
            // bmem->vkey written here immediately, instead, would already
            // reflect a *later* keypress by the time this line is finally
            // dequeued.
            if (!room) {
                src++;
                n--;
                continue;
            }
            discard_pending(&dst, &cur, line_start);
            *dst++ = *src;
            src++;
            bmem->bstate.g_mode = 0;
            if (echo) {
                hal_print_string(mode_g0());
                hal_print_string("\r\n");
            }
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
                    redraw_tail(cur - width, dst, cur, 0, echo);
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
    bool syntax_error = false;

    // Find first command end
    uint8_t *next = bmem->io_buffer;
    while (*next && !is_validation_key(*next))
        next++;

    // If no command: do nothing
    if (*next == 0)
        return BERROR_NONE;

    // VKEY reflects whichever key actually terminated THIS command — read
    // from the terminator byte itself (see is_validation_key()'s callers),
    // not from some separately-tracked "last key pressed" state that could
    // already have been overwritten by a later keypress still queued behind
    // this one.
    bmem->vkey = *next;

    // Mark first command end with 0 and point to next one. cmd_len is the
    // length of exactly what was typed for this command; tokenize() below
    // only ever mutates it by tagging bit 7 on the last character of each
    // recognized keyword (its own bookkeeping) — it never touches anything
    // else — so io_buffer[0..cmd_len) stays recoverable as plain text just
    // by clearing that bit, with no separate copy needed.
    uint16_t cmd_len = (uint16_t)(next - bmem->io_buffer);
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
    if (err < 0) {
        syntax_error = true;
        goto finalize;
    }

    // Check syntax before touching program memory: tokenize() already wrote
    // the tokenized line into bmem->bstate.token_buffer (the same scratch
    // slot bmem_prog_line_new() itself uses for line_no == 0), so a
    // temporary prog_t pointing there can be checked without inserting
    // anything yet. This matters because bmem_prog_line_new() unconditionally
    // deletes any existing line with this number as part of inserting the
    // new one — checking first means an invalid replacement (e.g. after
    // recalling a line and making a typo) never wipes out its original
    // content.
    uint16_t len = line.write_ptr - line.read_ptr;
    prog_t *check = (prog_t *)&bmem->bstate.token_buffer;
    check->line_no = line.line_no;
    check->len = len;
    err = eval_prog(check, false);
    if (err != BERROR_NONE) {
        syntax_error = true;
        goto finalize;
    }

    // Allocate memory for the prog line
    prog_t *prog = bmem_prog_line_new(line.line_no, line.read_ptr, len);
    if (prog == 0) {
        if (len != 0) {
            err = BERROR_MEMORY;
        }
        goto finalize;
    }

    // If line number is 0, evaluate and remove
    if (prog->line_no == 0) {
        bool is_load = prog->line[0] == TOKEN_KEYWORD_LOAD;
        err = eval_prog(prog, true);
        if (!is_load)
            bmem_prog_line_free(prog);
    }

    // A positively validated line becomes the new Up-arrow recall target,
    // replacing whatever was recallable before it. A numbered line is
    // recalled straight from program memory (bastos_load_edit_line()), so
    // no buffer bookkeeping is needed here. An immediate command has no
    // such permanent copy, so its raw text is instead kept resident at the
    // front of io_buffer below — but only when nothing is already queued
    // after it (no room for both without a second buffer), and not when it
    // was itself an EDIT (that takes priority and has its own mechanism),
    // nor when it was a system-injected command the user never typed.
    if (bmem->io_no_recall) {
        bmem->io_no_recall = 0;
    } else if (prog->line_no != 0) {
        bmem->io_last_line = prog->line_no;
        bmem->io_recall_len = 0;

        // SUITE (VKEY 4) / RETOUR (VKEY 5): behave like ENVOI/Enter, but
        // also automatically stage the next/previous program line for
        // editing (if there is one) — a shortcut for reviewing/editing a
        // run of consecutive lines forwards or backwards.
        if (bmem->vkey == 4) {
            prog_t *next_line = bmem_prog_next_line(prog);
            if (next_line)
                bmem->io_edit_line = next_line->line_no;
        } else if (bmem->vkey == 5) {
            prog_t *prev_line = bmem_prog_prev_line(prog);
            if (prev_line)
                bmem->io_edit_line = prev_line->line_no;
        }
    } else if (bmem->io_edit_line == 0) {
        bmem->io_last_line = 0;
        bmem->io_recall_len = (*src == 0) ? (uint8_t)cmd_len : 0;
    }

finalize:
    if (syntax_error) {
        // Not valid BASIC: beep and stay in edit mode instead of silently
        // discarding it, so it can be fixed and resubmitted. Recover the
        // original typed text in place (see cmd_len above) and fold back
        // in whatever else was queued after it.
        os_redir_print_string("\x07");
        for (uint8_t *p = bmem->io_buffer; p < bmem->io_buffer + cmd_len; p++)
            *p &= (uint8_t)~KEYWORD_END_TAG;
        size_t avail = sizeof(bmem->io_buffer) - 1 - cmd_len;
        size_t rem_len = strlen((char *)src);
        if (rem_len > avail)
            rem_len = avail;
        memmove(bmem->io_buffer + cmd_len, src, rem_len + 1);
        bmem->io_cursor = (uint8_t)cmd_len;
    } else if (bmem->io_recall_len != 0) {
        // The just-executed immediate command is being kept resident (see
        // above) for a possible Up-arrow recall: leave it exactly where it
        // is instead of the usual shift-down. It's already correctly
        // terminated (the '\n' this command ended with was overwritten
        // with a NUL above), and io_recall_len is only ever set when
        // nothing else was queued after it.
        bmem->io_cursor = 0;
    } else {
        // remove first command
        while (*src)
            *dst++ = *src++;
        *dst = 0;

        // The edit cursor for whatever the user already typed of the next
        // command shifts down by the same amount the buffer content just
        // did.
        uint16_t shift = (uint16_t)(next - bmem->io_buffer);
        bmem->io_cursor = bmem->io_cursor >= shift ? bmem->io_cursor - shift : 0;
    }

    // If EDIT staged a line (bmem->io_edit_line), load it now that
    // io_buffer holds only whatever is genuinely still queued (normally
    // nothing) — it was never echoed while EDIT ran, so the user needs to
    // see it now.
    if (!syntax_error && bmem->io_edit_line != 0) {
        uint16_t edit_line = bmem->io_edit_line;
        bmem->io_edit_line = 0;
        bastos_load_edit_line(edit_line);
    }

    // Handle error
    if (err != BERROR_NONE) {
        os_redir_print_integer("Error %d\r\n", (int)-err);
    }

    // "Error N" was printed below the line still being edited, so the
    // terminal cursor is now on a fresh row while io_cursor/io_buffer still
    // point into the (no longer visible) line above it. Re-echo the line so
    // what's on screen matches that state again.
    if (syntax_error) {
        hal_print_buffer(bmem->io_buffer, (int)cmd_len);
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
        } else if (prefix == 0xe2) {
            uint8_t code1 = *(src + 1);
            uint8_t code2 = *(src + 2);
            uint8_t seq[4] = {0};
            if (code1 == 0x86) {
                // Arrows (G2 set, single-shift SS2)
                seq[0] = '\x19'; // SS2
                switch (code2) {
                case 0x90: // ←
                    seq[1] = '\x2c';
                    break;
                case 0x91: // ↑
                    seq[1] = '\x2d';
                    break;
                case 0x92: // →
                    seq[1] = '\x2e';
                    break;
                case 0x93: // ↓
                    seq[1] = '\x2f';
                    break;
                default:
                    seq[0] = 0;
                    break;
                }
            } else if (code1 == 0x80 || code1 == 0x94 || code1 == 0x96) {
                // Line-drawing characters: plain G0 glyphs (same set as
                // digits/letters), not a G1 mosaic approximation — no
                // shift needed, just the single byte. (Middle vertical "|"
                // and bottom horizontal "_" need no entry here at all —
                // they're already plain ASCII, one byte, unchanged by this
                // whole function, and that byte already is the target
                // Minitel code.)
                if (code1 == 0x80) {
                    switch (code2) {
                    case 0xbe: // ▔ horizontal top (U+203E, overline)
                        seq[0] = '\x7e';
                        break;
                    default:
                        break;
                    }
                } else if (code1 == 0x94) {
                    switch (code2) {
                    case 0x80: // ─ horizontal middle
                        seq[0] = '\x60';
                        break;
                    default:
                        break;
                    }
                } else { // code1 == 0x96
                    switch (code2) {
                    case 0x8f: // ▏ vertical left (U+258F, left one eighth block)
                        seq[0] = '\x7b';
                        break;
                    case 0x95: // ▕ vertical right (U+2595, right one eighth block)
                        seq[0] = '\x7d';
                        break;
                    default:
                        break;
                    }
                }
            }
            size_t slen = strlen((char *)seq);
            if (slen < dst_size) {
                strcpy(dst, (char *)seq);
                dst += slen;
                dst_size -= slen;
            }
            src += 3;
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
