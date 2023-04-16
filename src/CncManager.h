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


#ifndef CncManager_h
#define CncManager_h

#include <IPAddress.h>
#include "Shell.h"

class ConnectionManager
{
public:
    ConnectionManager(Shell *Shell);

    // Connection parameters get/set
    void setSSID(const char* ssid);
    const char* getSSID();
    // Password is Write Only
    void setPassword(const char* pass);
    bool save();
    bool load();

    // Network connection
    bool connect();
    void disconnect();
    bool isConnected();

private:
    // Connection parameters
    String _ssid;
    String _secret;
    // Shell for UI (print, input)
    Shell *_shell;

    // Network wait loop w/ UI on _shell
    void _waitForWiFi(const char* message);
};

#endif // CncManager_h
