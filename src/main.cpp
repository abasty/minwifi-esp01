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
#include "CncManager.h"
#include "minitel.h"

#include "FS.h"

class MinitelShell : public Shell
{
  public:
    MinitelShell(Print *term = 0, Print *bin = 0) : Shell(term, bin) {}
#define INPUT_SIZE 64
    char input0[INPUT_SIZE];

    void connectServer();
  protected:
    virtual void runCommand();
};

// serial and TCP shells.
MinitelShell minitelShell(&Serial, &Serial);
MinitelShell TCPShell;

// Command server and client (just zero or one client for now)
WiFiServer *CommandTCPServer = 0;
WiFiClient *CommandTCPClient = 0;

// Connection manager
ConnectionManager cm(&minitelShell);

// Minitel TCP/IP server
WiFiClient MinitelServer;
bool minitelMode;

void initMinitelShell(bool clear)
{
    // Empty Serial buffer
    while (Serial && Serial.available() > 0)
    {
        uint8_t buffer[32];
        size_t n = Serial.readBytes(buffer, 32);
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
    CommandTCPServer = new WiFiServer(COMMAND_IP_PORT);
    CommandTCPServer->begin();

    Serial.setTimeout(0);
    initMinitelShell(false);

    // connect to Minitel server if any
    minitelShell.connectServer();
}

void MinitelShell::connectServer()
{
    if (cm.isConnected())
    {
        IPAddress addr = cm.getServerIP();
        int port = cm.getServerPort();
        if (port != 0)
        {
            _term->print("Connexion a ");
            _term->print(addr.toString());
            _term->print(":");
            _term->println(port);
            MinitelServer.connect(addr, port);
            if (MinitelServer.connected())
            {
                minitelMode = true;
                MinitelServer.setNoDelay(true);
            }
            else
            {
                Serial.println("ERROR");
            }
        }
        else
        {
            _term->println("Use CONFIGOPT to configure server.");
        }
    }
    else
    {
        if (_term)
            _term->println("Please connect first.");
    }
}

void MinitelShell::runCommand()
{
    if (_term)
        _term->printf("\r\n");

    if (strcasecmp(_command, "free") == 0)
    {
        if (_term)
        {
            _term->printf("Heap:              %u bytes free.\r\n", ESP.getFreeHeap());
            _term->printf("Flash Real Size:   %u bytes.\r\n", ESP.getFlashChipRealSize());
            _term->printf("Scketch Size:      %u bytes.\r\n", ESP.getSketchSize());
            _term->printf("Free Scketch Size: %u bytes.\r\n", ESP.getFreeSketchSpace());
        }
    }
    else if (strcasecmp(_command, "cats") == 0)
    {
        if (_bin)
            _bin->println("Hello from Cat-Labs\r\n");
        if (_term)
            _term->println("OK");
    }
    else if (strcasecmp(_command, "bin") == 0)
    {
        binaryMode();
        if (_term)
            _term->printf("CTRL-T CTRL-U to upload, terminate with %s\r", endOfBin);
    }
    else if (strcasecmp(_command, "reset") == 0)
    {
        ESP.restart();
    }
    else if (strcasecmp(_command, "config") == 0)
    {
        input("Enter SSID: ", input0, INPUT_SIZE, [](Shell *self) {
            cm.setSSID(((MinitelShell *)self)->input0);
            self->println("OK");
            self->input("Enter PASS: ", ((MinitelShell *)self)->input0, INPUT_SIZE, [](Shell *self) {
                cm.setPassword(((MinitelShell *)self)->input0);
                self->println("\r\nUse CONNECT to use this config.");
                self->println("OK");
            });
        });
    }
    else if (strcasecmp(_command, "connect") == 0)
    {
        cm.connect();
    }
    else if (strcasecmp(_command, "config save") == 0)
    {
        bool OK = cm.save();
        if (_term)
        {
            if (OK)
                _term->println("OK");
            else
                _term->println("Saved failed.");
        }
    }
    else if (strcasecmp(_command, "config load") == 0)
    {
        bool OK = cm.load();
        if (_term)
        {
            if (OK)
                _term->println("OK");
            else
                _term->println("Load failed.");
        }
    }
    else if (strcasecmp(_command, "clear") == 0)
    {
        if (_term)
            _term->println("\x0CReady.");
    }
    else if (strcasecmp(_command, "3615") == 0)
    {
        connectServer();
    }
    else if (strcasecmp(_command, "configopt") == 0)
    {
        input("Enter Server IP: ", input0, INPUT_SIZE, [](Shell *self) {
            cm.setServerIP(((MinitelShell *)self)->input0);
            self->println("OK");
            self->input("Enter Server Port: ", ((MinitelShell *)self)->input0, INPUT_SIZE, [](Shell *self) {
                cm.setServerPort(((MinitelShell *)self)->input0);
                self->println("\r\nUse 3615 to use this config.");
                self->println("OK");
            });
        });
    }
    else if (strcasecmp(_command, "configopt save") == 0)
    {
        bool OK = cm.saveOpt();
        if (_term)
        {
            if (OK)
                _term->println("OK");
            else
                _term->println("Saved failed.");
        }
    }
    else if (strcasecmp(_command, "configopt load") == 0)
    {
        bool OK = cm.loadOpt();
        if (_term)
        {
            if (OK)
                _term->println("OK");
            else
                _term->println("Load failed.");
        }
    }
    else
    {
        if (_term)
            _term->println("ERROR");
    }
}

void loop()
{
    ArduinoOTA.handle();

    // Handle Traces Client connection
    if (CommandTCPServer->hasClient())
    {
        // Delete active connection if any and accept new one
        // TODO: It leaks when a new session is accepted and one is active, do not know why....
        if (CommandTCPClient)
            delete CommandTCPClient;
        CommandTCPClient = new WiFiClient(CommandTCPServer->available());
        TCPShell.setTermBin((Print *)CommandTCPClient, &Serial);
        CommandTCPClient->printf("Ready\n");
    }

    // Handle commands from WiFi Client
    if (CommandTCPClient && CommandTCPClient->available() > 0)
    {
        // read client data
        uint8_t buffer[32];
        size_t n = CommandTCPClient->read(buffer, 32);
        TCPShell.handle((char *)buffer, n);
    }

    // Handle Minitel
    // TODO: end of connection (switch back to command mode)
    if (MinitelServer && MinitelServer.available() > 0)
    {
        // transparently forward bytes to serial
        uint8_t buffer[128];
        size_t n = MinitelServer.read(buffer, 128);
        Serial.write(buffer, n);
    }

    // If disconnected
    if (minitelMode && (!MinitelServer || !MinitelServer.connected()))
    {
        minitelMode = false;
        MinitelServer.stop();
        initMinitelShell(true);
    }

    // Handle Serial input
    if (Serial && Serial.available() > 0)
    {
        if (!minitelMode)
        {
            // Command mode mode
            uint8_t buffer[32];
            size_t n = Serial.readBytes(buffer, 32);
            minitelShell.handle((char *)buffer, n);
        }
        else
        {
            // Minitel mode
            // forward serial entry to MinitelServer
            uint8_t key;
            size_t n = Serial.readBytes(&key, 1);
            if (n > 0)
            {
                MinitelServer.setNoDelay(true); // This is just a flag to disable the nagle algo.
                MinitelServer.write((char *)&key, 1);
            }
        }
    }
}
