#pragma once

#include <stdint.h>

// Pure LiPo (1S) voltage -> percent estimate for the T-Deck's 2000 mAh cell.
// Rest-voltage curve, piecewise linear; no load compensation (the handheld's
// draw is modest and steady). Native-tested in tests/test_battery_model.cpp.
inline uint8_t lipoPercentFromMv(uint16_t mv) {
  struct Pt { uint16_t mv; uint8_t pct; };
  static const Pt kCurve[] = {
      {3300, 0},  {3500, 5},  {3600, 10}, {3700, 25}, {3750, 40},
      {3800, 55}, {3900, 70}, {4000, 85}, {4100, 95}, {4200, 100},
  };
  const int n = sizeof(kCurve) / sizeof(kCurve[0]);
  if (mv <= kCurve[0].mv) return 0;
  if (mv >= kCurve[n - 1].mv) return 100;
  for (int i = 1; i < n; ++i) {
    if (mv < kCurve[i].mv) {
      const Pt &a = kCurve[i - 1], &b = kCurve[i];
      return (uint8_t)(a.pct +
                       (uint32_t)(mv - a.mv) * (b.pct - a.pct) / (b.mv - a.mv));
    }
  }
  return 100;
}
