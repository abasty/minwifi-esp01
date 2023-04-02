/*
Copyright (c) 2017, Alain Basty.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other
materials provided with the distribution.

Neither the name of Alain Basty nor the names of other contributors may be used
to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    virtual void clear() {
        _out->print("\x0C");
    }
    virtual void home() {
        _out->print("\033[H");
    }
    virtual void gotoXY(int x, int y) {};
};

#endif // Terminal_h
