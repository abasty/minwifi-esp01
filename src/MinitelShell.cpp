#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <strings.h>

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
    Serial.printf("[WSc] Disconnected!\n");
    // Do something when disconnected
    break;
  case WStype_CONNECTED: {
    Serial.printf("[WSc] Connected to url: %s\n", payload);

    // send message to server when Connected
    // webSocket.sendTXT("Connected");
  }
  break;
  case WStype_TEXT:
    if (length > 0 && payload) {
      Serial.printf("%s", payload);
    }
    // Serial.printf("[WSc] get text: %u\n", length);

    // send message to server
    // webSocket.sendTXT("message here");
    break;
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    // hexdump(payload, length);

    // send data to server
    // webSocket.sendBIN(payload, length);
    break;
  case WStype_PING:
    // pong will be send automatically
    // Serial.printf("[WSc] get ping\n");
    break;
  case WStype_PONG:
    // answer to a ping we send
    // Serial.printf("[WSc] get pong\n");
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
    if (_term) {
      _term->println("Please connect first.");
    }
  }
}

void MinitelShell::runCommand()
{
  // if (_term)
  //     _term->println();

  if (strcasecmp(_command, "free") == 0) {
    if (_term) {
      _term->printf("Heap:             %u bytes free.\r\n", ESP.getFreeHeap());
      _term->printf("Flash Real Size:  %u bytes.\r\n", ESP.getFlashChipRealSize());
      _term->printf("Sketch Size:      %u bytes.\r\n", ESP.getSketchSize());
      _term->printf("Free Sketch Size: %u bytes.\r\n", ESP.getFreeSketchSpace());
    }
  } else if (strcasecmp(_command, "cats") == 0) {
    if (_term) {
      _term->println("Hello from Cat-Labs");
      _term->println("OK");
    }
  } else if (strcasecmp(_command, "reset") == 0) {
    ESP.restart();
  } else if (strcasecmp(_command, "config") == 0) {
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
  } else if (strcasecmp(_command, "connect") == 0) {
    cm.connect();
  } else if (strcasecmp(_command, "config save") == 0) {
    bool OK = cm.save();
    if (_term) {
      if (OK) {
        _term->println("OK");
      } else {
        _term->println("Saved failed.");
      }
    }
  } else if (strcasecmp(_command, "config load") == 0) {
    bool OK = cm.load();
    if (_term) {
      if (OK) {
        _term->println("OK");
      } else {
        _term->println("Load failed.");
      }
    }
  } else if (strcasecmp(_command, "clear") == 0) {
    if (_term) {
      _term->println("\x0CReady.");
    }
  } else if (strcasecmp(_command, "3615") == 0) {
    connectServer();
  } else if (strcasecmp(_command, "3611") == 0) {
    webSocket.begin("3611.re", 80, "/ws");
    webSocket.onEvent(webSocketEvent);
    _3611 = true;
    _term->println("OK");
  } else if (strcasecmp(_command, "configopt") == 0) {
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
  } else if (strcasecmp(_command, "configopt save") == 0) {
    bool OK = cm.saveOpt();
    if (_term) {
      if (OK) {
        _term->println("OK");
      } else {
        _term->println("Saved failed.");
      }
    }
  } else if (strcasecmp(_command, "configopt load") == 0) {
    bool OK = cm.loadOpt();
    if (_term) {
      if (OK) {
        _term->println("OK");
      } else {
        _term->println("Load failed.");
      }
    }
  } else {
    if (_term) {
      _term->println("ERROR");
      // for (int i = 0; i < strlen(_command); i++)
      // {
      //     _term->printf("%02X", _command[i]);
      // }
      // _term->println();
      // _term->println(_command);
    }
  }
}
