// Deterministic autonomous palette cadence. Sparse UTC anchors let every
// fixture change together without adding a new mesh command or coordinator.
#pragma once

#include <stdint.h>

#define RES_AUTONOMOUS_PALETTE_SLOT_S 1200UL
#define RES_AUTONOMOUS_PALETTE_STEPS 6u
#define RES_AUTONOMOUS_PALETTE_HUE_STEP 43u

// The configured program hue remains the base/phase choice. Invalid time or
// bridge-owned rendering preserves that hue exactly.
uint8_t showPaletteHue(uint8_t baseHue, bool synchronizedPalette,
                       bool utcValid, uint32_t utcS);
