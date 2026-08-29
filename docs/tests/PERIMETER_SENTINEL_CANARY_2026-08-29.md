# Perimeter sentinel canary -- Spyro F2BCF0 -- 2026-08-29

## Result

The first exact-target radio-off + perimeter-ToF A/B/A field campaign completed
its interaction sequence, but it did not produce an acceptable power trace.
Rapid sunrise changed solar input across the three phases, and the completed
PSRAM buffer was then erased by a task-watchdog reset while maintenance WiFi
was trying to start. Spyro was restored without opening its enclosure and is
back on its exact prior fleet artifact.

Treat this as useful interaction/retrieval-path evidence, not as a measured
sentinel current delta and not as a promotion gate pass.

## Declared operation

```text
OWNER: Ben + Codex, T-Deck 8EB508 / dashboard COM7
SOURCE: bb860980697aee93aca8fd37280f08ab249c4aca
ARTIFACT: fx-260829-f13e090-t
SHA-256: 249c500e3a9bf12ab4902ec1db1c18680605d3a1a35bf7933e2c162dc1061cb1
TARGET: F2BCF0 (Spyro)
OPERATION: single-target OTA, bounded sentinel A/B/A, exact-prior restore
```

The canary was a clean, immutable 1,225,488-byte `-t` artifact with recipe
SHA-256 `f13e0903eda26d75fcc2d95a1d8b288c7c72d03c7cf4c5c54cf10a52426cdb10`.
Its compile defaults matched the current field shape: LFP, channel 11, 300 mA
precharge, 120-second day sleep, 12-second wake listen, field profile, listener
commission posture, two maintenance WiFi profiles, and exact target lock
`F2BCF0`.

The retained restore artifact was:

```text
fw_rev: fx-260827-1254f04-p
bytes: 1207376
sha256: 2f9a93344e172b023ee8df473b7c747b26f38dc0ec5353f6efd00d50ec45f4af
```

## Preflight and OTA

Physical identity was confirmed by Ben. Fresh dashboard state matched callsign
Spyro and short MAC `F2BCF0`, with field profile, perimeter class, no class
mismatch, no BQ fault, LED rail off, and ordinary 120-second sleep cadence.
VBAT was about 3.16 V and the power policy still reported FULL. Sunrise moved
the input from below `supply_good` to valid input before upload.

The prior image reported `sensor_bits=2` (VL53L5CX only). The historical
commissioning failure did include the old MSA301-address class-probe bug, but
the corrected `0x62`/PART_ID probe in the canary also reported only bit 2 at
boot. Spyro's MSA311 therefore remained absent or unresponsive in this run;
the VL53 experiment was still correctly classified and allowed to proceed.

Job `736C52A6` used the exact canary bytes but timed out in discovery; it made
no upload. Retry job `B325E6FE` found exact Spyro at `192.168.1.99`, rechecked
fresh maintenance power at 3.186 V / 4.665 V input, uploaded the exact canary,
received a fresh software-reset heartbeat, and verified the revision beyond
the pending-verify gate at 31,199 ms uptime. Profile remained field, class
remained perimeter, and recovery state remained zero.

## Physical interaction

The automatic sequence ran approximately:

```text
06:50:44-07:00:44 local  baseline A, radio off, sensor rail off
07:00:44-07:01:14 local  sensor warmup, radio off
07:01:14-07:11:14 local  MSA311/VL53 runtime requested, radio off
07:11:14-07:21:14 local  baseline B, radio off, sensor rail off
```

Ben performed about 10 or more close palm holds near the middle of the active
phase and another 5-6 near its end, approximately 15-16 total. Each set used
roughly one-second holds with clears between them. This is useful field input,
but the trace was not recovered, so firmware presence-edge counts, misses, and
latency cannot be compared with the human count.

The sun rose rapidly during the campaign. The A/B/A shape can reject a modest
linear trend, but it cannot make nonlinear sunrise input controlled evidence.
No battery-current delta or daily energy projection is accepted from this run.

## Trace loss and recovery

At the expected retrieval time Spyro did not expose HTTP on either known
subnet. It retried maintenance while retaining the buffer, but the dashboard
later showed a new canary heartbeat with reset reason `task_watchdog` and about
30 seconds uptime. PSRAM does not survive that reset, so the completed trace
was lost.

The fixture task watchdog is eight seconds. Maintenance previously called the
synchronous `WiFi.scanNetworks()` and serviced the watchdog only before and
after that unbounded scan. This is the only unserviced blocking operation in
the observed retrieval/join path and is consistent with a crowded-field scan
exceeding eight seconds. Source now uses an asynchronous scan, polls it while
servicing the watchdog, and falls back to configured credential order after a
15-second bound.

Spyro recovered without opening the enclosure. A targeted restore gather found
the rebooted canary at `192.168.1.99`; exact restore job `DCE67738` uploaded the
retained prior binary and verified a fresh software-reset heartbeat beyond the
pending-verify gate at 42,123 ms uptime. Final evidence was exact prior revision,
field profile, perimeter class, FULL tier, no BQ fault, about 3.20 V VBAT, and
valid external input. No fleet or profile broadcast was used.

## Retained ledgers

- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-canary-ota-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-canary-ota-r2-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-135003-B325E6FE-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-restore-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-restore-r2-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-143320-DCE67738-fleet-ota-results.jsonl`

## Required rerun

1. Hardware-prove the watchdog-safe maintenance scan and trace retrieval before
   trusting another full campaign.
2. Re-run under consistently shaded/battery-isolated input for the incremental
   radio-off VL53 current.
3. Re-run in stable full sun for net energy and at least 20 deliberate palm
   approaches with recovered edge counts.
4. Re-probe Spyro's MSA311 at `0x62` and inspect its cable/chain if it remains
   absent; do not describe this run as MSA-inclusive without fresh health proof.
