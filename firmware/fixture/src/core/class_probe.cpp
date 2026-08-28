#include "class_probe.h"

uint8_t probeBitsMask(const ProbeBits &bits) {
  return (bits.tmf8820 ? 0x01 : 0) |
         (bits.vl53l5cx ? 0x02 : 0) |
         (bits.bmp581 ? 0x04 : 0) |
         (bits.msa311 ? 0x08 : 0) |
         (bits.samM8q ? 0x10 : 0) |
         (bits.ds3231 ? 0x20 : 0);
}

static uint8_t probedClass(const ProbeBits &bits) {
  if (bits.tmf8820) return FIXTURE_DOWNLIGHT;
  if (bits.vl53l5cx) return FIXTURE_PERIMETER;
  if (bits.msa311) return FIXTURE_UPLIGHT;
  // ADR 0067: no chandelier is installed in the 2026 fleet. Sensorless
  // fixtures therefore default to uplight; future chandelier MACs carry an
  // explicit persisted override before installation.
  return FIXTURE_UPLIGHT;
}

static bool tofBearingClass(uint8_t cls) {
  return cls == FIXTURE_DOWNLIGHT || cls == FIXTURE_PERIMETER;
}

ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last) {
  ClassDecision d = {};
  uint8_t probed = probedClass(bits);
  const bool conflictingTof = bits.tmf8820 && bits.vl53l5cx;
  const bool bmpOnly = bits.bmp581 && !bits.tmf8820 && !bits.vl53l5cx &&
                       !bits.msa311;
  const bool noClassSensors = !bits.tmf8820 && !bits.vl53l5cx && !bits.msa311;
  const bool msaOnly = bits.msa311 && !bits.tmf8820 && !bits.vl53l5cx;
  d.mismatch = conflictingTof || bmpOnly;

  if (ovr != FIXTURE_UNKNOWN && ovr <= FIXTURE_CHANDELIER) {
    d.cls = ovr;
    // Chandelier identity is intentionally MAC-rostered. No class sensor is
    // exactly the expected signature for that explicit override, even though
    // the automatic 2026 fallback for the same hardware is uplight.
    const bool explicitSensorlessChandelier =
        ovr == FIXTURE_CHANDELIER && noClassSensors && !bits.bmp581;
    d.mismatch = d.mismatch || (!explicitSensorlessChandelier && probed != ovr);
    d.persistLast = 0; // an override doesn't teach class_last
    return d;
  }

  if (tofBearingClass(last) && (noClassSensors || msaOnly)) {
    // An MSA311-only result is a valid uplight only when the remembered class
    // does not say a ToF-bearing fixture. A fully sensorless result gets the
    // same guard. Preserve a prior ToF identity when its discriminator dies.
    d.cls = last;
    d.mismatch = true;
    d.persistLast = 0;
    return d;
  }

  d.cls = probed;
  // A lone BMP581 is not a production class signature. Use the 2026 uplight
  // fallback for this boot, flag it, and do not teach that anomaly to NVS.
  d.persistLast = (!bmpOnly && probed != last) ? probed : 0;
  return d;
}
