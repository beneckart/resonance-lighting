// M5Stack HEX 37-px geometry (7 rows 4-5-6-7-6-5-4, center = index 18),
// extracted verbatim from led_studio's proven buildGeometry(). Pure/native.
#pragma once

#include <stdint.h>

#define HEX_NUMPIXELS 37

struct HexGeometry {
  uint8_t ringOf[HEX_NUMPIXELS];    // 0 (center) .. 3 (outer)
  float pxAngle[HEX_NUMPIXELS];
  uint8_t spiralOrder[HEX_NUMPIXELS]; // ring-then-angle order (center outward)
  uint8_t ringMembers[4][18];         // per ring, sorted by angle
  uint8_t ringSize[4];
  float x[HEX_NUMPIXELS], y[HEX_NUMPIXELS];
};

// Idempotent; returns the singleton built on first call.
const HexGeometry &hexGeometry();

uint8_t hexNearestPixel(float x, float y);

// Map a monotonic step counter to a path position. Orbit wraps seamlessly
// around a ring; spiral ping-pongs (0..n-1..1..0) so it reverses at the ends.
int hexPathIndex(long step, int n, bool orbit);
