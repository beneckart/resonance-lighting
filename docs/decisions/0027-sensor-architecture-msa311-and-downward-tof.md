# 0027 -- Sensor architecture: MSA311 accelerometer + multizone ToF, allocated by fixture class

**Date:** 2026-07-08 (records the 2026-07-02 presence-bench findings, the 2026-07-06/07
sway_demo validation and fused-IMU rejection, and the 2026-07-08 allocation)
**Status:** Accepted. Per-class allocation recorded below is tentative until
installation (ADR 0024); per-fixture presence choreography remains future firmware.
Annotation 2026-07-16: uplights are no longer sensor-less -- 30x BMP581
temp/barometric-pressure sensors were bought for the uplight STEMMA chain as
generic environmental loggers (playa weather telemetry for the 2027 design).
Motion/presence allocation is unchanged.
**Owners:** Ben + Claude

## Context

Elliot's interactivity ask ("what makes people spend quality time at the tree")
kicked off a sensor workstream (research note
`docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md`, explicitly
nothing-decided at the time). Framing that survives from that note: this is art, not
security -- ~80 % detection is success; the mesh choreography is the product; sensors
must be near-zero power and require no per-unit calibration.

Bench work since:

- **presence_bench (2026-07-02):** five sensors live behind a TCA9548A mux
  (VL53L5CX 8x8, TMF8821, MLX90640 thermal, XM125/A121 radar, VL53L1X single-zone).
  VL53L5CX software address-relocation proved a silicon dead end -- the mux is the
  multi-sensor bench answer. Ben's eyeball verdicts: TMF882x multizone "the sweet
  spot"; single-zone VL53L1X makes him nervous vs multizone robustness to dust and
  self-occlusion (the $3-vs-$10 question).
- **sway_demo .3-.6 (2026-07-06/07):** MSA311 accel + VL53L5CX 4x4 fused on real
  geometry. Robust LS plane fit over zone ranges gives geometric tilt + height at
  nadir; mount-zero stored as a unit normal is spin-invariant (exact 3D angle);
  heavy spin pollutes the accel more than the ToF (mount the accel near the spin
  axis). Full accel-vs-ToF chain verified on a propped rig (16/16 zones, outlier
  auto-rejected).
- **Fused-orientation IMUs rejected:** BNO055/085-class parts need per-device
  calibration, which disqualifies them at fleet scale (ADR 0009's no-per-unit-ritual
  rule). The MSA311 gives sway energy at ~zero power with no calibration; true tilt
  comes geometrically from the ToF instead.
- **Bus rule:** all sensors ride the shared 100 kHz Wire1 with the charger/gauge --
  ADR 0028 applies; per-frame reads must stay short (sway_demo disables per-SPAD
  data for exactly this reason).

Purchases (2026-07-07, see `ops/PROCUREMENT.md`): 150x MSA311 (Adafruit STEMMA),
100x TMF8820-mini (SparkFun), 48x VL53L5CX (Mouser) + 60x protective optical covers
(Gilisymo) for dust protection of exposed ToF apertures. With the bench/sample units
already on hand, total depth sensors = 150 -- parity with the accelerometers.

## Options considered

- **MSA311 accel + multizone ToF (chosen):** zero-cal, near-zero power, dust-robust
  multizone, geometric tilt for free.
- **Fused-orientation IMU (BNO055/085):** rejected -- per-device calibration at 150
  units.
- **Single-zone VL53L1X:** demoted to cost-reference -- multizone wins on dust /
  self-occlusion robustness for a few dollars more.
- **mmWave (LD2410-class):** continuous-power appetite unfit for the solar budget;
  not pursued for the fleet.
- **Thermal (MLX90640) / radar (XM125):** bench instruments and future choreography
  candidates, not fleet sensors.

## Decision

Production sensor payload, allocated by fixture class (TMF8820 is 3x3 multizone,
bench-validated on the same-family TMF8821; Ben confirms 3x3 suffices downward):

- **Hanging downlights (72): MSA311 + TMF8820-mini facing downward** -- presence
  below the lantern, sway energy, geometric tilt.
- **Perimeter fixtures (38-40): VL53L5CX facing outward** to catch passers-by at
  people height (MSA311 likely too; tentative). The Gilisymo covers protect these
  exposed apertures.
- **Uplights and chandelier: no sensors** (tentative).
- Remaining units are spares/bench stock (150 accel + 150 ToF total on hand
  against ~110-112 deployed sensor positions).

## Consequences

- Presence/sway data feeds the mesh choreography concepts (ripple, wand, CA modes)
  -- firmware work, not yet scheduled; the winning-sensor heartbeat-tail integration
  is on TODO.
- Enclosure needs a ToF aperture: downward beside the gobo on downlights, outward
  window on perimeter hats (Steve's track); covers/goggles integrate there.
- STEMMA cabling (250 cables bought) is the sensor interconnect; connector-standard
  detail lives in the BOM.
- Walk-under datasets, the lantern-rig splay-occlusion session, and an outdoor
  lantern test remain the open validation items before choreography tuning.
- Accel placement rule for the hat: near the spin axis (sway_demo .6 finding).

## References

- `docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md` (option space)
- LOG 2026-07-02 (presence bench + mux + eyeball verdicts), 2026-07-06/07
  (sway_demo .3-.6, IMU rejection, allocation context), 2026-07-08 (per-class
  allocation interview)
- `firmware/presence_bench/`, `firmware/sway_demo/`; ADR 0009, 0024, 0028
