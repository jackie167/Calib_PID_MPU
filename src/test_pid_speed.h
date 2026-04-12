#pragma once
#include <Arduino.h>
#include "motor_2_config.h"
#include "pid.h"
#include "motor_driver.h"

extern const bool PLOTTER_MODE;
extern const bool PLOTTER_CSV;

void testPidSpeed() {
  static float target_rpm = 120.0f;
  static unsigned long last_plotter_header = 0;
  static unsigned long last_report = 0;
  static unsigned long last_sample = 0;
  static unsigned long last_rpm_calc = 0;
  static long last_count = 0;
  static float last_rpm = 0.0f;
  static float rpm_ema = 0.0f;
  static long rpm_accum = 0;
  static int pwm_out = 0;
  static PIDState pid;
  static int stale_count = 0;
  static float KP = MOTOR_KP;
  static float KI = MOTOR_KI;
  static float KD = MOTOR_KD;
  static unsigned long step_start = 0;

  const int   PULSES_PER_REV = MOTOR_PULSES_PER_REV;
  const int   SAMPLE_MS      = MOTOR_SAMPLE_MS;
  const int   RPM_WINDOW_MS  = MOTOR_RPM_WINDOW_MS;
  const float BASE_PWM       = MOTOR_BASE_PWM;
  const bool  STEP_TARGET    = true;

  unsigned long now = millis();

  // Step target profile: 0 -> STEP1 -> STEP2 -> STEP1 -> repeat
  if (STEP_TARGET) {
    if (step_start == 0) step_start = now;
    unsigned long t = now - step_start;
    if      (t < 3000)  target_rpm = 0.0f;
    else if (t < 11000) target_rpm = MOTOR_RPM_STEP1;
    else if (t < 19000) target_rpm = MOTOR_RPM_STEP2;
    else { target_rpm = MOTOR_RPM_STEP1; step_start = now; }
  }

  // CSV header every 5 seconds
  if (PLOTTER_MODE && PLOTTER_CSV) {
    if (last_plotter_header == 0 || now - last_plotter_header >= 5000) {
      Serial.println("rpm,target_rpm,pwm,enc_count");
      Serial.println();
      last_plotter_header = now;
    }
  }

  // Keyboard tuning
  while (Serial.available() > 0) {
    char cmd = (char)Serial.read();
    if (cmd == 'p') KP += 0.1f;
    if (cmd == 'P') KP -= 0.1f;
    if (cmd == 'i') KI += 0.05f;
    if (cmd == 'I') KI -= 0.05f;
    if (cmd == 'd') KD += 0.01f;
    if (cmd == 'D') KD -= 0.01f;
    if (cmd == 'u') target_rpm += 5.0f;
    if (cmd == 'j') target_rpm -= 5.0f;
    if (cmd == 's') { target_rpm = 0.0f; Serial.println("[STOP] Motor stopped"); }
    if (cmd == 'a') Serial.printf("[GAINS] KP=%.2f KI=%.3f KD=%.3f\n", KP, KI, KD);
    if (KP < 0) KP = 0;
    if (KI < 0) KI = 0;
    if (KD < 0) KD = 0;
    if (cmd == 'p' || cmd == 'P' || cmd == 'i' || cmd == 'I' || cmd == 'd' || cmd == 'D')
      Serial.printf("[TUNE] KP=%.2f KI=%.3f KD=%.3f\n", KP, KI, KD);
  }

  // Fixed-rate encoder sampling
  if (now - last_sample >= (unsigned long)SAMPLE_MS) {
    last_sample = now;
    noInterrupts();
    long count_snapshot = enc_count;
    interrupts();
    long delta = count_snapshot - last_count;
    last_count = count_snapshot;
    if (delta < 0) delta = -delta;
    rpm_accum += delta;
  }

  // Compute RPM every RPM_WINDOW_MS
  if (now - last_rpm_calc >= (unsigned long)RPM_WINDOW_MS) {
    float dt = (now - last_rpm_calc) / 1000.0f;
    last_rpm_calc = now;
    float rpm = (rpm_accum / (float)PULSES_PER_REV) * (60.0f / dt);
    if (rpm_accum == 0 && target_rpm > 0.0f) stale_count++;
    else stale_count = 0;
    rpm_accum = 0;
    if (rpm < 0.0f) rpm = 0.0f;
    rpm_ema = (1.0f - MOTOR_EMA_ALPHA) * rpm_ema + MOTOR_EMA_ALPHA * rpm;
    last_rpm = rpm_ema;
  }

  // PID Control
  if (stale_count >= 1) {
    if (target_rpm > 0.0f) {
      int stale_pwm;
      if      (stale_count >= 5) stale_pwm = MOTOR_STALE_PWM_5;
      else if (stale_count >= 3) stale_pwm = MOTOR_STALE_PWM_3;
      else                       stale_pwm = MOTOR_STALE_PWM_1;
      pwm_out = stale_pwm;
      pid.integral = 0.0f;
      setMotor(pwm_out);
    }
  } else {
    const float dt_s = SAMPLE_MS / 1000.0f;
    pwm_out = pidUpdate(pid, last_rpm, target_rpm, KP, KI, KD, BASE_PWM, dt_s);
    setMotor(pwm_out);
  }

  // Report every 500ms
  if (now - last_report >= 500) {
    last_report = now;
    if (PLOTTER_MODE && PLOTTER_CSV) {
      Serial.print(last_rpm, 1);   Serial.print(",");
      Serial.print(target_rpm, 1); Serial.print(",");
      Serial.print(pwm_out);       Serial.print(",");
      Serial.println(enc_count);
    }
  }
}
