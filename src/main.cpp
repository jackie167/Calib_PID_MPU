#include <Arduino.h>

#include "test_framework.h"
#include "web_control.h"

volatile long motor1_enc_count = 0;
volatile long motor2_enc_count = 0;
volatile unsigned long motor1_last_isr_us = 0;
volatile unsigned long motor2_last_isr_us = 0;

const bool RUN_TEST_FRAMEWORK = false;
const bool RUN_WEB_CONTROL = true;

void setup() {
  if (RUN_WEB_CONTROL) {
    web_control::init();
    return;
  }
  setupTestFramework();
}

void loop() {
  if (RUN_WEB_CONTROL) {
    web_control::loop();
    return;
  }
  loopTestFramework();
}
