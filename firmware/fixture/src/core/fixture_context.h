// Platform-independent fixture vocabulary shared by core logic and ESP32 glue.
// This header must stay compilable by native g++ (no Arduino includes).
#pragma once

#include <stdint.h>

// Fixture classes (ADR 0024/0027). Values are wire/NVS-stable: class_ovr in NVS
// and the class byte in telemetry use them directly. 0 stays "unknown/auto".
enum FixtureClass : uint8_t {
  FIXTURE_UNKNOWN = 0,
  FIXTURE_DOWNLIGHT = 1,  // 1 px RGBW + gobo; MSA311 + TMF8820 + optional BMP581
  FIXTURE_PERIMETER = 2,  // 37 px SK6812 HEX "dancing gobo"; VL53L5CX (out)
  FIXTURE_UPLIGHT = 3,    // 1 px 4 W RGBW; BMP581
  FIXTURE_CHANDELIER = 4, // no sensors; LED mix TBD (1 px RGBW safe default)
};

inline const char *fixtureClassName(uint8_t c) {
  switch (c) {
  case FIXTURE_DOWNLIGHT: return "downlight";
  case FIXTURE_PERIMETER: return "perimeter";
  case FIXTURE_UPLIGHT: return "uplight";
  case FIXTURE_CHANDELIER: return "chandelier";
  default: return "unknown";
  }
}

// Runtime profile (NVS `profile`, flipped live by NB_PROFILE / serial 'F').
// Dev favors reachability and flash turnaround on the bringup bench; prod
// favors energy. PROTECT behavior is identical in both -- dev must not be able
// to weaken battery protection.
enum FixtureProfile : uint8_t {
  PROFILE_DEV = 0,
  PROFILE_PROD = 1,
};

// ADR 0023 LED tiers, ordered by severity. OFF is the tier net_bench lacked:
// LEDs verifiably off with a duty-cycled OTA window, between DIM and PROTECT.
enum class LedTier : uint8_t {
  FULL = 0,
  DIM = 1,
  OFF = 2,
  PROTECT = 3,
};

// Boot-guard stages persisted in NVS (`fc_stage`). Same semantics as
// net_bench's FIELD_SESSION_* ladder with LEDS_OFF inserted; PROTECT stays the
// max value so "clamp stored stage to PROTECT" keeps working on downgrades.
enum BootStage : uint8_t {
  STAGE_IDLE = 0,
  STAGE_FULL = 1,
  STAGE_DIM = 2,
  STAGE_LEDS_OFF = 3,
  STAGE_PROTECT = 4,
};

// One rendered frame, class-agnostic: programs write RGBW quads (W ignored on
// GRB hex pixels), the LED driver maps to the physical module.
#define FRAME_MAX_PIXELS 37
struct FrameBuffer {
  uint8_t count; // live pixel count for the current class profile
  uint8_t px[FRAME_MAX_PIXELS][4]; // R,G,B,W (0-255, pre-gamma)
};

inline void frameClear(FrameBuffer &f) {
  for (int i = 0; i < FRAME_MAX_PIXELS; i++)
    f.px[i][0] = f.px[i][1] = f.px[i][2] = f.px[i][3] = 0;
}
