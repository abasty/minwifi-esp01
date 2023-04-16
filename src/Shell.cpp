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

#include <ESP8266WiFi.h>
#include <strings.h>

#include "Shell.h"

Shell::Shell(Terminal *term)
{
    setTerm(term);
}

void Shell::setTerm(Terminal *term)
{
    _term = term;
    _clearCommand();
    _setEndOfSeq(endOfCommand);
}

void Shell::print(const char *str)
{
    _term->print(str);
}

void Shell::println(const char *str)
{
    print(str);
    print("\r\n");
}

void Shell::input(const char *str, char *buf, size_t s, InputCallback fn)
{
    print(str);
    _clearCommand();
    _inputP = buf;
    _inputCurP = _inputP;
    _inputSize = s;
    _inputCallback = fn;
    _setEndOfSeq(endOfInput);
}

void Shell::_clearCommand()
{
    _commandCurP = _command;
}

void Shell::_setEndOfSeq(const char *eosP)
{
    _endOfSeqP = eosP;
    _endOfSeqCurP = _endOfSeqP;
    _endOfSeqLength = strlen(_endOfSeqP);
}

void Shell::_handleCommand()
{
    // line mode
    if (_endOfSeqP == endOfCommand) {
        *_commandCurP = 0;
        runCommand();
    } else if (_endOfSeqP == endOfInput) {
        char *commandP = _command;

        while (commandP < _commandCurP && _inputCurP - _inputP < _inputSize - 1) {
            *_inputCurP++ = *commandP++;
        }

        *_inputCurP = 0;

        if (_endOfSeqCurP - _endOfSeqP == _endOfSeqLength) {
            _inputCallback();
        }
    } else {
        // an error occurred: _endOfSeqP has been corrupted
    }
    // reset _command buffer
    _commandCurP = _command;
}

size_t Shell::handle(char *src, size_t s)
{
    size_t i;
    for (i = 0, _srcCurP = src; i < s; i++) {
        // if char is in the EOS
        if (*_srcCurP == *_endOfSeqCurP) {

            _srcCurP++;
            _endOfSeqCurP++;
            // if EOS reached
            if (_endOfSeqCurP - _endOfSeqP == _endOfSeqLength) {
                // handle _command
                _handleCommand();

                // if EOS has not been reset by _handleCommand()
                // then reset it to default endOfCommand sequence.
                if (_endOfSeqCurP != _endOfSeqP) {
                    _setEndOfSeq(endOfCommand);
                }
            }
        } else {
            // the char is out of the EOS
            // copy left EOS chars if any
            for (const char *copySeqP = _endOfSeqP; copySeqP != _endOfSeqCurP;) {
                *_commandCurP++ = *copySeqP++;
            }
            // reset EOS
            _endOfSeqCurP = _endOfSeqP;
            if (*_srcCurP >= ' ') {
                // copy char
                *_commandCurP++ = *_srcCurP++;
            }
            // if buffer full
            if (_commandCurP - _command >= COMMAND_MAX_SIZE) {
                _handleCommand();
            }
        }
    }
    return s;
}
