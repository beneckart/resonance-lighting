#include "wand_visual.h"

uint8_t wandGatherLoadedPixels(uint32_t elapsedMs, uint32_t gatherMs) {
  if (gatherMs == 0 || elapsedMs >= gatherMs) return 37;
  return (uint8_t)(((uint64_t)elapsedMs * 37u) / gatherMs);
}

WandColorMode wandNextColor(WandColorMode color) {
  return color == WAND_COLOR_RGB
             ? WAND_COLOR_RED
             : (WandColorMode)((uint8_t)color + 1);
}

WandRgb wandLocalColor(WandColorMode color) {
  // Steve already validated all-740-pixel fills at channel value 8. Keep the
  // single-channel selections at 6 and RGB at 2+2+2 for the same RGB sum.
  switch (color) {
  case WAND_COLOR_RED: return {6, 0, 0};
  case WAND_COLOR_GREEN: return {0, 6, 0};
  case WAND_COLOR_BLUE: return {0, 0, 6};
  case WAND_COLOR_RGB: return {2, 2, 2};
  }
  return {0, 0, 0};
}

WandRgb wandTreeColor(WandColorMode color, uint8_t singleChannelValue,
                      uint8_t rgbChannelValue) {
  switch (color) {
  case WAND_COLOR_RED: return {singleChannelValue, 0, 0};
  case WAND_COLOR_GREEN: return {0, singleChannelValue, 0};
  case WAND_COLOR_BLUE: return {0, 0, singleChannelValue};
  case WAND_COLOR_RGB:
    return {rgbChannelValue, rgbChannelValue, rgbChannelValue};
  }
  return {0, 0, 0};
}

const char *wandColorName(WandColorMode color) {
  switch (color) {
  case WAND_COLOR_RED: return "red";
  case WAND_COLOR_GREEN: return "green";
  case WAND_COLOR_BLUE: return "blue";
  case WAND_COLOR_RGB: return "rgb-dim";
  }
  return "unknown";
}
