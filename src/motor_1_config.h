#pragma once

#include "motor_config_types.h"

// ============================================================
// Motor Config: DC Motor #1 (no gearbox, 9-slot encoder)
// Board: XIAO ESP32C3
// Driver: TB6612FNG
// Tuned: 2026-04-12
// Stable range: 4000-5000 RPM (no-load, bench supply)
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
	18,
	500,
	50,
	0.02f,
	0.05f,
	0.0f,
	113.0f,
	0.30f,
	100,
	115,
	130,
	3000.0f,
	8000.0f,
	5000.0f,
	6000.0f,
};

}  // namespace motor1_config

// --- Notes ---
// - Motor max ~5700 RPM on shaft (no gearbox)
// - Encoder: 9-slot optical, ISR on CHANGE (single direction, no quadrature)
// - Dead zones occur below ~1200 RPM motor shaft
// - Encoder2 on motor2 originally misdiagnosed as broken (was ISR bug + low RPM targets)
// - Integral clamp: ±150
