#pragma once

#include <stdint.h>

enum WandColorMode : uint8_t {
  WAND_COLOR_RED = 0,
  WAND_COLOR_GREEN = 1,
  WAND_COLOR_BLUE = 2,
  WAND_COLOR_RGB = 3,
};

struct WandRgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

uint8_t wandGatherLoadedPixels(uint32_t elapsedMs, uint32_t gatherMs);
WandColorMode wandNextColor(WandColorMode color);
WandRgb wandLocalColor(WandColorMode color);
WandRgb wandTreeColor(WandColorMode color, uint8_t singleChannelValue,
                      uint8_t rgbChannelValue);
const char *wandColorName(WandColorMode color);
