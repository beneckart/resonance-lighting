# Perimeter sentinel canary -- Spyro F2BCF0 -- 2026-08-29

## Result

Two exact-target radio-off + perimeter-ToF A/B/A field campaigns completed
their interaction sequences, but neither produced an acceptable power trace.
Rapid sunrise changed solar input across the first run, and both completed
PSRAM buffers were then erased by task-watchdog resets while maintenance WiFi
was trying to start. The second run held the panel consistently covered and
therefore fixed the environmental control, but it independently reproduced the
retrieval failure.

Treat these as useful interaction/retrieval-path evidence, not as a measured
sentinel current delta and not as a promotion gate pass. The first run was
restored without opening the enclosure. An exact-prior restore gather was
armed after the second reset and is retained in the job ledger below.

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

## Controlled-shade rerun

The rerun used clean source `61f2b0c` and a newly sealed exact-target artifact:

```text
fw_rev: fx-260829-9f140c3-t
bytes: 1225728
sha256: 03a100407b7a51e61f61cb9b81c855e39d7405dadc9ddd893c97ba51906c7512
recipe_sha256: 9f140c31f3c7056ba7a9ccad833077bd36de56d9bb83ef0b74eff02782e922ac
target: F2BCF0 (Spyro), test-only and not fleetable
```

Ben physically covered Spyro's panel and kept the fixture orientation fixed.
Fresh preflight showed `supply_good=false`, about 2.32-2.38 V input, zero input
current, about 3.15 V VBAT, and no BQ fault. Exact OTA job `A54875FD` found only
Spyro at `192.168.1.99`, rechecked the covered power state, and uploaded the
sealed binary. The new image produced a fresh exact-revision software-reset
heartbeat at 2,625 ms and a later same-boot heartbeat at 29,575 ms. That later
heartbeat proves survival beyond the 25-second pending-verify gate.

The host verifier nevertheless returned a false failure because its 5-second
freshness test was narrower than the bridge's cached-peer report cadence. It
saw the exact revision but did not accept the later 29,575 ms heartbeat after
the embedded peer age had crossed five seconds. Verification now latches one
fresh exact-revision boot and accepts a later same-boot uptime/sequence report;
uptime and sequence must remain monotonic, so cached evidence cannot cross a
reboot. The focused OTA/capture suite passes 20 tests.

The controlled campaign ran approximately:

```text
08:03:07-08:13:07 local  baseline A, radio off, sensor rail off
08:13:07-08:13:37 local  VL53 warmup, radio off
08:13:37-08:23:37 local  VL53 active, radio off
08:23:37-08:33:37 local  baseline B, radio off, sensor rail off
```

Ben performed about 10 or more deliberate close-palm approaches late in the
active phase and another 4-5 near its end, approximately 14-15 total, using
clear intervals to re-arm. Because the trace was again lost, no firmware edge
count or miss rate can be accepted.

The watchdog-safe asynchronous scan did not make the volatile retrieval path
acceptable. No HTTP endpoint appeared, and the dashboard then reported a new
`task_watchdog` boot. This second independent loss establishes that the trace
must not depend on PSRAM surviving any WiFi startup or request path. Source now
checkpoints the completed trace into the otherwise-unused 1.5 MB SPIFFS data
partition before WiFi starts. It erases/writes in watchdog-fed chunks, writes
the header last, and validates artifact tag, schema, sample size, header CRC,
sample CRC, count, overwrite state, and exact sequence before restoring. A
later watchdog boot re-enters retrieval with the flash copy instead of starting
a new measurement. The full native suite, 20 focused Python tests, and an
ESP32-S3 development compile pass. This persistence path is not yet proven on
hardware and is the next canary gate.

Exact-prior restore job `612D848D` was armed after the reset with the retained
`fx-260827-1254f04-p` binary and its exact SHA-256. It continuously addresses
only `F2BCF0` and waits for Spyro's next radio/maintenance window; no lid access
or fleet broadcast is required.

## Retained ledgers

- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-canary-ota-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-canary-ota-r2-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-135003-B325E6FE-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-restore-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-restore-r2-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-143320-DCE67738-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-canary-rerun-ota-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-150227-A54875FD-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-rerun-restore-job.jsonl`

## Required rerun

1. Hardware-prove completed-trace flash checkpoint, reset survival, and
   retrieval before trusting another full campaign. WiFi startup may still be
   diagnosed separately, but it can no longer be allowed to erase evidence.
2. Re-run under consistently shaded/battery-isolated input for the incremental
   radio-off VL53 current.
3. Re-run in stable full sun for net energy and at least 20 deliberate palm
   approaches with recovered edge counts.
4. Re-probe Spyro's MSA311 at `0x62` and inspect its cable/chain if it remains
   absent; do not describe this run as MSA-inclusive without fresh health proof.
