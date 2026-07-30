#include "power_integrator.h"

void integratorReset(PowerIntegrator &pi) {
  pi = PowerIntegrator{};
  pi.minMv = 0xFFFF;
}

void integratorTick(PowerIntegrator &pi, uint32_t nowMs, float ma, uint16_t mv) {
  if (mv > 100) {
    if (mv < pi.minMv) pi.minMv = mv;
    if (mv > pi.maxMv) pi.maxMv = mv;
  }
  if (pi.lastMs == 0) {
    pi.lastMs = nowMs ? nowMs : 1;
    return;
  }
  uint32_t dt = nowMs - pi.lastMs;
  if (dt == 0) return;
  // Full-resolution accumulation: mA * ms, no per-call quantization. The
  // anchor advances by exactly dt, so no remainder is ever dropped.
  pi.lastMs = nowMs;
  float maMs = ma * (float)dt;
  if (maMs >= 0.0f) pi.chargeMaMs += (uint64_t)(maMs + 0.5f);
  else pi.dischargeMaMs += (uint64_t)(-maMs + 0.5f);
}

uint32_t integratorChargeMah(const PowerIntegrator &pi) {
  return (uint32_t)(pi.chargeMaMs / 3600000ULL);
}
uint32_t integratorDischargeMah(const PowerIntegrator &pi) {
  return (uint32_t)(pi.dischargeMaMs / 3600000ULL);
}
