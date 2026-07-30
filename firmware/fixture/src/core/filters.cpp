#include "filters.h"

#include <math.h>

#define ALPHA_G 0.06f
#define ENV_ATTACK 0.5f
#define ENV_DECAY 0.96f

void accelFilterInit(AccelFilter &f) {
  f = AccelFilter{};
  f.gz = 1.0f;
  f.restZ = 1.0f;
}

void accelFilterSample(AccelFilter &f, float ax, float ay, float az) {
  if (!f.seeded) {
    f.gx = ax;
    f.gy = ay;
    f.gz = az;
    f.seeded = true;
  } else {
    f.gx += ALPHA_G * (ax - f.gx);
    f.gy += ALPHA_G * (ay - f.gy);
    f.gz += ALPHA_G * (az - f.gz);
  }
  float dx = ax - f.gx, dy = ay - f.gy, dz = az - f.gz;
  float sway = sqrtf(dx * dx + dy * dy + dz * dz);
  if (sway > f.swayEnv) f.swayEnv += ENV_ATTACK * (sway - f.swayEnv);
  else f.swayEnv *= ENV_DECAY;

  float mag = sqrtf(f.gx * f.gx + f.gy * f.gy + f.gz * f.gz);
  if (f.restZeroed && mag > 0.05f) {
    float nx = f.gx / mag, ny = f.gy / mag, nz = f.gz / mag;
    float dot = nx * f.restX + ny * f.restY + nz * f.restZ;
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    f.tiltDeg = acosf(dot) * 57.2957795f;
  }
}

bool accelFilterZero(AccelFilter &f) {
  float mag = sqrtf(f.gx * f.gx + f.gy * f.gy + f.gz * f.gz);
  if (!f.seeded || !(mag > 0.05f)) return false;
  f.restX = f.gx / mag;
  f.restY = f.gy / mag;
  f.restZ = f.gz / mag;
  f.restZeroed = true;
  f.tiltDeg = 0.0f;
  return true;
}

bool planeFitLS(const float *px, const float *py, const float *pz, const bool *keep,
                uint8_t n, uint8_t minPoints, float *a, float *b, float *c) {
  double sx = 0, sy = 0, sz = 0, sxx = 0, syy = 0, sxy = 0, sxz = 0, syz = 0;
  uint16_t m = 0;
  for (uint8_t k = 0; k < n; k++) {
    if (!keep[k]) continue;
    double x = px[k], y = py[k], z = pz[k];
    sx += x; sy += y; sz += z;
    sxx += x * x; syy += y * y; sxy += x * y;
    sxz += x * z; syz += y * z;
    m++;
  }
  if (m < minPoints) return false;
  double det = sxx * (syy * m - sy * sy) - sxy * (sxy * m - sy * sx) +
               sx * (sxy * sy - syy * sx);
  if (fabs(det) < 1e-3) return false;
  double da = sxz * (syy * m - sy * sy) - sxy * (syz * m - sy * sz) +
              sx * (syz * sy - syy * sz);
  double db = sxx * (syz * m - sz * sy) - sxz * (sxy * m - sy * sx) +
              sx * (sxy * sz - syz * sx);
  double dc = sxx * (syy * sz - syz * sy) - sxy * (sxy * sz - syz * sx) +
              sxz * (sxy * sy - syy * sx);
  *a = (float)(da / det);
  *b = (float)(db / det);
  *c = (float)(dc / det);
  return true;
}

float planeTiltDeg(float a, float b, float a0, float b0) {
  // Normals n = (-a, -b, 1) / |.|; tilt = acos(n . n0).
  float n1x = -a, n1y = -b, n1z = 1.0f;
  float n2x = -a0, n2y = -b0, n2z = 1.0f;
  float m1 = sqrtf(n1x * n1x + n1y * n1y + 1.0f);
  float m2 = sqrtf(n2x * n2x + n2y * n2y + 1.0f);
  float dot = (n1x * n2x + n1y * n2y + n1z * n2z) / (m1 * m2);
  if (dot > 1.0f) dot = 1.0f;
  if (dot < -1.0f) dot = -1.0f;
  return acosf(dot) * 57.2957795f;
}
