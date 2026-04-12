#pragma once

#include "motor_config_types.h"

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

namespace motor2_config {

constexpr MotorConfig CONFIG = {
	"M2",
	4,        // D2 -> IN3
	5,        // D3 -> IN4
	8,        // D8 -> Encoder A
	9,        // D9 -> Encoder B
	2,
	3,
	20000,
	8,
	18,
	500,
	50,
	0.04f,
	0.05f,
	0.0f,
	107.0f,
	0.30f,
	95,
	115,
	130,
	3000.0f,
	8000.0f,
	5000.0f,
	6000.0f,
};

}  // namespace motor2_config
