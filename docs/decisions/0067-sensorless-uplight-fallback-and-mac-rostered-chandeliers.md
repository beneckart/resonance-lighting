# 0067 -- Sensorless uplight fallback and MAC-rostered chandeliers

**Date:** 2026-08-28

**Status:** Accepted; canary pending

**Owner:** Ben

**Supersedes:** ADR 0041's no-sensor-to-chandelier fallback

## Context

About eight already-assembled trunk/uplights appear sensorless even though their
MSA311 power LEDs are lit. Opening that installed cohort to replace PowerFeathers
is not practical before deployment, and no chandelier fixtures are installed yet.

Donkey `F2BE10` provided the exact-target A/B. Source inspection found a common
software defect: the initial fixture-class probe polled `0x26`, the MSA301
address, while the Adafruit MSA311 driver and production hardware use `0x62`.
The later runtime driver used `0x62`, but it was skipped after the failed probe
misclassified an MSA-only uplight as chandelier.

An MSA-faulted uplight and intentional sensorless chandelier cannot be
distinguished from I2C alone. For 2026, the uplights exist now; the chandeliers
do not and can be rostered before installation.

## Decision

1. Probe MSA311 at `0x62` and require PART_ID register `0x01` to equal `0x13`.
   Keep Wire1 at 100 kHz.
2. Automatic class order is TMF8820/TMF8821-family ToF -> downlight;
   VL53L5CX -> perimeter; MSA311 -> uplight; no class sensor -> uplight.
3. Preserve a remembered downlight or perimeter when its ToF disappears and
   report a mismatch.
4. Migrate an old automatically remembered sensorless chandelier to uplight.
5. Before chandelier installation, choose its PowerFeathers by exact MAC, record
   registry role `chandelier`, and persist `class_ovr=4` with `O4`. A sensorless
   explicit chandelier is valid. Clear the override with `O0` before repurposing.
6. BMP581 remains non-classifying. A lone BMP581 is a mismatch and is not learned.

## Consequences

- The installed uplight cohort can be recovered by firmware without opening it.
- The fallback keeps an uplight usable even if its MSA hardware genuinely fails.
- Chandelier boards require one exact-MAC commissioning operation before install.
- An unassigned sensorless board reports uplight, but registry assignment remains
  authoritative for inventory allocation.

## Validation

The complete native fixture suite and guarded ESP32-S3 field compile passed before
canary preparation. Hardware acceptance requires a clean immutable artifact on
Donkey `F2BE10`, exact revision, healthy power, uplight class, fresh rejoin beyond
pending verify, and either MSA bit `0x08` with healthy samples or the explicitly
accepted sensorless-uplight fallback.

## References

- ADR 0040
- ADR 0041
- `firmware/fixture/src/core/class_probe.h`
- `firmware/fixture/src/core/class_probe.cpp`
- `firmware/fixture/src/esp32/sensors/sensor_bus.cpp`
- `firmware/fixture/tests/test_class_probe.cpp`
