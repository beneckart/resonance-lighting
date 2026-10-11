#include "wand_gesture.h"

#include <math.h>
#include <string.h>

static bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t intervalMs) {
  return (uint32_t)(nowMs - sinceMs) >= intervalMs;
}

static bool validSample(bool sensorsValid, float movementG,
                        float pressureHpa) {
  return sensorsValid && isfinite(movementG) && movementG >= 0.0f &&
         isfinite(pressureHpa) && pressureHpa >= 300.0f &&
         pressureHpa <= 1200.0f;
}

static void seekFromSample(WandGesture &gesture, uint32_t nowMs,
                           float pressureHpa) {
  uint32_t triggers = gesture.triggerCount;
  memset(&gesture, 0, sizeof(gesture));
  gesture.state = WAND_GESTURE_SEEK_STEADY;
  gesture.stateSinceMs = nowMs;
  gesture.baselinePressureHpa = pressureHpa;
  gesture.triggerCount = triggers;
}

void wandGestureInit(WandGesture &gesture) {
  memset(&gesture, 0, sizeof(gesture));
  gesture.state = WAND_GESTURE_SEEK_STEADY;
}

float wandPressureRiseM(float baselinePressureHpa, float pressureHpa) {
  if (!isfinite(baselinePressureHpa) || !isfinite(pressureHpa) ||
      baselinePressureHpa <= 0.0f || pressureHpa <= 0.0f)
    return 0.0f;
  float ratio = pressureHpa / baselinePressureHpa;
  return 44330.0f * (1.0f - powf(ratio, 0.19029495f));
}

bool wandGestureSample(WandGesture &gesture, uint32_t nowMs,
                       bool sensorsValid, float movementG,
                       float pressureHpa) {
  if (!validSample(sensorsValid, movementG, pressureHpa)) {
    wandGestureInit(gesture);
    return false;
  }

  const bool steady = movementG <= WAND_STEADY_MAX_G;
  float riseM = wandPressureRiseM(gesture.baselinePressureHpa, pressureHpa);

  switch (gesture.state) {
  case WAND_GESTURE_SEEK_STEADY:
    if (!steady) {
      gesture.stateSinceMs = nowMs;
      gesture.baselinePressureHpa = pressureHpa;
      return false;
    }
    if (gesture.baselinePressureHpa <= 0.0f) {
      gesture.baselinePressureHpa = pressureHpa;
      gesture.stateSinceMs = nowMs;
    } else {
      // Track slow weather drift only while seeking the deliberate still hold.
      gesture.baselinePressureHpa +=
          0.05f * (pressureHpa - gesture.baselinePressureHpa);
    }
    if (elapsed(nowMs, gesture.stateSinceMs, WAND_STEADY_ARM_MS)) {
      gesture.state = WAND_GESTURE_ARMED;
      gesture.stateSinceMs = nowMs;
      gesture.peakMotionG = 0.0f;
      gesture.maxRiseM = 0.0f;
    }
    return false;

  case WAND_GESTURE_ARMED:
    if (elapsed(nowMs, gesture.stateSinceMs, WAND_ARM_TIMEOUT_MS)) {
      seekFromSample(gesture, nowMs, pressureHpa);
      return false;
    }
    if (movementG >= WAND_REJECT_SHOCK_G) {
      seekFromSample(gesture, nowMs, pressureHpa);
      return false;
    }
    if (movementG >= WAND_LIFT_MOTION_MIN_G) {
      gesture.state = WAND_GESTURE_LIFTING;
      gesture.stateSinceMs = nowMs;
      gesture.liftStartedMs = nowMs;
      gesture.peakMotionG = movementG;
      gesture.maxRiseM = riseM;
    }
    return false;

  case WAND_GESTURE_LIFTING:
    if (movementG > gesture.peakMotionG) gesture.peakMotionG = movementG;
    if (riseM > gesture.maxRiseM) gesture.maxRiseM = riseM;
    if (movementG >= WAND_REJECT_SHOCK_G ||
        elapsed(nowMs, gesture.liftStartedMs, WAND_LIFT_TIMEOUT_MS)) {
      seekFromSample(gesture, nowMs, pressureHpa);
      return false;
    }
    if (riseM >= WAND_RISE_TRIGGER_M && steady) {
      gesture.state = WAND_GESTURE_HIGH_HOLD;
      gesture.stateSinceMs = nowMs;
    }
    return false;

  case WAND_GESTURE_HIGH_HOLD:
    if (movementG >= WAND_REJECT_SHOCK_G ||
        elapsed(nowMs, gesture.liftStartedMs, WAND_LIFT_TIMEOUT_MS)) {
      seekFromSample(gesture, nowMs, pressureHpa);
      return false;
    }
    if (!steady || riseM < WAND_RISE_HOLD_M) {
      gesture.state = WAND_GESTURE_LIFTING;
      gesture.stateSinceMs = nowMs;
      return false;
    }
    if (elapsed(nowMs, gesture.stateSinceMs, WAND_HIGH_HOLD_MS) &&
        gesture.peakMotionG >= WAND_LIFT_MOTION_MIN_G &&
        gesture.maxRiseM >= WAND_RISE_TRIGGER_M) {
      gesture.state = WAND_GESTURE_WAIT_RETURN;
      gesture.stateSinceMs = nowMs;
      gesture.triggeredMs = nowMs;
      ++gesture.triggerCount;
      return true;
    }
    return false;

  case WAND_GESTURE_WAIT_RETURN:
    // Normally the wand is lowered near its baseline before another gesture.
    // If it is intentionally kept at a new height, five stable seconds rebase
    // there; holding it high can never create repeated triggers by itself.
    if (!steady) {
      gesture.stateSinceMs = nowMs;
      return false;
    }
    if (fabsf(riseM) <= WAND_RETURN_BAND_M ||
        elapsed(nowMs, gesture.stateSinceMs, WAND_REBASE_HOLD_MS))
      seekFromSample(gesture, nowMs, pressureHpa);
    return false;
  }
  wandGestureInit(gesture);
  return false;
}

const char *wandGestureStateName(WandGestureState state) {
  switch (state) {
  case WAND_GESTURE_SEEK_STEADY: return "seek-steady";
  case WAND_GESTURE_ARMED: return "armed";
  case WAND_GESTURE_LIFTING: return "lifting";
  case WAND_GESTURE_HIGH_HOLD: return "high-hold";
  case WAND_GESTURE_WAIT_RETURN: return "wait-return";
  }
  return "unknown";
}
