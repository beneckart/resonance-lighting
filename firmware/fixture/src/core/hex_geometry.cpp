#include "hex_geometry.h"

#include <math.h>

static HexGeometry gGeo;
static bool gBuilt = false;

static void build() {
  static const uint8_t ROW_COUNT[7] = {4, 5, 6, 7, 6, 5, 4};
  uint8_t idx = 0;
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t n = ROW_COUNT[r];
    float y = (3.0f - (float)r) * 0.8660254f;
    for (uint8_t j = 0; j < n; j++) {
      float x = (float)j - (float)(n - 1) / 2.0f;
      gGeo.x[idx] = x;
      gGeo.y[idx] = y;
      float d = sqrtf(x * x + y * y);
      uint8_t ring = (uint8_t)lroundf(d);
      if (ring > 3) ring = 3;
      gGeo.ringOf[idx] = ring;
      gGeo.pxAngle[idx] = atan2f(y, x);
      idx++;
    }
  }
  for (uint8_t i = 0; i < HEX_NUMPIXELS; i++) gGeo.spiralOrder[i] = i;
  for (uint8_t i = 1; i < HEX_NUMPIXELS; i++) {
    uint8_t key = gGeo.spiralOrder[i];
    int8_t k = i - 1;
    while (k >= 0) {
      uint8_t a = gGeo.spiralOrder[k];
      bool greater = (gGeo.ringOf[a] > gGeo.ringOf[key]) ||
                     (gGeo.ringOf[a] == gGeo.ringOf[key] &&
                      gGeo.pxAngle[a] > gGeo.pxAngle[key]);
      if (!greater) break;
      gGeo.spiralOrder[k + 1] = gGeo.spiralOrder[k];
      k--;
    }
    gGeo.spiralOrder[k + 1] = key;
  }
  gGeo.ringSize[0] = gGeo.ringSize[1] = gGeo.ringSize[2] = gGeo.ringSize[3] = 0;
  for (uint8_t i = 0; i < HEX_NUMPIXELS; i++)
    gGeo.ringMembers[gGeo.ringOf[i]][gGeo.ringSize[gGeo.ringOf[i]]++] = i;
  for (uint8_t r = 0; r < 4; r++)
    for (uint8_t i = 1; i < gGeo.ringSize[r]; i++) {
      uint8_t key = gGeo.ringMembers[r][i];
      int8_t k = i - 1;
      while (k >= 0 && gGeo.pxAngle[gGeo.ringMembers[r][k]] > gGeo.pxAngle[key]) {
        gGeo.ringMembers[r][k + 1] = gGeo.ringMembers[r][k];
        k--;
      }
      gGeo.ringMembers[r][k + 1] = key;
    }
  gBuilt = true;
}

const HexGeometry &hexGeometry() {
  if (!gBuilt) build();
  return gGeo;
}

uint8_t hexNearestPixel(float x, float y) {
  const HexGeometry &g = hexGeometry();
  uint8_t best = 0;
  float bd = 1e9f;
  for (uint8_t i = 0; i < HEX_NUMPIXELS; i++) {
    float dx = g.x[i] - x, dy = g.y[i] - y, d = dx * dx + dy * dy;
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}

int hexPathIndex(long step, int n, bool orbit) {
  if (n <= 1) return 0;
  if (orbit) return (int)(((step % n) + n) % n);
  long period = 2 * (n - 1);
  long m = ((step % period) + period) % period;
  return (int)(m < n ? m : period - m);
}
