#pragma once

#include "motor_config_types.h"

// ============================================================
// Motor Config: DC Motor #2
// Board: XIAO ESP32C3
// Driver: DRV8833
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
	9,
};

}  // namespace motor2_config
