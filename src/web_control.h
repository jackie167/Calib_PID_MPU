#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "motor_1_config.h"
#include "motor_2_config.h"
#include "motor_driver.h"

extern volatile long motor1_enc_count;
extern volatile long motor2_enc_count;
extern volatile unsigned long motor1_last_isr_us;
extern volatile unsigned long motor2_last_isr_us;

namespace web_control {

constexpr char AP_SSID[] = "ESP-Car";
constexpr char AP_PASSWORD[] = "12345678";

constexpr int BASE_LEFT = 150;
constexpr int BASE_RIGHT = 150;
constexpr int SPEED_STEP = 10;
constexpr float PPR_OUTPUT = 900.0f;
constexpr unsigned long PID_INTERVAL_MS = 100;
constexpr float PID_KP = 0.30f;
constexpr float PID_KI = 0.02f;
constexpr float PID_ALPHA = 0.20f;
constexpr int PID_TRIM_LIMIT = 15;
constexpr int PID_SLEW_STEP = 2;

enum MotionCommand {
  CMD_STOP = 0,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
};

struct SyncPidRuntime {
  float kp = PID_KP;
  float ki = PID_KI;
  float alpha = PID_ALPHA;
  float error_ema = 0.0f;
  float integral = 0.0f;
  float trim = 0.0f;
  int trim_limit = PID_TRIM_LIMIT;
  int slew_step = PID_SLEW_STEP;
  long left_last_count = 0;
  long right_last_count = 0;
  unsigned long last_update_ms = 0;
};

static WebServer server(80);
static MotionCommand current_command = CMD_STOP;
static int current_left_pwm = BASE_LEFT;
static int current_right_pwm = BASE_RIGHT;
static int applied_left_pwm = 0;
static int applied_right_pwm = 0;
static SyncPidRuntime pid_runtime;

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

inline long readCount(volatile long &enc_count) {
  noInterrupts();
  const long snapshot = enc_count;
  interrupts();
  return snapshot;
}

inline int applySlew(int current_pwm, int target_pwm, int max_step) {
  if (target_pwm > current_pwm + max_step) return current_pwm + max_step;
  if (target_pwm < current_pwm - max_step) return current_pwm - max_step;
  return target_pwm;
}

inline void resetPidRuntime() {
  pid_runtime.error_ema = 0.0f;
  pid_runtime.integral = 0.0f;
  pid_runtime.trim = 0.0f;
  pid_runtime.left_last_count = readCount(motor1_enc_count);
  pid_runtime.right_last_count = readCount(motor2_enc_count);
  pid_runtime.last_update_ms = millis();
  applied_left_pwm = current_left_pwm;
  applied_right_pwm = current_right_pwm;
}

inline int motor1SignedPwm() {
  switch (current_command) {
    case CMD_FORWARD: return applied_left_pwm;
    case CMD_BACKWARD: return -applied_left_pwm;
    case CMD_LEFT: return -current_left_pwm;
    case CMD_RIGHT: return current_left_pwm;
    case CMD_STOP:
    default: return 0;
  }
}

inline int motor2SignedPwm() {
  switch (current_command) {
    case CMD_FORWARD: return applied_right_pwm;
    case CMD_BACKWARD: return -applied_right_pwm;
    case CMD_LEFT: return current_right_pwm;
    case CMD_RIGHT: return -current_right_pwm;
    case CMD_STOP:
    default: return 0;
  }
}

inline void applyMotion() {
  setMotorPwm(motor1_config::CONFIG, motor1SignedPwm());
  setMotorPwm(motor2_config::CONFIG, motor2SignedPwm());
}

inline String stateJson() {
  String s = "{";
  s += "\"command\":\"" + String(commandName(current_command)) + "\",";
  s += "\"baseL\":" + String(current_left_pwm) + ",";
  s += "\"baseR\":" + String(current_right_pwm) + ",";
  s += "\"appliedL\":" + String(applied_left_pwm) + ",";
  s += "\"appliedR\":" + String(applied_right_pwm) + ",";
  s += "\"m1\":" + String(motor1SignedPwm()) + ",";
  s += "\"m2\":" + String(motor2SignedPwm()) + ",";
  s += "\"trim\":" + String(pid_runtime.trim, 2);
  s += "}";
  return s;
}

inline void setCommand(MotionCommand cmd) {
  current_command = cmd;
  resetPidRuntime();
  applyMotion();
  Serial.print("[WEB] command = ");
  Serial.println(commandName(current_command));
}

inline String htmlPage() {
  String html;
  html += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>";
  html += "<title>ESP Car</title><style>";
  html += "body{font-family:sans-serif;background:#f4f1ea;color:#1e1f1c;text-align:center;padding:24px;margin:0}";
  html += ".wrap{max-width:360px;margin:0 auto}.row{display:flex;gap:12px;justify-content:center;margin:12px 0}";
  html += "button{width:110px;height:64px;font-size:18px;border:none;border-radius:14px;background:#283618;color:#fff;touch-action:manipulation}";
  html += ".stop{background:#bc4749}.status{margin:18px 0;padding:12px;border-radius:12px;background:#fff;line-height:1.6}";
  html += "</style></head><body><div class='wrap'><h2>ESP Car</h2>";
  html += "<div class='status'>Current: <strong id='state'>";
  html += commandName(current_command);
  html += "</strong><br>Base L/R: <span id='base'>";
  html += String(current_left_pwm) + " / " + String(current_right_pwm);
  html += "</span><br>Applied L/R: <span id='applied'>";
  html += String(applied_left_pwm) + " / " + String(applied_right_pwm);
  html += "</span><br>Motor M1/M2: <span id='motors'>";
  html += String(motor1SignedPwm()) + " / " + String(motor2SignedPwm());
  html += "</span><br>PID Trim: <span id='trim'>";
  html += String(pid_runtime.trim, 2);
  html += "</span></div>";
  html += "<div class='row'><button onclick=\"sendCmd('forward')\">Forward</button></div>";
  html += "<div class='row'><button onclick=\"sendCmd('left')\">Left</button><button class='stop' onclick=\"sendCmd('stop')\">Stop</button><button onclick=\"sendCmd('right')\">Right</button></div>";
  html += "<div class='row'><button onclick=\"sendCmd('backward')\">Backward</button></div>";
  html += "<div class='row'><button onclick=\"sendCmd('slower')\">Speed -</button><button onclick=\"sendCmd('faster')\">Speed +</button></div>";
  html += "<script>";
  html += "function updateUi(s){document.getElementById('state').textContent=s.command;document.getElementById('base').textContent=s.baseL+' / '+s.baseR;document.getElementById('applied').textContent=s.appliedL+' / '+s.appliedR;document.getElementById('motors').textContent=s.m1+' / '+s.m2;document.getElementById('trim').textContent=s.trim;}";
  html += "async function sendCmd(cmd){const r=await fetch('/'+cmd,{method:'POST',cache:'no-store'});const s=await r.json();updateUi(s);}";
  html += "setInterval(async()=>{try{const r=await fetch('/state',{cache:'no-store'});const s=await r.json();updateUi(s);}catch(e){}},300);";
  html += "</script></div></body></html>";
  return html;
}

inline void handleRoot() { server.send(200, "text/html", htmlPage()); }
inline void handleState() { server.send(200, "application/json", stateJson()); }
inline void sendStateResponse() { server.send(200, "application/json", stateJson()); }

inline void handleForward() { setCommand(CMD_FORWARD); sendStateResponse(); }
inline void handleBackward() { setCommand(CMD_BACKWARD); sendStateResponse(); }
inline void handleLeft() { setCommand(CMD_LEFT); sendStateResponse(); }
inline void handleRight() { setCommand(CMD_RIGHT); sendStateResponse(); }
inline void handleStop() { setCommand(CMD_STOP); sendStateResponse(); }

inline void handleFaster() {
  current_left_pwm = min(255, current_left_pwm + SPEED_STEP);
  current_right_pwm = min(255, current_right_pwm + SPEED_STEP);
  resetPidRuntime();
  applyMotion();
  Serial.printf("[WEB] speed up -> L=%d R=%d\n", current_left_pwm, current_right_pwm);
  sendStateResponse();
}

inline void handleSlower() {
  current_left_pwm = max(0, current_left_pwm - SPEED_STEP);
  current_right_pwm = max(0, current_right_pwm - SPEED_STEP);
  resetPidRuntime();
  applyMotion();
  Serial.printf("[WEB] speed down -> L=%d R=%d\n", current_left_pwm, current_right_pwm);
  sendStateResponse();
}

inline void runPidIfNeeded() {
  if (current_command != CMD_FORWARD && current_command != CMD_BACKWARD) return;

  const unsigned long now = millis();
  const unsigned long dt_ms = now - pid_runtime.last_update_ms;
  if (dt_ms < PID_INTERVAL_MS) return;
  pid_runtime.last_update_ms = now;

  const long left_count = readCount(motor1_enc_count);
  const long right_count = readCount(motor2_enc_count);
  const long delta_left = left_count - pid_runtime.left_last_count;
  const long delta_right = right_count - pid_runtime.right_last_count;
  pid_runtime.left_last_count = left_count;
  pid_runtime.right_last_count = right_count;

  const float raw_error = static_cast<float>(delta_left - delta_right);
  pid_runtime.error_ema = (1.0f - pid_runtime.alpha) * pid_runtime.error_ema +
                          pid_runtime.alpha * raw_error;

  pid_runtime.integral += pid_runtime.error_ema * (dt_ms / 1000.0f);
  if (pid_runtime.integral > 20.0f) pid_runtime.integral = 20.0f;
  if (pid_runtime.integral < -20.0f) pid_runtime.integral = -20.0f;

  float trim = pid_runtime.kp * pid_runtime.error_ema + pid_runtime.ki * pid_runtime.integral;
  if (trim > pid_runtime.trim_limit) trim = pid_runtime.trim_limit;
  if (trim < -pid_runtime.trim_limit) trim = -pid_runtime.trim_limit;
  pid_runtime.trim = trim;

  const int target_left = constrain(static_cast<int>(roundf(current_left_pwm - pid_runtime.trim)), 0, 255);
  const int target_right = constrain(static_cast<int>(roundf(current_right_pwm + pid_runtime.trim)), 0, 255);

  applied_left_pwm = applySlew(applied_left_pwm, target_left, pid_runtime.slew_step);
  applied_right_pwm = applySlew(applied_right_pwm, target_right, pid_runtime.slew_step);
  applyMotion();
}

inline void init() {
  initMotorDriver(motor1_config::CONFIG);
  initMotorDriver(motor2_config::CONFIG);
  attachMotorEncoder(motor1_config::CONFIG, onMotor1EncA);
  attachMotorEncoder(motor2_config::CONFIG, onMotor2EncA);
  stopAllMotors();
  resetEncoderCounter(motor1_enc_count, motor1_last_isr_us);
  resetEncoderCounter(motor2_enc_count, motor2_last_isr_us);
  resetPidRuntime();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/state", HTTP_GET, handleState);
  server.on("/forward", HTTP_POST, handleForward);
  server.on("/backward", HTTP_POST, handleBackward);
  server.on("/left", HTTP_POST, handleLeft);
  server.on("/right", HTTP_POST, handleRight);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/faster", HTTP_POST, handleFaster);
  server.on("/slower", HTTP_POST, handleSlower);
  server.begin();

  Serial.println("[WEB] AP mode ready");
  Serial.print("[WEB] SSID: ");
  Serial.println(AP_SSID);
  Serial.print("[WEB] Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("[WEB] Open: http://");
  Serial.println(WiFi.softAPIP());
  Serial.printf("[WEB] PID sync on forward/backward: kp=%.2f ki=%.2f ppr=%.0f\n", pid_runtime.kp, pid_runtime.ki, PPR_OUTPUT);
}

inline void loop() {
  server.handleClient();
  runPidIfNeeded();
}

}  // namespace web_control
