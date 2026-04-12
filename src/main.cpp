#include <Arduino.h>
#include "motor_2_config.h"
#include "pid.h"
#include "motor_driver.h"
#include "test_mpu6050.h"
#include "test_pid_speed.h"

// XIAO ESP32-C3 Motor1 + Encoder1 only
const int IN1_PIN = MOTOR_IN1_PIN;
const int IN2_PIN = MOTOR_IN2_PIN;
const int ENC_A_PIN = MOTOR_ENC_A_PIN;
const int ENC_B_PIN = MOTOR_ENC_B_PIN;

// PWM settings
const int PWM_FREQ = MOTOR_PWM_FREQ;
const int PWM_RES  = MOTOR_PWM_RES;
const int PWM_CH1  = MOTOR_PWM_CH1;
const int PWM_CH2  = MOTOR_PWM_CH2;

volatile long enc_count = 0;

const bool PLOTTER_MODE = true;
const bool PLOTTER_CSV = true;
const bool RUN_PID_TEST = false;
const bool RUN_MPU6050_TEST = true;

void setup() {
  Serial.begin(115200);
  delay(200);
  if (PLOTTER_MODE && PLOTTER_CSV) {
    Serial.println("rpm,target_rpm,pwm,enc_count");
    Serial.println();
  }

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  ledcSetup(PWM_CH1, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH2, PWM_FREQ, PWM_RES);
  ledcAttachPin(IN1_PIN, PWM_CH1);
  ledcAttachPin(IN2_PIN, PWM_CH2);

  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), onEncA, CHANGE);

  if (RUN_MPU6050_TEST) {
    mpu6050_test::initMpu6050();
  }
}

void loop() {
  if (RUN_PID_TEST) {
    testPidSpeed();
  }

  if (RUN_MPU6050_TEST) {
    mpu6050_test::testMpu6050();
  }
}
