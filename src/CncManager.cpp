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

#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "CncManager.h"

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
    if (file) {
        file.printf("%s\n", _ssid.c_str());
        file.printf("%s\n", _secret.c_str());
        file.close();
        return true;
    } else {
        return false;
    }
}

bool ConnectionManager::load()
{
    File file = LittleFS.open(CNCMGR_SAVE_FILE, "r");
    if (file) {
        _ssid = file.readStringUntil('\n');
        _secret = file.readStringUntil('\n');
        file.close();
        return true;
    } else {
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
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        _shell->print(".");
    }
    _shell->println("");
    if (WiFi.status() == WL_CONNECTED) {
        // TODO: This should not use Serial but _shell
        Serial.println("WiFi connected.");
        Serial.print("miniterm -e \"socket://");
        Serial.print(WiFi.localIP());
        Serial.printf(":%u\"", COMMAND_IP_PORT);
        Serial.println("");
    } else {
        _shell->println("Connection Failed.");
    }
}

bool ConnectionManager::connect()
{
    if (isConnected()) {
        disconnect();
        delay(500);
    }

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    _shell->print("Trying WiFi on ");
    _shell->println(_ssid.c_str());
    if (_ssid.length() == 0) {
        _shell->println("ERROR: No SSID set, use CONFIG.");
        return isConnected();
    }

    if (_secret.length() == 0) {
        _shell->println("WARNING: Empty password, use CONFIG.");
    }
    WiFi.begin(_ssid.c_str(), _secret.c_str());
    _waitForWiFi("Connecting");

    return isConnected();
}
