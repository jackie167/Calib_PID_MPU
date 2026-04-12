#pragma once

#include <Arduino.h>

#include "motor_1_config.h"
#include "motor_2_config.h"
#include "motor_driver.h"

extern const bool PLOTTER_MODE;
extern const bool PLOTTER_CSV;

struct EncoderTestRuntime {
  const MotorConfig *config;
  volatile long *enc_count;
  long last_count;
  float last_rpm;
  int pwm_out;
};

inline void initEncoderRuntime(EncoderTestRuntime &runtime,
                               const MotorConfig &config,
                               volatile long &enc_count,
                               int pwm_out) {
  runtime.config = &config;
  runtime.enc_count = &enc_count;
  runtime.last_count = 0;
  runtime.last_rpm = 0.0f;
  runtime.pwm_out = pwm_out;
}

inline void initEncoderTest() {
  initMotorDriver(motor1_config::CONFIG);
  initMotorDriver(motor2_config::CONFIG);
  attachMotorEncoder(motor1_config::CONFIG, onMotor1EncA);
  attachMotorEncoder(motor2_config::CONFIG, onMotor2EncA);

  Serial.println("[ENCODER TEST] Running motors immediately");
  Serial.println("[ENCODER TEST] M1 PWM=120, M2 PWM=150");

  if (PLOTTER_MODE && PLOTTER_CSV) {
    Serial.println("m1_pwm,m1_enc,m1_delta,m1_rpm,m2_pwm,m2_enc,m2_delta,m2_rpm");
    Serial.println();
  }
}

inline void testEncoder() {
  static bool initialized = false;
  static unsigned long last_report = 0;
  static unsigned long last_header = 0;
  static EncoderTestRuntime motor1;
  static EncoderTestRuntime motor2;

  constexpr int MOTOR1_TEST_PWM = 120;
  constexpr int MOTOR2_TEST_PWM = 150;
  constexpr unsigned long REPORT_MS = 200;

  if (!initialized) {
    initEncoderRuntime(motor1, motor1_config::CONFIG, motor1_enc_count, MOTOR1_TEST_PWM);
    initEncoderRuntime(motor2, motor2_config::CONFIG, motor2_enc_count, MOTOR2_TEST_PWM);
    initialized = true;
  }

  unsigned long now = millis();
  setMotorPwm(*motor1.config, motor1.pwm_out);
  setMotorPwm(*motor2.config, motor2.pwm_out);

  if (PLOTTER_MODE && PLOTTER_CSV) {
    if (last_header == 0 || now - last_header >= 5000) {
      Serial.println("m1_pwm,m1_enc,m1_delta,m1_rpm,m2_pwm,m2_enc,m2_delta,m2_rpm");
      Serial.println();
      last_header = now;
    }
  }

  if (now - last_report < REPORT_MS) {
    return;
  }

  float dt_s = (now - last_report) / 1000.0f;
  last_report = now;

  long m1_count = readEncoderCount(*motor1.enc_count);
  long m2_count = readEncoderCount(*motor2.enc_count);
  long m1_delta = m1_count - motor1.last_count;
  long m2_delta = m2_count - motor2.last_count;
  motor1.last_count = m1_count;
  motor2.last_count = m2_count;

  if (m1_delta < 0) m1_delta = -m1_delta;
  if (m2_delta < 0) m2_delta = -m2_delta;

  motor1.last_rpm = dt_s > 0.0f ? (m1_delta / (float)motor1.config->pulses_per_rev) * (60.0f / dt_s) : 0.0f;
  motor2.last_rpm = dt_s > 0.0f ? (m2_delta / (float)motor2.config->pulses_per_rev) * (60.0f / dt_s) : 0.0f;

  if (PLOTTER_MODE && PLOTTER_CSV) {
    Serial.print(motor1.pwm_out);
    Serial.print(',');
    Serial.print(m1_count);
    Serial.print(',');
    Serial.print(m1_delta);
    Serial.print(',');
    Serial.print(motor1.last_rpm, 1);
    Serial.print(',');
    Serial.print(motor2.pwm_out);
    Serial.print(',');
    Serial.print(m2_count);
    Serial.print(',');
    Serial.print(m2_delta);
    Serial.print(',');
    Serial.println(motor2.last_rpm, 1);
  }
}
