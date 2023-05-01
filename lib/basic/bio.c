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


#include "bmemory.h"
#include "token.h"
#include "bio.h"

t_bastos_io *io = 0;

void bastos_init(t_bastos_io *_io)
{
    // TODO handle errors
    io = _io;
    bmem_init();
}

size_t bastos_handle_keys(char *keys, size_t n)
{
    size_t i;
    for (i = 0, _srcCurP = keys; i < n; i++)
    {
        // if char is in the EOS
        if (*_srcCurP == *_endOfSeqCurP)
        {

            _srcCurP++;
            _endOfSeqCurP++;
            // if EOS reached
            if (_endOfSeqCurP - _endOfSeqP == _endOfSeqLength)
            {
                // handle _command
                _handleCommand();

                // if EOS has not been reset by _handleCommand()
                // then reset it to default endOfCommand sequence.
                if (_endOfSeqCurP != _endOfSeqP)
                {
                    _setEndOfSeq(endOfCommand);
                }
            }
        }
        else
        {
            // the char is out of the EOS
            // copy left EOS chars if any
            for (const char *copySeqP = _endOfSeqP; copySeqP != _endOfSeqCurP;)
            {
                *_commandCurP++ = *copySeqP++;
            }
            // reset EOS
            _endOfSeqCurP = _endOfSeqP;
            if (*_srcCurP >= ' ')
            {
                // copy char
                *_commandCurP++ = *_srcCurP++;
            }
            // if buffer full
            if (_commandCurP - _command >= COMMAND_MAX_SIZE)
            {
                _handleCommand();
            }
        }
    }
    return n;
}

void bastos_loop()
{
    // run a prog step
}
