#include <cassert>
#include <cstdio>

#include "core/battery_model.h"

int main() {
  assert(lipoPercentFromMv(3000) == 0);
  assert(lipoPercentFromMv(3300) == 0);
  assert(lipoPercentFromMv(4200) == 100);
  assert(lipoPercentFromMv(4300) == 100);
  assert(lipoPercentFromMv(3700) == 25);
  assert(lipoPercentFromMv(4000) == 85);

  // Monotonic across the whole range.
  uint8_t prev = 0;
  for (uint16_t mv = 3000; mv <= 4300; ++mv) {
    uint8_t pct = lipoPercentFromMv(mv);
    assert(pct >= prev);
    prev = pct;
  }

  // Midpoint interpolation lands strictly between its bracket points.
  uint8_t mid = lipoPercentFromMv(3850);
  assert(mid > 55 && mid < 70);

  printf("battery_model ok\n");
  return 0;
}
