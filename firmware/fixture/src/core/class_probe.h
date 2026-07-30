// Class identity from attached hardware (one image, zero-touch: ADR 0009).
// The I2C probing itself is esp32/sensors/sensor_bus; this is the pure
// decision table, natively tested.
#pragma once

#include <stdint.h>
#include "fixture_context.h"

struct ProbeBits {
  bool tmf8820;  // 0x41, ID-verified (bench INA219 shares the address)
  bool vl53l5cx; // 0x29 (plain ACK: TSL2591 is not used in production)
  bool bmp581;   // 0x47, CHIP_ID-verified
  bool msa311;   // 0x26 (logged only; present on multiple classes)
};

struct ClassDecision {
  uint8_t cls;       // FixtureClass to run as
  uint8_t persistLast; // value to store as class_last (0 = leave unchanged)
  bool mismatch;     // probe disagreed with override/last -> telemetry flag
};

// Rules:
//  - class_ovr wins outright (mismatch flagged if the probe disagrees).
//  - TMF8820 -> downlight (TMF+VL53 conflict -> downlight + mismatch).
//  - else VL53L5CX -> perimeter; else BMP581 -> uplight; else chandelier.
//  - Sensor-death guard: a probe that lands on "chandelier" while class_last
//    remembers a sensored class keeps class_last (mismatch flagged) -- a dead
//    sensor must not silently change a fixture's LED electrical profile.
ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last);
