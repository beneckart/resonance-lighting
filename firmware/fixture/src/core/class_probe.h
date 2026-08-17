// Class identity from attached hardware (one image, zero-touch: ADR 0009).
// The I2C probing itself is esp32/sensors/sensor_bus; this is the pure
// decision table, natively tested.
#pragma once

#include <stdint.h>
#include "fixture_context.h"

struct ProbeBits {
  bool tmf8820;  // 0x41, ID-verified (bench INA219 shares the address)
  bool vl53l5cx; // 0x29 (plain ACK: TSL2591 is not used in production)
  bool bmp581;   // 0x47, CHIP_ID-verified; environmental, not classifying
  bool msa311;   // 0x26; uplight discriminator only when neither ToF answers
};

struct ClassDecision {
  uint8_t cls;       // FixtureClass to run as
  uint8_t persistLast; // value to store as class_last (0 = leave unchanged)
  bool mismatch;     // probe disagreed with override/last -> telemetry flag
};

// Stable wire mask for ProbeBits (NbHeartbeat::sensor_bits).
uint8_t probeBitsMask(const ProbeBits &bits);

// Rules:
//  - class_ovr wins outright (mismatch flagged if the probe disagrees).
//  - TMF8820 -> downlight (TMF+VL53 conflict -> downlight + mismatch).
//  - else VL53L5CX -> perimeter; else MSA311 -> uplight; else chandelier.
//  - BMP581 never determines class. A lone BMP581 is anomalous and flagged.
//  - Sensor-death guard: an MSA311-only or no-sensor probe while class_last
//    remembers a ToF-bearing class keeps class_last (mismatch flagged). A
//    no-sensor probe also preserves a remembered uplight. A dead sensor must
//    not silently change a fixture's LED electrical profile.
ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last);
