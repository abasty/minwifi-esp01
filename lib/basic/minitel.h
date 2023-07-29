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

#ifndef minitel_h
#define minitel_h

// Cuseur ON
// #define CON "\x11"
// #define COFF "\x14"
#define CON "A"
#define COFF "Z"

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

// Echo ON en mode local
#define P_LOCAL_ECHO_ON PRO3 P_ON P_MODEM_RX P_CLAVIER_TX
#define P_LOCAL_ECHO_OFF PRO3 P_OFF P_MODEM_RX P_CLAVIER_TX

#define P_ROULEAU PRO2 "\x69\x43"

#endif // minitel_h
