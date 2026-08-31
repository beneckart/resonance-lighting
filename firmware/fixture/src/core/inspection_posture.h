// Temporary art-inspection posture: static maximum-visibility light with a
// bounded radio duty cycle. Platform-independent scheduling lives here so the
// visible rail never has to deep-sleep merely to turn WiFi off.
#pragma once

#include <stdint.h>

#ifndef RES_STATIC_INSPECTION
#define RES_STATIC_INSPECTION 1
#endif

constexpr bool inspectionStaticEnabled() {
  return RES_STATIC_INSPECTION != 0;
}

enum InspectionRadioAction : uint8_t {
  INSPECTION_RADIO_KEEP = 0,
  INSPECTION_RADIO_PAUSE = 1,
  INSPECTION_RADIO_RESUME = 2,
};

struct InspectionRadioDutyState {
  bool active;
  bool radioOn;
  uint32_t phaseStartMs;
};

void inspectionRadioDutyInit(InspectionRadioDutyState &state);

// Eligible means scheduled inspection light is active with trustworthy UTC.
// The first phase is always an ON/listen window. Turning eligibility off
// immediately requests radio restoration if the current phase is paused.
InspectionRadioAction inspectionRadioDutyTick(
    InspectionRadioDutyState &state, bool eligible, uint32_t nowMs,
    uint32_t listenMs, uint32_t radioOffMs);
