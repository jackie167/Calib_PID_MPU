#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace mpu6050_test {

constexpr uint8_t MPU6050_ADDR       = 0x68;
constexpr uint8_t REG_WHO_AM_I       = 0x75;
constexpr uint8_t REG_PWR_MGMT_1     = 0x6B;
constexpr uint8_t REG_ACCEL_CONFIG   = 0x1C;
constexpr uint8_t REG_GYRO_CONFIG    = 0x1B;
constexpr uint8_t REG_ACCEL_XOUT_H   = 0x3B;
constexpr uint8_t EXPECTED_WHO_AM_I  = 0x68;

inline bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

inline bool readBytes(uint8_t start_reg, uint8_t *buffer, size_t len) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(start_reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t read_len = Wire.requestFrom((int)MPU6050_ADDR, (int)len);
  if (read_len != len) {
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    buffer[i] = Wire.read();
  }
  return true;
}

inline int16_t toInt16(uint8_t msb, uint8_t lsb) {
  return (int16_t)((msb << 8) | lsb);
}

inline bool initMpu6050() {
  Wire.begin();
  delay(50);

  uint8_t who_am_i = 0;
  if (!readBytes(REG_WHO_AM_I, &who_am_i, 1)) {
    Serial.println("[MPU] I2C read failed");
    return false;
  }

  if (who_am_i != EXPECTED_WHO_AM_I) {
    Serial.printf("[MPU] Unexpected WHO_AM_I: 0x%02X\n", who_am_i);
    return false;
  }

  if (!writeRegister(REG_PWR_MGMT_1, 0x00)) {
    Serial.println("[MPU] Wake-up failed");
    return false;
  }

  if (!writeRegister(REG_ACCEL_CONFIG, 0x00)) {
    Serial.println("[MPU] Accel config failed");
    return false;
  }

  if (!writeRegister(REG_GYRO_CONFIG, 0x00)) {
    Serial.println("[MPU] Gyro config failed");
    return false;
  }

  Serial.println("[MPU] MPU6050 ready");
  Serial.println("ax_g,ay_g,az_g,temp_c,gx_dps,gy_dps,gz_dps");
  return true;
}

inline void testMpu6050() {
  static unsigned long last_report = 0;
  const unsigned long report_period_ms = 200;

  unsigned long now = millis();
  if (now - last_report < report_period_ms) {
    return;
  }
  last_report = now;

  uint8_t raw[14];
  if (!readBytes(REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
    Serial.println("[MPU] Sensor read failed");
    return;
  }

  int16_t ax_raw   = toInt16(raw[0], raw[1]);
  int16_t ay_raw   = toInt16(raw[2], raw[3]);
  int16_t az_raw   = toInt16(raw[4], raw[5]);
  int16_t temp_raw = toInt16(raw[6], raw[7]);
  int16_t gx_raw   = toInt16(raw[8], raw[9]);
  int16_t gy_raw   = toInt16(raw[10], raw[11]);
  int16_t gz_raw   = toInt16(raw[12], raw[13]);

  float ax_g   = ax_raw / 16384.0f;
  float ay_g   = ay_raw / 16384.0f;
  float az_g   = az_raw / 16384.0f;
  float temp_c = (temp_raw / 340.0f) + 36.53f;
  float gx_dps = gx_raw / 131.0f;
  float gy_dps = gy_raw / 131.0f;
  float gz_dps = gz_raw / 131.0f;

  Serial.print(ax_g, 3);
  Serial.print(',');
  Serial.print(ay_g, 3);
  Serial.print(',');
  Serial.print(az_g, 3);
  Serial.print(',');
  Serial.print(temp_c, 2);
  Serial.print(',');
  Serial.print(gx_dps, 2);
  Serial.print(',');
  Serial.print(gy_dps, 2);
  Serial.print(',');
  Serial.println(gz_dps, 2);
}

}  // namespace mpu6050_test
