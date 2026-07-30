#include "class_probe.h"

static uint8_t probedClass(const ProbeBits &bits) {
  if (bits.tmf8820) return FIXTURE_DOWNLIGHT;
  if (bits.vl53l5cx) return FIXTURE_PERIMETER;
  if (bits.bmp581) return FIXTURE_UPLIGHT;
  return FIXTURE_CHANDELIER;
}

ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last) {
  ClassDecision d = {};
  uint8_t probed = probedClass(bits);
  d.mismatch = bits.tmf8820 && bits.vl53l5cx; // conflicting discriminators

  if (ovr != FIXTURE_UNKNOWN && ovr <= FIXTURE_CHANDELIER) {
    d.cls = ovr;
    d.mismatch = d.mismatch || (probed != ovr && probed != FIXTURE_CHANDELIER);
    d.persistLast = 0; // an override doesn't teach class_last
    return d;
  }

  if (probed == FIXTURE_CHANDELIER && last != FIXTURE_UNKNOWN &&
      last != FIXTURE_CHANDELIER && last <= FIXTURE_CHANDELIER) {
    // No discriminating sensor answered but we used to be a sensored class:
    // treat as sensor death, keep the remembered class, flag it.
    d.cls = last;
    d.mismatch = true;
    d.persistLast = 0;
    return d;
  }

  d.cls = probed;
  d.persistLast = (probed != last) ? probed : 0;
  return d;
}
