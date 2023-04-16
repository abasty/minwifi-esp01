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

#ifndef Shell_h
#define Shell_h

#include <Print.h>
#include <functional>

#include "Terminal.h"

#define COMMAND_IP_PORT 8267

class Shell
{
public:
    typedef std::function<void()> InputCallback;

    Shell(Terminal *term);
    virtual ~Shell() {}

    void setTerm(Terminal *term = 0);
    size_t handle(char *src, size_t s);

    void print(const char *str);
    void println(const char *str);
    void input(const char *str, char *buf, size_t n, InputCallback fn);

    // TODO using Print::write

private:
    char *_srcCurP;
    char *_commandCurP;

    const char *_endOfSeqP;
    const char *_endOfSeqCurP;
    uint8_t _endOfSeqLength;

    char *_inputP;
    char *_inputCurP;
    ssize_t _inputSize;
    InputCallback _inputCallback;

    void _clearCommand();
    void _handleCommand();
    void _setEndOfSeq(const char *eosP);

protected:
    // If one wants \r\n as line ending: add \r before \n *and*
    // *and* *and*, increase COMMAND_EOS_MAX_LENGTH by one
    // For \r, just replace \n with \r
#define COMMAND_MAX_SIZE 32
#define COMMAND_EOS_MAX_LENGTH 4
    char endOfCommand[2] = { '\r', 0 };
    char endOfInput[2] = { '\r', 0 };

    char _command[COMMAND_MAX_SIZE + COMMAND_EOS_MAX_LENGTH];
    Terminal *_term;

    // This function must be overloaded to implement a command interpreter
    virtual void runCommand() = 0;
};

#endif // Shell_h
