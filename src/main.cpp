#include <Arduino.h>
#include "motor_driver.h"
#include "test_encoder.h"
#include "test_mpu6050.h"
#include "test_dual_motor_pid.h"
#include "wifi_logger.h"

volatile long motor1_enc_count = 0;
volatile long motor2_enc_count = 0;

const bool PLOTTER_MODE = true;
const bool PLOTTER_CSV = true;
const bool RUN_ENCODER_TEST = false;
const bool RUN_DUAL_PID_TEST = true;
const bool RUN_MPU6050_TEST = false;
const bool RUN_GPIO_RAW_TEST = false;
const bool RUN_WIFI_LOG = true;  // set false to skip WiFi (serial only)

void loopGpioRawTest() {
  static uint8_t prev7 = 255, prev10 = 255, prev8 = 255;
  static unsigned long count7 = 0, count10 = 0, count8 = 0;
  static unsigned long last_report = 0;
  uint8_t s7  = digitalRead(7);   // M1 candidate EncA/B
  uint8_t s10 = digitalRead(10);  // M1 candidate EncA/B
  uint8_t s8  = digitalRead(8);   // M2 EncA
  if (s7  != prev7)  { count7++;  prev7  = s7;  }
  if (s10 != prev10) { count10++; prev10 = s10; }
  if (s8  != prev8)  { count8++;  prev8  = s8;  }
  unsigned long now = millis();
  if (now - last_report >= 500) {
    last_report = now;
    Serial.print("gpio7="); Serial.print(s7);
    Serial.print(" t7="); Serial.print(count7);
    Serial.print("  gpio10="); Serial.print(s10);
    Serial.print(" t10="); Serial.print(count10);
    Serial.print("  gpio8="); Serial.print(s8);
    Serial.print(" t8="); Serial.println(count8);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  while (!Serial && millis() < 3000) delay(10);  // Wait for USB CDC

  if (RUN_WIFI_LOG) { wifi_logger::init(); }

  if (RUN_GPIO_RAW_TEST) {
    pinMode(7,  INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(8,  INPUT_PULLUP);
    Serial.println("[GPIO RAW TEST] Rotate motors by hand and watch transitions");
    return;
  }

  if (RUN_ENCODER_TEST) {
    initEncoderTest();
  }

  if (RUN_DUAL_PID_TEST) {
    initDualMotorPidTest();
  }

  if (RUN_MPU6050_TEST) {
    mpu6050_test::initMpu6050();
  }
}

void loop() {
  if (RUN_GPIO_RAW_TEST) {
    loopGpioRawTest();
    return;
  }

  if (RUN_ENCODER_TEST) {
    testEncoder();
  }

  if (RUN_DUAL_PID_TEST) {
    testDualMotorPid();
  }

  if (RUN_MPU6050_TEST) {
    mpu6050_test::testMpu6050();
  }
}
