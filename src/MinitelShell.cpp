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

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <strings.h>

#include "token.h"
#include "keywords.h"

#include "minitel.h"
#include "Terminal.h"
#include "CncManager.h"
#include "MinitelShell.h"

extern ConnectionManager cm;
extern WiFiClient tcpMinitelConnexion;
extern bool minitelMode;

extern WebSocketsClient webSocket;
extern bool _3611;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
    switch(type) {
    case WStype_DISCONNECTED:
#ifndef MINITEL
        Serial.printf("[WSc] Disconnected!\n");
#endif
        break;
    case WStype_CONNECTED:
        minitelMode = true;
#ifndef MINITEL
        Serial.printf("[WSc] Connected to url: %s\n", payload);
#endif
        break;
    case WStype_TEXT:
        if (length > 0 && payload) {
            Serial.printf("%s", payload);
        }
        break;
    case WStype_PING:
#ifndef MINITEL
        Serial.printf("[WSc] get ping\n");
#endif
        break;
    default:
        break;
    }
}

void MinitelShell::runCommand()
{
    _term->newLineIfNeeded();

    tokenizer_state_t state;
    tokenize(&state, _command);
    uint8_t token1 = token_get_next(&state);
    uint16_t value = 0;
    if (token1 == TOKEN_NUMBER)
    {
        value = token_number_get_value(&state);
        _term->printf("value: %u\n", value);
    }
    uint8_t token2 = token_get_next(&state);
    if (token1 == TOKEN_KEYWORD_FREE) {
        _term->printf("Heap:             %u bytes free.\r\n", ESP.getFreeHeap());
        _term->printf("Flash Real Size:  %u bytes.\r\n", ESP.getFlashChipRealSize());
        _term->printf("Sketch Size:      %u bytes.\r\n", ESP.getSketchSize());
        _term->printf("Free Sketch Size: %u bytes.\r\n", ESP.getFreeSketchSpace());
    } else if (token1 == TOKEN_KEYWORD_CATS) {
        _term->println("Hello from Cat-Labs");
    } else if (token1 == TOKEN_KEYWORD_RESET) {
        ESP.restart();
    } else if (token1 == TOKEN_KEYWORD_CONNECT) {
        cm.connect();
    } else if (token1 == TOKEN_KEYWORD_CONFIG && token2 == 0) {
        input(
            "Enter SSID: ", inputBuffer, INPUT_BUFFER_SIZE,
        [&]() {
            cm.setSSID(inputBuffer);
            println("OK");
            input(
                "Enter PASS: ", inputBuffer, INPUT_BUFFER_SIZE,
            [&]() {
                cm.setPassword(inputBuffer);
                println("\r\nUse CONNECT to use this config.");
                println("OK");
            });
        });
    } else if (token1 == TOKEN_KEYWORD_CONFIG && token2 == TOKEN_KEYWORD_SAVE) {
        bool OK = cm.save();
        if (OK) {
            _term->println("OK");
        } else {
            _term->println("Saved failed.");
        }
    } else if (token1 == TOKEN_KEYWORD_CONFIG && token2 == TOKEN_KEYWORD_LOAD) {
        bool OK = cm.load();
        if (OK) {
            _term->println("OK");
        } else {
            _term->println("Load failed.");
        }
    } else if (token1 == TOKEN_KEYWORD_CLEAR) {
        _term->clear();
    } else if (token1 == TOKEN_NUMBER && value == 3615) {
#ifdef MINITEL
        _term->print((char *)P_LOCAL_ECHO_OFF);
#endif
        webSocket.begin("3615co.de", 80, "/ws");
        webSocket.onEvent(webSocketEvent);
        _3611 = true;
        return;
    } else if (token1 == TOKEN_NUMBER && value == 3611) {
#ifdef MINITEL
        _term->print((char *)P_LOCAL_ECHO_OFF);
#endif
        webSocket.begin("3611.re", 80, "/ws");
        webSocket.onEvent(webSocketEvent);
        _3611 = true;
        return;
    } else {
        if (_command && *_command) {
            _term->println("ERROR");
        }
    }
    _term->prompt();
}
