// Wire1 (STEMMA) presence probes for the class decision. ADR 0028: the bus is
// shared with the charger/gauge -- 100 kHz, loop context only, never raised.
#pragma once

#include "../../core/class_probe.h"

// Probe after VSQT is up and settled (~150 ms). Cheap, read-only.
ProbeBits sensorBusProbe();
