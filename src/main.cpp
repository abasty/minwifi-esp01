/*
 * Copyright © 2023-2025 Alain Basty
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
#include <LittleFS.h>
#include <ArduinoHttpClient.h>
#include <FTPClient.h>

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

// Files and sockets
static File g_file0;
static int g_net_proto = -1;
static WiFiClient g_tcp_socket;
static WebSocketClient *g_web_socket = 0;
static int g_web_socket_unread_bytes = 0;
static FTPClient g_ftp_client;
static bool g_ftp_connected = false;

void hal_print_oem_string(void)
{
    hal_print_string("ESP8285");
    hal_print_integer(", OS free RAM: %u", ESP.getFreeHeap());
}

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

int hal_print_buffer(uint8_t *buffer, int n)
{
    return Serial.write(buffer, n);
}

int hal_open(const char *pathname, int flags)
{
    const char *access = "r";
    if (flags & B_CREAT)
    {
        access = "w+";
    }
    g_file0 = LittleFS.open(pathname, access);
    if (!g_file0)
        return -1;
    return 0;
}

int hal_close(int fd)
{
    g_file0.close();
    return 0;
}

int hal_write(int fd, const void *buf, int count)
{
    return g_file0.write((const uint8_t *)buf, count);
}

int hal_read(int fd, void *buf, int count)
{
    return g_file0.read((uint8_t *)buf, count);
}

size_t hal_cat()
{
    Dir dir = LittleFS.openDir("/");
    while (dir.next())
    {
        os_cat_file(dir.fileName().c_str(), dir.fileSize());
    }
    FSInfo info;
    LittleFS.info(info);
    return info.totalBytes - info.usedBytes;
}

int hal_erase(const char *pathname)
{
    bool ret = LittleFS.remove(pathname);
    if (ret)
        return 0;
    return -1;
}

void hal_reset()
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

int hal_wifi_scan()
{
    int n = WiFi.scanNetworks(false, true);

    String ssid;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t *BSSID;
    int32_t channel;
    bool isHidden;

    for (int i = 0; i < n; i++)
    {
        WiFi.getNetworkInfo(i, ssid, encryptionType, RSSI, BSSID, channel, isHidden);
        os_wifi_print_network(i + 1, ssid.c_str(), encryptionType, RSSI);
    }
    return n;
}

int hal_wifi_connect(const char* ssid, const char* secret)
{
    if (WiFi.isConnected())
    {
        WiFi.disconnect();
        delay(1000);
    }
    WiFi.begin(ssid, secret);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
        delay(500);
    }
    return WiFi.status() == WL_CONNECTED ? 0 : -1;
}

void hal_wifi_disconnect()
{
    if (WiFi.isConnected())
    {
        WiFi.disconnect();
    }
}

bool hal_wifi_is_connected()
{
    bool connected = WiFi.isConnected();
    if (connected)
    {
        os_wifi_set_info(WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    }
    else
    {
        os_wifi_set_info("", "");
    }
    return connected;
}

static void web_socket_terminate()
{
    g_web_socket->flush();
    g_web_socket->stop();
    delete g_web_socket;
    g_web_socket = 0;
    g_web_socket_unread_bytes = 0;
}

int hal_net_connect(split_t *urn)
{
    if (urn->proto == URN_PROTO_TCP)
    {
        g_net_proto = urn->proto;
        g_tcp_socket.connect(urn->parts[URN_PART_HOST], urn->port);
        if (!g_tcp_socket.connected())
        {
            g_tcp_socket.stop();
            return -1;
        }

        // Disable nagle's algo
        g_tcp_socket.setDefaultNoDelay(true);
        return 0;
    }

    if (urn->proto == URN_PROTO_WS || urn->proto == URN_PROTO_WSS)
    {
        g_net_proto = urn->proto;
        g_web_socket = new WebSocketClient(g_tcp_socket, urn->parts[URN_PART_HOST], urn->port);
        g_web_socket->begin(urn->parts[URN_PART_PATH]);
        if (!g_web_socket->connected())
        {
            web_socket_terminate();
            return -1;
        }
        return 0;
    }

    if (urn->proto == URN_PROTO_FTP)
    {
        uint16_t port = urn->port ? urn->port : 21;
        const char *login = *urn->parts[URN_PART_LOGIN] ? urn->parts[URN_PART_LOGIN] : "anonymous";
        const char *pass = *urn->parts[URN_PART_PASS] ? urn->parts[URN_PART_PASS] : "pat@frites.be";
        g_ftp_connected = g_ftp_client.open(urn->parts[URN_PART_HOST], port, login, pass);
        if (!g_ftp_connected)
            return -1;

        if (*urn->parts[URN_PART_PATH])
            g_ftp_client.change_directory(urn->parts[URN_PART_PATH]);

        return 0;
    }
    return -1;
}

void hal_net_disconnect(int n)
{
    // If connected, disconnect and remove associated resources
    if (g_net_proto == URN_PROTO_TCP)
    {
        if (g_tcp_socket.connected())
        {
            g_tcp_socket.stop();
        }
    }

    if (g_net_proto == URN_PROTO_WS || g_net_proto == URN_PROTO_WSS)
    {
        if (g_web_socket && g_web_socket->connected())
        {
            web_socket_terminate();
        }
    }

    g_net_proto = -1;
}

int hal_net_send(int fd, const uint8_t *buffer, int n)
{
    if (g_net_proto == URN_PROTO_TCP)
    {
        return g_tcp_socket.write(buffer, n);
    }
    if (g_net_proto == URN_PROTO_WS || g_net_proto == URN_PROTO_WSS)
    {
        if (!g_web_socket || !g_web_socket->connected())
            return -1;

        g_web_socket->beginMessage(TYPE_TEXT);
        int written = g_web_socket->write(buffer, n);
        g_web_socket->endMessage();
        return written;
    }
    return -1;
}

int hal_net_recv(int fd, uint8_t *buffer, int n)
{
    // TODO: Try to virtualize the socket
    if (g_net_proto == URN_PROTO_TCP)
    {
        int available = g_tcp_socket.available();
        if (available == 0)
            return 0;

        if (n > available)
            n = available;

        return g_tcp_socket.read(buffer, n);
    }
    if (g_net_proto == URN_PROTO_WS || g_net_proto == URN_PROTO_WSS)
    {
        if (!g_web_socket || !g_web_socket->connected())
            return -1;
        if (g_web_socket_unread_bytes == 0)
        {
            g_web_socket_unread_bytes = g_web_socket->parseMessage();
            if (g_web_socket_unread_bytes == 0)
                return 0;
        }
        if (n > g_web_socket_unread_bytes)
        {
            n = g_web_socket_unread_bytes;
        }
        g_web_socket_unread_bytes -= n;
        return g_web_socket->read(buffer, n);
    }
    return -1;
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
#else
    Serial.begin(115200);
    serial_flush();
    delay(250);
#endif
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
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    // Bootstrap the OS
    os_setup();
}

void loop()
{
    os_loop();
}
