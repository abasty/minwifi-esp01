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
#include <ArduinoOTA.h>
#include <WebSocketsClient.h>
#include <LittleFS.h>

#include "minitel.h"

#include "berror.h"
#include "bmemory.h"
#include "bio.h"

// 0:	BUTTON
// 12:	RELAY
// 13:	LED
// 14:	EXTRA GPIO

int relayPin = 12;
int ledPin = 13;

#define COMMAND_IP_PORT 23

#ifndef MINITEL
WiFiClient *wifiClient = 0;
// wifi server
WiFiServer *wifiServer = 0;
#endif

// Minitel server TCP/IP connexion
WiFiClient tcpMinitelConnexion;
WebSocketsClient webSocket;
bool _3611 = false;
bool fkey = false;
bool minitelMode;


static int print_float(float f)
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->printf("%g", f);
#endif
    return Serial.printf("%g", f);
}

static int print_string(const char *s)
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->printf("%s", s);
#endif
    return Serial.printf("%s", s);
}

extern "C" int print_integer(const char *format, int i)
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->printf(format, i);
#endif
    return Serial.printf(format, i);
}

static inline void cls()
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->print("\033[2J" "\033[H");
#endif
#ifdef MINITEL
    Serial.print("\x0C");
#else
    Serial.print("\033[2J" "\033[H");
#endif
}

static inline void del()
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->print("\x08 \x08");
#endif
    Serial.print("\x08 \x08");
}

File bastos_file0;

extern "C" int bopen(const char *pathname, int flags)
{
    const char *access = "r";
    if (flags & B_CREAT)
    {
        access = "w+";
    }
    bastos_file0 = LittleFS.open(pathname, access);
    if (!bastos_file0)
        return -1;
    return 0;
}

extern "C" int bclose(int fd)
{
    bastos_file0.close();
    return 0;
}

extern "C" int bwrite(int fd, const void *buf, int count)
{
    return bastos_file0.write((const uint8_t *) buf, count);
}

extern "C" int bread(int fd, void *buf, int count)
{
    return bastos_file0.read((uint8_t *) buf, count);
}

static inline void bcat()
{
    Dir dir = LittleFS.openDir("/");
    while (dir.next())
    {
        uint8_t len = strlen(dir.fileName().c_str());
        print_string(dir.fileName().c_str());
        for (int i = 16 - len; i > 0; i--)
            print_string(" ");
        print_integer("%u\r\n", dir.fileSize());
    }
    FSInfo info;
    LittleFS.info(info);
    print_integer("Bytes: %u/", info.usedBytes);
    print_integer("%u\r\n", info.totalBytes);
}

extern "C" int berase(const char *pathname)
{
    bool ret = LittleFS.remove(pathname);
    if (ret)
        return 0;
    return -1;
}

static inline void breset()
{
    ESP.restart();
}

static void GotoXY(uint8_t c, uint8_t l)
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->printf("\x1B" "[%d;%dH", l, c);
#endif
#ifdef MINITEL
    Serial.printf("\x1F%c%c", l + 0x40, c + 0x40);
#else
    Serial.printf("\x1B" "[%d;%dH", l, c);
#endif
}

static void color(uint8_t color, uint8_t foreground)
{
#ifndef MINITEL
    if (wifiClient)
        wifiClient->printf("\033" "[%dm", (foreground ? 30 : 40) + color);
#endif
#ifdef MINITEL
    Serial.printf("\x1B" "%c", color + (foreground ? 0x40 : 0x50));
#else
    Serial.printf("\033" "[%dm", (foreground ? 30 : 40) + color);
#endif
}

bastos_io_t io = {
    .print_integer = print_integer,
    .bopen = bopen,

    .bclose = bclose,

    .bwrite = bwrite,
    .bread = bread,
};

int biocop(void)
{
    switch (bastos_io_argv[0].as_int)
    {
    case B_IO_CAT:
        bcat();
        return 0;

    case B_IO_CLS:
        cls();
        return 0;

    case B_IO_DEL:
        del();
        return 0;

    case B_IO_RESET:
        //breset();
        return 0;

    case B_IO_AT:
        GotoXY(bastos_io_argv[2].as_int, bastos_io_argv[1].as_int);
        return 0;

    case B_IO_INK:
        color(bastos_io_argv[1].as_int, 1);
        return 0;

    case B_IO_PAPER:
        color(bastos_io_argv[1].as_int, 0);
        return 0;

    case B_IO_PRINT_STRING:
        return print_string(bastos_io_argv[1].as_string);

    case B_IO_PRINT_FLOAT:
        return print_float(bastos_io_argv[1].as_float);
    }
    return 0;
}

static void initMinitel(bool clear)
{
    // Empty Serial buffer
    while (Serial && Serial.available() > 0) {
        uint8_t buffer[32];
        Serial.readBytes(buffer, 32);
    }
    Serial.flush();
    if (clear) {
        cls();
    }
#ifdef MINITEL
    Serial.print((char *)P_ACK_OFF_PRISE);
    Serial.print((char *)P_LOCAL_ECHO_OFF);
    Serial.print((char *)P_ROULEAU);
    Serial.print((char *)CON);
#endif
}

#define config_prog \
    "1CLS\n" \
    "2PRINT\"* WiFi parameters *\"\n" \
    "4INPUT\"SSID: \",WSSID$\n" \
    "5INPUT\"PASS: \",WSECRET$\n" \
    "6SAVE\"config$$$\"\n"

static void setup_wifi()
{
    unsigned long startTime = millis();
    char *wssid = 0;
    char *wsecret = 0;
    var_t *var = 0;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    int err = bastos_load("config$$$");
    if (err != BERROR_NONE)
        goto finalize;

    var = bmem_var_find("\021WSSID");
    if (!var)
        goto finalize;
    wssid = var->string;

    var = bmem_var_find("\021WSECRET");
    if (!var)
        goto finalize;
    wsecret = var->string;

    WiFi.begin(wssid, wsecret);

    print_string("Connecting");
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        print_string(".");
    }
    print_string("\r\n");
    if (WiFi.status() != WL_CONNECTED)
        goto finalize;

    print_string("WiFi connected with IP: ");
    Serial.print(WiFi.localIP());
#if COMMAND_IP_PORT != 23
    Serial.printf(" %u", COMMAND_IP_PORT);
#endif
    print_string("\r\n");

    if (LittleFS.exists("format$$$"))
    {
        LittleFS.end();
        LittleFS.format();
        LittleFS.begin();
        bastos_save("config$$$");
    }
    bmem_prog_new();
    return;

finalize:
    Serial.println("Connection failed.");
    bmem_prog_new();
    bastos_send_keys(config_prog, strlen(config_prog));
    bastos_send_keys("RUN\n", 4);
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
    cls();
    Serial.println("Loading and connecting.");

    // Initialize file system
    LittleFS.begin();
    bastos_init(&io, biocop);

    // connect Wifi if configuration available
    setup_wifi();

    // Initialize OTA
    ArduinoOTA.setPort(8266);

#ifdef MINITEL
    // Hostname
    ArduinoOTA.setHostname("esp-minitel");
#else
    ArduinoOTA.setHostname("esp-minitel-dev");
#endif

    ArduinoOTA.begin();

#ifndef MINITEL
    // Launch traces server
    wifiServer = new WiFiServer(COMMAND_IP_PORT);
    wifiServer->begin();
#endif

    Serial.setTimeout(0);
    initMinitel(false);
}

void loop()
{
    ArduinoOTA.handle();

    digitalWrite(ledPin, HIGH);

#ifndef MINITEL
    // Accept TCP shell connections
    if (wifiServer->hasClient()) {
        // Delete active connection if any and accept new one
        // TODO: It leaks when a new session is accepted and one is active, do not know why....
        if (wifiClient) {
            delete wifiClient;
        }
        wifiClient = new WiFiClient(wifiServer->available());
    }

    // Handle commands from WiFi Client
    if (wifiClient && wifiClient->available() > 0) {
        // read client data
        char buffer[32];
        size_t n = wifiClient->read(buffer, 32);
        // wifiShell->handle((char *)buffer, n);
        bastos_send_keys(buffer, n);
    }
#endif

    // Forward Minitel server incoming data to serial output
    if (tcpMinitelConnexion && tcpMinitelConnexion.available() > 0) {
        // transparently forward bytes to serial
        uint8_t buffer[128];
        size_t n = tcpMinitelConnexion.read(buffer, 128);
        // conversion stream Videotex vers ANSI
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
        uint8_t key;
        size_t n = Serial.readBytes(&key, 1);
        if (n > 0) {
            if (!_3611) {
#ifdef MINITEL
                if (key == 0x13) {
                    fkey = true;
                }
                else {
                    if (fkey) {
                        if (key == 'G') { // CORRECTION
                            key = 0x7F;
                        } else { // ENVOI
                            key = '\r';
                        }
                        fkey = false;
                    }
                    bastos_send_keys((char *)&key, 1);
                }
#else
                bastos_send_keys((char *)&key, 1);
#endif
            } else {
                // Minitel mode: Forward serial input to Minitel sever
                // tcpMinitelConnexion.setNoDelay(true); // Disable nagle's algo.
                // tcpMinitelConnexion.write((char *)&key, 1);
                webSocket.sendTXT(key);
            }
        }
    }
    bastos_loop();
}
