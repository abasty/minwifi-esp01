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

#ifndef Terminal_h
#define Terminal_h

#include <Print.h>

class Terminal : public Print
{
public:
    Print *_out;
    Terminal(Print *out) {
        _out = out;
    };
    virtual ~Terminal() {}
    virtual size_t write(uint8_t byte) {
        return _out->write(byte);
    }
    virtual size_t write(const uint8_t *buffer, size_t size) {
        return _out->write(buffer, size);
    }

    virtual void newLineIfNeeded() {};
    virtual void prompt() {
        _out->print("minOS> ");
    };
    virtual void clear() {};
    virtual void home() {};
    virtual void gotoXY(int x, int y) {};
};

class TerminalVT100 : public Terminal
{
public:
    TerminalVT100(Print *out = 0) : Terminal(out) {}

    virtual void clear() {
        _out->print("\033[2J" "\033[H");
    }
    virtual void home() {
        _out->print("\033[H");
    }
    virtual void gotoXY(int x, int y) {};
};

class TerminalMinitel : public Terminal
{
public:
    TerminalMinitel(Print *out = 0) : Terminal(out) {}

    virtual void newLineIfNeeded() {
        _out->println();
    }
    virtual void clear() {
        _out->print("\x0C");
    }
    virtual void home() {
        _out->print("\033[H");
    }
    virtual void gotoXY(int x, int y) {};
};

#endif // Terminal_h
