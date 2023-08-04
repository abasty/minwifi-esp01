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

#ifndef TTY_MINITEL_H
#define TTY_MINITEL_H

#define CODE_SEQUENCE_MAX_SIZE 8

#define DEL "\x08 \x08"

#define CLS "\x0C"
#define CLEOL "\x18"

#define CON "\x11"
#define COFF "\x14"

#define CUR "\x1F%c%c"
#define CUR_DELTA_V 64
#define CUR_DELTA_H 64

#define INV "\x1B\x5D"
#define BLINK "\x1B\x48"

#define INK "\x1B%c"
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

#define P_ROULEAU PRO2 "\x69\x43"

// Déconnexion
#define P_DECONNEXION PRO1 "\x67"

// Serial 4800bds
#define P_PRISE_1200 PRO2 "\x6B\x64"
#define P_PRISE_4800 PRO2 "\x6B\x76"

#endif // TTY_MINITEL_H
