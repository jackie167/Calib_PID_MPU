#pragma once
#include <Arduino.h>
#include "motor_2_config.h"

extern volatile long enc_count;

void IRAM_ATTR onEncA() {
  enc_count++;
}

// Motor control: speed 0-255 (positive = forward, negative = backward)
void setMotor(int speed) {
  if (speed >= 0) {
    ledcWrite(MOTOR_PWM_CH1, speed);
    ledcWrite(MOTOR_PWM_CH2, 0);
  } else {
    ledcWrite(MOTOR_PWM_CH1, 0);
    ledcWrite(MOTOR_PWM_CH2, -speed);
  }
}
