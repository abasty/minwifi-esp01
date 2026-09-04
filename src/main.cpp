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

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif


#include <FS.h>
#include <LittleFS.h>

#include "tty-minitel.h"
#include "bio.h"
#include "os.h"

#ifdef ESP32
const int button_pin = 9;
const int relay_pin = 4;
const int led_pin = 6;
#define BLUE_LED_ON do { digitalWrite(led_pin, HIGH); } while(0);
#define BLUE_LED_OFF do { digitalWrite(led_pin, LOW); } while(0);
#else
const int button_pin = 0;
const int relay_pin = 12;
const int led_pin = 13;
#define BLUE_LED_ON do { digitalWrite(led_pin, LOW); } while(0);
#define BLUE_LED_OFF do { digitalWrite(led_pin, HIGH); } while(0);
#endif

#define MAX_CONNECTIONS (4)

// Files and sockets
static File g_file0;
static WiFiClient g_tcp_socket[MAX_CONNECTIONS];
static bool g_used_sockets[MAX_CONNECTIONS] = {0};

// LittleFS has no notion of a process-wide current directory: unlike the PC
// simulator (which can just chdir()), CD/MD/RD here track a virtual current
// directory ourselves and resolve every relative filename against it.
#define CWD_SIZE (128)
#define FULL_PATH_SIZE (CWD_SIZE + FILE_NAME_SIZE + 2)
static char g_cwd[CWD_SIZE] = "/";

// Removes the last "/segment" from an absolute path in place, e.g.
// "/a/b" -> "/a", "/a" -> "/". path must start with '/' and already be "/"
// if there's nothing left to remove (never called on an empty root).
static void pop_last_segment(char *path)
{
    if (path[1] == 0) // already "/"
        return;
    char *slash = strrchr(path, '/');
    slash[slash == path ? 1 : 0] = 0;
}

// name is "../rest" only for MOVE's dest == ".." (its one caller that's
// allowed a literal ".."; every other name reaching here has already been
// rejected by os_valid_name() if it contained one). Resolved against a
// copy of g_cwd here, rather than left in the string for LittleFS/
// esp_littlefs to canonicalize on its own — not something to rely on
// without hardware to verify it against.
static void build_path(const char *name, char *out, size_t out_size)
{
    if (strncmp(name, "../", 3) == 0) {
        char parent[CWD_SIZE];
        strncpy(parent, g_cwd, sizeof(parent) - 1);
        parent[sizeof(parent) - 1] = 0;
        pop_last_segment(parent);
        name += 3;
        if (parent[1] == 0) // parent == "/"
            snprintf(out, out_size, "/%s", name);
        else
            snprintf(out, out_size, "%s/%s", parent, name);
        return;
    }

    if (g_cwd[1] == 0) // g_cwd == "/"
        snprintf(out, out_size, "/%s", name);
    else
        snprintf(out, out_size, "%s/%s", g_cwd, name);
}

void hal_print_oem_string(void)
{
    #ifdef ESP32
    hal_print_string("ESP32c");
    #else
    hal_print_string("ESP8285");
    #endif
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
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    pathname = rname;

    if (flags & B_CREAT)
    {
        access = "w+";
    }
    g_file0 = LittleFS.open(pathname, access);
    if (!g_file0) {
        return -1;
    }
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

int hal_get_file_size(const char* pathname)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    pathname = rname;

    int size = 0;
    File f = LittleFS.open(pathname, "r");
    if (f)
    {
        size = f.size();
        f.close();
    }
    return size;
}

int hal_file(const char* pathname, char *buffer, uint16_t offset, uint16_t size)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    pathname = rname;

    int r = 0;
    File f = LittleFS.open(pathname, "r");
    if (!f)
        return 0;

    if (f.seek(offset)) {
        r = f.read((uint8_t*) buffer, size);
    }

    f.close();
    return r;
}

#ifdef ESP32
size_t hal_cat()
{
    File dir = LittleFS.open(g_cwd);
    if(!dir || !dir.isDirectory())
        return 0;

    File file = dir.openNextFile();
    while(file){
        // Capture what's needed and close the handle before calling out:
        // os_cat_file()/os_cat_dir() may rename or delete this very entry
        // (MOVE, ERASE), which esp_littlefs refuses while it's still open.
        bool is_dir = file.isDirectory();
        String name = file.name();
        size_t size = file.size();
        file.close();

        if (is_dir) {
            os_cat_dir(name.c_str());
        } else {
            os_cat_file(name.c_str(), size);
        }
        file = dir.openNextFile();
    }
    return LittleFS.totalBytes() - LittleFS.usedBytes();
}
#else
size_t hal_cat()
{
    Dir dir = LittleFS.openDir(g_cwd);
    while (dir.next())
    {
        if (dir.isDirectory())
            os_cat_dir(dir.fileName().c_str());
        else
            os_cat_file(dir.fileName().c_str(), dir.fileSize());
    }
    FSInfo info;
    LittleFS.info(info);
    return info.totalBytes - info.usedBytes;
}
#endif

int hal_erase(const char *pathname)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    pathname = rname;

    bool ret = LittleFS.remove(pathname);
    if (ret)
        return 0;
    return -1;
}

int hal_mkdir(const char *pathname)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    return LittleFS.mkdir(rname) ? 0 : -1;
}

int hal_rmdir(const char *pathname)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    return LittleFS.rmdir(rname) ? 0 : -1;
}

// pathname is a full LittleFS path (already resolved against g_cwd).
static bool is_directory(const char *pathname)
{
    #ifdef ESP32
    File f = LittleFS.open(pathname);
    #else
    File f = LittleFS.open(pathname, "r");
    #endif
    if (!f)
        return false;
    bool result = f.isDirectory();
    f.close();
    return result;
}

int hal_is_dir(const char *pathname)
{
    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));
    return is_directory(rname) ? 1 : 0;
}

int hal_rename(const char *oldpath, const char *newpath)
{
    char roldname[FULL_PATH_SIZE];
    char rnewname[FULL_PATH_SIZE];
    build_path(oldpath, roldname, sizeof(roldname));
    build_path(newpath, rnewname, sizeof(rnewname));
    return LittleFS.rename(roldname, rnewname) ? 0 : -1;
}

int hal_chdir(const char *pathname)
{
    if (strcmp(pathname, "..") == 0) {
        if (g_cwd[1] == 0) // already at root: same no-op as a real chdir("..")
            return 0;
        pop_last_segment(g_cwd);
        return 0;
    }

    char rname[FULL_PATH_SIZE];
    build_path(pathname, rname, sizeof(rname));

    if (!is_directory(rname))
        return -1;

    strncpy(g_cwd, rname, sizeof(g_cwd) - 1);
    g_cwd[sizeof(g_cwd) - 1] = 0;
    return 0;
}

int hal_at_root(void)
{
    return g_cwd[1] == 0;
}

void hal_reset()
{
    BLUE_LED_OFF;
    hal_print_string(P_ACK_OFF_PRISE P_PRISE_1200);
    delay(250);
    ESP.restart();
}

void hal_speed(uint8_t fn)
{
    const char *p_speed = fn == TOKEN_KEYWORD_FAST2  ? P_PRISE_9600
                          : fn == TOKEN_KEYWORD_FAST ? P_PRISE_4800
                                                     : P_PRISE_1200;
    unsigned long speed = fn == TOKEN_KEYWORD_FAST2  ? 9600
                          : fn == TOKEN_KEYWORD_FAST ? 4800
                                                     : 1200;
    hal_print_string(p_speed);
    delay(250); // Be sure chars are sent to Minitel before changing speed
    Serial.updateBaudRate(speed);
}

int hal_wifi_scan()
{
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        int32_t RSSI = WiFi.RSSI(i);
        uint8_t encryptionType = WiFi.encryptionType(i);
        os_wifi_print_network(i + 1, ssid.c_str(), encryptionType, RSSI);
    }
    WiFi.scanDelete();

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

int hal_net_connect(split_t *urn)
{
    int fd = -1;
    for (int n = 0; n < MAX_CONNECTIONS; n++) {
        if (!g_used_sockets[n]) {
            fd = n;
            break;
        }
    }

    if (fd < 0)
        return fd;

    g_tcp_socket[fd].connect(urn->parts[URN_PART_HOST], urn->port);
    g_used_sockets[fd] = true;
    if (!g_tcp_socket[fd].connected())
    {
        hal_net_disconnect(0, fd);
        return -1;
    }

    // Disable nagle's algo
    g_tcp_socket[fd].setNoDelay(true);

    return fd;
}

void hal_net_disconnect(uint8_t set, int fd)
{
    if (fd < 0 || fd >= MAX_CONNECTIONS)
        return;
    g_tcp_socket[fd].stop();
    g_used_sockets[fd] = false;
}

int hal_net_send(int fd, const uint8_t *buffer, int n)
{
    return g_tcp_socket[fd].write(buffer, n);
}

int hal_net_recv(int fd, uint8_t *buffer, int n)
{
    int available = g_tcp_socket[fd].available();
    if (available == 0)
        return 0;

    if (n > available)
        n = available;

    return g_tcp_socket[fd].read(buffer, n);
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

    return function;
}

// static void serial_flush_rx()
// {
//     Serial.setTimeout(0);
//     // Empty Serial buffer
//     while (Serial && Serial.available() > 0)
//     {
//         uint8_t buffer[32];
//         Serial.readBytes(buffer, 32);
//         delay(10);
//     }
//     Serial.flush();
// }

static bool wait_serial(unsigned long speed, String wait_for)
{
    BLUE_LED_ON;
    Serial.begin(speed, SERIAL_7E1);
    Serial.setTimeout(0);
    // serial_flush_rx();
    delay(500);
    BLUE_LED_OFF;
    hal_print_string("\e9t");
    delay(500);
    String reply = Serial.readString();
    if (reply == wait_for)
        return true;
    Serial.end();
    delay(500);
    return false;
}

static void setup_serial()
{
    Serial.end();
    delay(500);
    while (true)
    {
        if (wait_serial(9600, "\x1b\x3au\x7f"))
            break;
        if (wait_serial(4800, "\x1b\x3auv"))
            break;
        if (wait_serial(1200, "\x1b\x3aud"))
            break;
    }
}

void setup()
{
    // Setup sonoff pins
    pinMode(relay_pin, OUTPUT);
    pinMode(led_pin, OUTPUT);
    pinMode(button_pin, INPUT_PULLUP);

    setup_serial();

    // Setup file system
    #ifdef ESP32
    LittleFS.begin(true);
    #else
    LittleFS.begin();
    #endif

    // Setup WiFi
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    // Bootstrap the OS
    os_setup();
    BLUE_LED_ON;
}

void loop()
{
    os_loop();
}
