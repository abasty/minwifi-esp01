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

#include <ESP8266WiFi.h>
#include <strings.h>
#include "Shell.h"

Shell::Shell(Print *term)
{
    setTerm(term);
}

void Shell::setTerm(Print *term)
{
    _term = term;
    _clearCommand();
    _setEndOfSeq(endOfCommand);
}

void Shell::print(const char *str)
{
    if (_term)
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

void Shell::runCommand()
{
    if (strcasecmp(_command, "free") == 0)
    {
        if (_term)
        {
            _term->printf("Heap:             %u bytes free.\n", ESP.getFreeHeap());
            _term->printf("Flash Real Size:  %u bytes.\n", ESP.getFlashChipRealSize());
            _term->printf("Sketch Size:      %u bytes.\n", ESP.getSketchSize());
            _term->printf("Free Sketch Size: %u bytes.\n", ESP.getFreeSketchSpace());
        }
    }
    else if (strcasecmp(_command, "cats") == 0)
    {
        if (_term)
        {
            _term->println("Hello from Cat-Labs");
            _term->println("OK");
        }
    }
    else if (strcasecmp(_command, "reset") == 0)
    {
        ESP.restart();
    }
    else
    {
        if (_term)
        {
            _term->println("ERROR");
        }
    }
}

void Shell::_handleCommand()
{
    // Nothing to do
    if (_commandCurP == _command)
        return;

    // line mode
    if (_endOfSeqP == endOfCommand)
    {
        *_commandCurP = 0;
        runCommand();
    }
    else if (_endOfSeqP == endOfInput)
    {
        char *commandP = _command;

        while (commandP < _commandCurP && _inputCurP - _inputP < _inputSize - 1)
            *_inputCurP++ = *commandP++;

        *_inputCurP = 0;

        if (_endOfSeqCurP - _endOfSeqP == _endOfSeqLength)
        {
            _inputCallback();
        }
    }
    else
    {
        // an error occurred: _endOfSeqP has been corrupted
    }
    // reset _command buffer
    _commandCurP = _command;
}

size_t Shell::handle(char *src, size_t s)
{
    size_t i;
    for (i = 0, _srcCurP = src; i < s; i++)
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
                    _setEndOfSeq(endOfCommand);
            }
        }
        else
        {
            // the char is out of the EOS
            // copy left EOS chars if any
            for (const char *copySeqP = _endOfSeqP; copySeqP != _endOfSeqCurP;)
                *_commandCurP++ = *copySeqP++;
            // reset EOS
            _endOfSeqCurP = _endOfSeqP;
            if (*_srcCurP >= ' ')
            {
                // copy char
                *_commandCurP++ = *_srcCurP++;
            }
            // if buffer full
            if (_commandCurP - _command >= COMMAND_MAX_SIZE)
                _handleCommand();
        }
    }
    return s;
}
