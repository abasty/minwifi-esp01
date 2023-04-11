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
#include <ArduinoOTA.h>
#include <WebSocketsClient.h>

#include "Terminal.h"
#include "Shell.h"
#include "MinitelShell.h"
#include "CncManager.h"
#include "minitel.h"

#include "LittleFS.h"

// 0:	BUTTON
// 13:	LED
// 12:	RELAY
// 14:	EXTRA GPIO

int relayPin = 12;
int ledPin = 13;

// wifi server
WiFiServer *wifiServer = 0;

// serial shell, terminal and client
#ifdef MINITEL
Terminal *serialTerminal = new TerminalMinitel(&Serial);
#else
Terminal *serialTerminal = new TerminalVT100(&Serial);
#endif
MinitelShell *serialShell = new MinitelShell(serialTerminal);

// wifi shell, terminal and client
WiFiClient *wifiClient = 0;
Shell *wifiShell = 0;
Terminal *wifiTerminal = 0;

// Connection manager
ConnectionManager cm(serialShell);

// Minitel server TCP/IP connexion
WiFiClient tcpMinitelConnexion;
WebSocketsClient webSocket;
bool _3611 = false;

bool minitelMode;

void initMinitel(bool clear)
{
    // Empty Serial buffer
    while (Serial && Serial.available() > 0) {
        uint8_t buffer[32];
        Serial.readBytes(buffer, 32);
    }
    Serial.flush();
    if (clear) {
        serialTerminal->clear();
    }
#ifdef MINITEL
    Serial.print((char *)P_ACK_OFF_PRISE);
    Serial.print((char *)P_LOCAL_ECHO_ON);
    Serial.print((char *)CON);
#endif
}

void setup()
{
    // Initialize Sonoff pins
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH);

    // Initialize serial
#ifdef MINITEL
    Serial.begin(1200, SERIAL_7E1);
#else
    Serial.begin(115200);
#endif

    Serial.flush();
    Serial.println("");
    Serial.println("");
    serialTerminal->clear();
    Serial.println("Loading and connecting.");

    // Initialize file system
    LittleFS.begin();

    // connect Wifi if configuration available
    cm.load();
    cm.connect();

    // Initialize OTA
    ArduinoOTA.setPort(8266);

#ifdef MINITEL
    // Hostname
    ArduinoOTA.setHostname("esp-minitel");
#else
    ArduinoOTA.setHostname("esp-minitel-dev");
#endif

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

#ifndef MINITEL
    // TODO: Serial can be for another usage. So remove Serial at some time.
    ArduinoOTA.onStart([]() {
        Serial.println("FOTA Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("FOTA End");
        Serial.println("RESET");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        //Serial.printf("FOTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("FOTA Error %u: ", error);
    });
#endif

    ArduinoOTA.begin();

    // Launch traces server
    wifiServer = new WiFiServer(COMMAND_IP_PORT);
    wifiServer->begin();

    Serial.setTimeout(0);
    initMinitel(false);

    // connect to Minitel server if any
    serialTerminal->prompt();
}

void loop()
{
    ArduinoOTA.handle();

    digitalWrite(ledPin, HIGH);

    // Accept TCP shell connections
    if (wifiServer->hasClient()) {
        // Delete active connection if any and accept new one
        // TODO: It leaks when a new session is accepted and one is active, do not know why....
        if (wifiClient) {
            delete wifiClient;
        }
        if (wifiShell) {
            delete wifiShell;
        }
        if (wifiTerminal) {
            delete wifiTerminal;
        }
        wifiClient = new WiFiClient(wifiServer->available());
        wifiTerminal = new TerminalVT100(wifiClient);
        wifiShell = new MinitelShell(wifiTerminal);

        wifiTerminal->prompt();
    }

    // Handle commands from WiFi Client
    if (wifiClient && wifiClient->available() > 0) {
        // read client data
        uint8_t buffer[32];
        size_t n = wifiClient->read(buffer, 32);
        wifiShell->handle((char *)buffer, n);
    }

    // Forward Minitel server incoming data to serial output
    if (tcpMinitelConnexion && tcpMinitelConnexion.available() > 0) {
        // transparently forward bytes to serial
        uint8_t buffer[128];
        size_t n = tcpMinitelConnexion.read(buffer, 128);
        Serial.write(buffer, n);
    }

    // Handle Ws client
    //if (webSocket.isConnected()) {
    if (_3611) {
        webSocket.loop();
    }

    // If disconnected
    // if (minitelMode && (!tcpMinitelConnexion || !tcpMinitelConnexion.connected())) {
    //     minitelMode = false;
    //     tcpMinitelConnexion.stop();
    //     initMinitel(true);
    // }

    // Handle Serial input
    if (Serial && Serial.available() > 0) {
        if (!_3611) {
            // Command mode: Handle serial input with command shell
            uint8_t buffer[32];
            size_t n = Serial.readBytes(buffer, 32);
            serialShell->handle((char *)buffer, n);
        } else {
            // Minitel mode: Forward serial input to Minitel sever
            uint8_t key;
            size_t n = Serial.readBytes(&key, 1);
            if (n > 0) {
                // tcpMinitelConnexion.setNoDelay(true); // Disable nagle's algo.
                // tcpMinitelConnexion.write((char *)&key, 1);
                webSocket.sendTXT(key);
            }
        }
    }
}
