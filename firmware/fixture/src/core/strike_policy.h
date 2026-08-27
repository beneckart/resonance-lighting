#pragma once

#include <stdint.h>

enum class StrikeOrigin : uint8_t {
  OPERATOR_CONTROL = 0,
  AUTONOMOUS_PROGRAM = 1,
};

// A deliberate control-plane knock is an attempt, not a promise of motion.
// It bypasses renewable/lifecycle/tier qualification and still flows through
// solenoidStrike(), which owns arm, rest, pulse, load-marker, and failsafe
// enforcement. Autonomous programs retain the energy permission.
bool strikePolicyMayAttempt(StrikeOrigin origin, bool energyPermitted);
