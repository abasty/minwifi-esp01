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
#include <LittleFS.h>

#ifdef MINITEL
#include "tty-minitel.h"
#else
#include "tty-vt100.h"
#endif

#include "bio.h"
#include "os.h"

// 0:	BUTTON
// 12:	RELAY
// 13:	LED
// 14:	EXTRA GPIO

const int buttonPin = 0;
const int relayPin = 12;
const int ledPin = 13;

#define COMMAND_IP_PORT 23

// Minitel server TCP/IP connexion
WiFiClient tcpMinitelConnexion;
WebSocketsClient webSocket;
bool _3611 = false;
bool fkey = false;
bool minitelMode;
File bastos_file0;

uint8_t hal_get_key()
{
    if (!Serial)
        return 0;

    if (Serial.available() <= 0)
        return 0;

    uint8_t key = 0;
    size_t n = Serial.readBytes(&key, 1);
    return n > 0 ? key : 0;
}

int hal_print_float(float f)
{
    return Serial.printf("%g", f);
}

int hal_print_string(const char *s)
{
    return Serial.printf("%s", s);
}

int hal_print_integer(const char *format, int i)
{
    return Serial.printf(format, i);
}

int hal_open(const char *pathname, int flags)
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

int hal_close(int fd)
{
    bastos_file0.close();
    return 0;
}

int hal_write(int fd, const void *buf, int count)
{
    return bastos_file0.write((const uint8_t *)buf, count);
}

int hal_read(int fd, void *buf, int count)
{
    return bastos_file0.read((uint8_t *)buf, count);
}

void hal_cat()
{
    hal_print_string("\r\nDrive: A\r\n\r\n");
    Dir dir = LittleFS.openDir("/");
    while (dir.next())
    {
        uint8_t len = strlen(dir.fileName().c_str());
        hal_print_string(dir.fileName().c_str());
        for (int i = 16 - len; i > 0; i--)
            hal_print_string(" ");
        hal_print_integer("%u\r\n", dir.fileSize());
    }
    FSInfo info;
    LittleFS.info(info);
    hal_print_integer("\r\n%3uK free\r\n\r\nReady\r\n", (info.totalBytes - info.usedBytes) / 1024);
}

int hal_erase(const char *pathname)
{
    bool ret = LittleFS.remove(pathname);
    if (ret)
        return 0;
    return -1;
}

static inline void hal_reset()
{
#ifdef MINITEL
    hal_print_string(P_ACK_OFF_PRISE P_PRISE_1200);
    delay(250);
#endif
    ESP.restart();
}

void hal_speed(uint8_t fn)
{
    if (fn == TOKEN_KEYWORD_FAST || fn == TOKEN_KEYWORD_SLOW)
    {
#ifdef MINITEL
        hal_print_string(fn == TOKEN_KEYWORD_FAST ? P_PRISE_4800 : P_PRISE_1200);
        delay(500);
        Serial.end();
        Serial.begin(fn == TOKEN_KEYWORD_FAST ? 4800 : 1200, SERIAL_7E1);
#endif
        return;
    }
}

static void serial_flush()
{
    Serial.setTimeout(0);
    // Empty Serial buffer
    while (Serial && Serial.available() > 0)
    {
        uint8_t buffer[32];
        Serial.readBytes(buffer, 32);
    }
    Serial.flush();
}

static void setup_serial()
{
#ifdef MINITEL
    Serial.begin(1200, SERIAL_7E1);
    serial_flush();
    delay(1000);
    hal_print_string(COFF P_ACK_OFF_PRISE P_LOCAL_ECHO_OFF P_ROULEAU CLS);
#else
    Serial.begin(115200);
    serial_flush();
    delay(250);
#endif
}

#define config_prog                   \
    "1CLS\n"                          \
    "2PRINT\"* WiFi parameters *\"\n" \
    "4INPUT\"SSID: \",WSSID$\n"       \
    "5INPUT\"PASS: \",WSECRET$\n"     \
    "6SAVE\"config$$$\"\n"            \
    "7RESET\n"

static void setup_wifi()
{
    unsigned long startTime = millis();
    char *wssid = 0;
    char *wsecret = 0;
    var_t *var = 0;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    hal_print_string(CLS " Connecting");

    int err = bastos_load("config$$$");
    if (err != BERROR_NONE)
        goto config_new;

    var = bastos_var_get("WSSID$");
    if (!var)
        goto config_run;
    wssid = var->string;

    var = bastos_var_get("WSECRET$");
    if (!var)
        goto config_run;
    wsecret = var->string;

    WiFi.begin(wssid, wsecret);

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
        delay(500);
        hal_print_string(".");
    }

    hal_print_string("\r" CLEOL);

    if (WiFi.status() != WL_CONNECTED)
        goto config_run;

    // print_string(" WiFi connected with IP: ");
    // Serial.print(WiFi.localIP());
    // print_string("\r\n");

    if (LittleFS.exists("format$$$"))
    {
        LittleFS.end();
        LittleFS.format();
        LittleFS.begin();
        bastos_save("config$$$");
    }
    bastos_prog_new();
    bastos_send_keys("bastos\n", 7, false);
    return;

config_new:
    bastos_prog_new();
    bastos_send_keys(config_prog, strlen(config_prog), false);

config_run:
    hal_print_string("WiFi connection failed\r\n");
    bastos_send_keys("RUN\n", 4, false);
}

void setup()
{
    // Setup sonoff pins
    pinMode(relayPin, OUTPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(relayPin, HIGH); // On R2, light the red led (relay state)
    digitalWrite(ledPin, HIGH);   // On R2, light the blue led (red + blue => purple)

    setup_serial();

    // Setup file system
    LittleFS.begin();

    // Setup WiFi
    // setup_wifi();

    os_bootstrap();
}

void loop_connected()
{
    // Forward Minitel server incoming data to serial output
    if (tcpMinitelConnexion && tcpMinitelConnexion.available() > 0)
    {
        // transparently forward bytes to serial
        uint8_t buffer[128];
        size_t n = tcpMinitelConnexion.read(buffer, 128);
        // conversion stream Videotex vers ANSI
        Serial.write(buffer, n);
    }
    // Handle Ws client
    // if (webSocket.isConnected()) {
    if (_3611)
    {
        webSocket.loop();
    }

    // If disconnected
    // if (minitelMode && (!tcpMinitelConnexion || !tcpMinitelConnexion.connected())) {
    //     minitelMode = false;
    //     tcpMinitelConnexion.stop();
    //     init_minitel(true);
    // }

    // Minitel mode: Forward serial input to Minitel sever
    // tcpMinitelConnexion.setNoDelay(true); // Disable nagle's algo.
    // tcpMinitelConnexion.write((char *)&key, 1);
    // webSocket.sendTXT(key);
}

void loop()
{
    char key = os_get_key();
    bastos_send_keys((char *)&key, key != 0 ? 1 : 0, true);
    bastos_loop();
    if (bastos_is_reset())
    {
        bastos_done();
        hal_reset();
    }
}
