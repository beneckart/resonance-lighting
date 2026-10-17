#include <stdio.h>

#include "chime_light.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static int maxc(const uint8_t c[4]) { int m = c[0]; if (c[1] > m) m = c[1]; if (c[2] > m) m = c[2]; return m; }

int main() {
  const uint8_t a[3] = {0x9F, 0x26, 0xB0}, b[3] = {0x9F, 0x26, 0xBC};
  CHECK(chimeLightSeedHue(a) != chimeLightSeedHue(b));  // neighbours start apart

  ChimeLight c;
  uint8_t px[4];
  chimeLightNoteStrikes(c, 300, 0);  // first sight: no chime
  CHECK(!c.flashing && c.jumps == 0);
  chimeLightRgbw(c, 1000, px);
  CHECK(maxc(px) == CHIME_LIGHT_AMBIENT && px[3] == 0);  // ambient, no white

  // Ambient drifts: a quarter turn later the hue has moved.
  uint8_t later[4];
  chimeLightRgbw(c, 1000 + CHIME_LIGHT_TURN_MS / 4, later);
  CHECK(later[0] != px[0] || later[1] != px[1] || later[2] != px[2]);

  // A strike flashes and jumps the colour.
  chimeLightNoteStrikes(c, 301, 5000);
  CHECK(c.flashing && c.jumps == CHIME_LIGHT_JUMP);
  chimeLightRgbw(c, 5050, px);
  CHECK(maxc(px) == CHIME_LIGHT_FLASH && px[3] > 0);
  chimeLightRgbw(c, 5000 + CHIME_LIGHT_FLASH_MS + CHIME_LIGHT_FADE_MS / 2, px);
  CHECK(maxc(px) > CHIME_LIGHT_AMBIENT && maxc(px) < CHIME_LIGHT_FLASH);  // fading
  chimeLightRgbw(c, 5000 + CHIME_LIGHT_FLASH_MS + CHIME_LIGHT_FADE_MS + 10, px);
  CHECK(maxc(px) == CHIME_LIGHT_AMBIENT && px[3] == 0 && !c.flashing);
  // Several strikes in one tick are one jump; no strike, no change.
  chimeLightNoteStrikes(c, 305, 9000);
  CHECK(c.jumps == (uint8_t)(2 * CHIME_LIGHT_JUMP));
  chimeLightNoteStrikes(c, 305, 9100);
  CHECK(c.jumps == (uint8_t)(2 * CHIME_LIGHT_JUMP));
  // Same instant, different jump count = a visibly different colour.
  ChimeLight d = c; d.jumps = 0; d.flashing = false; c.flashing = false;
  uint8_t x[4], y[4];
  chimeLightRgbw(c, 20000, x); chimeLightRgbw(d, 20000, y);
  CHECK(x[0] != y[0] || x[1] != y[1] || x[2] != y[2]);
  return 0;
}
