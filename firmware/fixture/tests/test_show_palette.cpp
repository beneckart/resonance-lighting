#include <assert.h>
#include <stdio.h>

#include "show_palette.h"

int main() {
  const uint8_t base = 160;

  assert(showPaletteHue(base, false, true, 1200) == base);
  assert(showPaletteHue(base, true, false, 1200) == base);
  assert(showPaletteHue(base, true, true, 0) == base);

  uint8_t first = showPaletteHue(base, true, true, 1200);
  assert(first == (uint8_t)(base + RES_AUTONOMOUS_PALETTE_HUE_STEP));
  assert(showPaletteHue(base, true, true, 2399) == first);
  assert(showPaletteHue(base, true, true, 2400) != first);

  // Six 20-minute slots make a deterministic two-hour palette loop.
  assert(showPaletteHue(base, true, true,
                        RES_AUTONOMOUS_PALETTE_SLOT_S *
                            RES_AUTONOMOUS_PALETTE_STEPS) == base);

  // Every fixture observes the same step boundary even if its configured hue
  // intentionally uses a different phase/tint.
  uint8_t perimeterBase = 30;
  assert((uint8_t)(showPaletteHue(base, true, true, 2400) - base) ==
         (uint8_t)(showPaletteHue(perimeterBase, true, true, 2400) -
                   perimeterBase));

  printf("show palette tests passed\n");
  return 0;
}
