#include "test_util.h"

#include <cmath>
#include "../src/core/hex_geometry.h"

int main() {
  const HexGeometry &g = hexGeometry();

  // Ring structure: 1 center + 6 + 12 + 18 = 37.
  CHECK_EQ(g.ringSize[0], 1u);
  CHECK_EQ(g.ringSize[1], 6u);
  CHECK_EQ(g.ringSize[2], 12u);
  CHECK_EQ(g.ringSize[3], 18u);
  CHECK_EQ(g.ringOf[18], 0u); // row-major center of 4-5-6-7-6-5-4

  // spiralOrder is a permutation of 0..36, ring-monotonic.
  {
    bool seen[HEX_NUMPIXELS] = {};
    bool perm = true, monotonic = true;
    for (int i = 0; i < HEX_NUMPIXELS; i++) {
      uint8_t p = g.spiralOrder[i];
      if (p >= HEX_NUMPIXELS || seen[p]) perm = false;
      else seen[p] = true;
      if (i > 0 && g.ringOf[g.spiralOrder[i]] < g.ringOf[g.spiralOrder[i - 1]])
        monotonic = false;
    }
    CHECK(perm);
    CHECK(monotonic);
    CHECK_EQ(g.spiralOrder[0], 18u); // spiral starts at the center
  }

  // pathIndex: spiral ping-pong reverses at the ends (period 2(n-1)); orbit wraps.
  CHECK_EQ(hexPathIndex(0, 37, false), 0);
  CHECK_EQ(hexPathIndex(36, 37, false), 36);
  CHECK_EQ(hexPathIndex(37, 37, false), 35); // bounce, no jump to 0
  CHECK_EQ(hexPathIndex(72, 37, false), 0);  // full period
  CHECK_EQ(hexPathIndex(-1, 37, false), 1);  // negative steps stay in range
  CHECK_EQ(hexPathIndex(6, 6, true), 0);     // orbit wrap
  CHECK_EQ(hexPathIndex(-1, 6, true), 5);
  CHECK_EQ(hexPathIndex(5, 1, false), 0);    // degenerate single-pixel path

  // nearestPixel round-trips every pixel's own coordinates.
  {
    bool ok = true;
    for (uint8_t i = 0; i < HEX_NUMPIXELS; i++)
      if (hexNearestPixel(g.x[i], g.y[i]) != i) ok = false;
    CHECK(ok);
  }

  return testReport("test_geometry_pattern");
}
