#pragma once

#include <Arduino.h>

#include "motor_1_config.h"
#include "motor_2_config.h"
#include "motor_driver.h"
#include "pid.h"

extern const bool PLOTTER_MODE;
extern const bool PLOTTER_CSV;

struct MotorPidRuntime {
  const MotorConfig *config;
  volatile long *enc_count;
  PIDState pid;
  unsigned long last_sample;
  unsigned long last_rpm_calc;
  long last_count;
  float last_rpm;
  float rpm_ema;
  long rpm_accum;
  int pwm_out;
  int stale_count;
  float kp;
  float ki;
  float kd;
  float target_rpm;
};

inline void initMotorPidRuntime(MotorPidRuntime &runtime,
                                const MotorConfig &config,
                                volatile long &enc_count) {
  runtime.config = &config;
  runtime.enc_count = &enc_count;
  runtime.pid = PIDState{};
  runtime.last_sample = 0;
  runtime.last_rpm_calc = 0;
  runtime.last_count = 0;
  runtime.last_rpm = 0.0f;
  runtime.rpm_ema = 0.0f;
  runtime.rpm_accum = 0;
  runtime.pwm_out = 0;
  runtime.stale_count = 0;
  runtime.kp = config.kp;
  runtime.ki = config.ki;
  runtime.kd = config.kd;
  runtime.target_rpm = 0.0f;
}

inline float stepTargetFor(const MotorConfig &config, unsigned long elapsed_ms) {
  if (elapsed_ms < 3000) {
    return 0.0f;
  }
  if (elapsed_ms < 11000) {
    return config.rpm_step1;
  }
  if (elapsed_ms < 19000) {
    return config.rpm_step2;
  }
  return config.rpm_step1;
}

inline void updateMotorPid(MotorPidRuntime &runtime, unsigned long now) {
  const MotorConfig &config = *runtime.config;

  if (now - runtime.last_sample >= (unsigned long)config.sample_ms) {
    runtime.last_sample = now;
    long count_snapshot = readEncoderCount(*runtime.enc_count);
    long delta = count_snapshot - runtime.last_count;
    runtime.last_count = count_snapshot;
    if (delta < 0) {
      delta = -delta;
    }
    runtime.rpm_accum += delta;
  }

  if (now - runtime.last_rpm_calc >= (unsigned long)config.rpm_window_ms) {
    float dt_s = (now - runtime.last_rpm_calc) / 1000.0f;
    runtime.last_rpm_calc = now;
    float rpm = 0.0f;
    if (dt_s > 0.0f) {
      rpm = (runtime.rpm_accum / (float)config.pulses_per_rev) * (60.0f / dt_s);
    }

    if (runtime.rpm_accum == 0 && runtime.target_rpm > 0.0f) {
      runtime.stale_count++;
    } else {
      runtime.stale_count = 0;
    }
    runtime.rpm_accum = 0;

    if (rpm < 0.0f) {
      rpm = 0.0f;
    }

    runtime.rpm_ema = (1.0f - config.ema_alpha) * runtime.rpm_ema + config.ema_alpha * rpm;
    runtime.last_rpm = runtime.rpm_ema;
  }

  if (runtime.target_rpm <= 0.0f) {
    runtime.pid = PIDState{};
    runtime.pwm_out = 0;
    setMotorPwm(config, runtime.pwm_out);
    return;
  }

  if (runtime.stale_count >= 1) {
    int stale_pwm = config.stale_pwm_1;
    if (runtime.stale_count >= 5) {
      stale_pwm = config.stale_pwm_5;
    } else if (runtime.stale_count >= 3) {
      stale_pwm = config.stale_pwm_3;
    }
    runtime.pwm_out = stale_pwm;
    runtime.pid.integral = 0.0f;
    setMotorPwm(config, runtime.pwm_out);
    return;
  }

  const float dt_s = config.sample_ms / 1000.0f;
  runtime.pwm_out = pidUpdate(runtime.pid,
                              runtime.last_rpm,
                              runtime.target_rpm,
                              runtime.kp,
                              runtime.ki,
                              runtime.kd,
                              config.base_pwm,
                              dt_s);
  setMotorPwm(config, runtime.pwm_out);
}

inline void initDualMotorPidTest() {
  initMotorDriver(motor1_config::CONFIG);
  initMotorDriver(motor2_config::CONFIG);
  attachMotorEncoder(motor1_config::CONFIG, onMotor1EncA);
  attachMotorEncoder(motor2_config::CONFIG, onMotor2EncA);

  if (PLOTTER_MODE && PLOTTER_CSV) {
    Serial.println("m1_rpm,m1_target,m1_pwm,m1_enc,m2_rpm,m2_target,m2_pwm,m2_enc");
    Serial.println();
  }
}

inline void testDualMotorPid() {
  static bool initialized = false;
  static unsigned long step_start = 0;
  static unsigned long last_plotter_header = 0;
  static unsigned long last_report = 0;
  static MotorPidRuntime motor1;
  static MotorPidRuntime motor2;

  if (!initialized) {
    initMotorPidRuntime(motor1, motor1_config::CONFIG, motor1_enc_count);
    initMotorPidRuntime(motor2, motor2_config::CONFIG, motor2_enc_count);
    initialized = true;
  }

  unsigned long now = millis();
  if (step_start == 0 || now - step_start >= 19000) {
    step_start = now;
  }

  unsigned long elapsed_ms = now - step_start;
  motor1.target_rpm = stepTargetFor(*motor1.config, elapsed_ms);
  motor2.target_rpm = stepTargetFor(*motor2.config, elapsed_ms);

  updateMotorPid(motor1, now);
  updateMotorPid(motor2, now);

  if (PLOTTER_MODE && PLOTTER_CSV) {
    if (last_plotter_header == 0 || now - last_plotter_header >= 5000) {
      Serial.println("m1_rpm,m1_target,m1_pwm,m1_enc,m2_rpm,m2_target,m2_pwm,m2_enc");
      Serial.println();
      last_plotter_header = now;
    }
  }

  if (now - last_report >= 500) {
    last_report = now;
    if (PLOTTER_MODE && PLOTTER_CSV) {
      Serial.print(motor1.last_rpm, 1);
      Serial.print(',');
      Serial.print(motor1.target_rpm, 1);
      Serial.print(',');
      Serial.print(motor1.pwm_out);
      Serial.print(',');
      Serial.print(motor1_enc_count);
      Serial.print(',');
      Serial.print(motor2.last_rpm, 1);
      Serial.print(',');
      Serial.print(motor2.target_rpm, 1);
      Serial.print(',');
      Serial.print(motor2.pwm_out);
      Serial.print(',');
      Serial.println(motor2_enc_count);
    }
  }
}
