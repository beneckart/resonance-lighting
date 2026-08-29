# Perimeter sentinel canary -- Spyro F2BCF0 -- 2026-08-29

## Result

Three exact-target radio-off + perimeter-ToF A/B/A field campaigns completed
their interaction sequences, but none produced an acceptable power trace.
Rapid sunrise changed solar input across the first run. The first two completed
PSRAM buffers were then erased by task-watchdog resets while maintenance WiFi
was trying to start. The second and third runs held the panel consistently
covered and therefore fixed the environmental control, but independently
reproduced the retrieval/checkpoint failure.

Treat these as useful interaction/retrieval-path evidence, not as a measured
sentinel current delta and not as a promotion gate pass. Every canary was
restored without opening the enclosure. The final exact-prior restore was
armed while the canary was stuck in an unintended reset/rerun loop, intercepted
the next brief mesh window, and completed without operator involvement.

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
`fx-260827-1254f04-p` binary and its exact SHA-256. It addressed only `F2BCF0`,
found Spyro at `192.168.1.99` with 3.148 V VBAT, rechecked maintenance power,
uploaded the exact prior image, and received a fresh software-reset rejoin. The
fixed verifier accepted a later same-boot heartbeat at 27,720 ms, beyond the
25-second pending gate. Final dashboard evidence was exact prior revision,
field profile, perimeter class, `sensor_bits=2`, recovery state zero, no BQ
fault, and charging input after Ben uncovered the panel. No lid access, fleet
broadcast, or profile mutation was required.

## Persistent-canary post-mortem

The third canary used clean source `ab27ea3` and immutable exact-target artifact:

```text
fw_rev: fx-260829-96862d8-t
bytes: 1227152
sha256: bcb5b98c616a5ef578600f6e67623748d48bd8bf27808ca910b06dcfb80986cc
recipe_sha256: 96862d8b4b1117cce4d1a256bc0e951fb11e4a6acc9d6c14b85ef2944de31be6
target: F2BCF0 (Spyro), test-only and not fleetable
```

OTA job `34CB76FC` found only Spyro at `192.168.1.99`, rechecked zero
covered-panel input, uploaded the sealed SHA-256, and formally verified the
exact revision at 28,168 ms same-boot uptime. Ben then made at least 18-21
reported close approaches during the active window (12-15 followed by about
6); four additional cycles were requested but not explicitly confirmed. The
interaction count is operator context only because no trace was recovered.

The critical failure sequence was reconstructed from live mesh timing:

```text
~09:43:30 local  third measurement window ended; checkpoint/retrieval began
~09:44:30        task-watchdog reboot on the exact canary
~09:45:10        fresh boot reached about 31 s uptime, then radio disappeared
~09:55:16        another task-watchdog reboot
~09:55:47        fresh boot again reached about 31 s uptime, then radio disappeared
 10:06:02        restore gather caught the next maintenance window and uploaded prior
```

The first post-reset radio pattern is the exact-target canary's 30-second radio
settle followed by baseline A. It proves that boot did not accept the completed
checkpoint and silently started a new campaign. The old persistence code
returned only `false`: it did not report whether the header was missing,
partially committed, CRC-invalid, capacity-rejected, or sample-invalid, and it
did not read the stored bytes back after the write. The precise rejected field
is therefore not recoverable from this run. This is an observability defect as
well as a persistence defect; the evidence supports "checkpoint absent or
rejected," not a more specific flash claim.

The next reboot occurred about one 30-second settle plus one 10-minute baseline
after the first. That places it at `startTof()`. The perimeter initializer
uploads the VL53L5CX firmware blob synchronously over the mandatory 100 kHz bus.
The high-level call had watchdog resets only before and after it, while the
vendored I2C chunk loop had none. A healthy complete upload can therefore exceed
the fixture's eight-second task watchdog. The timing plus the only unserviced
operation at that transition makes this the supported cause of the repeating
rerun resets.

Fresh post-watchdog heartbeats reported `sensor_bits=10`, meaning MSA311 plus
VL53L5CX were both acknowledged on this boot. This confirms that the corrected
MSA311 `0x62` address can see Spyro's device. The earlier bit-2-only boots remain
evidence of intermittent probe/rail-chain behavior, not proof that MSA311 is
absent.

### Corrective actions

The persistence format is now a two-sector fail-closed journal:

1. Before measurement, the exact artifact erases and writes a CRC-protected run
   marker and reads it back. Failure enters maintenance; measurement never starts.
2. The completed header and sample data live after that marker. The header is
   still written last, but the marker is never erased during checkpointing.
3. Firmware reads back the complete stored trace, validates artifact tag,
   schema, sample size, capacity, no overwrite, contiguous sequence, header CRC,
   and sample CRC, and only then reports `persisted=true`.
4. A reset with a matching run marker but no valid checkpoint enters retrieval
   recovery and reports the exact rejection state. It cannot start another
   campaign or ask for another interaction sequence.

The vendored VL53 I/O now services the already-armed fixture watchdog between
bounded 100 kHz write/read chunks and ULD waits. The hook is a no-op during
ordinary boot before the watchdog is armed. This preserves the eight-second hang
boundary while supervising a long but advancing firmware transfer.

The operator process also failed. The unproven persistence path was tested by
running another full 30-minute campaign and asking Ben for another interaction
set. Later, radio silence was interpreted from estimated phase time rather than
positive device state, leading to yet another request after the canary had
already restarted. No further human-assisted campaign is permitted until the
new `--sentinel-trace-smoke` build completes its 40-second automatic sequence,
survives a deliberate same-artifact OTA reset, and returns the same persisted
count/sequence over HTTP. Smoke telemetry is explicitly tagged, and the power
capture tool refuses to accept it as measurement evidence.

Exact-prior recovery job `E1428FE2` ran unattended with a 900-second discovery
window and addressed only `F2BCF0`. It found the canary at `192.168.1.99`,
uploaded the retained 1,207,376-byte `fx-260827-1254f04-p` binary with SHA-256
`2f9a93344e172b023ee8df473b7c747b26f38dc0ec5353f6efd00d50ec45f4af`,
and formally verified a fresh software-reset rejoin at 30,316 ms uptime. Final
state was field profile, perimeter class, recovery zero, and no commission
residue. No lid access or physical action was required.

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
- `ops/bench/data/Black Rock City/20260829-154531-612D848D-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-persist-canary-ota-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-161217-34CB76FC-fleet-ota-results.jsonl`
- `ops/bench/data/Black Rock City/20260829-spyro-F2BCF0-sentinel-persist-restore-job.jsonl`
- `ops/bench/data/Black Rock City/20260829-170602-E1428FE2-fleet-ota-results.jsonl`

## Required rerun

1. Hardware-prove the 40-second no-human smoke artifact, including complete
   checkpoint readback, same-artifact OTA reset survival, identical post-reset
   count/sequence, and exact-prior restore. Do not run another full campaign or
   request physical interaction before this passes.
2. Re-run under consistently shaded/battery-isolated input for the incremental
   radio-off VL53 current.
3. Re-run in stable full sun for net energy and at least 20 deliberate palm
   approaches with recovered edge counts.
4. Re-probe Spyro's MSA311 at `0x62` and inspect its cable/chain if it remains
   absent; do not describe this run as MSA-inclusive without fresh health proof.
