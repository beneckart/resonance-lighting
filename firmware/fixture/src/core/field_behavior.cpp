#include "field_behavior.h"

#include <string.h>

static constexpr uint16_t kPerimeterCloseMm = 380;
static constexpr uint32_t kPerimeterInvalidHoldMs = 1500;

void presenceSeedGateInit(PresenceSeedGate &gate) {
  memset(&gate, 0, sizeof(gate));
  gate.armed = true;
}

bool presenceSeedGateObserve(PresenceSeedGate &gate, bool rising, bool active,
                             uint32_t nowMs, uint16_t minimumIntervalS,
                             uint16_t clearRearmS) {
  uint32_t minimumMs = (uint32_t)minimumIntervalS * 1000UL;
  uint32_t clearMs = (uint32_t)clearRearmS * 1000UL;

  if (active) {
    gate.clearTracking = false;
  } else if (!gate.clearTracking) {
    gate.clearTracking = true;
    gate.clearSinceMs = nowMs;
  }

  if (!gate.armed) {
    bool intervalReady = nowMs - gate.lastAcceptedMs >= minimumMs;
    bool clearReady = gate.clearTracking &&
                      nowMs - gate.clearSinceMs >= clearMs;
    if (intervalReady && clearReady) gate.armed = true;
  }

  if (!rising || !gate.armed) return false;
  gate.armed = false;
  gate.lastAcceptedMs = nowMs;
  gate.clearTracking = false;
  return true;
}

void perimeterCloseHoldInit(PerimeterCloseHold &hold) {
  memset(&hold, 0, sizeof(hold));
}

bool perimeterCloseHoldObserve(PerimeterCloseHold &hold, uint32_t readSeq,
                               bool validDistance, uint16_t distanceMm,
                               bool rawTargetPresent, uint32_t nowMs) {
  if (readSeq && readSeq != hold.lastReadSeq) {
    hold.lastReadSeq = readSeq;
    if (validDistance) {
      if (distanceMm <= kPerimeterCloseMm) {
        hold.latched = true;
        hold.holdUntilMs = nowMs + kPerimeterInvalidHoldMs;
      } else {
        hold.latched = false;
      }
    } else if (hold.latched && rawTargetPresent) {
      hold.holdUntilMs = nowMs + kPerimeterInvalidHoldMs;
    }
  }
  if (hold.latched && !validDistance &&
      (int32_t)(nowMs - hold.holdUntilMs) >= 0)
    hold.latched = false;
  return hold.latched;
}

bool fieldChanceSelected(uint8_t chanceX256, uint32_t randomValue) {
  return chanceX256 == 255 ||
         (chanceX256 != 0 && (randomValue & 0xFFU) < chanceX256);
}

