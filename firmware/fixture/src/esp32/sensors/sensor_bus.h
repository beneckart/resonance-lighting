// Wire1 (STEMMA) presence probes for the class decision. ADR 0028: the bus is
// shared with the charger/gauge -- 100 kHz, loop context only, never raised.
#pragma once

#include "../../core/class_probe.h"

// Probe after VSQT is up and settled (~150 ms). Cheap, read-only.
ProbeBits sensorBusProbe();

// Read-only DS3231 UTC observation. Refuses oscillator-stop (OSF), malformed
// BCD, or dates outside the deployed firmware's accepted time horizon.
bool sensorBusReadRtcUtc(uint32_t &utcS);
