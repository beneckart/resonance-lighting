#include "class_probe.h"

uint8_t probeBitsMask(const ProbeBits &bits) {
  return (bits.tmf8820 ? 0x01 : 0) |
         (bits.vl53l5cx ? 0x02 : 0) |
         (bits.bmp581 ? 0x04 : 0) |
         (bits.msa311 ? 0x08 : 0);
}

static uint8_t probedClass(const ProbeBits &bits) {
  if (bits.tmf8820) return FIXTURE_DOWNLIGHT;
  if (bits.vl53l5cx) return FIXTURE_PERIMETER;
  if (bits.msa311) return FIXTURE_UPLIGHT;
  return FIXTURE_CHANDELIER;
}

static bool sensoredClass(uint8_t cls) {
  return cls == FIXTURE_DOWNLIGHT || cls == FIXTURE_PERIMETER ||
         cls == FIXTURE_UPLIGHT;
}

ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last) {
  ClassDecision d = {};
  uint8_t probed = probedClass(bits);
  const bool conflictingTof = bits.tmf8820 && bits.vl53l5cx;
  const bool bmpOnly = bits.bmp581 && !bits.tmf8820 && !bits.vl53l5cx &&
                       !bits.msa311;
  d.mismatch = conflictingTof || bmpOnly;

  if (ovr != FIXTURE_UNKNOWN && ovr <= FIXTURE_CHANDELIER) {
    d.cls = ovr;
    d.mismatch = d.mismatch || probed != ovr;
    d.persistLast = 0; // an override doesn't teach class_last
    return d;
  }

  const bool lostAllClassSensors = probed == FIXTURE_CHANDELIER;
  const bool ambiguousMsaOnly = probed == FIXTURE_UPLIGHT &&
                                (last == FIXTURE_DOWNLIGHT ||
                                 last == FIXTURE_PERIMETER);
  if (sensoredClass(last) && (lostAllClassSensors || ambiguousMsaOnly)) {
    // An MSA311-only result is a valid uplight only when the remembered class
    // does not say a ToF-bearing fixture. Preserve a prior sensored identity
    // when its distinguishing sensor disappears.
    d.cls = last;
    d.mismatch = true;
    d.persistLast = 0;
    return d;
  }

  d.cls = probed;
  // A lone BMP581 is not a production class signature. Run the safe
  // chandelier profile for this boot, but do not teach that anomaly to NVS.
  d.persistLast = (!bmpOnly && probed != last) ? probed : 0;
  return d;
}
