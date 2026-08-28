// Physical LED format selected by the wire/NVS-stable fixture class.
// Platform-independent so native tests pin the class-to-module contract.
#pragma once

#include "fixture_context.h"

struct FixtureLedProfile {
  uint8_t pixelCount;
  bool rgbw;
};

inline FixtureLedProfile fixtureLedProfile(uint8_t fixtureClass) {
  switch (fixtureClass) {
  case FIXTURE_PERIMETER:
    return {(uint8_t)FRAME_MAX_PIXELS, false}; // 37-pixel SK6812 GRB HEX
  case FIXTURE_UPLIGHT:
    return {1, false}; // one lensed 3 W SK6812 GRB point source
  default:
    // Downlights are one 4 W RGBW point source. Chandelier hardware is not yet
    // deployed; retain the historical one-pixel RGBW safe default for it and
    // unknown/auto until an exact hardware roster decides otherwise.
    return {1, true};
  }
}
