// Gamma with a dim floor. Adafruit's gamma8 (2.6 curve) maps inputs 1..23 to 0
// -- the bottom ~9% quantizes to OFF, exactly where the ambient-dim spec lives
// (POWERFEATHER_NOTES "8-bit gamma dead zone"). resGamma8 keeps the curve but
// never maps a nonzero intent to zero.
#pragma once

#include <stdint.h>
#include <math.h>

inline uint8_t resGamma8(uint8_t v) {
  static uint8_t table[256];
  static bool built = false;
  if (!built) {
    for (int i = 0; i < 256; i++) {
      float g = powf((float)i / 255.0f, 2.6f) * 255.0f + 0.5f;
      uint8_t out = (uint8_t)(g > 255.0f ? 255.0f : g);
      table[i] = (i > 0 && out == 0) ? 1 : out; // dim floor
    }
    built = true;
  }
  return table[v];
}
