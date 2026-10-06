#pragma once

#include <stdint.h>

// Deliberate no-button gesture for Steve's Magic Wand:
//
//   hold still -> lift at least 0.45 m -> hold still high
//
// The MSA311 must observe real translation during the lift and the BMP581 must
// independently observe the height change. A bump, pressure-only drift, or
// motion without a rise cannot fire. Thresholds are intentionally exposed as
// constants because the physical wand still needs a measured tuning pass.

enum WandGestureState : uint8_t {
  WAND_GESTURE_SEEK_STEADY = 0,
  WAND_GESTURE_ARMED = 1,
  WAND_GESTURE_LIFTING = 2,
  WAND_GESTURE_HIGH_HOLD = 3,
  WAND_GESTURE_WAIT_RETURN = 4,
};

static constexpr uint32_t WAND_STEADY_ARM_MS = 1600;
static constexpr uint32_t WAND_ARM_TIMEOUT_MS = 6000;
static constexpr uint32_t WAND_LIFT_TIMEOUT_MS = 6000;
static constexpr uint32_t WAND_HIGH_HOLD_MS = 800;
static constexpr uint32_t WAND_REBASE_HOLD_MS = 5000;
static constexpr float WAND_STEADY_MAX_G = 0.035f;
static constexpr float WAND_LIFT_MOTION_MIN_G = 0.045f;
static constexpr float WAND_REJECT_SHOCK_G = 0.90f;
static constexpr float WAND_RISE_TRIGGER_M = 0.45f;
static constexpr float WAND_RISE_HOLD_M = 0.35f;
static constexpr float WAND_RETURN_BAND_M = 0.20f;

struct WandGesture {
  WandGestureState state;
  uint32_t stateSinceMs;
  uint32_t liftStartedMs;
  uint32_t triggeredMs;
  float baselinePressureHpa;
  float peakMotionG;
  float maxRiseM;
  uint32_t triggerCount;
};

void wandGestureInit(WandGesture &gesture);

// Positive result means exactly one complete gesture was accepted on this
// sample. `movementG` is the MSA311 gravity-removed envelope. Pressure must be
// filtered BMP581 pressure in hPa. Invalid/stale input resets the recognizer.
bool wandGestureSample(WandGesture &gesture, uint32_t nowMs,
                       bool sensorsValid, float movementG,
                       float pressureHpa);

float wandPressureRiseM(float baselinePressureHpa, float pressureHpa);

const char *wandGestureStateName(WandGestureState state);
