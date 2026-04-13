#pragma once

#include "motor_config_types.h"

// ============================================================
// Motor Config: DC Motor #1 (no gearbox, 9-slot encoder)
// Board: XIAO ESP32C3
// Driver: DRV8833
// EncA moved D6(GPIO21)->D10(GPIO10): GPIO21 is USB D+, unusable while USB connected
// ============================================================

namespace motor1_config {

constexpr MotorConfig CONFIG = {
	"M1",
	2,        // D0 -> IN1
	3,        // D1 -> IN2
	10,       // D10(GPIO10) -> Encoder A  (was D6/GPIO21=USB D+)
	7,        // GPIO7      -> Encoder B  (was D7/GPIO20=USB D-)
	0,
	1,
	20000,
	8,
	9,
};

}  // namespace motor1_config
