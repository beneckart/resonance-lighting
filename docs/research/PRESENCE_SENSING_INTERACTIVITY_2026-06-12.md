# Presence sensing / interactivity -- sensor landscape (2026-06-12)

Prompted by Elliot after the 06-11 LED demo: interactivity is what makes people spend
quality time at the tree. This note captures the option space and the test plan.
Status: RESEARCH -- nothing decided; bench kit ordered/queued (see TODO).

## Reframe that sets the requirements

This is ART, not security. A false positive = the tree twinkles at nobody (benign);
a false negative = no magic this time. ~80 % detection reliability is success. The
real product is the MESH CHOREOGRAPHY: one lantern detects -> ESP-NOW propagates ->
light ripples outward from where people stand. The packet layer already supports this
shape (append a PRESENCE event type, same append-only pattern as supply/env telemetry).
The per-lantern sensor only has to be good enough to seed it.

## Constraints (playa + our architecture)

- 100 units -> ~$2-6/unit sensor budget; no per-unit configuration (fungible).
- Night operation (the show window) -> sunlight degradation of optical ToF irrelevant.
- Playa dust: exposed transducers/windows degrade; through-enclosure sensing is gold.
- Lanterns SWAY: any sensor that keys on motion sees its own scene move. The IMU
  stretch-goal doubles as a sway-veto channel.
- Power: the 06-11/06-12 budget work prices everything. A continuously-on mmWave
  module (~80 mA) costs as much as the whole LED show; ToF duty-cycled at a few Hz
  rounds to ~1-5 mA.
- Lantern bottom aperture is the GOBO's -- a downward "eye" needs a port beside or
  through the gobo margin (enclosure design item, Steve).

## Option ranking

1. **ToF lidar (VL53L1X class, ~$2-4, I2C)** -- PRIMARY CANDIDATE. Downward eye:
   ground baseline ~3 m, person = abrupt step to ~1.5-2 m; sway = slow periodic
   baseline wander -> one-line temporal filter separates them. Narrow FoV (15-27 deg)
   physically excludes neighboring lanterns. 940 nm invisible. Duty-cycled power ~zero.
   Cost: needs a window/cutout; downward-facing recess is naturally dust-shedding but
   whiteouts still coat it (ToF crosstalk-calibration for dirty glass = part of test).
2. **mmWave radar (LD2410/LD2420 class, ~$4-6, UART not I2C)** -- bench-test in
   parallel. Through-enclosure = zero ports/dust exposure (the big playa win). Risks:
   power (only viable duty-cycled or on a subset of "scout lanterns"); self-sway makes
   the static ground look like a moving target to cheap module firmware (IMU veto may
   rescue it); 100 co-located 24 GHz emitters = mutual-interference question (2-unit
   bench test). Per-gate range/sensitivity config (LD2410) clamps the detection zone.
3. **Mesh-RSSI presence (FREE)** -- a human body attenuates 2.4 GHz ~20 dB (measured,
   T4 obstruction tests) and every heartbeat already carries per-link RSSI. People
   under the tree should print step-changes on specific links, distinguishable from
   sway (slow/correlated/IMU-confirmed). Zero BOM; crude localization; possibly the
   aggregate "people at the tree" channel with ToF as per-lantern precision. One yard
   evening with 3-5 nodes answers it.
4. **IMU (LIS3DH/MPU6050 class, ~$1-2, I2C)** -- include regardless: wind-sway ambient
   response (original stretch goal) + sway-veto for radar/ToF + possible structure-
   touch detection. One part, three features.
5. Ultrasonic -- SKIP (exposed transducers in dust; 100 units cross-talking at 40 kHz;
   wind noise).
6. PIR -- SKIP (a swaying PIR self-triggers on the moving warm scene; fatal).
7. Camera/audio -- out of scope (power/complexity/privacy; playa audio = music).

## Test plan (Steve-compatible; same harness as the env sensors)

Sensor readings ride the net_bench heartbeat (the validated append-only pattern) ->
walk under the rig in the yard -> tune gates/thresholds from the desk, all wireless.
Bench kit: VL53L1X + LD2420 + LIS3DH (~$10 total). Success metric per sensor:
detection rate + false-positive rate vs sway (fan/manual swing) at 2.5-3.5 m hang
height, person walking under/standing/leaving.
