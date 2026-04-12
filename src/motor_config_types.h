#pragma once

#include <Arduino.h>

struct MotorConfig {
  const char *name;
  uint8_t in1_pin;
  uint8_t in2_pin;
  uint8_t enc_a_pin;
  uint8_t enc_b_pin;
  uint8_t pwm_ch1;
  uint8_t pwm_ch2;
  uint32_t pwm_freq;
  uint8_t pwm_res;
  int pulses_per_rev;
  int rpm_window_ms;
  int sample_ms;
  float kp;
  float ki;
  float kd;
  float base_pwm;
  float ema_alpha;
  int stale_pwm_1;
  int stale_pwm_3;
  int stale_pwm_5;
  float rpm_min;
  float rpm_max;
  float rpm_step1;
  float rpm_step2;
};
