#pragma once

// ============================================================
// Motor Config: DC Motor #2
// Board: XIAO ESP32C3
// Driver: TB6612FNG
// Tuned: 2026-04-12
// Status: CONFIRMED WORKING — gains transferred from Motor1
// Stable range: 1300-1500 RPM (motor shaft), same hardware as Motor1
// Encoder: CONFIRMED WORKING — original "broken" diagnosis was false positive
//          (caused by ISR bug + RPM targets too low in old dual-motor code)
// ============================================================

// --- Pins (same as Motor1) ---
#define MOTOR_IN1_PIN     2
#define MOTOR_IN2_PIN     3
#define MOTOR_ENC_A_PIN   21
#define MOTOR_ENC_B_PIN   20

// --- PWM ---
#define MOTOR_PWM_CH1     0
#define MOTOR_PWM_CH2     1
#define MOTOR_PWM_FREQ    20000
#define MOTOR_PWM_RES     8

// --- Encoder (same 9-slot as Motor1) ---
#define MOTOR_PULSES_PER_REV  18
#define MOTOR_RPM_WINDOW_MS   1200
#define MOTOR_SAMPLE_MS       50

// --- PID Gains (confirmed working — same as Motor1) ---
#define MOTOR_KP        0.04f
#define MOTOR_KI        0.08f
#define MOTOR_KD        0.0f
#define MOTOR_BASE_PWM  135.0f

// --- EMA Filter ---
#define MOTOR_EMA_ALPHA     0.15f

// --- Stale Encoder Recovery PWM ---
#define MOTOR_STALE_PWM_1   155
#define MOTOR_STALE_PWM_3   170
#define MOTOR_STALE_PWM_5   190

// --- Operating Range (confirmed) ---
#define MOTOR_RPM_MIN   1000.0f
#define MOTOR_RPM_MAX   5000.0f
#define MOTOR_RPM_STEP1 1300.0f
#define MOTOR_RPM_STEP2 1500.0f
