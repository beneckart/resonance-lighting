#include "show_palette.h"

uint8_t showPaletteHue(uint8_t baseHue, bool synchronizedPalette,
                       bool utcValid, uint32_t utcS) {
  if (!synchronizedPalette || !utcValid) return baseHue;
  uint32_t slot = utcS / RES_AUTONOMOUS_PALETTE_SLOT_S;
  uint8_t step = (uint8_t)(slot % RES_AUTONOMOUS_PALETTE_STEPS);
  return (uint8_t)(baseHue + step * RES_AUTONOMOUS_PALETTE_HUE_STEP);
}
