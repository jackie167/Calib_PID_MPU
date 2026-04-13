#pragma once

#include <Arduino.h>
#include <math.h>

#include "motor_1_config.h"
#include "motor_2_config.h"
#include "motor_driver.h"

extern volatile long motor1_enc_count;
extern volatile long motor2_enc_count;
extern volatile unsigned long motor1_last_isr_us;
extern volatile unsigned long motor2_last_isr_us;

enum TestMode {
  MODE_IDLE = 0,
  MODE_COUNT_PULSE = 1,
  MODE_PWM_HOLD = 2,
  MODE_PWM_SWEEP = 3,
  MODE_RPM_TEST = 4,
  MODE_BASE_PWM = 5,
  MODE_SYNC_TEST = 6,
  MODE_PID_TEST = 7
};

struct EncoderSample {
  long count = 0;
  long delta = 0;
  unsigned long dt_ms = 0;
  float rpm = 0.0f;
};

struct SingleTestState {
  const MotorConfig *config = nullptr;
  volatile long *enc_count = nullptr;
  long last_count = 0;
  unsigned long last_sample_ms = 0;
  int manual_pwm = 100;
  float target_value = 0.0f;
  float ppr_output = 9.0f;
};

struct SyncTestState {
  long left_last_count = 0;
  long right_last_count = 0;
  unsigned long last_sample_ms = 0;
  int left_pwm = 100;
  int right_pwm = 100;
  float left_ppr = 900.0f;
  float right_ppr = 900.0f;
};

struct SyncPidState {
  int base_left = 128;
  int base_right = 121;
  int pwm_left = 128;
  int pwm_right = 121;
  float kp = 0.3f;
  float ki = 0.02f;
  float alpha = 0.2f;
  float error_ema = 0.0f;
  float integral = 0.0f;
  float trim = 0.0f;
  int trim_limit = 15;
  int slew_step = 2;
  long left_last_count = 0;
  long right_last_count = 0;
  unsigned long last_sample_ms = 0;
};

static TestMode currentMode = MODE_IDLE;
static int activeMotorIndex = 1;
static unsigned long lastPrintMs = 0;
static int sweepIndex = 0;
static unsigned long sweepStepStartMs = 0;

static SingleTestState singleState;
static SyncTestState syncState;
static SyncPidState syncPidState;

static int sweepList[] = {40, 60, 80, 100, 120, 140, 160};
static const int sweepSize = sizeof(sweepList) / sizeof(sweepList[0]);

inline const MotorConfig &motorConfigForIndex(int motor_index) {
  return motor_index == 2 ? motor2_config::CONFIG : motor1_config::CONFIG;
}

inline volatile long &encoderCountForIndex(int motor_index) {
  return motor_index == 2 ? motor2_enc_count : motor1_enc_count;
}

inline void resetEncoderCounter(volatile long &enc_count, volatile unsigned long &last_isr_us) {
  noInterrupts();
  enc_count = 0;
  last_isr_us = 0;
  interrupts();
}

inline void resetAllEncoderCounters() {
  resetEncoderCounter(motor1_enc_count, motor1_last_isr_us);
  resetEncoderCounter(motor2_enc_count, motor2_last_isr_us);
}

inline void stopAllMotors() {
  setMotorPwm(motor1_config::CONFIG, 0);
  setMotorPwm(motor2_config::CONFIG, 0);
}

inline void setActiveMotor(int pwm) {
  setMotorPwm(*singleState.config, pwm);
}

inline EncoderSample sampleEncoder(volatile long &enc_count,
                                   long &last_count,
                                   unsigned long &last_sample_ms,
                                   float ppr_output) {
  EncoderSample sample;
  const unsigned long now = millis();

  if (last_sample_ms == 0) {
    last_sample_ms = now;
  }

  sample.dt_ms = now - last_sample_ms;
  if (sample.dt_ms == 0) {
    sample.dt_ms = 1;
  }
  last_sample_ms = now;

  sample.count = readEncoderCount(enc_count);
  sample.delta = sample.count - last_count;
  last_count = sample.count;

  const float dt_s = sample.dt_ms / 1000.0f;
  if (ppr_output > 0.0f && dt_s > 0.0f) {
    sample.rpm = (sample.delta / ppr_output) * (60.0f / dt_s);
  }
  return sample;
}

inline EncoderSample sampleEncoderWithWindow(volatile long &enc_count,
                                             long &last_count,
                                             unsigned long dt_ms,
                                             float ppr_output) {
  EncoderSample sample;
  sample.dt_ms = (dt_ms == 0) ? 1 : dt_ms;
  sample.count = readEncoderCount(enc_count);
  sample.delta = sample.count - last_count;
  last_count = sample.count;

  const float dt_s = sample.dt_ms / 1000.0f;
  if (ppr_output > 0.0f && dt_s > 0.0f) {
    sample.rpm = (sample.delta / ppr_output) * (60.0f / dt_s);
  }
  return sample;
}

inline int applySlew(int current_pwm, int target_pwm, int max_step) {
  if (target_pwm > current_pwm + max_step) return current_pwm + max_step;
  if (target_pwm < current_pwm - max_step) return current_pwm - max_step;
  return target_pwm;
}

inline void printCurrentSelection() {
  Serial.print("[ACTIVE_MOTOR] M");
  Serial.print(activeMotorIndex);
  Serial.print("  encA=");
  Serial.print(singleState.config->enc_a_pin);
  Serial.print("  encB=");
  Serial.print(singleState.config->enc_b_pin);
  Serial.print("  ppr_output=");
  Serial.println(singleState.ppr_output, 2);
}

inline void printModeHeader(TestMode mode) {
  switch (mode) {
    case MODE_COUNT_PULSE:
      Serial.println("enc_count,delta_count,dt_ms");
      break;
    case MODE_PWM_HOLD:
    case MODE_PWM_SWEEP:
    case MODE_RPM_TEST:
    case MODE_BASE_PWM:
      Serial.println("pwm_cmd,enc_count,delta_count,dt_ms,rpm");
      break;
    case MODE_SYNC_TEST:
      Serial.println("baseL,encL,deltaL,baseR,encR,deltaR,dt_ms,rpmL,rpmR,error");
      break;
    case MODE_PID_TEST:
      Serial.println("baseL,baseR,pwmL,pwmR,deltaL,deltaR,error,trim,dt_ms");
      break;
    default:
      break;
  }
}

inline void resetSingleState() {
  singleState.config = &motorConfigForIndex(activeMotorIndex);
  singleState.enc_count = &encoderCountForIndex(activeMotorIndex);
  singleState.last_count = readEncoderCount(*singleState.enc_count);
  singleState.last_sample_ms = millis();
  singleState.ppr_output = (float)singleState.config->pulses_per_rev;
}

inline void resetSyncState() {
  syncState.left_last_count = readEncoderCount(motor1_enc_count);
  syncState.right_last_count = readEncoderCount(motor2_enc_count);
  syncState.last_sample_ms = millis();
  syncState.left_ppr = 900.0f;
  syncState.right_ppr = 900.0f;
}

inline void resetSyncPidState() {
  syncPidState.pwm_left = syncPidState.base_left;
  syncPidState.pwm_right = syncPidState.base_right;
  syncPidState.error_ema = 0.0f;
  syncPidState.integral = 0.0f;
  syncPidState.trim = 0.0f;
  syncPidState.left_last_count = readEncoderCount(motor1_enc_count);
  syncPidState.right_last_count = readEncoderCount(motor2_enc_count);
  syncPidState.last_sample_ms = millis();
}

inline void resetModeState(bool clearEncoder = false) {
  stopAllMotors();

  if (clearEncoder) {
    resetAllEncoderCounters();
  }

  lastPrintMs = millis();
  sweepIndex = 0;
  sweepStepStartMs = millis();

  resetSingleState();
  resetSyncState();
  resetSyncPidState();
}

inline void enterMode(TestMode newMode, bool clearEncoder = true) {
  currentMode = newMode;
  resetModeState(clearEncoder);

  Serial.println();
  Serial.print("[MODE] -> ");
  Serial.println((int)currentMode);
  printCurrentSelection();
  printModeHeader(currentMode);
}

inline void printHelp() {
  Serial.println("===== COMMANDS =====");
  Serial.println("m0 : idle");
  Serial.println("m1 : count pulse");
  Serial.println("m2 : pwm hold");
  Serial.println("m3 : pwm sweep");
  Serial.println("m4 : rpm test");
  Serial.println("m5 : base pwm test");
  Serial.println("m6 : sync test");
  Serial.println("m7 : soft sync PI test");
  Serial.println("s1 : select motor 1");
  Serial.println("s2 : select motor 2");
  Serial.println("pXXX : set single-mode pwm, vd p120");
  Serial.println("plXXX : set sync left pwm/base, vd pl128");
  Serial.println("prXXX : set sync right pwm/base, vd pr121");
  Serial.println("blXXX : set pid base left");
  Serial.println("brXXX : set pid base right");
  Serial.println("tXXX : set target value for base/pid test");
  Serial.println("kXXX : set single-mode ppr output, vd k450");
  Serial.println("klXXX : set sync left ppr");
  Serial.println("krXXX : set sync right ppr");
  Serial.println("kpX.XX : set pid kp");
  Serial.println("kiX.XX : set pid ki");
  Serial.println("r : reset encoder count + mode state");
  Serial.println("h : help");
  Serial.println("====================");
}

inline void readSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.isEmpty()) return;

  if (cmd == "m0") enterMode(MODE_IDLE);
  else if (cmd == "m1") enterMode(MODE_COUNT_PULSE);
  else if (cmd == "m2") enterMode(MODE_PWM_HOLD);
  else if (cmd == "m3") enterMode(MODE_PWM_SWEEP);
  else if (cmd == "m4") enterMode(MODE_RPM_TEST);
  else if (cmd == "m5") enterMode(MODE_BASE_PWM);
  else if (cmd == "m6") enterMode(MODE_SYNC_TEST);
  else if (cmd == "m7") enterMode(MODE_PID_TEST);
  else if (cmd == "s1" || cmd == "s2") {
    activeMotorIndex = (cmd == "s2") ? 2 : 1;
    resetModeState(true);
    printCurrentSelection();
    printModeHeader(currentMode);
  } else if (cmd.startsWith("pl")) {
    syncState.left_pwm = constrain(cmd.substring(2).toInt(), -255, 255);
    syncPidState.base_left = syncState.left_pwm;
    syncPidState.pwm_left = syncPidState.base_left;
    Serial.print("[SYNC] left_pwm = ");
    Serial.println(syncState.left_pwm);
  } else if (cmd.startsWith("pr")) {
    syncState.right_pwm = constrain(cmd.substring(2).toInt(), -255, 255);
    syncPidState.base_right = syncState.right_pwm;
    syncPidState.pwm_right = syncPidState.base_right;
    Serial.print("[SYNC] right_pwm = ");
    Serial.println(syncState.right_pwm);
  } else if (cmd.startsWith("bl")) {
    syncPidState.base_left = constrain(cmd.substring(2).toInt(), -255, 255);
    syncPidState.pwm_left = syncPidState.base_left;
    Serial.print("[PID] base_left = ");
    Serial.println(syncPidState.base_left);
  } else if (cmd.startsWith("br")) {
    syncPidState.base_right = constrain(cmd.substring(2).toInt(), -255, 255);
    syncPidState.pwm_right = syncPidState.base_right;
    Serial.print("[PID] base_right = ");
    Serial.println(syncPidState.base_right);
  } else if (cmd.startsWith("kl")) {
    syncState.left_ppr = cmd.substring(2).toFloat();
    Serial.print("[SYNC] left_ppr = ");
    Serial.println(syncState.left_ppr, 2);
  } else if (cmd.startsWith("kr")) {
    syncState.right_ppr = cmd.substring(2).toFloat();
    Serial.print("[SYNC] right_ppr = ");
    Serial.println(syncState.right_ppr, 2);
  } else if (cmd.startsWith("kp")) {
    syncPidState.kp = cmd.substring(2).toFloat();
    Serial.print("[PID] kp = ");
    Serial.println(syncPidState.kp, 3);
  } else if (cmd.startsWith("ki")) {
    syncPidState.ki = cmd.substring(2).toFloat();
    Serial.print("[PID] ki = ");
    Serial.println(syncPidState.ki, 3);
  } else if (cmd.startsWith("p")) {
    singleState.manual_pwm = constrain(cmd.substring(1).toInt(), -255, 255);
    Serial.print("[PWM] single pwm = ");
    Serial.println(singleState.manual_pwm);
  } else if (cmd.startsWith("t")) {
    singleState.target_value = cmd.substring(1).toFloat();
    Serial.print("[TARGET] target_value = ");
    Serial.println(singleState.target_value, 2);
  } else if (cmd.startsWith("k")) {
    singleState.ppr_output = cmd.substring(1).toFloat();
    Serial.print("[PPR] single ppr_output = ");
    Serial.println(singleState.ppr_output, 2);
  } else if (cmd == "r") {
    resetModeState(true);
    Serial.println("[RESET] encoders and mode state reset");
    printModeHeader(currentMode);
  } else if (cmd == "h") {
    printHelp();
  } else {
    Serial.print("[UNKNOWN] ");
    Serial.println(cmd);
  }
}

inline void modeIdle() {
  stopAllMotors();
  const unsigned long now = millis();
  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.print("[IDLE] m");
    Serial.print(activeMotorIndex);
    Serial.print("_count=");
    Serial.println(readEncoderCount(*singleState.enc_count));
  }
}

inline void modeCountPulse() {
  stopAllMotors();
  const unsigned long now = millis();
  if (now - lastPrintMs >= 200) {
    lastPrintMs = now;
    EncoderSample sample = sampleEncoder(*singleState.enc_count,
                                         singleState.last_count,
                                         singleState.last_sample_ms,
                                         singleState.ppr_output);
    Serial.print(sample.count);
    Serial.print(",");
    Serial.print(sample.delta);
    Serial.print(",");
    Serial.println(sample.dt_ms);
  }
}

inline void printSingleMotorRow(int pwm_cmd) {
  EncoderSample sample = sampleEncoder(*singleState.enc_count,
                                       singleState.last_count,
                                       singleState.last_sample_ms,
                                       singleState.ppr_output);
  Serial.print(pwm_cmd);
  Serial.print(",");
  Serial.print(sample.count);
  Serial.print(",");
  Serial.print(sample.delta);
  Serial.print(",");
  Serial.print(sample.dt_ms);
  Serial.print(",");
  Serial.println(sample.rpm, 2);
}

inline void modePWMHold() {
  setActiveMotor(singleState.manual_pwm);
  const unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    lastPrintMs = now;
    printSingleMotorRow(singleState.manual_pwm);
  }
}

inline void modePWMSweep() {
  const unsigned long now = millis();

  if (now - sweepStepStartMs >= 3000) {
    sweepIndex = (sweepIndex + 1) % sweepSize;
    sweepStepStartMs = now;
    singleState.last_count = readEncoderCount(*singleState.enc_count);
    singleState.last_sample_ms = now;
  }

  const int pwm_cmd = sweepList[sweepIndex];
  setActiveMotor(pwm_cmd);

  if (now - lastPrintMs >= 100) {
    lastPrintMs = now;
    printSingleMotorRow(pwm_cmd);
  }
}

inline void modeRPMTest() {
  setActiveMotor(singleState.manual_pwm);
  const unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    lastPrintMs = now;
    printSingleMotorRow(singleState.manual_pwm);
  }
}

inline void modeBasePWM() {
  setActiveMotor(singleState.manual_pwm);
  const unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    lastPrintMs = now;
    EncoderSample sample = sampleEncoder(*singleState.enc_count,
                                         singleState.last_count,
                                         singleState.last_sample_ms,
                                         singleState.ppr_output);
    Serial.print(singleState.manual_pwm);
    Serial.print(",");
    Serial.print(sample.count);
    Serial.print(",");
    Serial.print(sample.delta);
    Serial.print(",");
    Serial.print(sample.dt_ms);
    Serial.print(",");
    Serial.print(sample.rpm, 2);
    Serial.print("  # target=");
    Serial.println(singleState.target_value, 2);
  }
}

inline void modeSyncTest() {
  setMotorPwm(motor1_config::CONFIG, syncState.left_pwm);
  setMotorPwm(motor2_config::CONFIG, syncState.right_pwm);

  const unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    const unsigned long dt_ms = now - syncState.last_sample_ms;
    syncState.last_sample_ms = now;
    lastPrintMs = now;

    EncoderSample left = sampleEncoderWithWindow(motor1_enc_count,
                                                 syncState.left_last_count,
                                                 dt_ms,
                                                 syncState.left_ppr);
    EncoderSample right = sampleEncoderWithWindow(motor2_enc_count,
                                                  syncState.right_last_count,
                                                  dt_ms,
                                                  syncState.right_ppr);

    Serial.print(syncState.left_pwm);
    Serial.print(",");
    Serial.print(left.count);
    Serial.print(",");
    Serial.print(left.delta);
    Serial.print(",");
    Serial.print(syncState.right_pwm);
    Serial.print(",");
    Serial.print(right.count);
    Serial.print(",");
    Serial.print(right.delta);
    Serial.print(",");
    Serial.print(dt_ms);
    Serial.print(",");
    Serial.print(left.rpm, 2);
    Serial.print(",");
    Serial.print(right.rpm, 2);
    Serial.print(",");
    Serial.println(left.delta - right.delta);
  }
}

inline void modePIDTest() {
  const unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    const unsigned long dt_ms = now - syncPidState.last_sample_ms;
    syncPidState.last_sample_ms = now;
    lastPrintMs = now;

    EncoderSample left = sampleEncoderWithWindow(motor1_enc_count,
                                                 syncPidState.left_last_count,
                                                 dt_ms,
                                                 syncState.left_ppr);
    EncoderSample right = sampleEncoderWithWindow(motor2_enc_count,
                                                  syncPidState.right_last_count,
                                                  dt_ms,
                                                  syncState.right_ppr);

    const float raw_error = (float)(left.delta - right.delta);
    syncPidState.error_ema =
        (1.0f - syncPidState.alpha) * syncPidState.error_ema +
        syncPidState.alpha * raw_error;

    syncPidState.integral += syncPidState.error_ema * (dt_ms / 1000.0f);
    if (syncPidState.integral > 20.0f) syncPidState.integral = 20.0f;
    if (syncPidState.integral < -20.0f) syncPidState.integral = -20.0f;

    float trim_cmd = syncPidState.kp * syncPidState.error_ema +
                     syncPidState.ki * syncPidState.integral;
    if (trim_cmd > syncPidState.trim_limit) trim_cmd = syncPidState.trim_limit;
    if (trim_cmd < -syncPidState.trim_limit) trim_cmd = -syncPidState.trim_limit;
    syncPidState.trim = trim_cmd;

    const int target_left_pwm = constrain((int)roundf(syncPidState.base_left - syncPidState.trim), 0, 255);
    const int target_right_pwm = constrain((int)roundf(syncPidState.base_right + syncPidState.trim), 0, 255);

    syncPidState.pwm_left = applySlew(syncPidState.pwm_left, target_left_pwm, syncPidState.slew_step);
    syncPidState.pwm_right = applySlew(syncPidState.pwm_right, target_right_pwm, syncPidState.slew_step);

    setMotorPwm(motor1_config::CONFIG, syncPidState.pwm_left);
    setMotorPwm(motor2_config::CONFIG, syncPidState.pwm_right);

    Serial.print(syncPidState.base_left);
    Serial.print(",");
    Serial.print(syncPidState.base_right);
    Serial.print(",");
    Serial.print(syncPidState.pwm_left);
    Serial.print(",");
    Serial.print(syncPidState.pwm_right);
    Serial.print(",");
    Serial.print(left.delta);
    Serial.print(",");
    Serial.print(right.delta);
    Serial.print(",");
    Serial.print(raw_error, 2);
    Serial.print(",");
    Serial.print(syncPidState.trim, 2);
    Serial.print(",");
    Serial.println(dt_ms);
  }
}

inline void setupTestFramework() {
  Serial.begin(115200);
  delay(500);
  while (!Serial && millis() < 3000) delay(10);

  initMotorDriver(motor1_config::CONFIG);
  initMotorDriver(motor2_config::CONFIG);
  attachMotorEncoder(motor1_config::CONFIG, onMotor1EncA);
  attachMotorEncoder(motor2_config::CONFIG, onMotor2EncA);

  activeMotorIndex = 1;
  resetModeState(true);
  printHelp();
  printCurrentSelection();
}

inline void loopTestFramework() {
  readSerialCommand();

  switch (currentMode) {
    case MODE_IDLE:        modeIdle(); break;
    case MODE_COUNT_PULSE: modeCountPulse(); break;
    case MODE_PWM_HOLD:    modePWMHold(); break;
    case MODE_PWM_SWEEP:   modePWMSweep(); break;
    case MODE_RPM_TEST:    modeRPMTest(); break;
    case MODE_BASE_PWM:    modeBasePWM(); break;
    case MODE_SYNC_TEST:   modeSyncTest(); break;
    case MODE_PID_TEST:    modePIDTest(); break;
    default:               modeIdle(); break;
  }
}
