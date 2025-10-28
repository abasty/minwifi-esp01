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

const int button_pin = 0;
const int relay_pin = 12;
const int led_pin = 13;

// Files and sockets
static File g_file0;
static int g_net_proto = -1;
static WiFiClient g_tcp_socket;
static WebSocketClient *g_web_socket = 0;
static int g_web_socket_unread_bytes = 0;
static FTPClient g_ftp_client;
static bool g_ftp_connected = false;

static void serial_flush();

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

ssize_t hal_ftp_cat()
{
    if (!hal_ftp_is_connected())
        return -1;

    ssize_t n = g_ftp_client.list_directory();
    return n;
}

bool hal_ftp_files(uint8_t func, const char *filename)
{
    if (!hal_ftp_is_connected())
        return false;

    if (func == TOKEN_KEYWORD_PUT)
        return g_ftp_client.write_file(filename, filename);

    if (func == TOKEN_KEYWORD_GET)
        return g_ftp_client.read_file(filename, filename);

    return false;
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
    digitalWrite(led_pin, HIGH);
    hal_print_string(P_ACK_OFF_PRISE P_PRISE_1200);
    delay(250);
    ESP.restart();
}

void hal_speed(uint8_t fn)
{
    if (fn == TOKEN_KEYWORD_FAST || fn == TOKEN_KEYWORD_SLOW)
    {
        hal_print_string(fn == TOKEN_KEYWORD_FAST ? P_PRISE_4800 : P_PRISE_1200);
        delay(250);
        Serial.end();
        Serial.begin(fn == TOKEN_KEYWORD_FAST ? 4800 : 1200, SERIAL_7E1);
        delay(250);
        serial_flush();
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

bool hal_ftp_is_connected()
{
    if (!g_ftp_connected)
        return false;

    if (!g_ftp_client.connected())
    {
        g_ftp_client.close();
        g_ftp_connected = false;
    }

    return g_ftp_connected;
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
        if (hal_ftp_is_connected())
            return -1;

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

void hal_net_disconnect(uint8_t set, int n)
{
    // If connected, disconnect and remove associated resources
    if (set == DB_MIN_SET)
    {
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
    else if (set == DB_FTP_SET)
    {
        if (g_ftp_connected)
        {
            g_ftp_client.close();
            g_ftp_connected = false;
        }
    }
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

uint64_t hal_get_ms(void)
{
    return millis();
}

int hal_get_function_key(void)
{
    static unsigned long t_tap = 0;
    static unsigned long t_untap = 0;
    static int last_state = 0;

    int function = 0;
    int current_state = 1 - digitalRead(button_pin);
    if (current_state != last_state) {
        if (current_state == 0) {
            t_untap = millis();
            if (t_untap - t_tap < 100) {
                // Avoid bouncing
                function = 0;
            } else if (t_untap - t_tap < 2000) {
                function = 1;
            } else {
                function = 2;
            }
        } else {
            t_tap = millis();
        }
        last_state = current_state;
    }

    if (function != 0) {
        digitalWrite(led_pin, HIGH);
        delay(200);
    }
    return function;
}

static void serial_flush()
{
    Serial.setTimeout(0);
    // Empty Serial buffer
    while (Serial && Serial.available() > 0)
    {
        uint8_t buffer[32];
        Serial.readBytes(buffer, 32);
        delay(10);
    }
    Serial.flush();
}

static bool wait_serial(unsigned long speed, String wait_for)
{
    Serial.begin(speed, SERIAL_7E1);
    serial_flush();
    hal_print_string("\e9t");
    delay(500);
    // Serial.setTimeout(500);
    String reply = Serial.readString();
    if (reply == wait_for)
        return true;
    Serial.end();
    return false;
}

static void setup_serial()
{
    while (true)
    {
        // TODO: Add M2 9600

        if (wait_serial(4800, "\x1b\x3auv"))
            break;
        if (wait_serial(1200, "\x1b\x3aud"))
            break;
    }

    Serial.setTimeout(0);
}

void setup()
{
    // Setup sonoff pins
    pinMode(relay_pin, OUTPUT);
    pinMode(led_pin, OUTPUT);
    pinMode(button_pin, INPUT_PULLUP);
    digitalWrite(relay_pin, HIGH); // On R2, light the red led (relay state)
    digitalWrite(led_pin, LOW);   // On R2, light the blue led (red + blue => purple)

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
