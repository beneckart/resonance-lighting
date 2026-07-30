// Coulomb/energy integrator. Two donor bugs pinned here:
//   1. ms remainder loss (fixed in the donor 2026-07-2x; regression-pinned) --
//      advance the anchor only by integrated whole seconds;
//   2. per-call mA rounding: the donor quantized current to whole mA every
//      integration call, biasing dithering currents. Accumulate in mA*ms
//      (uint64) and quantize only on read-out.
#pragma once

#include <stdint.h>

struct PowerIntegrator {
  uint32_t lastMs;       // 0 = not yet anchored
  uint64_t chargeMaMs;   // integral of +current
  uint64_t dischargeMaMs;// integral of -current (stored positive)
  uint16_t minMv, maxMv;
};

void integratorReset(PowerIntegrator &pi);
// Called with each sample; ma is corrected battery current (+ = charging).
void integratorTick(PowerIntegrator &pi, uint32_t nowMs, float ma, uint16_t mv);

uint32_t integratorChargeMah(const PowerIntegrator &pi);
uint32_t integratorDischargeMah(const PowerIntegrator &pi);
