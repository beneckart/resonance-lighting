// Small, native-testable field behavior gates used by the final 2026 burn
// image. Platform glue owns sensors, NVS, clocks, and physical loads.
#pragma once

#include <stdint.h>

static constexpr uint8_t FIELD_CHIME_CHANCE_DEFAULT = 255;
static constexpr uint16_t FIELD_PRESENCE_SEED_MIN_S_DEFAULT = 300;
static constexpr uint16_t FIELD_PRESENCE_REARM_CLEAR_S_DEFAULT = 30;
static constexpr uint8_t FIELD_SHOW_SCHEDULE_DEFAULT = 1;

struct PresenceSeedGate {
  bool armed;
  bool clearTracking;
  uint32_t lastAcceptedMs;
  uint32_t clearSinceMs;
};

void presenceSeedGateInit(PresenceSeedGate &gate);

// `rising` is the sensor gate's ordinary debounced edge and `active` is its
// current latched state. Returns at most one propagation/program seed until
// both the minimum interval and a continuous-clear re-arm have elapsed.
bool presenceSeedGateObserve(PresenceSeedGate &gate, bool rising, bool active,
                             uint32_t nowMs, uint16_t minimumIntervalS,
                             uint16_t clearRearmS);

struct PerimeterCloseHold {
  uint32_t lastReadSeq;
  uint32_t holdUntilMs;
  bool latched;
};

void perimeterCloseHoldInit(PerimeterCloseHold &hold);

// A valid close return is the only way to enter. Once entered, raw targets
// (including too-close/error-status targets) extend the center-pixel hold;
// an empty/invalid scene releases after a short grace. A valid farther return
// releases immediately so normal distance rings take over.
bool perimeterCloseHoldObserve(PerimeterCloseHold &hold, uint32_t readSeq,
                               bool validDistance, uint16_t distanceMm,
                               bool rawTargetPresent, uint32_t nowMs);

// 255 is deliberately exact 100%; other values are a 0..254/256 stochastic
// threshold. This makes 32 and 64 exact 12.5% and 25% options.
bool fieldChanceSelected(uint8_t chanceX256, uint32_t randomValue);

