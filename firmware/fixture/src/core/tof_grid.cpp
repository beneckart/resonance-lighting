#include "tof_grid.h"

uint8_t l5cxSelectGround(const int16_t *distanceMm, const uint8_t *targetStatus,
                         const uint8_t *nbTargetDetected, uint8_t zones,
                         uint8_t targetsPerZone, int16_t minMm, int16_t maxMm,
                         uint16_t *zoneMm, uint16_t *closestMm) {
  uint8_t kept = 0;
  uint16_t closest = 0;
  for (uint8_t z = 0; z < zones; z++) {
    uint8_t nt = nbTargetDetected[z];
    if (nt > targetsPerZone) nt = targetsPerZone;
    int16_t best = 0;
    for (uint8_t t = 0; t < nt; t++) {
      uint16_t i = (uint16_t)(z * targetsPerZone + t);
      uint8_t st = targetStatus[i];
      int16_t d = distanceMm[i];
      if ((st != 5 && st != 9) || d <= minMm || d >= maxMm) continue;
      if (d > best) best = d; // farthest valid = ground
      if (!closest || (uint16_t)d < closest) closest = (uint16_t)d;
    }
    zoneMm[z] = (uint16_t)best;
    if (best) kept++;
  }
  *closestMm = closest;
  return kept;
}
