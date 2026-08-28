# 0067 -- Sensorless uplight fallback and MAC-rostered chandeliers

**Date:** 2026-08-28

**Status:** Accepted; native and embedded compile validation passed, release
artifact and fleet rollout pending

**Owner:** Ben

**Supersedes:** ADR 0041's no-sensor-to-chandelier fallback

## Context

About eight already-assembled trunk/uplights appear as sensorless even though
their MSA311 breakout power LEDs are lit. Opening that installed cohort to swap
PowerFeathers is not practical before deployment, and no chandelier fixtures are
installed yet.

Donkey `F2BE10` provided the exact-target A/B. A new battery restored stable power,
and a new MSA311 plus replacement STEMMA cable remained absent on the current
accepted artifact. Source inspection then found a fleet-wide software defect:
the initial fixture-class probe polled `0x26`, the MSA301 address, while the
Adafruit MSA311 driver and production MSA311 hardware use `0x62`. The later
runtime driver used `0x62`, which explains why a ToF-bearing fixture could first
classify from its ToF and subsequently initialize its MSA311, while an MSA-only
uplight could never pass the initial class probe.

The physical sensor signatures of an MSA-faulted uplight and an intentional
sensorless chandelier are identical. They cannot be distinguished from I2C
alone. The installation state provides the safe discriminator for 2026: the
uplights exist now; the chandeliers do not and can be rostered before installation.

## Decision

1. Probe the MSA311 at I2C address `0x62` and require PART_ID register `0x01` to
   equal `0x13`. Keep all Wire1 traffic at 100 kHz.
2. Automatic class identity is ordered: ID-verified TMF8820/TMF8821-family ToF
   -> downlight; otherwise VL53L5CX -> perimeter; otherwise MSA311 -> uplight;
   otherwise no class sensor -> uplight.
3. Preserve a remembered downlight or perimeter when its ToF discriminator
   disappears, whether the remaining signature is MSA-only or fully sensorless.
   Report that condition as a mismatch.
4. A sensorless board remembered as chandelier by old automatic firmware
   migrates to uplight on its first boot under this policy. A normal sensorless
   uplight is not a mismatch.
5. Before chandelier installation, choose those PowerFeathers by exact MAC,
   record registry role `chandelier`, and persist `class_ovr=4` with the existing
   exact-target commissioning path (`O4`). A sensorless fixture with that explicit
   override is a valid chandelier and is not a mismatch. Clear the override with
   `O0` before repurposing such a board.
6. BMP581 remains environmental telemetry and never determines class. A lone
   BMP581 is anomalous: run the safe uplight fallback for that boot, report a
   mismatch, and do not learn the anomalous signature into `class_last`.

## Consequences

- The installed uplight cohort can be recovered by firmware instead of opening
  approximately eight fixtures or replacing PowerFeathers.
- Correcting `0x26` to `0x62` should restore MSA311 discovery where the powered
  hardware and cable are healthy. The sensorless fallback still keeps an uplight
  usable when its MSA path genuinely fails.
- A future chandelier is no longer zero-touch by sensor signature alone. Its MAC
  assignment and one persistent override are required commissioning operations.
  This is bounded to the chandelier cohort and occurs before installation.
- An unassigned sensorless board now appears as uplight, not chandelier. Registry
  assignment remains authoritative for allocation; automatic class is an
  electrical/rendering safety input, not an inventory assignment.
- This source change does not authorize a dirty-worktree build or deployment.
  Rollout requires a clean commit, a new immutable ADR 0040 artifact, an exact-MAC
  canary, and fresh post-reboot/pending-verify evidence.

## Validation evidence -- 2026-08-28

The complete native fixture suite passed, including 50 class-probe checks and
the build-wrapper contract. A guarded `--dev-cache --profile field --channel 11`
ESP32-S3 PowerFeather compile also passed with Arduino-ESP32 3.3.7. That compile
exercised the Adafruit-library address static assertion and produced a 1,207,808
byte development binary with SHA-256
`7e30b9e8a2688e38f3f680cc3acc91d0c949c78735b8900e3840bb479595d74f`.
The `dev-local` binary is compile evidence only and must not be deployed.

## Remaining validation and rollout

1. Deploy a new immutable artifact first to Donkey `F2BE10`. Require exact
   revision, healthy power, uplight class, and either MSA bit `0x08` with healthy
   samples or an explicitly accepted sensorless-uplight result.
2. Promote to the affected uplight cohort only after the canary passes. Do not
   deploy chandelier hardware until its exact MAC roster and `O4` overrides have
   been audited.

## References

- ADR 0040
- ADR 0041
- `firmware/fixture/src/core/class_probe.h`
- `firmware/fixture/src/core/class_probe.cpp`
- `firmware/fixture/src/esp32/sensors/sensor_bus.cpp`
- `firmware/fixture/tests/test_class_probe.cpp`
