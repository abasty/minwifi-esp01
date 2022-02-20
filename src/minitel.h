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

#ifndef minitel_h
#define minitel_h

// Cuseur ON
#define CON "\x11"

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

#endif // minitel_h