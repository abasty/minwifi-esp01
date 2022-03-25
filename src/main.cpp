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
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Print.h>
#include <IPAddress.h>
#include <strings.h>
#include "Shell.h"
#include "MinitelShell.h"
#include "CncManager.h"
#include "minitel.h"

#include "FS.h"

// serial and TCP shells.
MinitelShell serialShell(&Serial);
MinitelShell tcpShell;

// Command server and client (just zero or one client for now)
WiFiServer *tcpShellServer = 0;
WiFiClient *tcpShellClient = 0;

// Connection manager
ConnectionManager cm(&serialShell);

// Minitel server TCP/IP connexion
WiFiClient tcpMinitelConnexion;
bool minitelMode;

void initMinitel(bool clear)
{
    // Empty Serial buffer
    while (Serial && Serial.available() > 0)
    {
        uint8_t buffer[32];
        Serial.readBytes(buffer, 32);
    }
    Serial.flush();
    if (clear)
        Serial.print("\x0C");
    Serial.print((char *)P_ACK_OFF_PRISE);
    Serial.print((char *)P_LOCAL_ECHO_ON);
    Serial.println("Ready.");
    Serial.print((char *)CON);
}

void setup()
{
    // Initialize serial
    Serial.begin(1200);
    Serial.flush();
    Serial.println("");
    Serial.println("");
    Serial.println("\x0CLoading and connecting.");

    // Initialize file system
    SPIFFS.begin();

    // connect Wifi if configuration available
    cm.load();
    cm.loadOpt();
    cm.connect();

    // Initialize OTA
    ArduinoOTA.setPort(8266);

    // Hostname
    ArduinoOTA.setHostname("esp-minitel");

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    // TODO: Serial can be for another usage. So remove Serial at some time.
    /*    ArduinoOTA.onStart([]() {
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
    });*/

    ArduinoOTA.begin();

    // Launch traces server
    tcpShellServer = new WiFiServer(COMMAND_IP_PORT);
    tcpShellServer->begin();

    Serial.setTimeout(0);
    initMinitel(false);

    // connect to Minitel server if any
    serialShell.connectServer();
}

void loop()
{
    ArduinoOTA.handle();

    // Accept TCP shell connections
    if (tcpShellServer->hasClient())
    {
        // Delete active connection if any and accept new one
        // TODO: It leaks when a new session is accepted and one is active, do not know why....
        if (tcpShellClient)
            delete tcpShellClient;
        tcpShellClient = new WiFiClient(tcpShellServer->available());
        tcpShell.setTerm((Print *)tcpShellClient);
        tcpShellClient->printf("Ready\n");
    }

    // Handle commands from WiFi Client
    if (tcpShellClient && tcpShellClient->available() > 0)
    {
        // read client data
        uint8_t buffer[32];
        size_t n = tcpShellClient->read(buffer, 32);
        tcpShell.handle((char *)buffer, n);
    }

    // Forward Minitel server incoming data to serial output
    if (tcpMinitelConnexion && tcpMinitelConnexion.available() > 0)
    {
        // transparently forward bytes to serial
        uint8_t buffer[128];
        size_t n = tcpMinitelConnexion.read(buffer, 128);
        Serial.write(buffer, n);
    }

    // If disconnected
    if (minitelMode && (!tcpMinitelConnexion || !tcpMinitelConnexion.connected()))
    {
        minitelMode = false;
        tcpMinitelConnexion.stop();
        initMinitel(true);
    }

    // Handle Serial input
    if (Serial && Serial.available() > 0)
    {
        if (!minitelMode)
        {
            // Command mode: Handle serial input with command shell
            uint8_t buffer[32];
            size_t n = Serial.readBytes(buffer, 32);
            serialShell.handle((char *)buffer, n);
        }
        else
        {
            // Minitel mode: Forward serial input to Minitel sever
            uint8_t key;
            size_t n = Serial.readBytes(&key, 1);
            if (n > 0)
            {
                tcpMinitelConnexion.setNoDelay(true); // Disable nagle's algo.
                tcpMinitelConnexion.write((char *)&key, 1);
            }
        }
    }
}
