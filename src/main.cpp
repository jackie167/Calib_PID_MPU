#include <Arduino.h>

#include "test_framework.h"

volatile long motor1_enc_count = 0;
volatile long motor2_enc_count = 0;
volatile unsigned long motor1_last_isr_us = 0;
volatile unsigned long motor2_last_isr_us = 0;

void setup() {
  setupTestFramework();
}

void loop() {
  loopTestFramework();
}
