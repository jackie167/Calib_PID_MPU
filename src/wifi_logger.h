#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifi_config.h"

namespace wifi_logger {

static WiFiUDP _udp;
static bool _connected = false;

inline void init() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting to ");
  Serial.print(WIFI_SSID);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    _connected = true;
    Serial.print("\n[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Sending UDP to ");
    Serial.print(UDP_TARGET_IP);
    Serial.print(":");
    Serial.println(UDP_PORT);
  } else {
    Serial.println("\n[WiFi] FAILED — Serial only mode");
  }
}

inline bool isConnected() { return _connected; }

// Send a line via UDP (and always mirror to Serial)
inline void println(const String &s) {
  Serial.println(s);
  if (!_connected) return;
  _udp.beginPacket(UDP_TARGET_IP, UDP_PORT);
  _udp.print(s);
  _udp.print("\n");
  _udp.endPacket();
}

inline void println(const char *s) {
  Serial.println(s);
  if (!_connected) return;
  _udp.beginPacket(UDP_TARGET_IP, UDP_PORT);
  _udp.print(s);
  _udp.print("\n");
  _udp.endPacket();
}

}  // namespace wifi_logger
