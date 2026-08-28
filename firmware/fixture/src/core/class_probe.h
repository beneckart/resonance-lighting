// Class identity from attached hardware (one image, zero-touch: ADR 0009).
// The I2C probing itself is esp32/sensors/sensor_bus; this is the pure
// decision table, natively tested.
#pragma once

#include <stdint.h>
#include "fixture_context.h"

// MSA311 hardware identity. Keep this distinct from the older MSA301 address
// (0x26); production uses Adafruit MSA311 breakouts at 0x62.
static constexpr uint8_t MSA311_I2C_ADDR = 0x62;
static constexpr uint8_t MSA311_PART_ID_REG = 0x01;
static constexpr uint8_t MSA311_PART_ID = 0x13;

struct ProbeBits {
  bool tmf8820;  // 0x41, ID-verified (bench INA219 shares the address)
  bool vl53l5cx; // 0x29 (plain ACK: TSL2591 is not used in production)
  bool bmp581;   // 0x47, CHIP_ID-verified; environmental, not classifying
  bool msa311;   // 0x62, PART_ID-verified; uplight when neither ToF answers
  bool samM8q;   // 0x42; SparkFun SAM-M8Q Qwiic GPS time anchor
  bool ds3231;   // 0x68; Adafruit DS3231 STEMMA RTC holdover anchor
};

struct ClassDecision {
  uint8_t cls;       // FixtureClass to run as
  uint8_t persistLast; // value to store as class_last (0 = leave unchanged)
  bool mismatch;     // probe disagreed with override/last -> telemetry flag
};

// Stable wire mask for ProbeBits (NbHeartbeat::sensor_bits).
uint8_t probeBitsMask(const ProbeBits &bits);

// Rules:
//  - class_ovr wins outright (mismatch flagged if the probe disagrees). An
//    explicit chandelier override with no class sensor is valid by definition.
//  - TMF8820 -> downlight (TMF+VL53 conflict -> downlight + mismatch).
//  - else VL53L5CX -> perimeter; else MSA311 -> uplight; else uplight for the
//    2026 installed fleet (chandeliers are explicit MAC-rostered overrides).
//  - BMP581 never determines class. A lone BMP581 is anomalous and flagged.
//  - Sensor-death guard: an MSA311-only or no-class-sensor probe while
//    class_last remembers a ToF-bearing class keeps class_last (mismatch
//    flagged). A dead ToF must not silently change the LED electrical profile.
//  - A remembered auto-probed chandelier with no sensors migrates to uplight;
//    future chandeliers persist class_ovr=FIXTURE_CHANDELIER before install.
ClassDecision classDecide(const ProbeBits &bits, uint8_t ovr, uint8_t last);
