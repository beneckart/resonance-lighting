# Perimeter sentinel radio-off + ToF power trace

Use this runbook before implementing any persistent daytime sentinel. The test
image is a short-lived exact-target instrument for one physically confirmed
perimeter fixture. It is never a fleet artifact.

## What the campaign does

After reboot and the 20-second OTA pending-verify window, the fixture runs this
automatic sequence:

1. 10 minutes: ESP-NOW off, VSQT sensor rail off (baseline A).
2. 30 seconds: ESP-NOW off, perimeter MSA311 + VL53L5CX initialize and settle.
3. 10 minutes: ESP-NOW off, MSA311 + VL53L5CX active at the production cadence.
4. 10 minutes: ESP-NOW off, VSQT sensor rail off (baseline B).
5. The fixture starts maintenance WiFi and serves the retained trace.

One-second records include corrected/raw battery current, battery and supply
voltage/current, advisory SOC, power validity, charging and BQ state, explicit
radio and sensor-rail state, VL53 read count/health/closest/near-zone fields,
and the production palm-cover rising edge. A 4,096-sample PSRAM ring holds the
whole run; a reported 1,024-sample internal-RAM fallback cannot hold all 30.5
minutes and is not acceptable for the primary campaign.

## Safety and identity contract

1. Use Fleet Identify and physically confirm one healthy perimeter fixture.
   Confirm its outward VL53L5CX window is unobstructed and reachable for a palm
   test. Do not infer its physical location from telemetry alone.
2. Record exact short MAC, callsign/location, current firmware revision,
   profile, class, sensor state, battery tier, voltage/current, and the exact
   prior artifact path plus SHA-256.
3. Use one OTA writer. Build with `--sentinel-trace-target <SHORT_MAC>` and an
   immutable `-t` revision. The wrapper refuses another special target mode in
   the same image.
4. Target exactly that MAC. Require a production battery, exact endpoint
   identity, fresh exact-revision reboot, and pending-verify survival.
5. Do not send `/resume` during the campaign. The fixture intentionally leaves
   mesh and returns only as maintenance WiFi after the bounded run.
6. Never select test or restore binaries by newest mtime or `latest`.
7. After capture, OTA the exact prior fleet binary and prove fresh mesh rejoin,
   exact revision, pending-verify survival, field profile, perimeter class,
   power state, and MSA311/VL53L5CX health.

## Build shape

Replace every placeholder with the exact named target and a fresh ADR 0040
artifact directory/revision:

```text
firmware/fixture/build.sh \
  --profile field \
  --channel 11 \
  --day-sleep-s 120 \
  --wake-listen-ms 12000 \
  --sentinel-trace-target ABCDEF \
  --fw-rev fx-260829-1234567-t \
  --artifact-dir firmware/fixture/build/fx-260829-1234567-t
```

Build once, inspect the manifest/build options/revision/SHA, and OTA those exact
binary bytes. Do not let an upload command trigger another compile.

## Physical scene

For the cleanest load delta, first run battery-isolated or with the panel
shaded consistently. Do not change cables, fixture orientation, or battery
during A/B/A.

During `tof-active`, perform at least 20 deliberate palm approaches in direct
daylight:

- start from a clearly released sensor;
- bring one hand within the proven close-cover range;
- hold for about one second;
- fully clear for long enough to re-arm;
- note missed responses and rough latency on the field sheet.

Do not trigger the cymbal or LEDs during the power campaign. Repeat later in
full sun to establish net battery current and sunlight detection reliability;
the A/B/A shape helps reject a changing solar or battery trend but does not
make a rapidly moving cloud field controlled evidence.

## Download

Once the fixture automatically appears on maintenance WiFi, recover the entire
campaign. The tool refuses identity, revision, role, target, phase, radio-off,
sensor-rail, and VL53-progress failures and exclusive-creates its output.

```text
python ops/bench/capture_sentinel_trace.py \
  --host 192.168.1.123 \
  --target ABCDEF \
  --expect-fw fx-260829-1234567-t \
  --label battery-isolated-radio-off-vl53-aba \
  --out "ops/bench/data/Black Rock City/2026-08-29-abcdef-sentinel-power.jsonl"
```

The footer reports per-phase mean/median corrected battery current, mean supply
current, palm edges, and VL53 frame delta. Preserve the raw rows; summary values
alone are not the evidence.

## Analysis and promotion gate

- Compare `tof-active` against the mean of baseline A and B. Plot phase time,
  corrected battery current, supply current, VBAT, and charger phase before
  interpreting a single delta.
- Confirm `radio_on=0` throughout all three measurement phases and
  `sensor_rail_on=1` only during `tof-active`.
- Confirm VL53 reads advance steadily, no reset/fault interrupts the run, and
  the palm test has acceptable misses and latency in full sun.
- Convert the measured incremental current into the intended daily sentinel
  duty cycle and subtract it from the 6 Ah perimeter/night budget. Also check
  the full-sun run remains net positive with realistic panel orientation.
- Do not implement fleet wake propagation until Ben accepts the measured
  energy and interaction result. Any later session must have a non-extendable
  deadline and cooldown so visitor popularity cannot make the fleet perpetual.
