#pragma once

#include <Arduino.h>
#include "motor_config_types.h"
#include "motor_2_config.h"

extern volatile long motor1_enc_count;
extern volatile long motor2_enc_count;
extern volatile unsigned long motor1_last_isr_us;
extern volatile unsigned long motor2_last_isr_us;

inline void IRAM_ATTR onMotor1EncA() {
  unsigned long now = micros();
  if (now - motor1_last_isr_us < 150) return;
  motor1_last_isr_us = now;
  motor1_enc_count++;
}

inline void IRAM_ATTR onMotor2EncA() {
  unsigned long now = micros();
  if (now - motor2_last_isr_us < 150) return;
  motor2_last_isr_us = now;
  motor2_enc_count++;
}

inline void initMotorDriver(const MotorConfig &config) {
  pinMode(config.in1_pin, OUTPUT);
  pinMode(config.in2_pin, OUTPUT);
  ledcSetup(config.pwm_ch1, config.pwm_freq, config.pwm_res);
  ledcSetup(config.pwm_ch2, config.pwm_freq, config.pwm_res);
  ledcAttachPin(config.in1_pin, config.pwm_ch1);
  ledcAttachPin(config.in2_pin, config.pwm_ch2);
  pinMode(config.enc_a_pin, INPUT_PULLUP);
  pinMode(config.enc_b_pin, INPUT_PULLUP);
}

inline void attachMotorEncoder(const MotorConfig &config, void (*isr)()) {
  attachInterrupt(digitalPinToInterrupt(config.enc_a_pin), isr, RISING);
}

inline long readEncoderCount(volatile long &enc_count) {
  noInterrupts();
  long snapshot = enc_count;
  interrupts();
  return snapshot;
}

inline void setMotorPwm(const MotorConfig &config, int speed) {
  const bool invert_direction = (config.in1_pin == motor2_config::CONFIG.in1_pin &&
                                 config.in2_pin == motor2_config::CONFIG.in2_pin);
  const int applied_speed = invert_direction ? -speed : speed;

  if (applied_speed >= 0) {
    ledcWrite(config.pwm_ch1, applied_speed);
    ledcWrite(config.pwm_ch2, 0);
  } else {
    ledcWrite(config.pwm_ch1, 0);
    ledcWrite(config.pwm_ch2, -applied_speed);
  }
}
