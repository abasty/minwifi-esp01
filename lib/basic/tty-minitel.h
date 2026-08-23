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

#ifndef TTY_MINITEL_H
#define TTY_MINITEL_H

#define CODE_SEQUENCE_MAX_SIZE 32

#define DEL "\x08 \x08"

#define CLS "\x0C"
#define CLEOL "\x18"

#define CON "\x11"
#define COFF "\x14"

#define G0 "\x0F"
#define SI (0x0f)
#define G1 "\x0E"
#define SO (0x0e)
#define G2 "\x19"
#define SS2 (0x19)
#define SEP (0x13)

#define CUR "\x1F%c%c"
#define CUR_DELTA_V 64
#define CUR_DELTA_H 64
#define LINE0 "\x1F\x40\x41"

#define INV "\x1B\x5D"
#define NORMAL "\x1B\x5C"
#define BLINK "\x1B\x48"

#define INK "\x1B%c"
#define INK1 "\x1B\x41"
#define INK7 "\x1B\x47"
#define INK_DELTA 0x40

#define PAPER "\x1B%c"
#define PAPER_DELTA 0x50

// Protocole
#define PRO1 "\x1B\x39"
#define PRO2 "\x1B\x3A"
#define PRO3 "\x1B\x3B"

#define P_OFF "\x60"
#define P_ON "\x61"
#define P_NON_RETOUR_ACQUITEMENT "\x64"

#define P_CLAVIER_TX "\x51"
#define P_MODEM_RX "\x5A"
#define P_PRISE_TX "\x53"

// Non retour d'acquitement sur prise
#define P_ACK_OFF_PRISE PRO2 P_NON_RETOUR_ACQUITEMENT P_PRISE_TX

// Echo ON/OFF en mode local
#define P_LOCAL_ECHO_ON PRO3 P_ON P_MODEM_RX P_CLAVIER_TX
#define P_LOCAL_ECHO_OFF PRO3 P_OFF P_MODEM_RX P_CLAVIER_TX

#define P_ROULEAU_ON PRO2 "\x69\x43"
#define P_ROULEAU_OFF PRO2 "\x6A\x43"

#define P_CLAVIER_MINUSCULE PRO2 "\x69\x45"
#define P_CLAVIER_MAJUSCULE PRO2 "\x6A\x45"
#define P_CLAVIER_ETENDU PRO3 "\x69\x59\x41"
#define P_CLAVIER_VIDEOTEX PRO3 "\x6A\x59\x41"

// Force 40-column mode at startup: the terminal may still be in 80-column
// mode left over from before BASTOS started (bstate.screen_mode always
// starts at 0/40-column internally, but that's only BASTOS's own state —
// nothing otherwise tells the physical terminal to match it).
#define P_MODE_40 PRO2 "\x32\x7e"

#define MODE_INIT_STRING P_ACK_OFF_PRISE P_LOCAL_ECHO_OFF P_MODE_40 P_ROULEAU_ON P_CLAVIER_MINUSCULE P_CLAVIER_ETENDU CON G0 INK7

// Déconnexion
#define P_DECONNEXION PRO1 "\x67"

// Vitesse Serial
#define P_PRISE_1200 PRO2 "\x6B\x64"
#define P_PRISE_4800 PRO2 "\x6B\x76"
#define P_PRISE_9600 PRO2 "\x6B\x7f"

// Mode terminal (40/80 cols)
#define P_80_COLS PRO2 "\x32\x7d"
#define P_40_COLS PRO2 "\x32\x7e"

// Mode 80 colonnes - codes ANSI-like pour mode 80 colonnes
#define MODE80_CLS "\x1B[2J\x1B[H"
#define MODE80_CLEOL "\x1B[K"
#define MODE80_CON "\x1B[<1l"
#define MODE80_COFF "\x1B[<1h"
#define MODE80_CUR "\x1B[%d;%dH"
#define MODE80_CUR_DELTA_V 0
#define MODE80_CUR_DELTA_H 0
#define MODE80_CUD1 "\x1B[1B"

#define MODE80_BOLD "\x1B[1m"
#define MODE80_BOLD_OFF "\x1B[22m"
#define MODE80_INV "\x1B[7m"
#define MODE80_INV_OFF "\x1B[27m"
#define MODE80_NORMAL "\x1B[m"
#define MODE80_BLINK "\x1B[5m"
#define MODE80_BLINK_OFF "\x1B[25m"
#define MODE80_UNDERLINE "\x1B[4m"
#define MODE80_UNDERLINE_OFF "\x1B[24m"
#define MODE80_ROULEAU_ON "\x1B\x3A\x69\x43"
#define MODE80_ROULEAU_OFF "\x1B\x3A\x6A\x43"
#define MODE80_INSC_ON "\x1B[4h"
#define MODE80_INSC_OFF "\x1B[4l"

#endif // TTY_MINITEL_H
