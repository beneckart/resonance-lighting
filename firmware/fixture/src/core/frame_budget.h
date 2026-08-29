// Pure frame-level LED current budget. The 37-pixel perimeter HEX may render
// any pattern, but its production wiring cannot support every channel at full
// scale. Sparse gobos remain full-bright; dense frames are scaled as a whole.
#pragma once

#include <stdint.h>

#include "fixture_context.h"

// Three full-scale RGB channel units: one white pixel or three saturated
// single-color pixels. This is intentionally expressed in linear channel
// units so it is independent of pattern shape and hue.
#define RES_HEX_RGB_CHANNEL_BUDGET (3u * 255u)

struct FramePowerBudget {
  uint8_t scale;
  bool currentLimited;
};

// Compose the hard power-policy cap with the dense-frame HEX limit. Point
// sources and RGBW profiles retain the caller's cap unchanged.
FramePowerBudget framePowerBudget(const FrameBuffer &frame,
                                  uint16_t physicalPixels, bool isRgbw,
                                  uint8_t brightnessCap);

// Match the LED driver's historical rounded scaling when the current budget
// is inactive. A current-limited frame rounds down so integer quantization can
// never push the physical output back over the limit.
uint8_t framePowerScaleChannel(uint8_t value,
                               const FramePowerBudget &budget);
