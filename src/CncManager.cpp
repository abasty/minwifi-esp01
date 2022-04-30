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

#include "CncManager.h"
#include "LittleFS.h"

#define CNCMGR_SAVE_FILE "/etc/cncmgr.save"
#define CNCMGR_SAVE_OPT_FILE "/etc/cncmgr_opt.save"

ConnectionManager::ConnectionManager(Shell *shell)
{
    _shell = shell;
    _ssid = "";
    _secret = "";
}

void ConnectionManager::setSSID(const char *ssid)
{
    _ssid = ssid;
}

const char *ConnectionManager::getSSID()
{
    return _ssid.c_str();
}

void ConnectionManager::setPassword(const char *pass)
{
    _secret = pass;
}

bool ConnectionManager::save()
{
    File file = LittleFS.open(CNCMGR_SAVE_FILE, "w");
    if (file)
    {
        file.printf("%s\n", _ssid.c_str());
        file.printf("%s\n", _secret.c_str());
        file.close();
        return true;
    }
    else
    {
        return false;
    }
}

bool ConnectionManager::load()
{
    File file = LittleFS.open(CNCMGR_SAVE_FILE, "r");
    if (file)
    {
        _ssid = file.readStringUntil('\n');
        _secret = file.readStringUntil('\n');
        file.close();
        return true;
    }
    else
    {
        return false;
    }
}

void ConnectionManager::setServerIP(char *ip)
{
    _serverIP.fromString(ip);
}

IPAddress ConnectionManager::getServerIP()
{
    return _serverIP;
}

void ConnectionManager::setServerPort(char* port)
{
    _serverPort = String(port).toInt();
}

int ConnectionManager::getServerPort()
{
    return _serverPort;
}

bool ConnectionManager::saveOpt()
{
    File file = LittleFS.open(CNCMGR_SAVE_OPT_FILE, "w");
    if (file)
    {
        file.printf("%s\n", _serverIP.toString().c_str());
        file.printf("%s\n", String(_serverPort).c_str());
        file.close();
        return true;
    }
    else
    {
        return false;
    }
}

bool ConnectionManager::loadOpt()
{
    File file = LittleFS.open(CNCMGR_SAVE_OPT_FILE, "r");
    if (file)
    {
        String line;
        line = file.readStringUntil('\n');
        _serverIP.fromString(line);
        line = file.readStringUntil('\n');
        _serverPort = line.toInt();
        file.close();
        return true;
    }
    else
    {
        return false;
    }
}

bool ConnectionManager::isConnected()
{
    return WiFi.isConnected();
}

void ConnectionManager::disconnect()
{
    WiFi.disconnect();
}

void ConnectionManager::_waitForWiFi(const char *message)
{
    unsigned long startTime;

    // TODO: _shell can be NULL, test against
    _shell->print(message);
    // Give ESP 10 seconds to connect
    startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
        delay(500);
        _shell->print(".");
    }
    _shell->println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        // TODO: This should not use Serial but _shell
        Serial.println("WiFi connected.");
        Serial.print("miniterm -e \"socket://");
        Serial.print(WiFi.localIP());
        Serial.printf(":%u\"", COMMAND_IP_PORT);
        Serial.println("");
    }
    else
    {
        _shell->println("Connection Failed.");
    }
}

bool ConnectionManager::connect()
{
    if (isConnected())
    {
        disconnect();
        delay(500);
    }

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    _shell->print("Trying WiFi on ");
    _shell->println(_ssid.c_str());
    if (_ssid.length() == 0)
    {
        _shell->println("ERROR: No SSID set, use CONFIG.");
        return isConnected();
    }

    if (_secret.length() == 0)
    {
        _shell->println("WARNING: Empty password, use CONFIG.");
    }
    WiFi.begin(_ssid.c_str(), _secret.c_str());
    _waitForWiFi("Connecting");

    return isConnected();
}
