#include "test_util.h"

#include <cmath>
#include "../src/core/power_integrator.h"

int main() {
  // Regression pin for the donor's ms-carry bug: 1.3 s ticks at constant
  // current must integrate to the exact wall-clock total (the broken version
  // lost the 300 ms remainder every tick: ~23% low; the "fixed" whole-seconds
  // donor version still dropped sub-second residue at the END of a session).
  {
    PowerIntegrator pi;
    integratorReset(pi);
    uint32_t t = 5;
    integratorTick(pi, t, -860.0f, 3200);
    for (int i = 0; i < 10000; i++) {
      t += 1300;
      integratorTick(pi, t, -860.0f, 3200);
    }
    // 10000 * 1.3 s = 13000 s at 860 mA = 3105.6 mAh.
    uint32_t mah = integratorDischargeMah(pi);
    CHECK(mah >= 3104 && mah <= 3106);
    CHECK_EQ(integratorChargeMah(pi), 0u);
  }

  // Dithering current: +-0.4 mA around 0 must NOT be quantized to whole mA
  // per call (the donor's uint64 cast dropped sub-mA values entirely).
  {
    PowerIntegrator pi;
    integratorReset(pi);
    uint32_t t = 0;
    integratorTick(pi, t, 0.0f, 3200);
    for (int i = 0; i < 720000; i++) { // 200 h of 1 s ticks
      t += 1000;
      integratorTick(pi, t, 0.4f, 3200);
    }
    // 0.4 mA * 200 h = 80 mAh. Per-call whole-mA rounding would read 0.
    uint32_t mah = integratorChargeMah(pi);
    CHECK(mah >= 79 && mah <= 81);
  }

  // Sign split + min/max tracking.
  {
    PowerIntegrator pi;
    integratorReset(pi);
    integratorTick(pi, 1000, 0.0f, 3300);
    integratorTick(pi, 3601000, 500.0f, 3350);  // 1 h at +500 mA
    integratorTick(pi, 7201000, -250.0f, 3100); // 1 h at -250 mA
    CHECK_EQ(integratorChargeMah(pi), 500u);
    CHECK_EQ(integratorDischargeMah(pi), 250u);
    CHECK_EQ(pi.minMv, 3100u);
    CHECK_EQ(pi.maxMv, 3350u);
  }

  // Invalid voltage samples don't poison min/max.
  {
    PowerIntegrator pi;
    integratorReset(pi);
    integratorTick(pi, 1000, 0.0f, 0);
    CHECK_EQ(pi.minMv, 0xFFFFu);
  }

  return testReport("test_power_integrator");
}
