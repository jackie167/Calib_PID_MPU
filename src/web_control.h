#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "motor_1_config.h"
#include "motor_2_config.h"
#include "motor_driver.h"

namespace web_control {

constexpr char AP_SSID[] = "ESP-Car";
constexpr char AP_PASSWORD[] = "12345678";

constexpr int BASE_LEFT = 128;
constexpr int BASE_RIGHT = 121;
constexpr int DRIVE_BOOST = 10;

constexpr int FORWARD_LEFT_PWM = BASE_LEFT + DRIVE_BOOST;
constexpr int FORWARD_RIGHT_PWM = BASE_RIGHT + DRIVE_BOOST;

constexpr int TURN_INNER_DROP = 60;

enum MotionCommand {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
};

static WebServer server(80);
static MotionCommand current_command = CMD_STOP;

inline const char *commandName(MotionCommand cmd) {
  switch (cmd) {
    case CMD_FORWARD: return "FORWARD";
    case CMD_BACKWARD: return "BACKWARD";
    case CMD_LEFT: return "LEFT";
    case CMD_RIGHT: return "RIGHT";
    case CMD_STOP:
    default: return "STOP";
  }
}

inline void applyMotion(MotionCommand cmd) {
  switch (cmd) {
    case CMD_FORWARD:
      setMotorPwm(motor1_config::CONFIG, FORWARD_LEFT_PWM);
      setMotorPwm(motor2_config::CONFIG, FORWARD_RIGHT_PWM);
      break;
    case CMD_BACKWARD:
      setMotorPwm(motor1_config::CONFIG, -FORWARD_LEFT_PWM);
      setMotorPwm(motor2_config::CONFIG, -FORWARD_RIGHT_PWM);
      break;
    case CMD_LEFT:
      setMotorPwm(motor1_config::CONFIG, FORWARD_LEFT_PWM - TURN_INNER_DROP);
      setMotorPwm(motor2_config::CONFIG, FORWARD_RIGHT_PWM);
      break;
    case CMD_RIGHT:
      setMotorPwm(motor1_config::CONFIG, FORWARD_LEFT_PWM);
      setMotorPwm(motor2_config::CONFIG, FORWARD_RIGHT_PWM - TURN_INNER_DROP);
      break;
    case CMD_STOP:
    default:
      stopAllMotors();
      break;
  }
}

inline void setCommand(MotionCommand cmd) {
  current_command = cmd;
  applyMotion(current_command);
  Serial.print("[WEB] command = ");
  Serial.println(commandName(current_command));
}

inline String htmlPage() {
  String html;
  html += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP Car</title><style>";
  html += "body{font-family:sans-serif;background:#f4f1ea;color:#1e1f1c;text-align:center;padding:24px}";
  html += ".wrap{max-width:360px;margin:0 auto}.row{display:flex;gap:12px;justify-content:center;margin:12px 0}";
  html += "button{width:110px;height:64px;font-size:18px;border:none;border-radius:14px;background:#283618;color:#fff}";
  html += ".stop{background:#bc4749}.status{margin:18px 0;padding:12px;border-radius:12px;background:#fff}";
  html += "</style></head><body><div class='wrap'><h2>ESP Car</h2>";
  html += "<div class='status'>Current: <strong id='state'>";
  html += commandName(current_command);
  html += "</strong><br>Base L/R: ";
  html += String(BASE_LEFT);
  html += " / ";
  html += String(BASE_RIGHT);
  html += "<br>Drive L/R: ";
  html += String(FORWARD_LEFT_PWM);
  html += " / ";
  html += String(FORWARD_RIGHT_PWM);
  html += "</div>";
  html += "<div class='row'><button onclick=\"sendCmd('forward')\">Forward</button></div>";
  html += "<div class='row'><button onclick=\"sendCmd('left')\">Left</button><button class='stop' onclick=\"sendCmd('stop')\">Stop</button><button onclick=\"sendCmd('right')\">Right</button></div>";
  html += "<div class='row'><button onclick=\"sendCmd('backward')\">Backward</button></div>";
  html += "<script>";
  html += "async function sendCmd(cmd){await fetch('/'+cmd,{method:'POST'});document.getElementById('state').textContent=cmd.toUpperCase();}";
  html += "</script></div></body></html>";
  return html;
}

inline void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

inline void handleState() {
  server.send(200, "text/plain", commandName(current_command));
}

inline void handleForward() { setCommand(CMD_FORWARD); server.send(200, "text/plain", "OK"); }
inline void handleBackward() { setCommand(CMD_BACKWARD); server.send(200, "text/plain", "OK"); }
inline void handleLeft() { setCommand(CMD_LEFT); server.send(200, "text/plain", "OK"); }
inline void handleRight() { setCommand(CMD_RIGHT); server.send(200, "text/plain", "OK"); }
inline void handleStop() { setCommand(CMD_STOP); server.send(200, "text/plain", "OK"); }

inline void init() {
  initMotorDriver(motor1_config::CONFIG);
  initMotorDriver(motor2_config::CONFIG);
  stopAllMotors();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/state", HTTP_GET, handleState);
  server.on("/forward", HTTP_POST, handleForward);
  server.on("/backward", HTTP_POST, handleBackward);
  server.on("/left", HTTP_POST, handleLeft);
  server.on("/right", HTTP_POST, handleRight);
  server.on("/stop", HTTP_POST, handleStop);
  server.begin();

  Serial.println("[WEB] AP mode ready");
  Serial.print("[WEB] SSID: ");
  Serial.println(AP_SSID);
  Serial.print("[WEB] Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("[WEB] Open: http://");
  Serial.println(WiFi.softAPIP());
}

inline void loop() {
  server.handleClient();
  applyMotion(current_command);
}

}  // namespace web_control
