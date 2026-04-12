#pragma once

// ============================================================
// Motor Config: DC Motor #1 (no gearbox, 9-slot encoder)
// Board: XIAO ESP32C3
// Driver: TB6612FNG
// Tuned: 2026-04-12
// Stable range: 1300-1500 RPM (motor shaft)
// ============================================================

// --- Pins ---
#define MOTOR_IN1_PIN     2
#define MOTOR_IN2_PIN     3
#define MOTOR_ENC_A_PIN   21
#define MOTOR_ENC_B_PIN   20

// --- PWM ---
#define MOTOR_PWM_CH1     0
#define MOTOR_PWM_CH2     1
#define MOTOR_PWM_FREQ    20000
#define MOTOR_PWM_RES     8

// --- Encoder ---
#define MOTOR_PULSES_PER_REV  18    // 9 slots × 2 edges (CHANGE interrupt)
#define MOTOR_RPM_WINDOW_MS   1200  // Averaging window (ms)
#define MOTOR_SAMPLE_MS       50    // PID sample rate (ms)

// --- PID Gains ---
#define MOTOR_KP        0.04f
#define MOTOR_KI        0.08f
#define MOTOR_KD        0.0f
#define MOTOR_BASE_PWM  135.0f      // Feedforward offset

// --- EMA Filter ---
#define MOTOR_EMA_ALPHA     0.15f   // New sample weight (0.85 historical)

// --- Stale Encoder Recovery PWM ---
#define MOTOR_STALE_PWM_1   (int)(MOTOR_BASE_PWM + 1300 * 0.015f)  // 155 (~1300 RPM)
#define MOTOR_STALE_PWM_3   170
#define MOTOR_STALE_PWM_5   190

// --- Operating Range ---
#define MOTOR_RPM_MIN   1300.0f     // Below this: encoder unreliable
#define MOTOR_RPM_MAX   5000.0f     // Above this: untested
#define MOTOR_RPM_STEP1 1300.0f     // Step profile low target
#define MOTOR_RPM_STEP2 1500.0f     // Step profile high target

// --- Notes ---
// - Motor max ~5700 RPM on shaft (no gearbox)
// - Encoder: 9-slot optical, ISR on CHANGE (single direction, no quadrature)
// - Dead zones occur below ~1200 RPM motor shaft
// - Encoder2 on motor2 originally misdiagnosed as broken (was ISR bug + low RPM targets)
// - Integral clamp: ±150
