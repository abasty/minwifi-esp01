#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <strings.h>

#include "basic.h"
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

void MinitelShell::connectServer()
{
    if (cm.isConnected()) {
        IPAddress addr = cm.getServerIP();
        int port = cm.getServerPort();
        if (port != 0) {
            _term->print("Connecting to ");
            _term->print(addr.toString());
            _term->print(":");
            _term->println(port);
            tcpMinitelConnexion.connect(addr, port);
            if (tcpMinitelConnexion.connected()) {
                minitelMode = true;
                tcpMinitelConnexion.setNoDelay(true);
            } else {
                _term->println("ERROR");
            }
        } else {
            _term->println("Use CONFIGOPT to configure server.");
        }
    } else {
        _term->println("Please connect first.");
    }
}

void MinitelShell::runCommand()
{
    _term->newLineIfNeeded();

    t_tokenizer_state state;
    tokenize(&state, _command);
    uint8_t token1 = token_get_next(&state);
    uint16_t value = 0;
    if (token1 == TOKEN_INTEGER)
    {
        value = token_integer_get_value(&state);
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
    } else if (token1 == TOKEN_KEYWORD_CONFIGOPT && token2 == 0) {
        input(
            "Enter Server IP: ", inputBuffer, INPUT_BUFFER_SIZE,
        [&]() {
            cm.setServerIP(inputBuffer);
            println("OK");
            input(
                "Enter Server Port: ", inputBuffer, INPUT_BUFFER_SIZE,
            [&]() {
                cm.setServerPort(inputBuffer);
                println("\r\nUse 3615 to use this config.");
                println("OK");
            });
        });
        return;
    } else if (token1 == TOKEN_KEYWORD_CONFIGOPT && token2 == TOKEN_KEYWORD_SAVE) {
        bool OK = cm.saveOpt();
        if (OK) {
            _term->println("OK");
        } else {
            _term->println("Saved failed.");
        }
    } else if (token1 == TOKEN_KEYWORD_CONFIGOPT && token2 == TOKEN_KEYWORD_LOAD) {
        bool OK = cm.loadOpt();
        if (OK) {
            _term->println("OK");
        } else {
            _term->println("Load failed.");
        }
    } else if (token1 == TOKEN_INTEGER && value == 3615) {
#ifdef MINITEL
        _term->print((char *)P_LOCAL_ECHO_OFF);
#endif
        webSocket.begin("3615co.de", 80, "/ws");
        webSocket.onEvent(webSocketEvent);
        _3611 = true;
        return;
    } else if (token1 == TOKEN_INTEGER && value == 3611) {
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
