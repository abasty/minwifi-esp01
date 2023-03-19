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

#ifndef Shell_h
#define Shell_h

#include <Print.h>
#include <functional>

#define COMMAND_IP_PORT 8267

class Shell
{
public:
  typedef std::function<void()> InputCallback;

  Shell(Print *term = 0);

  void setTerm(Print *term = 0);
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
  Print *_term;

  // This function must be overloaded to implement a command interpreter
  virtual void runCommand() = 0;
};

#endif // Shell_h
