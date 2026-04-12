#pragma once
#include <math.h>

struct PIDState {
  float integral    = 0.0f;
  float last_error  = 0.0f;
  float prev_target = -1.0f;
};

// Returns PWM output given current RPM, target RPM, and gains.
// BASE_PWM: feedforward offset added to PID output.
// integral_limit: max abs value of integral term.
// Resets integral when target changes (anti-windup).
inline int pidUpdate(PIDState &state,
                     float current_rpm,
                     float target_rpm,
                     float kp, float ki, float kd,
                     float base_pwm,
                     float dt_s,
                     float integral_limit = 150.0f)
{
  // Reset integral on target change
  if (target_rpm != state.prev_target) {
    state.integral   = 0.0f;
    state.last_error = 0.0f;
    state.prev_target = target_rpm;
  }

  float error = target_rpm - current_rpm;

  // Proportional
  float output = kp * error;

  // Integral
  state.integral += error * dt_s;
  if (state.integral >  integral_limit) state.integral =  integral_limit;
  if (state.integral < -integral_limit) state.integral = -integral_limit;
  output += ki * state.integral;

  // Derivative
  float derivative = (error - state.last_error) / dt_s;
  output += kd * derivative;
  state.last_error = error;

  // PWM output clamped to 0-255
  int pwm = (int)(base_pwm + output);
  if (pwm > 255) pwm = 255;
  if (pwm < 0)   pwm = 0;
  return pwm;
}
