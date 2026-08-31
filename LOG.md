# LOG

Append-only session journal for the Resonance Lighting workstream. Most recent first.

Format per entry:

```
## YYYY-MM-DD -- author -- short subject

Body. What changed, what was decided, what's next.
```

## 2026-08-30 -- Ben + Codex -- Final-burn autonomous art bundle implemented

Implemented the deadline-driven single field bundle in ADR 0072. Canopy
downlights now attempt one battery-safe cymbal hit after a trustworthy daytime
wake sample and a second before sleep, with a durable stochastic attenuation
knob defaulting to 100 percent. The UTC-hour ritual now uses the same FULL/DIM
safety boundary instead of requiring solar surplus; OFF/PROTECT and every hard
mechanism gate remain vetoes.

Perimeter interaction now holds the crisp center pixel through transient
too-close invalid/zero distances only after a proven valid close approach.
Program/wave origins now default to a five-minute minimum plus 30 seconds
continuously clear before re-arm, without slowing local sensor rendering. A
rootless UTC schedule rotates every 10 minutes through existing CA, Color
Virus, Epidemic, and a K=2/no-spark visitor-started CA. Canopy and perimeter
ToF fixtures may originate these program events; other classes relay/render.

Added canonical append-only packet type 31 and one versioned NVS blob for the
four field knobs, plus a confirmed/audited T-Deck Field screen. The first full
fixture native run passed. A fresh T-Deck embedded build passed at 49 percent
flash and 59 percent static RAM. The T-Deck native wrapper remains blocked by
two newly preserved commissioned fixtures (`9F268C`, `F402A8`) lacking callsign
assignments; no physical identity was invented to bypass that data contract.

## 2026-08-30 -- Ben + Codex -- Laptop-retirement field evidence preserved

Fetched current `origin/main` before retiring the Nevada City/BRC laptop and
reconciled every linked worktree against the remote. Preserved the uncommitted
canonical fleet registry work from the 2026-08-30 perimeter recovery session:
one newly registered fixture and 13 updated perimeter fixtures, all with unique
IDs and parseable CSV records. Several records deliberately leave firmware
identity blank where the reported image had no matching immutable manifest or
binary SHA in this checkout.

Also retained six previously untracked Black Rock City JSONL records: the
arrival censuses, the successful Dixie and Froakie canary OTA jobs, Kairi's
freeze-unconfirmed recovery attempt, and Sakura's dry-run ledger. The short
arrival trace carries incorrect default battery metadata and is explicitly
excluded by the following authoritative trace; it remains as raw append-only
evidence rather than being silently discarded. The two Sakura canary/post-TMF
records found in the older release worktree were byte-identical to files already
on `origin/main` and were not duplicated.

## 2026-08-29 -- Ben + Codex -- Completed-work integration prepared for main

Collected every unique completed change from the active fixture, daytime
ritual, canopy canary, and PUCA worktrees onto current `origin/main` without
mutating or discarding their source histories. The integration retains the raw
Spyro, Gible, Astro, Donkey, and fleet-operation ledgers; combines the canopy
motion/presence diagnostics with automatic immutable artifact identity; keeps
PROTECT sleep ownership in power policy; and includes the hourly cymbal ritual,
fail-closed sentinel persistence, Spyro post-mortem, installed-height findings,
and capacitive-paw validation. Two retained Spyro JSONL records were normalized
from CRLF to LF with no JSON content change.

The complete fixture native suite passes, including the build-wrapper artifact
contract and all combined lifecycle, ritual, motion, and sentinel regressions.
The PUCA native suite passes 129 checks, and 41 focused Python motion, sentinel,
dashboard OTA, and dashboard tests pass. Final integration is ready for the
explicit `origin/main` update requested by Ben.

## 2026-08-29 -- Ben + Codex -- Eevee recovered from low-voltage PROTECT

Exact outer-ring downlight Eevee `9F0E54` / `D8:85:AC:9F:0E:54` appeared on
COM70 after Ben tapped physical RESET. Read-only telemetry on dual-site image
`fx-260828-658b7d2-p` showed a genuine retained 3.030 V PROTECT entry, 3.304 V
battery with about +1.05 A charging, good USB input, no BQ fault, and stale
6 Ah NVS capacity despite the registry's physical 15 Ah assignment. The old
image's longer physical-reset service window stayed awake long enough to
complete its healthy-battery proof, persist OFF, and reboot. MSA311, TMF8820,
and required outer-ring BMP581 then all sampled normally.

Reused already-built immutable canary `fx-260829-b0ff5db-b`, SHA-256
`276d6558116a40da15f32eccf6bc7a940ef6d827ba965ddb81a8a8f5b0e27ae0`; no
compile ran. Exact USB commissioning restored 15,000 mAh capacity and passed
the shared-WiFi endpoint at `192.168.1.119`. After explicitly returning from
maintenance WiFi to COMMS, settled telemetry showed 3.314 V / +1.108 A,
good 4.637 V input, 2 A charge configuration, no BQ fault, tier 2 / stage 3,
field profile, channel 11, ESP-NOW up, exact revision, `pending_verify=false`,
and healthy MSA/TMF/BMP samples with zero TMF errors. Evidence is in
`ops/bench/data/usb/2026-08-29-9F0E54-protect-canary.jsonl`.

Eevee is ready to unplug, but this variant-`b` service image has only the
Party In The Woods maintenance profile. Add it to the future dual-site
production tail before fleet closure.

## 2026-08-29 -- Ben + Codex -- Logan proves the fixed timer-wake PROTECT release

Exact downlight Logan `9E5A88` / `D8:85:AC:9E:5A:88` appeared on COM158 during
a short deep-sleep wake. Read-only telemetry on dual-site production image
`fx-260828-658b7d2-p` showed 3.595 V / 100 percent, good 6.245 V input,
charging enabled, no BQ fault, and durable power tier 3 / guard stage 4
PROTECT. Its retained sleep receipt named the immediately preceding failure:
120-second `day-charge` sleep at tier 3. That is the exact lifecycle race
previously diagnosed on Toad. The port disappeared at the old 12-second wake
boundary as expected.

Caught the next exact wake and uploaded already-built immutable bench canary
`fx-260829-b0ff5db-b`, binary SHA-256
`276d6558116a40da15f32eccf6bc7a940ef6d827ba965ddb81a8a8f5b0e27ae0`;
no compile ran. Ben confirmed this physical downlight has a 15 Ah cell, so its
stale 6 Ah NVS capacity was corrected to 15,000 mAh. The retained PROTECT latch
survived both upload and the configuration reboot.

Issued one exact-target, one-second serial sleep while the canary still held
PROTECT. On the resulting genuine deep-sleep wake, Logan remained awake at
21.636 seconds with tier 3 / stage 4, good input, no fault, and the exact image
-- directly beyond the old 12-second day-sleep cutoff. It then completed the
sustained full-battery release, persisted stage 3 / LEDS_OFF, software-rebooted,
and advanced to stage 2 / DIM. Settled telemetry showed field profile, channel
11, exact image, no interruption, healthy MSA311 and TMF8820 samples with zero
TMF errors, ESP-NOW, and `pending_verify=false`. The first WiFi HTTP read had an
empty body after successful association; one explicit COMMS resume and bounded
transport-only retry passed the exact endpoint at `192.168.1.186` without a
second upload. This is the missing real-hardware acceptance of the fixed
timer-wake branch. Evidence is in
`ops/bench/data/usb/2026-08-29-9E5A88-protect-timer-release-canary.jsonl` and
`ops/bench/data/usb/2026-08-29-9E5A88-protect-timer-release-proof.jsonl`.

Like Toad and Groot, Logan currently carries the one-profile variant-`b`
service image and must receive the future dual-site production artifact before
fleet closure.

## 2026-08-29 -- Ben + Codex -- Shuckle isolated to a bad sensor-chain cable

Exact downlight Shuckle `F4031C` / `68:EE:8F:F4:03:1C` appeared on COM33 on old
commission image `fx-260817-ec7f28d-b`. It initially reported the last reset as
panic, `pf_ready=false`, no battery, no sensors, and no rail. A non-writing ROM
flash-ID check proved healthy ESP32-S3 revision 0.2, 8 MB flash, 2 MB physical
PSRAM, and exact MAC. The following captured boot reproduced four failed
`Board.init` attempts, failed VSQT off/on control, a class mismatch, and repeated
LED-rail pad verification failures. No image was built, flashed, or erased.

Ben fully depowered the fixture and disconnected the external STEMMA chain.
USB-only boot immediately restored the PowerFeather, charger/gauge telemetry,
300 mA precharge readback, no-fault BQ, and rail control. TMF8820-only then
passed presence and ranging with zero errors; MSA311-only passed presence and a
healthy sample. Reattaching both sensors reproduced the hard bus failure with
the original and replacement chain attempts. The suspect inter-sensor cable
finally failed even when connected directly from the PowerFeather to the
known-good MSA311, isolating a defective or miswired cable rather than a dead
PowerFeather or sensor.

Before a separately proven replacement cable was available, Ben accidentally
connected solar without the battery. Shuckle then stopped enumerating over USB;
the planned USB-only bootloader recovery was deferred when Ben moved to the next
fixture. Registry status is now quarantined with explicit do-not-install notes.
Next service must remain USB-only, preserve flash/NVS, replace and independently
prove the cable, restore the full chain and confirmed 15 Ah battery, then pass
exact USB/WiFi commissioning before quarantine is removed.

## 2026-08-29 -- Ben + Codex -- Groot recovered and received the PROTECT-sleep fix

The next USB-connected problem fixture did not enumerate during a no-reset
watch lasting longer than its ordinary 120-second day cycle. One deliberate
physical RESET exposed exact downlight Groot `9F2724` /
`D8:85:AC:9F:27:24` on COM76. Read-only telemetry on accepted production image
`fx-260827-1254f04-p` showed a real installed 15 Ah LFP, durable guard stage 4 /
power tier 3 PROTECT, good 4.597 V USB input, +547.5 mA battery charging, and no
BQ fault. The retained entry was at 2.974 V, so unlike Rikku this was consistent
with a genuine earlier low-voltage latch.

The physical reset provided the ten-minute cold-boot listen window. Groot
therefore completed the existing 60-second charge-current release, persisted
stage 2 / DIM, and clean-software-rebooted before any flash. Both required
downlight sensors then returned healthy. This distinguishes the initial USB
silence from a bad cable or dead board: the fixture had been in its 900-second
PROTECT sleep cadence, and USB insertion alone is not a wake source. The old
image remained vulnerable on ordinary 12-second timer wakes to the independent
day-charge sleep bug already proven on Toad.

Uploaded the already-built immutable bench canary `fx-260829-b0ff5db-b`; no
second compile occurred. Exact USB identity, ESP32-S3 revision 0.2, 8 MB flash,
2 MB physical PSRAM, binary SHA-256, field/channel-11 configuration, 15 Ah /
2 A settings, 300 mA precharge, no-fault charging, downlight class, MSA311 and
TMF8820 samples, ESP-NOW, and shared-WiFi maintenance endpoint all passed. A
settled post-check reported the exact revision, stage 2 / DIM, no interruption,
3.274 V at +543.1 mA, and `pending_verify=false`. This remains a one-profile
variant-`b` service image, not the future dual-profile fleet artifact. Evidence
is in
`ops/bench/data/usb/2026-08-29-9F2724-protect-day-sleep-canary.jsonl`.

## 2026-08-29 -- Ben + Codex -- Toad fixed; artifact identity made automatic

Fixed the lifecycle ownership bug diagnosed below. `lifeTick` now suppresses
ordinary field `DAY_CHARGE` sleep whenever the power tier is PROTECT, leaving
the PROTECT cadence and qualified 60-second release entirely under power
policy. Native regression coverage keeps ordinary FULL/DIM/OFF day sleep
unchanged and pins PROTECT `wantSleep=false`. Integrated the accepted ADR 0069
and ADR 0070 source before validation; the complete native suite passed, as did
a guarded ESP32-S3 field/basic-listener build.

Built immutable canary `fx-260829-b0ff5db-b` from clean commit
`280598802a306c7d3c2b901b480f2dac864f4ca5`. Its canonical recipe SHA-256 is
`b0ff5db9edd5f65d1e53fbcbba85de6a291ee2e83527dd802e0e451183020071`;
the 1,212,640-byte binary SHA-256 is
`276d6558116a40da15f32eccf6bc7a940ef6d827ba965ddb81a8a8f5b0e27ae0`.
It is deliberately variant `b` and labelled `party-in-the-woods-v1`, because
this checkout has one local maintenance profile rather than the accepted
dual-profile fleet credential set.

The exact USB serial identity on COM56 matched Toad `F2BEE4` before upload.
Flash-ID, upload, embedded revision, field/channel-11 configuration, 15 Ah / 2 A
settings, no-fault BQ, downlight class, class match, MSA311, TMF8820, BMP581,
ESP-NOW, and pending-verify=false checks passed. A later deep-sleep wake still
reported the exact image; by 9.712 s it was charging at +536.7 mA from 4.637 V /
504 mA good input with all three sensors healthy. The retained boot stage was
already DIM after flash, so this hardware run did not recreate a live PROTECT
release; the native lifecycle test is the direct proof of that branch. Toad is
no longer parked and its ordinary 120-second field cycle is operating. Evidence
is in `ops/bench/data/usb/2026-08-29-F2BEE4-protect-day-sleep-canary.jsonl`.

The first artifact candidate exposed the recurring manual identity hazard: its
hash was calculated without the canonical recipe's final LF. The mismatch was
caught before upload and `fx-260829-e9350b7-b` was marked rejected/never
flashed. To remove that class of wasted cold build, `build.sh` now owns the
immutable transaction via `--artifact-variant`: clean-source check, exact
compact-JSON-plus-LF recipe bytes, derived revision and path, embedded identity,
fresh build, manifest/binary hashes, and post-build cross-check. Manual
`--artifact-dir`/`--fw-rev` is refused, direct flash/OTA is refused in artifact
mode, and profile/channel/non-secret WiFi label must be explicit. A compile-free
golden test reproduces accepted recipe hash `d374034...`, so newline, encoding,
key order, or normalization drift fails before compilation. The full native
suite passes with this contract. Also quoted three pre-existing registry notes
whose commas had malformed CSV rows, then recorded Toad's exact canary identity.

## 2026-08-29 -- Ben + Codex -- Toad PROTECT recovery defeated by day sleep

Onboarded from the repository and watched Windows USB long enough to catch the
short field wake of exact outer-ring downlight Toad `F2BEE4` /
`68:EE:8F:F2:BE:E4` on COM56. No firmware, profile, NVS, reboot, rail, or other
device mutation was requested. The host added the repository-documented
`pyserial` dependency only so the existing no-reset telemetry path could run.

Read-only telemetry identifies accepted artifact `fx-260827-1254f04-p`, field
profile, channel 11, correct 15,000 mAh capacity, downlight class, valid OTA
state, and durable guard stage 4 / power tier 3 PROTECT with the LED rail off.
The retained PROTECT entry was at 3.039 V in field profile after about 1,637 s
of fixture uptime, consistent with the known low-VBAT history. A separate
durable receipt records a prior six-hour radio-all sleep from exact T-Deck
`8EB508`, sequence 10, but the current recurring sleep is local rather than a
still-active operator command.

USB power and the charge path are healthy. On the next 120-second wake, the
early stale gauge sample corrected by 5.9 s; at 11.151 s telemetry reported
3.283 V, +506 mA corrected battery current, 4.637 V / 486 mA good input,
charging enabled, no BQ fault, and verified 300 mA precharge. The port then
disappeared at the approximately 12-second wake boundary. The following boot
record named the preceding sleep `day-charge` for 120 seconds, not PROTECT for
900 seconds.

This proves a control-plane race, not a failed cell, charger, cable, or USB
port. `power_policy` correctly suppresses its own PROTECT sleep while the
60-second charge-current release proof is accumulating, but `behavior_glue`
independently takes ordinary field `DAY_CHARGE` sleep once the ADR 0064 power
sample window completes. That path does not exclude `LedTier::PROTECT` or
otherwise honor the in-progress release hold. Deep sleep clears the RAM-only
release timer, so each 12-second wake restarts a proof that requires 60
continuous seconds and Toad can never release. The current Aug-28 source and
accepted artifact change PROTECT proof quality/provenance but leave this
day-sleep gate unchanged, so updating alone is not yet an adequate repair.

Leave Toad connected: it is safely dark and gains charge during each wake and
subsequent sleep, but the durable latch will remain until the sleep ownership
bug is fixed or an explicitly controlled service posture keeps it awake long
enough for the automatic release. Next is a native regression and source fix
making the power-policy path the sole owner of PROTECT sleep, followed by a
clean immutable artifact and exact-Toad canary under ADR 0040.

## 2026-08-29 -- Ben + Codex -- Primary T-Deck updated to current Bridge OS

USB-flashed the other known T-Deck Plus, exact identity `8EB508`
(`44:1B:F6:8E:B5:08`), observed on `COM152`. The guarded development-cache
wrapper rebuilt from pushed commit `30c4124` before upload. The resulting
`tdeck-dev-local` binary is 1,565,584 bytes with SHA-256
`345f150d53ceec57de6925531953a38e49cff2de87523ab359cd6fa36992721d`.
Esptool verified every written region and reset the board successfully.
Post-reset serial reported the exact `8EB508` identity on mesh channel 11,
healthy 8 MB PSRAM, keyboard, touch, ES7210, and GPS probes, active mesh receive,
and zero send failures. The camp-labelled TSwift `979604` was not connected or
changed during this flash.
## 2026-08-29 -- Ben + Codex -- Spyro sentinel post-mortem; fail-closed recovery and VL53 watchdog fix

The first flash-persistence hardware follow-up used exact Spyro `F2BCF0` and
target-only artifact `fx-260829-96862d8-t`. OTA job `34CB76FC` verified the
test image through pending verify, but the run did not produce an accepted
retrievable trace. Exact restore job `E1428FE2` later returned Spyro to retained
fleet artifact `fx-260827-1254f04-p` and verified the restore through pending
verify. Retain both operation ledgers, but do not count this as the required
hardware persistence acceptance.

Ben reported at least 18-21 close approaches during the controlled run. After
the expected retrieval transition, Spyro rebooted with `task_watchdog`, failed
to accept the checkpoint without reporting why, and silently started another
30-minute campaign. It rebooted again one 30-second settle plus one 10-minute
baseline later, exactly at `startTof()`. The synchronous VL53 firmware upload
over the required 100 kHz bus serviced the watchdog only before and after the
whole transfer; the timing and source isolate that unserviced transfer as the
rerun watchdog. Fresh boot telemetry reported `sensor_bits=10`, proving MSA311
at corrected address `0x62` plus VL53 on that boot. Earlier bit-2-only boots are
therefore intermittent probe/rail-chain evidence, not proof of absence.

Source hardening now treats an interrupted or corrupted campaign as a recovery
condition instead of silently starting the physical experiment again. A
separate CRC-protected run marker is written before acquisition. On boot, a
matching marker without a valid artifact-bound checkpoint enters rail-off,
recovery-only maintenance. Completed checkpoints are read back and fully
validated before persistence is advertised. Validation reasons are explicit in
telemetry and the HTTP trace header, and the host capture tool refuses a
recovery-only fixture. The pure core module pins the packed marker/header
layouts, artifact tag, CRCs, sequence/count constraints, and named failure
reasons. Long VL53 transfers service the task watchdog through the shared
watchdog seam. A new `--sentinel-trace-smoke` option compresses the automatic
sequence to 40 seconds. Smoke telemetry is tagged and the power downloader
rejects it, so checkpoint/readback, same-artifact reset survival, and retrieval
can be proven without operator interaction before another full campaign.

The complete native fixture suite passes, including the expanded 23-check
sentinel regression; 25 focused Python tests pass; and an ESP32-S3 development
compile passes at 36% flash / 20% globals. The process failure is explicit: an
unproven persistence path was tested with another full campaign, and phase was
later inferred from silence, causing unnecessary repeated requests to Ben. No
further human-assisted campaign is permitted until smoke checkpoint, deliberate
same-artifact reset, exact trace recovery without rerun, and exact-prior restore
all pass.

## 2026-08-29 -- Ben + Codex -- Shaded Spyro rerun reproduced retrieval reset; trace persistence added

Re-ran the exact-target perimeter sentinel A/B/A on Spyro `F2BCF0` with the
panel physically covered and orientation fixed. Immutable artifact
`fx-260829-9f140c3-t` from source `61f2b0c`, 1,225,728 bytes, SHA-256
`03a100407b7a51e61f61cb9b81c855e39d7405dadc9ddd893c97ba51906c7512`,
uploaded only to Spyro. Fresh input stayed below `supply_good` at zero input
current, and Ben made about 14-15 close approaches during the VL53 phase.

The image reported exact revision at 2,625 ms and again from the same boot at
29,575 ms, proving the 25-second pending gate. The host verifier falsely timed
out because its five-second live-age limit was narrower than the bridge's
cached-peer report cadence. It now latches a fresh exact-revision boot and can
accept a later same-boot monotonic uptime/sequence report without allowing
cached identity to cross a reboot.

The completed controlled trace was again erased by a `task_watchdog` reset
before HTTP retrieval. This independently reproduces the retrieval fault and
shows that watchdog-safe async scan alone cannot make volatile PSRAM evidence
acceptable. Exact-target sentinel builds now checkpoint the completed trace to
the otherwise-unused SPIFFS data partition before WiFi starts. Sector-bounded
erase/write operations feed the watchdog; the header is written last; and boot
requires matching artifact tag, schema, sample size, header/sample CRCs, count,
no overwrite, and exact sequence before resuming retrieval. A later WiFi reset
therefore retries with the flash copy instead of re-running the experiment.
The full native suite, 20 OTA/capture Python tests, and an ESP32-S3 development
compile pass. Hardware persistence proof is still required. Exact-prior restore
job `612D848D` subsequently found only Spyro at `192.168.1.99`, uploaded the
retained `fx-260827-1254f04-p` bytes, and formally verified a fresh same-boot
heartbeat at 27,720 ms. Final state was exact prior revision, field profile,
perimeter class, `sensor_bits=2`, recovery zero, no BQ fault, and charging input
after the panel was uncovered. No lid access was required.

## 2026-08-29 -- Ben + Codex -- Spyro sentinel run restored safely; retrieval watchdog fixed

Ran the first exact-target ADR 0071 sentinel campaign on physically identified
perimeter fixture Spyro `F2BCF0`. Clean immutable artifact
`fx-260829-f13e090-t` from source `bb86098`, 1,225,488 bytes, SHA-256
`249c500e3a9bf12ab4902ec1db1c18680605d3a1a35bf7933e2c162dc1061cb1`,
passed single-target OTA, fresh exact-revision rejoin, and pending verify. Ben
performed about 15-16 close hand holds during the 10-minute radio-off VL53
phase. The corrected canary probe still reported `sensor_bits=2`, so Spyro's
MSA311 was not freshly proven despite the historical wrong-address bug.

The run is not an accepted power result. Rapid sunrise made the A/B/A solar
input nonlinear, and maintenance retrieval later hit an 8-second task watchdog
while no HTTP endpoint was available, erasing the completed PSRAM trace. The
unbounded gap was the synchronous all-channel WiFi scan; join loops already
serviced the watchdog. Maintenance now runs that scan asynchronously, polls
while resetting the watchdog, and falls back to configured credential order
after 15 seconds. Native tests, 17 Python OTA/capture tests, and an ESP32-S3
development compile pass.

Spyro was recovered without opening its lid. Exact restore job `DCE67738` found
it at `192.168.1.99`, uploaded retained prior artifact
`fx-260827-1254f04-p` / SHA-256
`2f9a93344e172b023ee8df473b7c747b26f38dc0ec5353f6efd00d50ec45f4af`,
and proved fresh software-reset rejoin beyond pending verify with field profile,
perimeter class, FULL tier, no BQ fault, and valid input. The full evidence and
required stable-shade/full-sun reruns are in
`docs/tests/PERIMETER_SENTINEL_CANARY_2026-08-29.md`.

## 2026-08-29 -- Ben + Codex -- Hourly cymbal ritual and sentinel power gate staged

Recorded the installed noisemaker truth: all 72 canopy/downlight mallets strike
small finger cymbals mounted on the bamboo, not bare bamboo. ADR 0071 accepts a
rootless hourly daytime ritual for energy-ready field downlights. Its fixed
T-20 through T+47-second window attempts a high-time-quality fleet unison at
T+5, one deterministic 500 ms hash-slot roll from T+12 through T+35.5, and a
one-quarter-fleet after-ring from T+42 through T+45.5. The unison abstains above
500 ms reported UTC uncertainty; the organic acts tolerate at most 3,000 ms.
Invalid time, scheduled night, weak energy, a bridge/program lease, wrong
class, or any existing solenoid hard gate remains an abstention.

DAY_ACTIVE now means energy permission rather than an all-day radio lease. A
field fixture returns to the ordinary bounded sleep/listen cadence from either
day state. A confirmed energy-ready state may cross one timer sleep, is
consumed at boot, and re-arms only if the fresh wake still satisfies the real
strike gate. With valid UTC, an eligible downlight shortens only the final
ordinary sleep needed to wake 20 seconds before the hour. The per-event ledger
is RTC-retained and marked before actuation so a mechanism refusal or reset
cannot retry the same event into a brownout loop.

Persistent perimeter sensing remains deliberately unimplemented. Instead, a
new exact-target `--sentinel-trace-target` test image runs an automatic 10-
minute radio-off/sensor-off baseline, 30-second VL53 warm-up, 10-minute radio-
off MSA311/VL53L5CX phase, and second 10-minute radio-off/sensor-off baseline.
One-second PSRAM records retain corrected/raw battery current, battery/supply
telemetry, charger state, explicit radio/rail truth, VL53 progress/range/zones,
and palm-cover edges. Maintenance WiFi starts only after the campaign. The
downloader verifies exact identity/revision/target/perimeter/completion, proves
radio and rail phase truth plus advancing VL53 frames, and exclusive-creates
the JSONL. The runbook requires physical target confirmation, one writer, an
immutable `-t` artifact, and restoration of the exact prior fleet binary.

The complete native fixture suite passes, including 18 daytime-ritual checks,
15 sentinel-ring checks, and the expanded build-wrapper contract. Six Python
capture tests and syntax compilation pass. A guarded, throwaway field/channel-
11 sentinel test compile with dummy target `A1B2C3` passes at 1,207,669 bytes
program / 68,804 bytes globals and emits a 1,207,984-byte binary. The temporary
binary was automatically discarded; no artifact was published or flashed.
Next is one named cymbal/downlight hourly-boundary canary and one physically
confirmed perimeter power campaign before any sentinel implementation.

## 2026-08-29 -- Ben + Codex -- Windy-night canopy presence passes

Ben physically located Sakura `F2BE0C` with Fleet Identify and corrected the
census inference: Sakura was in a bin, not installed high. Its all-zero TMF
scene therefore meant an open/clear view from the bin, not evidence of canopy
height. Ben pointed Sakura at himself and the ADR 0070 canary correctly changed
from its low-white baseline to full dedicated white. This closes the new
artifact's sensor -> presence latch -> visible output path. Separately, people
walking beneath the installed tree reliably caused canopy lights to turn white,
which is the intended artistic response. The precise named installed-height
artifact canary remains useful for quantitative margin, but the installation's
observable interaction is working.

The test occurred on a windy night and invalidated the mental model of a rigid
sensor observing a moving target. Each lantern swings chaotically as a pendulum
over roughly two degrees of freedom, so its TMF cone scans the space. A person
standing still can be discovered sequentially by several nearby fixtures as
their cones swing across them. This feels alive and should be treated as an
artistic affordance, not automatically motion-compensated away.

No firmware threshold changed from this observation. The next tuning step is a
correlated empty-wind/person trace of TMF depth, per-zone confidence, MSA311 sway
envelope, and visible presence state. Use it to distinguish desirable swinging-
scanner encounters from background/branch/ground false positives and to choose
minimum white hold, release hysteresis, and retrigger cooldown. Preserve the
current proven behavior until that evidence exists; do not use MSA as a blanket
veto because it would suppress the exact wind-assisted discovery Ben observed.

## 2026-08-29 -- Ben + Codex -- Exact-target canopy flight recorder staged

Implemented the evidence path requested by the windy-night observation without
changing the production interaction. An exact-target `-t` image now records a
bounded motion flight trace during ordinary mesh/show operation, so capture
does not require the hanging light to stop behaving. Each 25 Hz MSA sample
retains raw XYZ, low-pass gravity, tilt/sway, all nine current TMF zones and
confidences, the production presence edge/latch, lifecycle/program/power tier,
and the rendered LED snapshot. PSRAM holds 8,192 samples (about 5.5 minutes); a
reported 1,024-sample fallback is used if PSRAM allocation fails.

The recorder is compile-bound to one six-digit fixture ID and disables itself
on a physical-ID mismatch. After the scene, maintenance WiFi exposes a bounded
NDJSON batch endpoint. `ops/bench/capture_motion_trace.py` verifies exact
fixture identity, exact `-t` revision, compiled target match, downlight class,
MSA health, maintenance state, and buffer availability before exclusive-
creating a JSONL trace. The field runbook requires a visually confirmed hanging
target, one OTA writer, a named pre-trace artifact/SHA, exact single-target OTA,
and restoration of that prior artifact through fresh heartbeat and pending-
verify proof.

All native fixture tests pass, including the new ring overwrite/cursor test;
the downloader unit tests pass. A full ESP32-S3 commission/channel-11 test
compile also passes at 1,204,237 bytes program / 68,780 bytes globals and emits
a 1,204,528-byte binary. No target artifact was built or flashed because the
physical hanging fixture ID still needs visual confirmation.

## 2026-08-29 -- Ben + Codex -- Sakura 15 ft canopy canary staged

Built immutable production candidate `fx-260829-8790f6d-p` from clean pushed
source `ab71b8989b12e860ad1d82a5e1cf4767773a86f7` with the current field recipe:
channel 11, 300 mA precharge, 120-second day sleep, 12-second wake listen,
listener commission default, and dual-site maintenance credentials. The binary
is 1,212,640 bytes with SHA-256
`5bd65970990a34e511b44cb9a9ef1a7dc4e93a7b3cd3746df08abafb3e0ab6bf`.
Recipe hash, embedded revision, clean commit, build options, manifest, exact
binary bytes, and SHA file all agree.

Declared one OTA writer and targeted only healthy zero-return installed-height
candidate Sakura `F2BE0C`. Exact job `C804CF1F` passed dashboard and fresh
maintenance power preflights at 3.31-3.32 V, identity-matched
`192.168.1.109`, uploaded the exact retained artifact, produced a fresh exact-
revision software-reboot heartbeat, and remained on that revision past the
pending-verify gate at 31,044 ms uptime. It retained field profile, downlight
class, and recovery state 0. The complete job and uploader ledgers are retained
in the Black Rock City data folder.

A post-update exact TMF census then recorded six consecutive raw zero-depth
reports while TMF remained read-OK with 405-456 completed reads, zero errors,
zero recoveries, and zero sensor-domain resets. This proves that accepting the
full 5 m range does not turn the high empty scene into a permanent ground
target. A 10-minute exact CA lease produced the expected visible low-white
baseline (`rail=1`, one pixel, `W=25`). An eight-minute telemetry watch saw no
spontaneous `W=255` event. An exact 20-second identify blink was sent to help
locate the fixture, but no person-under-Sakura attempt was confirmed during the
watch, so human detection is unexercised rather than failed. Eight exact release
acknowledgements ended the lease and a fresh mesh heartbeat completed cleanup.

Next: have a person walk/stand directly under Sakura while an exact visible
lease is active. If no `W=255` transition or raw 2.7-3.5 m return appears, treat
15 ft height/aim/sunlight as a P0 gating concern and test one lower or better-
aimed canopy fixture before any fleet promotion.
## 2026-08-29 -- Ben + Codex -- Astro control puts useful raw range below 1.6 m

Repeated Gible's raw 1-5 m height/aim experiment on lower-hung downlight Astro
`9E5B44`, reported near 12 ft. Built exact-target-only artifact
`fx-260829-066846f-t` from the same pushed source `d9cb775`; its 1,221,136-byte
binary SHA-256 is
`b9dba40884967fd182093d1e9ab370a7d40fd89b74f5ccc3ec6588d967636877`
and immutable recipe SHA-256 is
`066846f139e79f02aaa1ef6709e8618563d544e28bebada4e7db3ecf2cfeba09`.
The only diagnostic recipe change from Gible was the target lock. Job
`861C9C57` updated only Astro and verified it through pending verify.

Dawn suppressed the field show, so Astro alone received a temporary,
nonpersistent commission profile. A pre-existing RAM-only fleet dark lease was
released; scheduled field fixtures remained dark while Astro rendered the raw
red/white sentinel. Ben observed only a split-second flicker while walking
beneath Astro, but a substantially denser response when he raised his hand.
Astro is about 12 ft high and Ben is 6 ft 3 in, leaving about 1.75 m from sensor
to head before the raised gesture.

The exact rolling drain retained 913 samples over 304.3 seconds with no sequence
gap. It captured 35 white samples in 18 short runs. Every qualifying zone return
was physically plausible at 1,003-1,561 mm, confidence 20-41, across zones 1,
2, 3, and 5. There were no sampled 1.562-5 m returns. Empty intervals spanning
49.676 seconds before and 212.752 seconds after the interaction contained no
sampled 1-5 m false positives. This directly contrasts with Gible's nearby
raised target being misreported at 3.24-5.00 m.

Lowering therefore helps, but roughly 12 ft remains marginal for passive
walk-under interaction: Ben's head gap was just outside Astro's demonstrated
reliable band and a raised hand moved into it. Test roughly 10 ft or equivalent
re-aim before choosing an installed height; do not threshold-tune around
Gible's far artifacts. Exact job `AFF7F2E7` restored Astro's original
`fx-260828-658b7d2-p` image and verified field profile, downlight class, recovery
state 0, and pending-verify survival at 32,039 ms uptime.

## 2026-08-29 -- Ben + Codex -- Raw Gible test rejects 5 m as usable physical range

Ben challenged the earlier interpretation correctly: Gible is less than 5 m
high, yet a normal person did not trigger it and a bamboo split had to be held
roughly 2 m from the sensor. The retained 4.8-5.0 m values proved that firmware
no longer discards far reports, but they did not prove accurate or useful
physical ranging at those distances.

Built exact-target-only artifact `fx-260829-4192016-t` from pushed commit
`d9cb775`, 1,221,136 bytes, SHA-256
`27b9d412d4fe51c44fc37d4adc75a7c2b0481ab2750c13b08f020482773b7fc2`.
Its immutable recipe SHA-256 is
`419201645ff0321b58030837faac989ebb8648e108e5429131e0ad696907f113`.
The diagnostic excludes Gible's known 166-363 mm self-splay, then treats any
confident zone from 1,000 mm inclusive through 5,000 mm exclusive as immediate
presence: no learned background, hit debounce, or hold hysteresis. Red means no
such zone and full RGB white means one is present. Compile-time isolation
prevents the aggressive predicate from originating or relaying a mesh presence
wave. All native checks passed, including 565 presence checks, and the fresh
ESP32-S3 build ended at 1,220,833 bytes program / 68,788 bytes globals. Job
`AB8F3FA6` updated only Gible and verified the exact image through pending
verify at 32,289 ms uptime.

The 860-sample trace retained 300.500 seconds with no overwrite or sequence
gap. It was red for 123.895 seconds before and 142.250 seconds after the sole
34.355-second interaction window. No sampled 1-5 m return and no white output
occurred in either empty interval. During Ben's raised-bamboo interval, 45
samples were white across 23 raw rising edges. The detector contract matched
exactly: every sampled 1-5 m return was white and no sample without one was
white.

The ranges inside that interval are the decisive anomaly. Despite Ben placing
the bamboo split roughly 2 m from the sensor, the TMF produced no 1-3 m zone
return. It produced eight 3-4 m returns and 65 4-<5 m returns across zones 0-4,
plus one raw exactly-5,000 mm report. Confidences were 20-79. Maximum frame
summary was 4,988 mm and maximum zone was 5,000 mm. These are not isolated
empty-scene false positives; they appear only when the nearby raised target
changes the optical scene. They are therefore evidence of aim, partial
occlusion, edge/background ranging, or multipath, not calibrated 5 m person
range.

The software parser and visible path work, but Gible's installed sensor geometry
fails the physical usability gate even with every background/debounce
discriminator removed. Do not tune production thresholds around these far
numbers. Lower or re-aim Gible, clear the TMF aperture/field of view, then repeat
a normal-person test. Exact final job `105DD74F` restored normal canary
`fx-260829-7906e6f-p` and verified it past pending verify at 26,599 ms with
field profile, downlight class, and recovery state 0.

## 2026-08-29 -- Ben + Codex -- Gible sentinel proves the path but not useful interaction

The earlier rolling captures left Ben unconvinced that the installed-height
experiment was timed and signaled clearly enough. Built exact-target-only trace
artifact `fx-260829-e98247b-t` from pushed test commit `ce785c5`, 1,222,416
bytes, SHA-256
`f19c8e2f597e0baa3145c0c2c450becf07b83a398685da4a21140a6e118d2bdc`.
Its immutable recipe SHA-256 is
`e98247b9ddff406e8e16769073792e839b14a9751200aa55853cfa166c15bbb5`.
The build is locked to Gible `9E5B34`, marked target-test-only and never
fleetable, and retains just over five minutes at one 300 ms sample per record.
It leaves the production TMF gate/debounce unchanged but replaces visible
output on that exact target with an unmistakable sentinel: solid red without a
presence latch and full RGB white while the latch is active. Exact Identify
still has higher precedence and supplied green start/end markers.

Job `C2DFDDB2` updated only Gible, received fresh 3.297 V maintenance evidence,
and verified the exact trace image through pending verify at 23,972 ms uptime.
Ben physically confirmed the red baseline. The final 1,024-sample capture
contained two approximately 10-second green markers at sequences 1031-1060 and
1501-1530. Between them, the first 111.855 seconds after the first marker were
red with no false latch. Ben then had to hold a split high to trigger Gible.
The recorder captured five separate presence rises at 2,504-4,497 mm and 49
active samples across 27.356 seconds. Every active sample rendered exact
`R=255,G=255,B=255,W=0`; no inactive sample rendered white. The sequence then
returned to red for 8.030 seconds before the second green marker. This rules out
capture timing and proves the sensor -> full-range parser -> production
debounce/latch -> render arbitration path on installed hardware. Exact capture
campaign `29CC104A` selected only Gible, froze before drain, and made no NVS or
profile mutation.

The trace also answers the range-cap question directly. Its maximum retained
post-parser zone was 4,982 mm at confidence 32; the maximum frame summary was
4,781 mm. The parser therefore demonstrably accepts the TMF through its 5,000 mm
limit. Presence intentionally has a separate 4,500 mm empty-background ceiling,
so the 4,982 and 4,781 mm frames did not initiate a latch. Rising-edge depths
were 4,497, 2,508, 2,860, 3,829, and 2,504 mm. Ben's impression that the useful
trigger was mostly near 2.5-3 m is consistent with the trace and is not evidence
that the retired 2.5 m parser cap remains.

It does not pass ADR 0070's usability gate: ordinary standing and walking had
not triggered, and the successful gesture required holding the reported split
high. Treat the remaining issue as installed aim/range/sensitivity, not a dead
sensor or broken output path. No production threshold or fleet artifact changed.
Exact restore job `F02313D7` put normal canary `fx-260829-7906e6f-p` back on
Gible and verified it past pending verify at 27,560 ms with downlight class,
field profile, and recovery state 0.

## 2026-08-29 -- Ben + Codex -- Gible installed-height canopy canary failed

Built immutable normal canary `fx-260829-7906e6f-p` (1,212,736 bytes, SHA-256
`95df1a6b18f21c0f0643949e70474a4d2af019e85efbb86f21877af350dadb7d`)
from clean test commit `fe6619f`. Exact one-target job `A04CDF37` updated only
hanging downlight Gible `9E5B34`, then proved fresh exact-revision rejoin past
pending verify with field profile, downlight class, healthy TMF/MSA, no class
mismatch, and recovery state 0.

The initial scalar censuses and timed/condensed person choreography were kept
as diagnostic evidence but rejected as acceptance runs. An exact-target trace
variant `fx-260829-1170f20-t` continuously recorded sensor, presence, motion,
and rendered LED state into a rolling buffer. Physical PSRAM was not exposed to
this image, so its explicit 1,024-sample internal-RAM fallback was used. Two
human-triggered drains retained about 70 seconds each without asking Ben to
race a timer: one while standing beneath Gible and one after a natural walk.

The standing window contained 48 valid depth frames at only 168-363 mm; the
walking window contained 23 at only 166-362 mm. Both were confined to the same
two near-field zones. Neither contained a person-range return, presence edge,
held presence, or full dedicated-white `W=255` response. Ben's observation of
repeated warm-white changes is also present in the trace: seven separate
pre-trigger `W=25` intervals match GH CA's ordinary quiescent point-source
state and provide a useful timing control. They are not the ADR 0070 response,
which requires the presence latch to force `W=255`.

The installed-height interaction gate therefore failed cleanly without a
threshold change or fleet promotion. Exact final job `32F8BF78` restored the
normal canary and verified it past pending verify at 28,353 ms; a later fresh
heartbeat showed ordinary mesh mode, field NIGHT_SHOW/FULL/GH CA, downlight
class, sensor bits `9`, no mismatch, and recovery 0. Next is mechanical
TMF-window/aim diagnosis using raw range evidence, not a wider parser limit or
weaker debounce.

## 2026-08-28 -- Ben + Codex -- Art-site TMF height census and 15 ft canopy fix

Ran a read-only exact-target TMF census while canopy installation was underway.
Campaign `207B660F` selected 61 live TMF-bearing downlights and discovered 46
shared-WiFi maintenance endpoints across one complete wake window. No firmware,
NVS, profile, or persistence write occurred. Six raw telemetry samples per node
separated 42 close returns at 145-575 mm, consistent with fixtures in bins, from
four healthy all-zero nodes: Panther `9E5A84`, Gible `9E5B34`, Sakura `F2BE0C`,
and Leia `F40384`. The latter are the strongest installed-height candidates,
though physical location is not proven. Fifteen nodes did not present an HTTP
endpoint and remain unsampled, not failed. Every sampled fixture subsequently
returned to mesh with a fresh heartbeat. Evidence is retained in the Black Rock
City data folder and summarized in the build-week TMF test note.

The census and Ben's report of roughly 15 ft sensor-to-ground mounting exposed
an artificial 2,500 mm firmware cap. TMF8820 hardware supports 5,000 mm, while a
standing head under this geometry is commonly about 2,700-3,200 mm away. ADR
0070 therefore accepts confident scene returns through 5,000 mm, extends the
empty-background presence window to 4,500 mm, clears a stale per-zone baseline
after 12 empty reports, and slowly follows a farther non-presence background.
This lets a powered fixture re-baseline after moving from a close shipping bin
to the tree without a reboot.

ADR 0069's downlight absolute distance/color/380 mm gobo rule is superseded.
Downlights now use the existing debounced per-zone presence latch and hold full
dedicated white while a person is present. Perimeter keeps its 150-1,800 mm
continuous hue and 37 -> 19 -> 7 -> 1 ring peel. Blackout, program suppression,
battery tiers, rail policy, and the physical HEX budget keep authority.

The full native fixture suite passed, including 558 presence checks. An
ESP32-S3 commission/listener development compile passed at 1,201,065 bytes
program and 68,716 bytes globals; its 1,201,360-byte `dev-local` binary SHA-256
is `c19b95df3e65aaade0a43625716defe0bf16911c6d1c307a1e9aaf32762775a2`.
No binary was flashed. Next is one immutable update on a named healthy installed
zero-return candidate and a person-under-fixture daylight/night canary.

## 2026-08-28 -- Ben + Codex -- Build-week palette, HEX power, and ToF sprint

Started the final build-week interaction sprint in an isolated clean worktree
from current `origin/main`; the active checkout's unrelated dirty T-Deck work
was not moved or modified. Upstream already recorded Ben's physical fleet truth:
20 manufactured uplights, with four former planning allocations returned to
unassigned, so no duplicate roster edit was needed.

ADR 0069 adds three bounded fixture-side improvements. First, the physical
37-pixel RGB HEX driver now budgets the rendered frame to 765 linear RGB channel
units after composing the existing battery-tier brightness cap. A sparse gobo
at or below one full RGB pixel/three full saturated channels remains unscaled;
dense washes are uniformly scaled with fail-safe downward rounding. Point RGBW
and uplight RGB output behavior is unchanged.

Second, autonomous GH CA uses the existing trusted UTC estimate for a
synchronized palette step every 20 minutes, with a six-step/two-hour loop.
Invalid time preserves the configured hue, and a bridge-owned program keeps its
requested palette. This is the first synchronized artistic cadence, not yet
program-ID rotation.

Third, a post-program local interaction layer restores continuous ToF behavior
without changing `packet.h`. From 150 through 1,800 mm, distance drives color;
the perimeter HEX peels 37 -> 19 -> 7 -> 1 pixels as someone approaches, and a
downlight switches to its full dedicated-W point source at <=380 mm for the
crisp gobo pop. It applies to any visible program, including direct bridge
frames, but an all-zero/suppressed frame remains dark and the physical current
budget plus all existing power vetoes remain downstream. MSA311 motion input is
plumbed into the same pure layer with a canary switch, but normal builds leave
it disabled until installed wind-versus-human traces set threshold/hysteresis/
hold behavior. BMP581 and multi-fixture aggregate gestures remain open.

The complete native fixture suite passed. A guarded commission/listener
PowerFeather development compile also passed at 1,201,937 bytes program and
68,708 bytes globals; its 1,202,240-byte binary has SHA-256
`9f9cd933e3c1a51929824d625e49d916be1fab298cf1aa9aea554d2d8e4a060f`.
This dirty `dev-local` binary is compile evidence only: nothing was flashed or
published. Named battery-installed perimeter and downlight canaries are next.
## 2026-08-29 -- Ben + Codex -- Sakura synchronized palette accepted

Reconnected exact T-Deck `8EB508` on `COM7` with fresh master timestamps and
advancing RX/TX counters, then took a strictly read-only Sakura sample. Exact
Sakura `F2BE0C` reported `fx-260829-8790f6d-p`, GH CA state 1, and RGB
`51,0,70`. That ratio exactly matches the firmware's expected UTC-derived
purple phase, while ordinary baseline downlights retained their static blue
hue. Ben simultaneously identified Sakura physically and described it as a
nice purple. Combined with the earlier red observation, later non-blue drift,
normal CA animation, exact canary provenance, and correct Fleet Blink/LED
Studio lease behavior, Ben accepted the synchronized palette feature without
requiring the remainder of a formal two-hour watch. The finite read-only
heartbeat monitor was deleted before any later run. No fixture command, OTA,
reboot, NVS, profile, or persistence write occurred. Palette cadence is no
longer a fleet-release gate; the electrical, installed-height, and ADR 0068
negative-safety gates remain open.

## 2026-08-29 -- Ben + Codex -- Canary inventory scopes unified fixture release

Audited the exact 110-fixture night census by artifact and source provenance.
The fleet is split across 76 general dual-site fixtures on
`fx-260828-658b7d2-p`, 16 freshly heard members of the completed 20-uplight
cohort on `fx-260828-d8f62c3-p`, 13 older baseline/holdback fixtures, and five
special canary instances: Akuma `9E668C` on mutable `dev-local`, Rikku `9F26B0`
on the ADR 0068 intermediate `fx-260828-abd893c-p`, Dixie `F40314` plus Froakie
`9E5B18` on ADR 0069 artifact `fx-260829-97aed8e-p`, and Sakura `F2BE0C` on the
ADR 0069/0070 superset `fx-260829-8790f6d-p`.

The source does have a coherent convergence path. Current `main` already
contains the dual-site maintenance, short-wake telemetry, full-battery PROTECT
recovery, MSA/uplight classification, and RGB uplight framing changes. Remote
branch `origin/codex/build-week-interaction-sprint` is exactly six commits ahead
from `main` with no divergent main commits. Fixture source through clean commit
`ab71b8989b12e860ad1d82a5e1cf4767773a86f7` adds ADR 0069's count-aware HEX
budget, synchronized palette, and local interaction plus ADR 0070's full-range
canopy presence. Its exact Sakura artifact passed identity, OTA/rejoin, pending
verify, basic sensor-to-output, and no-empty-trigger checks. The later
`fa2c9d9` motion flight recorder is exact-target diagnostic instrumentation and
is not part of the ordinary fleet-release cutoff.

Do not reuse or combine the existing canary binaries. After the remaining
hardware gates, merge the accepted source, build one new clean immutable ADR
0040 artifact, repeat a small mixed-role canary, and widen with explicit safe
target lists. Remaining gates are the ADR 0068 negative release/fault matrix,
one named installed-height canopy raw-range margin check, and a measured
perimeter HEX current/rail check. Ben subsequently accepted the synchronized
palette cadence from physical and exact telemetry evidence without requiring a
formal two-hour watch. Akuma's mutable development identity, low-voltage/PROTECT
fixtures, recovery exceptions, and target-bound diagnostics remain exclusions
until separately cleared. No build, OTA, fixture command, or persistent state
change occurred during this inventory.

## 2026-08-29 -- Ben + Codex -- Night census identifies Sakura palette canary

Onboarded through the canonical repository documents, then opened the existing
read-only host dashboard on the sole connected ESP32-S3 serial device. Exact
T-Deck `8EB508` was positively identified on `COM7`, channel 11, with zero
reported send failures. Its already-running census had seen 111 identities;
110 field-profile fixtures were heard within 15 minutes, 98 within 60 seconds,
and the five-second live count varied around 80-95 with the weak field links.
The 15-minute fixture cohort reported 108 NIGHT_SHOW, 105 CA, 96 FULL, 3 DIM,
and 11 PROTECT. Firmware was mixed across 76 `fx-260828-658b7d2-p`, 16
`fx-260828-d8f62c3-p`, 11 `fx-260827-1254f04-p`, two
`fx-260826-51d1fe1-p`, two `fx-260829-97aed8e-p`, and one each of
`dev-local`, `fx-260828-abd893c-p`, and `fx-260829-8790f6d-p`.

The same session exposed a bridge-side control failure. LED Studio and the
T-Deck Fleet/Blink action produced no visible response while fixtures continued
CA. Bridge receive frames kept climbing, but `sendok` remained exactly 9,058
through multiple attempted actions and `sendfail` remained zero. Source review
showed why the zero is misleading: transmit accounting observes only ESP-NOW
completion callbacks, while all ordinary `esp_now_send()` call sites discard a
synchronous enqueue error. LED Studio also reports the number of planned fresh
targets, not confirmed radio submissions. The failure is therefore isolated to
the T-Deck transmit path before fixture command acceptance, not CA arbitration;
the exact ESP-NOW error and initiating mechanism remain unknown.

Releasing COM7 for a read-only `show` query unexpectedly reset the USB-native
T-Deck. It returned mesh-only, channel 11, SSID unset, and ESP-NOW marked up.
After reboot, an exact-target diagnostic immediately advanced successful send
callbacks, and a new all-fleet green LED Studio stream advanced TX continuously
with zero callback failures. Ben confirmed every visible fixture green. Treat
reboot as recovery evidence, not root cause or an adequate permanent fix: add
synchronous-submit telemetry, a truthful UI fault state, and bounded recovery
for a flat TX counter while RX remains healthy.

The first telemetry interpretation did not identify the physical lantern. No
fresh class-bearing downlight heartbeat reported red output, while the fresh
red/orange renderers with authoritative LED telemetry reported perimeter class.
Ben eliminated Daxter `9D7884` because it is a known real perimeter light, then
showed that the anomalous lantern joined Fleet's filtered downlight green blink.
The lantern also followed the normal changing CA pattern and accepted arbitrary
LED Studio colors, ruling out a stuck LED channel, retired constant-red ready
behavior, and general direct-control failure.

Ben then filtered Fleet by firmware and identified the physical lantern on the
first try as Sakura `F2BE0C`, the sole observed `fx-260829-8790f6d-p` fixture.
Artifact provenance resolves the color anomaly: that exact canary was built
from clean pushed source `ab71b8989b12e860ad1d82a5e1cf4767773a86f7`, which
inherits `f5b86070931a6b32272735930fa8abf869ce02d2` and ADR 0069's synchronized
autonomous palette experiment. With trusted UTC and no bridge lease, GH CA
advances its base hue by 43 every 1,200 seconds in a six-step, two-hour loop.
A bridge lease deliberately preserves the requested hue. Sakura therefore can
appear red, later drift to a different non-blue hue, continue the same CA
animation, and still obey green Fleet Blink or any LED Studio color. This is
intentional canary behavior, not a class mismatch, missed OTA, stale hue lease,
or LED fault. Sakura was deployed as the installed-height canopy-presence
canary, but its artifact also inherited the palette-cadence experiment. That
feature remains on `origin/codex/build-week-interaction-sprint`; current main
does not contain it. Decide whether to promote the cadence or return Sakura to
the static fleet palette after the canary evaluation.

The post-blink host investigation also found that the dashboard's last master
and peer snapshot was frozen at 01:55 even though localhost continued serving
normally. COM7 no longer enumerated, but the page exposed neither serial loss nor
master age, so later apparent telemetry was cached rather than live. No firmware,
profile, NVS, OTA, or persistent fixture setting changed; the earlier diagnostic
COM reopen reset only T-Deck `8EB508`.

## 2026-08-29 -- Ben + Codex -- PUCA capacitive paw root-caused and boot arming proven

Retested exact PUCA `A4EB10` with both faceplate audio plugs removed. A confirmed
12-second stationary paw hold produced 11/11 `pawpin=0` samples, disproving the
idea that the earlier J8/J9 output-jack mistake was the continuing cause. Ohmic's
published trigger test uses `digitalRead(GPIO15)`, but its carrier schematic
routes the paw separately from the TL074 audio-output stages and the exact unit's
digital input never tracked touch.

Added an inactive-only raw ESP32 touch-channel probe in `0.5.4-dev`. The released
paw was exceptionally stable at 867-870 counts; a held paw fell cleanly to 216.
This proves the electrode, front-panel/header path, GPIO15, and ESP32 touch
peripheral. The first blocking probe exposed and then fixed two diagnostic-only
artifacts: one transient digital HIGH on peripheral restoration and receive-queue
drops while the loop was blocked.

Production paw handling now uses the actual ESP32 T3/GPIO15 capacitance channel,
not the vendor example's digital input. `0.5.5-dev` applies exact-unit hysteresis
(press <=650, release >=750) and rejects near-zero/shorted readings below 50 so
the arming gate fails safe. The 1.2-second continuous boot hold, five-second
opportunity, setup window, and SAFE-IDLE policy are unchanged. All 129 native
checks pass. Fresh artifact
`firmware/puca_bridge/build/puca-bridge-20260829-paw-cap-touch-v055-r1/puca_bridge.ino.bin`
is 1,038,176 bytes, SHA-256
`b7d4db31f339a14d079a273170544f6b4218a367075799736f1229a6c1c2f2c1`,
with `esp32:esp32:pico32:PartitionScheme=default`. Exact-target shared-WiFi OTA
uploaded once to identity-matching `A4EB10` and verified a fresh
`puca-bridge-0.5.5-dev` software-reset rejoin.

Ben then held the paw before a real Pod20 power cycle. The PUCA rejoined with
fresh `reset=poweron`; preserved USB telemetry proved `bootarmed=1`, `active=1`,
DJ + line input, released `pawcap=865-868`, 3,000+ direct frames, and zero send,
capture, queue, I2C, or clipping errors. USB `A` stopped publication and froze
the direct-frame count. A final released `P` probe remained 866-868 while
servicing mesh traffic, with `paw=0`, zero queue drops, and no synthetic edge.
The physical paw and boot arming are accepted; real RODE gain and the timed
post-arm setup gestures remain open.

## 2026-08-28 -- Ben + Codex -- PUCA mic modes proven; line and paw faults isolated

After the separate fleet OTA ended, declared this laptop the sole PUCA operator
and used T-Deck bridge `8EB508` on channel 11 plus exact PUCA `A4EB10`. A
service-started onboard-MEMS run exercised DJ, HEARTBEAT, EMBER, and HUE on the
live fleet, and Ben visibly confirmed audio-reactive fixture behavior. Serial
evidence covered about 75-92 fresh fixtures, RMS roughly
1,300-5,500, peaks through 14,685, useful rendered levels through 0.56, and zero
send, I2S-read, I2C, or receive-queue errors. Ben directly observed the audio-
reactive fixture behavior. Each run ended with `A`, a final blackout, frozen
direct-frame count, and return to inactive DJ/line state.

The laptop headphone proof did not reach the PUCA faceplate line input. Windows
recognized `Realtek HD Audio 2nd output`; a directed 96 kHz WASAPI stream played
successfully at about -8 dBFS. The endpoint was unmuted, and tests at its original
10 percent level and a temporary 25 percent level left PUCA at the same RMS 7-10
noise floor. The level was restored to 10 percent. This isolates the remaining
fault to the analog adapter/faceplate route or the WM8978 line-routing setup; it
does not implicate the shared codec/I2S/DSP/render/mesh path, which the onboard
mics proved. Verify adapter continuity and both J5/J6 inputs before changing
codec registers.

Ben later identified the exact passive chain: JSAUX `B07D8M5DML` 3.5 mm TRS
stereo male to dual RCA male, then two Bolvek `B09K3DHL82` RCA-female-to-3.5 mm
TS-mono adapters. This is electrically the correct one-channel-per-J5/J6
topology and does not inherently short or combine channels. Remaining physical
checks are continuity, full plug seating, and confirming J5/J6 AUDIO IN rather
than J8/J9 AUDIO OUT before investigating WM8978 line-register routing.

Correction and positive retest: the two TS plugs had in fact been in faceplate
J8/J9 AUDIO OUT. Ben moved them to J5/J6 AUDIO IN and the identical directed
96 kHz laptop waveform at the restored 10 percent Windows level immediately
raised line RMS from 6-12 at quiet to 37-41, with rendered level about 0.13-0.15,
zero clipping, and zero transport/capture errors across about 83-91 fresh
fixtures. `A` then stopped the publisher and returned level to zero. This proves
the exact cable chain, faceplate J5/J6 path, WM8978 line routing, I2S capture,
envelope, renderer, and fleet transport. Only real RODE gain/clipping and visual
per-mode/fallback acceptance remain on this audio axis.

A follow-up 28.5-second laptop burst run temporarily used 25 percent headphone
level and advanced DJ -> HEARTBEAT -> EMBER -> HUE at seven-second intervals.
The final status proved HUE active after the complete serial sequence, direct
frames advanced from 1,871 to 4,591, send callbacks reached 5,225 with zero
send/read/I2C/queue/clipping errors, and line level returned to zero with the
bursts stopped. Cleanup sent `A`, returned mode to DJ, and restored the Windows
endpoint to 10 percent.

A dedicated HEARTBEAT repeat used a synthesized 16-second paired-pulse waveform
through the same laptop/J5/J6 line path. With the top sensitivity control first
confirmed at its 4.00x maximum and the headphone endpoint temporarily at 60
percent, telemetry captured rendered peaks through 0.222 across about 84-93
fresh fixtures. Capture peaks reached 2,063 without a single clip, send failure,
I2S read failure, I2C error, or receive-queue drop. Cleanup stopped publication,
returned mode to inactive DJ, and restored the headphone endpoint to its 10
percent baseline; final telemetry reconfirmed gain 4.00x and zero error counters.
Ben visibly confirmed that the paired deep-red HEARTBEAT response works. He was
turning the sensitivity knob during the run, which explains the logged 0.26x-
4.00x changes; the complete top-knob sweep is therefore also proven. Physical
orientation is counterclockwise for more sensitivity and clockwise for less,
opposite Ben's initial expectation.

Ben requested the conventional carrier-control direction after that proof.
`puca-bridge-0.5.3-dev` now explicitly normalizes both carrier pots: fully
clockwise raises the top audio-sensitivity control to 4.00x and the bottom
brightness ceiling to 1.00; in HUE, clockwise also advances the color wheel.
The pure mapping and all existing safety/render checks pass 121/121. Fresh
combined artifact
`firmware/puca_bridge/build/puca-bridge-20260828-knob-direction-v053-r2/puca_bridge.ino.bin`
is 1,024,656 bytes, SHA-256
`97e43bf5e2686057474e95d4404a839967b9681e05832919ea81e77079b4bcb5`,
with the explicit `esp32:esp32:pico32:PartitionScheme=default` recipe. The prior
top-only `r1` build was never deployed. Exact-target `UA4EB10` maintenance found
identity-matching `192.168.1.159`, uploaded `r2` once, and verified a fresh
`puca-bridge-0.5.3-dev` software-reset rejoin. Post-update USB telemetry remained
SAFE-IDLE with zero direct frames and zero capture/transport errors.
After a later full power cycle, fully clockwise hardware telemetry read exactly
`gain=4.00`, `ceil=1.00`, and HUE endpoint 360, accepting both corrected control
directions. The original PUCA USB cable then enumerated only as a failed USB
device descriptor; moving the known-good T-Deck cable to PUCA immediately
restored its exact CP2102 identity on COM6. Treat the original cable as suspect;
the PUCA USB interface itself passed the swap test.

The first polarity guess produced and exact-target OTA-installed
`0.5.1-dev`, but official Ohmic Original Edition trigger source and later raw
telemetry disproved active-low operation. It was superseded in the same session.
Source and exact-target OTA now run `puca-bridge-0.5.2-dev`: active-high paw
handling, raw `pawpin`/`paw` telemetry, a five-second steady-red boot opportunity,
and elapsed-time audio calibration. Native tests pass 105/105. The immutable
candidate is
`firmware/puca_bridge/build/puca-bridge-20260828-paw-telemetry-v052-r1/puca_bridge.ino.bin`,
1,024,608 bytes, SHA-256
`3e3e8d9fa0ce3c8d60950abc027840eb345c95cf1f9438d1f0ff44cbac5a20e8`.
Exact-target shared-WiFi OTA found `A4EB10` at `192.168.1.159`, uploaded once,
and verified fresh `0.5.2-dev` software-reset rejoin. A subsequent full power
cycle also rejoined cleanly.

Hardware validated the calibration correction: `calibrated` was false at
1,500 ms and true at 2,505 ms before direct frames began. Paw release is
definitively GPIO15 low. Runtime sampling caught two intermittent high readings
about 4.2 seconds apart during one attempt, but a later confirmed ten-second
stationary hold produced no transition, and the five-second held-paw power-cycle
still returned `bootarmed=0`. Do not weaken the physical arming interlock around
an intermittent signal. Until the carrier touch path is electrically inspected
or traced at higher rate, USB service `A` is the repeatable bench start path;
SAFE-IDLE remains the no-touch boot result.

## 2026-08-28 -- Ben + Codex -- Daytime interaction and bounded fleet wake brainstorm

Onboarded from the current `main` architecture and reviewed the measured daytime
radio floor, field sleep cadence, UTC/civil-twilight scheduler, conditional solar
probe, perimeter palm gate, distributed CA/Contagion seams, and solarnoid policy.
No fixture, bridge, serial port, artifact, or shared bench was changed or
commanded. This is exploratory context, not a new ADR or implementation decision.

Continuous radio receive remains the wrong default: the measured dark-awake
floor is 126-144 mA before perimeter ToF load, or 1.51-1.73 Ah across 12 hours.
The promising architecture separates local renewable/strike eligibility from
radio availability. Strong solar may permit a strike or a richer rendezvous, but
should not by itself require all-day receive.

The leading artistic candidate combines scheduled and interactive behavior. A
bounded hourly daytime rendezvous wakes the fleet for a short roll call or
ring-wise composition and opens a fixed invitation window. Fresh perimeter palm
edges contribute engagement credits; distinct source IDs or sectors are more
valuable than a held/repeated gesture. Successive thresholds grow the response
from a few nearby downlights, to one ring, to a three-ring ripple, to a full
chorus. The session has an absolute hard end and cooldown that activity cannot
extend, while every downlight retains its autonomous solar/power/mechanism gates.

Between scheduled windows, a future low-duty perimeter-sentinel posture could
turn the radio on only after local ToF detection and let a small expiring event
spread as ordinary sleepers wake. Do not select that posture until the existing
named-perimeter A/B measures MCU plus VL53L5CX ranging with the radio off; the
small 2 W / 6 Ah perimeter class is the tightest daytime budget. The existing
generic choreography, event-fabric, sensor-power, wake-window, localization, and
solarnoid validation TODOs already cover the needed work, so no duplicate TODO
was added.

## 2026-08-28 -- Ben + Codex -- TMF8820 twenty-foot installation range reviewed

Onboarded through the canonical repository documents and compared the exact
ordered TMF8820-mini, current fixture implementation, open height-validation
TODO, and ams OSRAM datasheet performance. No firmware, fixture, bridge, serial
port, or shared bench was changed or commanded.

The TMF8820's specified maximum target distance is 5,000 mm (16.4 ft), while a
20 ft sensor-to-ground distance is 6,096 mm and cannot produce a ground return.
A roughly 6 ft person's head would be about 4,267 mm (14 ft) from the sensor,
inside only the best-case low-ambient raw range. The datasheet's 3x3 / 550 k
iteration table reaches 5 m on a full-FoV 18 percent grey center target under
350 lux LED lighting, but falls to 3.0 m center / 2.8 m edge / 2.0 m corner at
the equivalent of 1 klux sunlight; a person fills less than the test target.

The deployed source is more restrictive than the silicon: sensor parsing drops
returns beyond 2,500 mm and the no-background presence path admits only targets
within 2,200 mm. Therefore the current artifact will not detect a person from a
20 ft mounting height. A night-only foreground interaction may be recoverable
with a deliberately tested long-range configuration and gate change, but this
is not yet evidence: the exact-part downlight-height test remains open. If ToF
interaction matters, pause further high placement until one exact enclosed
fixture passes a 20 ft walk-under test; otherwise treat the high TMFs as class-ID
devices, not reliable presence sensors.

## 2026-08-28 -- Ben + Codex -- RODE-only PUCA mode proof scoped

Fast-forward pull confirmed local `main` already matched `origin/main`. Onboarded
through the canonical repository documents and reviewed the current PUCA hardware
record, ADRs 0035/0063, firmware, native tests, and audio operating guides while a
separate laptop retained ownership of the active fleet OTA. The in-progress
untracked fleet-OTA ledger was preserved and no serial port, bridge, fixture, or
PUCA was opened, commanded, reset, built, or flashed.

The RODE VideoMic NTG is sufficient to validate the PUCA line-input path and
visibly distinguish DJ, HEARTBEAT, EMBER, and HUE with speech, claps, or a played
click track. It cannot prove fidelity to the performer's exact arbitrary
waveform; that remains a later generator/photodiode gate. Current `0.5.0-dev`
has no dry-run or named-canary output: once armed it sends direct frames to the
complete fresh fixture census. SAFE-IDLE also returns from audio processing
before updating RMS, peak, clipping, envelope, or color evidence. Therefore do
not arm it in parallel with the fleet OTA. After the OTA operator releases the
radio, run the RODE test in a declared one-publisher window, capture serial
level/peak/clip evidence for all four modes, observe a controlled mixed RGB/RGBW
cohort, and finish by proving the roughly three-second autonomous fallback.

## 2026-08-28 -- Ben + Codex -- All 20 manufactured uplights updated

Declared one OTA writer and reused immutable artifact
`fx-260828-d8f62c3-p`, SHA-256
`57f40023e1e599d60cf2a309e6a7af2f94bf45716421309b2b3a15048b239097`.
The first two four-target campaigns (`70E694A4` and `385B7EF8`) completed their
full gather and freeze contracts but discovered no maintenance endpoints, so no
upload occurred. Windows exposed only the 5 GHz `Party In The Woods` BSSID,
which initially suggested a missing 2.4 GHz service. An exact no-upload Togepi
maintenance control then found `9E5AB0` at `192.168.1.207`, disproving that
hypothesis: Starlink's 2.4 GHz path was healthy and the Windows scan had omitted
it. The earlier selected fixtures simply had not woken into the campaign.

Job `45208DD7` targeted four freshly heard uplights (`F2BE1C`, `F3FC8C`,
`F2BEB4`, and `9F2694`). All four identity-matched, accepted the exact binary,
rejoined with the exact revision, survived pending verify, and retained field
profile. A direct post-update maintenance check on Psyduck `F3FC8C` reported
`fixture_class=uplight`, `class_ovr=0`, `sensor_bits=8`, and no mismatch; an
explicit second `/resume` returned it to mesh after the diagnostic HTTP client
timed out while the endpoint was dropping WiFi.

Job `444537E1` then targeted the other 15 uplights present in the dashboard.
All 15 maintenance endpoints were found, including previously unreachable
Donkey `F2BE10`; all 15 uploads acknowledged, all 15 produced fresh exact
revision heartbeats, all 15 survived pending verify, and none remained in
commission profile. Together with Togepi and the staged four, all 20 physically
manufactured uplights now report the exact artifact. Ben confirmed that only 20
uplights were made; the 24-row roster had incorrectly promoted four earlier
planning allocations into the physical cohort. Ken `F2B8DC`, Pikachu `F2BCE0`,
Kirby `F2BE64`, and Haunter `F40438` are not manufactured uplights, were never
rollout targets, and have been returned to role-unassigned registry state. The
uplight rollout is therefore complete at 20 of 20.

## 2026-08-28 -- Ben + Codex -- Combined safety/class artifact and Rikku canary

Built one immutable fleet artifact from clean pushed commit `5865282`, folding
the ADR 0068 full-battery PROTECT recovery together with the ADR 0067 MSA311
classification correction. The production recipe is field profile, channel 11,
300 mA precharge, 120-second day sleep, 12-second wake listen, and listener
commission default. Artifact `fx-260828-abd893c-p` is 1,211,504 bytes with
SHA-256
`6a0126f205be2cb6be034a71de5c5caa75f0af0acd00e1684ddc27377fb175f5`;
its clean commit, canonical recipe, embedded revision, toolchain, build options,
binary digest, and manifest all passed the ADR 0040 identity checks. The earlier
classification-only sibling artifact was never uploaded and is obsolete.

Exact Donkey `F2BE10` job `1004BAFA` acknowledged the single-target maintenance
campaign, but no endpoint appeared on either maintenance subnet across the full
360-second discovery window. Cleanup freeze was acknowledged. The host therefore
made no HTTP upload and Donkey remains unchanged on `fx-260828-658b7d2-p`.

Exact Rikku `9F26B0` job `A357B979` found a fresh endpoint at 3.597 V with good
6.138 V input and no power fault, uploaded the combined artifact, received a
fresh exact-revision heartbeat, and survived the 20-second pending-verify gate.
The retained false PROTECT state then satisfied the new sustained full-battery
proof, persisted LEDS_OFF, and made the intentional clean software reboot. It
subsequently advanced OFF -> DIM -> FULL while keeping the daytime LED rail off.
Fresh post-recovery telemetry through more than eight minutes shows field
profile, exact revision, FULL tier, downlight class, sensor bits 9 (TMF8820 plus
MSA311), no class mismatch, healthy BQ state, and no pending verify. This passes
the positive ADR 0068 hardware canary without erasing NVS or bypassing the guard.
The negative-gate fault matrix and fleet audit for other high-VBAT PROTECT
entries remain open.

Ben then brought in an installed uplight whose solar panel had been found
disconnected and connected it to USB. Read-only identity established exact
Togepi `9E5AB0`, a registry-rostered 6 Ah trunk/uplight. On the prior
`fx-260828-658b7d2-p` image it reproduced ADR 0067 exactly: automatic class was
chandelier, sensor bits were zero, and MSA311 was absent. Battery/input preflight
was safe at 3.430 V and about +418 mA charge with good USB, no BQ fault, FULL
tier, field profile, and rail off. An exact COM12 upload of the same combined
artifact verified every written region against hardware MAC
`D8:85:AC:9E:5A:B0`. Post-reset telemetry changed the same hardware to uplight,
sensor bits 8, MSA311 present with a good live read, no mismatch, exact revision,
and no pending verify. The first identity-gated `L1` smoke attempt asserted
`smoke_render=true` and the 3V3-enable pad high, but Ben saw no light. That was a
real second defect, not a bad rail: uplights are the lensed 3 W RGB module, while
`ledProfileForClass()` had incorrectly grouped them with downlight RGBW modules
and emitted four-byte `NEO_RGBW` frames. The W-only point-source smoke frame was
therefore invisible, and ordinary RGB frames also had the wrong byte stride.

Source commit `0f08904` fixes the physical class contract: perimeter and uplight
use three-byte `NEO_GRB`, downlight retains slot-proven `NEO_RGBW`, and the smoke
renderer uses equal RGB on an RGB point source versus dedicated W on RGBW. A new
platform-independent profile helper and 10 native checks pin those mappings;
the complete fixture native suite passed. The guarded ESP32 development compile
passed at 1,210,377 bytes program / 68,740 bytes globals. Clean immutable
artifact `fx-260828-d8f62c3-p` was then built from pushed commit `0f08904` with
the same production recipe: 1,210,720 bytes, SHA-256
`57f40023e1e599d60cf2a309e6a7af2f94bf45716421309b2b3a15048b239097`.
Recipe hash/prefix, clean source, manifest, build flags, binary digest, and
embedded revision all match. The prior `fx-260828-abd893c-p` remains valid for
Rikku's ADR 0068 canary but is superseded for every uplight.

An exact second COM12 upload verified every region on Togepi's hardware MAC.
Fresh telemetry retained uplight class, sensor bits 8, a good MSA sample, field
profile, channel 11, FULL tier, no fault/mismatch/pending verify, and ESP-NOW.
Corrected `L1` visibly produced the requested white breathing pattern; the
battery charge current fell from about +398 mA to +279 mA under the real LED
load. Ben confirmed the physical output was "beautifully breathing." `L0`
returned the rail off and charge current to about +420 mA; a 150 ms RTS reset
then restored ordinary field behavior with the rail off. ADR 0067 classification,
physical RGB framing, and optical smoke acceptance now pass on exact Togepi.

## 2026-08-28 -- Ben + Codex -- ADR 0068 full-battery PROTECT recovery developed

Implemented the fail-closed fix for Rikku's high-VBAT PROTECT deadlock without
weakening the loaded-collapse boot guard. OTA already parks all loads and clears
`load_arm` before writing, but correctly preserves an existing PROTECT stage;
high open-circuit VBAT also cannot disprove a real under-load collapse. The
entry ladder therefore remains fail-safe. The release plane now has two separate
60-second proofs, and changing proof or losing one battery sample restarts the
clock. The existing >=+20 mA / >=3.25 V proof now explicitly requires a valid,
enabled, no-fault BQ. The new full-battery proof requires a corroborated real
cell, >=3.45 V, good external input, valid enabled/no-fault BQ, and `CHG_STAT`
CV, top-off, or not-charging/done. CC with low current is rejected. Either proof
persists LEDS_OFF and makes the existing clean reboot; neither releases on
rebound voltage alone. A high plausible but uncorroborated cell requests the
rate-limited BQ presence test so a tapered cell can establish independent proof.
The same audit closed a pre-existing NVS failure hole: a failed PROTECT-stage
write now stays parked awake and retries instead of timer-sleeping back into the
older brighter stage; a failed release-stage write still returns to PROTECT and
cannot clean-reboot.

Added durable entry provenance without changing the 32-byte version-1 sleep
audit layout: local PROTECT reuses otherwise-empty remote-source fields for
origin, predecessor stage, reset reason, prior `load_arm`, and unexpected-reset
streak. Old records are annotated once as `legacy-unknown` without losing their
original voltage/profile/uptime. Append-only heartbeat tail 18 exposes the
context through T-Deck census/`nb-peer`, the logger, and fleet dashboard.
ADR 0068 records the decision and exact Rikku/fault-injection canary gates.

The complete fixture native suite passed, including 195 power-policy, 26
sleep-audit, and 70 packet-layout checks. The Donkey registry change was carried
into the generated T-Deck health roster; its freshness check and all directly
compiled T-Deck native test binaries passed. All 18 fleet-dashboard parser tests
passed. A guarded
field-profile PowerFeather development build passed at 1,210,353 bytes program /
68,740 bytes globals and produced a 1,210,656-byte `dev-local` binary with
SHA-256
`ca910470953cd3ebff9c952d04becf15b6e220931d74ddc19ebb9043c01648ee`.
That dirty-worktree binary is compile evidence only. Nothing was flashed or
mutated on Rikku; next is a clean commit, immutable ADR 0040 artifact, exact
Rikku automatic-recovery canary, and supervised negative-gate matrix.

## 2026-08-28 -- Ben + Codex -- Rikku false high-VBAT PROTECT deadlock

Ben connected a second canopy/downlight that was dark the prior night despite
a roughly 3.6 V battery. A timestamped read-only USB catch identified exact
Rikku `9F26B0` / `D8:85:AC:9F:26:B0` on COM11, running accepted artifact
`fx-260827-1254f04-p` with the correct field profile and 15,000 mAh capacity.
No flash, erase, maintenance, profile, rail, or NVS command was sent.

Telemetry proved durable guard stage 4 / power tier 3 PROTECT with the LED rail
off. The retained entry audit records PROTECT at 3.596 V in field profile only
2.809 seconds into that boot. This is far above the 3.15/3.10/3.05 V power
ladder and cannot be a voltage-floor transition. The leading mechanism is an
unexpected reset while the durable load marker was armed, possibly after the
one bounded FULL-to-DIM retry; the current audit does not retain the predecessor
stage or reset reason, so the initiating reset class is not proven.

Fleet history shows the latch predates the latest artifact: Rikku was already
handled on the 900-second PROTECT cadence, then accepted the exact
`fx-260827-1254f04-p` OTA at 3.582 V and survived pending verify. OTA correctly
preserved NVS, including the bad latch. A later gather still found it at
3.536 V on the same protected cadence.

The current USB observation exposed a release deadlock. Input was good, the BQ
had no fault, VBAT stabilized near 3.56-3.58 V / 99 percent, and the configured
15 Ah capacity was correct, but the nearly full cell accepted only about
0-2 mA. The compound release requires battery current >=20 mA continuously for
60 seconds, so a healthy full battery can never clear false PROTECT merely by
remaining on USB. Fix both axes before fleet use: make high-VBAT false
escalation diagnosable/less trigger-happy without weakening real collapse
safety, and add a sustained charger-termination/full-battery release proof that
cannot release a rebounded depleted cell. Rikku remains protected pending an
explicit recovery decision.

## 2026-08-28 -- Ben + Codex -- All canopy batteries confirmed 15 Ah

Ben confirmed that every canopy/downlight without exception has a 15 Ah
battery. Ponyta `F2B7DC` therefore has a live persisted-capacity configuration
error: telemetry reports 6,000 mAh while its physical class and existing
registry record correctly require 15,000 mAh. No device command or NVS write
was sent in this clarification; correct Ponyta through the declared exact-target
configuration path before relying on gauge SOC or capacity telemetry.

## 2026-08-28 -- Ben + Codex -- Sensorless uplight fallback and MSA311 probe fix

Ben reported that roughly eight installed trunk/uplights appear to share Donkey
`F2BE10`'s MSA311 detection failure. Opening that cohort to replace PowerFeathers
is not practical before deployment, while no chandelier fixtures are installed.
Ben therefore chose an installation-stage compatibility policy: automatic
sensorless fixtures are uplights, and future chandelier PowerFeathers will be
selected by exact MAC and persist an explicit class override before installation.

Source inspection found the fleet-wide cause behind the apparent hardware pattern.
The fixture's initial class probe polled `0x26`, the older MSA301 address, although
the installed Adafruit library and production MSA311 hardware use `0x62`; the later
runtime MSA311 driver already used the correct default. That split explains why a
ToF-bearing fixture could classify first and initialize its MSA311 later, while an
MSA-only uplight always appeared sensorless during class selection. The prior LOG
inference that the boot class probe used `0x62` was therefore incorrect.

ADR 0067 now supersedes ADR 0041's no-sensor-to-chandelier clause. Source probes
MSA311 at `0x62` and pins PART_ID `0x13`; automatic order is TMF -> downlight,
VL53L5CX -> perimeter, MSA311 -> uplight, then sensorless -> uplight. Existing
downlight/perimeter ToF-loss guards remain. Old sensorless `class_last=chandelier`
records migrate to uplight. A future sensorless chandelier is valid only with its
exact-MAC-rostered persistent `O4` / `class_ovr=4`; `O0` clears that assignment.
BMP581 remains non-classifying.

The complete native fixture suite passed, including 50 class-probe checks. A
guarded field-profile ESP32-S3 PowerFeather build passed with Arduino-ESP32 3.3.7,
using 1,207,517 bytes of program storage and producing a 1,207,808-byte `dev-local`
binary with SHA-256
`7e30b9e8a2688e38f3f680cc3acc91d0c949c78735b8900e3840bb479595d74f`.
That binary is compile evidence only. No revised firmware was flashed: the dirty
worktree must first become a clean commit and immutable ADR 0040 artifact. Next is
an exact Donkey canary, then exact-MAC promotion to the affected uplight cohort;
future chandelier installation is blocked on its MAC/override roster audit.

## 2026-08-28 -- Ben + Codex -- Ponyta dark-night cause and PROTECT release

Onboarded from the repository and diagnosed the PowerFeather that Ben found
dark overnight after a 3.22 V voltmeter reading. A timestamped Windows USB
watcher caught its short wake as exact outer-ring downlight Ponyta `F2B7DC` /
`68:EE:8F:F2:B7:DC` on COM10. The first 11.6-second enumeration was the
fixture's protected field wake, not a failed cable or ROM-downloader problem.
No image was built, selected, flashed, or erased.

Read-only telemetry on `fx-260827-1254f04-p` proved the immediate cause of
darkness: durable guard stage 4 / power tier 3 PROTECT with the LED rail off.
The retained entry audit records PROTECT at 3.236 V while `profile=0`
(commission); Ponyta has since been corrected to field, but a profile change
does not clear the safety latch. This supports the known commission-energy /
low-voltage chain, although the trace does not preserve the minimum loaded
voltage or distinguish a direct voltage transition from a load-armed reset.
The current live configuration also reports 6,000 mAh even though the registry
records this large-enclosure downlight as 15,000 mAh. Ben subsequently confirmed
that every canopy/downlight without exception has a 15 Ah cell, making this a
proven persisted-capacity configuration error.

With USB held continuously, charging enabled after the normal guard and stayed
near +192 to +260 mA at 3.25-3.27 V from a good 4.65-4.78 V input. The BQ
reported no fault and the configured 300 mA precharge read back correctly.
After 60 seconds of qualified charging, firmware released PROTECT to
LEDS_OFF, persisted guard stage 3, and performed its intentional clean software
reboot. MSA311, TMF8820, and BMP581 then initialized and sampled cleanly. The
fixture remains intentionally dark in the intermediate LEDS_OFF tier and needs
continued charging before a nighttime readiness test. Only read-only `t`
queries were sent; the recovery state transition and clean reboot were the
firmware's automatic guarded behavior.

## 2026-08-28 -- Ben + Codex -- Donkey exact current USB flash; MSA still absent

Ben requested the newest accepted production fixture firmware on the red
trunk/uplight. Codex declared this laptop/session the sole firmware and
configuration writer, and exact target Donkey `F2BE10` /
`68:EE:8F:F2:BE:10` on COM8. The dirty worktree was not built. Preflight selected
retained immutable artifact `fx-260828-658b7d2-p` from its manifest and accepted
promotion record, not by mtime: 1,208,640 bytes, source commit `91663fd`, SHA-256
`95de59286831bcbb9d8f610f84b09e3ac761be558f106b10b9aee8dfb01bd8cc`.
Manifest, recipe, embedded revision, local binary digest, FQBN, and Arduino CLI
1.5.1 matched.

Fresh target preflight proved the old `fx-260816-prtrel1-b` application, exact
MAC, 3.341 V battery, +221 mA charging from good USB, no BQ fault, FULL power
tier, and guard stage 1. Direct `arduino-cli upload` reused the retained build
directory and wrote only COM8; esptool identified exact `F2BE10`, verified every
bootloader/partition/application region hash, and reset normally. No build,
OTA, profile/class command, manual NVS mutation, or second target was involved.

Continuous read-only post-flash telemetry at 24.9 through 54.8 seconds proved
exact `fx-260828-658b7d2-p`, field profile, channel 11, app0, no pending verify,
3.343-3.346 V battery, about +254 to +284 mA charging, no BQ fault, configured
300 mA precharge, FULL tier, guard stage 1, and live ESP-NOW. The current image
still reports `sensor_bits=0`, `msa311_present=false`, and chandelier class.
This rules out the old firmware as the MSA detection cause. The flash passed,
but the uplight commissioning gate did not; no completion salute was sent and
the fixture is not installation-ready until the external STEMMA data path or
module is repaired.

## 2026-08-27 -- Ben + Codex -- Donkey replacement MSA and cable still do not ACK

Ben replaced Donkey `F2BE10`'s already-new MSA311 STEMMA cable and reconnected
the fixture. Fresh normal application telemetry on COM8 showed a fully healthy
power posture (guard stage 1, tier FULL, shared 3V3 rail on, good USB, no BQ
fault), but the boot's verified VSQT off/on cycle again reported
`tmf8820=0 vl53l5cx=0 bmp581=0 msa311=0` and classified chandelier. The charger
and gauge continue to communicate on internal Wire1, so the MCU/bus is not
generally dead. With a replacement sensor and cable still receiving power but
no ACK at `0x62`, next isolate the external PowerFeather STEMMA connector/data
contacts against one complete known-good MSA-plus-cable assembly; a new part is
not yet a proven-good A/B reference. No device mutation was sent.

## 2026-08-27 -- Ben + Codex -- Donkey MSA powered but absent on Wire1

Ben observed the MSA311 breakout's power LED lit on recovered Donkey `F2BE10`.
A fresh read at 181.9 seconds uptime simultaneously proved the fixture fully
recovered to power tier 0 / guard stage 1 with the shared 3V3 LED rail on, while
`msa311_present=false` remained. The firmware's probe addresses the MSA311 at
its default `0x62` on Wire1 at the required 100 kHz; the earlier verified VSQT
off/on cycle also received no acknowledgement. Powered-but-absent therefore
narrows the fault to the STEMMA cable/connectors' SDA/SCL path, an unusual
module/address mismatch, or the MSA board itself. No I2C scan command exists in
this older image, and no device mutation was sent.

## 2026-08-27 -- Ben + Codex -- Donkey new-battery PROTECT release; MSA still absent

Ben installed a new battery in Donkey `F2BE10`, left USB connected, and pressed
normal RESET. The application enumerated on COM8 immediately, so ROM bootloader
mode was unnecessary. One continuous DTR/RTS-low, read-only telemetry session
observed the old `fx-260816-prtrel1-b` recovery path without using bare-board
`X`, forcing the LED rail, entering maintenance, or changing configuration.

The new cell was recognized at 3.30-3.32 V / reported 51 percent. After the
normal six-second guard, charging enabled and held about +254 to +265 mA from
roughly 286-296 mA good USB input with no BQ fault. The qualified recovery then
released PROTECT automatically, persisted guard stage 3 / LEDS_OFF, and made a
clean software reboot. Post-reboot charging remained healthy; the LED rail
correctly stayed off in the intermediate power tier. LED smoke acceptance and
current-artifact promotion remain open.

The same reboot performed a verified VSQT off/on recovery cycle and then probed
`tmf8820=0 vl53l5cx=0 bmp581=0 msa311=0`. Repository history contains only the
original sensorless August 16 bare-board commissioning for this MAC, also
classified as chandelier; no retained evidence ever shows an MSA311. The
`uplight` label is a later registry allocation, not a previously proven runtime
class. Inspect/install the MSA311 and its STEMMA cable before treating this as an
accepted trunk light.

## 2026-08-27 -- Ben + Codex -- Archived one-off USB perimeter gobo controller

Retained a dependency-free Windows Forms stopgap for field-testing one perimeter
gobo without Starlink or the T-Deck under
`ops/bench/archive/perimeter_gobo_usb/`. The local launcher automatically
selects the sole COM port, opens native USB CDC with DTR/RTS low, and requires
fresh read-only `t` telemetry proving an exact healthy perimeter fixture before
enabling Start.

Start sends the existing RAM-only `L1` command, whose production renderer walks
one white pixel through the HEX spiral. Stop sends `L0` and explicitly explains
that the boot remains forced dark. Return to Normal sends `L0`, waits for the
rail-off path, and applies the documented 150 ms USB RTS reset pulse so normal
field behavior resumes. The app refuses non-perimeter, deep-recovery,
uninitialized, PROTECT, and LED-OFF targets; fixture-side power/boot/ramp gates
remain authoritative. No firmware, WiFi, OTA, profile, lifecycle, or NVS change
is performed. The code and its self-test are preserved for posterity, but the
installed USB-extension/reset path was never hardware-validated, so this is not
advertised as a supported production workflow.

## 2026-08-27 -- Ben + Codex -- Donkey uplight dark in pre-ADR-0051 PROTECT

Onboarded from the repository and diagnosed the USB-connected trunk/uplight as
Donkey `F2BE10` / `68:EE:8F:F2:BE:10` on COM8. The port was opened with DTR/RTS
low and only the read-only `t` telemetry query was sent; native USB CDC produced
one boot, but no firmware, profile, class, lifecycle, maintenance, rail, NVS, or
other device mutation was requested.

The immediate reason for darkness is authoritative: old artifact
`fx-260816-prtrel1-b` booted with persisted guard stage 4 (`stored=4`, `park=1`),
power tier 3, and the physical LED rail off. USB itself was healthy at 4.760 V /
70 mA with `supply_good=true` and no BQ fault. The BAT node read 2.345 V at about
+2.2 mA, but firmware judged the battery absent/implausible and kept charging
off. Because this artifact predates ADR 0051, its history matches the known
floating-BAT failure mode that could durably latch PROTECT on an empty board;
an actually installed but disconnected or severely depleted cell cannot be
excluded without physical inspection.

Registry history confirms `F2BE10` was OTA-bootstrapped bare and later allocated
to the 6 Ah trunk-light pool. Live auto-probe saw no MSA311 or other STEMMA
sensor, so it classified as `chandelier`, not `uplight`. Check the battery and
MSA311 connections before choosing the battery-absent guarded-clear path versus
the installed-battery recovery lane. The LED module/data wiring remains untested
because firmware never energized the rail.

## 2026-08-27 -- Ben + Codex -- Dual-site WiFi artifact and split-fleet rollout

Ben confirmed that camp and art-site Starlinks must retain distinct network
credentials. ADR 0066 replaces the queued one-virtual-SSID requirement for
fixture maintenance: the image may carry two gitignored credential profiles,
scan once, prefer visible known APs by RSSI, fall back in declaration order,
and share one bounded 30-second join budget. Ordinary COMMS remains ESP-NOW
only. Serial output, recipes, manifests, and git use profile labels rather than
credential values. Added the pure planner plus ten native checks, optional
second-pair compile guards, example placeholders, and firmware documentation.
The full fixture native suite and production ESP32-S3 build passed.

Clean source commit `91663fd0688f6fa903d1086fb023337a372ff179`
produced immutable artifact `fx-260828-658b7d2-p`, 1,208,640 bytes, SHA-256
`95de59286831bcbb9d8f610f84b09e3ac761be558f106b10b9aee8dfb01bd8cc`.
Its recipe label is non-secret. Swablu `F2BE70` passed the exact-target canary:
upload ACK, software-reset mesh rejoin on the exact revision, survival past the
25-second pending-verify gate, and FIELD profile. The first configured network
then carried the wider rollout. The second site profile is compiled into the
same image but still needs one explicit art-site hardware association check.

The final read-only census before releasing T-Deck `8EB508` reported 98 of the
110 intended fixtures on the exact new artifact and 12 on the prior known-good
`fx-260827-1254f04-p`. Every one of the 98 has fresh exact-revision and
pending-window proof; every verified profile was FIELD. No lingering commission
mode was found among the promoted cohort. The 12 intended holdouts are Rikku
`9F26B0`, Groot `9F2724`, Ponyta `F2B7DC`, Cammy `F2B900`, Spyro `F2BCF0`,
Gambit `F2BCF4`, Batman `F2BDC4`, Gengar `F2BDD4`, Toad `F2BEE4`, Daisy
`F40308`, Dratini `F4035C`, and Sneasel `F403DC`. No accepted or ambiguous
upload was automatically retried.

The field crew departed mid-rollout with all 24 physical perimeter fixtures.
The working historical/live perimeter reconstruction shows 13 already
contract-verified on the new image. The art-site USB queue is Cammy `F2B900`,
Spyro `F2BCF0`, Gambit `F2BCF4`, Batman `F2BDC4`, Gengar `F2BDD4`, uncalled
`F2BE80`, Clank `F2BF60`, uncalled `F2BFEC`, Thor `F40344`, Dratini `F4035C`,
and Sneasel `F403DC`. The physical slot-to-MAC map remains incomplete, so read
the identity over USB before writing rather than trusting box position alone.
Clank remains the known unsafe/commission exception and needs supervised power
recovery, not a casual OTA.

The six-way uploader again delivered the predicted transfer speed: the final
20- and 21-target ordinary camp waves each found every target and completed all
41 full upload-plus-verification contracts in 176 and 225 seconds. The overall
campaign did not approach ADR 0062's roughly ten-minute target. A 101-target
roster received only 93 bridge add acknowledgements and failed closed before
discovery; splitting fixed that control-plane loss. A later 53-target wave
found all peers but eight maintenance endpoints vanished before the fresh-power
preflight, so it also failed closed without uploads. The 48-target recovery
wave passed, and an exact nine-target recovery passed eight; Cammy's HTTP peer
closed without an ACK and its mesh revision remained old, so it was held.

Movement of the perimeter load then made three discovered endpoints disappear
before preflight and correctly produced no upload. A separate five-target
PROTECT job found only Rikku during the complete sparse cadence. Its automatic
freeze hit one transient dashboard HTTP 500; the finalizer also missed the
acknowledgement, and a manual exact-job freeze then cleared the bridge before
any upload. One final no-write Rikku attempt expired in bridge phase 3 without
discovery. Follow-up tooling should retry a transient freeze transport error,
accept a fresh stopped/expired phase as safe cleanup evidence, demote vanished
partial-discovery endpoints instead of failing the whole selected wave, and
make large roster add acknowledgement robust. All maintenance campaigns were
inactive, all OTA/dashboard processes were stopped, and COM7 was opened and
released successfully before Ben unplugged the T-Deck.
## 2026-08-27 -- Ben + Codex -- TSwift shell color and copy polish

Ben's physical review accepted the permanent shell and found three small
legibility issues: the `P` in Stop was clipped, app identity blended into the
scrolling ribbon, and the idle text alternated between ambiguous phrases such
as `join ch11 mesh` and the redundant `mesh ch11 mesh`. Widened Stop from 62 to
66 pixels and reduced its internal padding. App identity now uses a blue accent,
the ribbon retains its primary text color, and the independent clock uses cyan.
The idle ticker now spells out `WiFi`, `WiFi joining`, `mesh-only`, or `GUARD`,
followed by separated `ch11`, `live N/M`, and `bat N%` fields. Here `live N/M`
means fresh fixtures now / distinct fixtures seen since this T-Deck booted.

The complete native suite and build-wrapper contract passed. The locked build
produced a 1,565,280-byte binary with SHA-256
`3267a1b237a2a4708e5c999cf1730f497ecbc30bd80e299a8b76374764931d4c`;
the linker reports 49 percent flash and 59 percent global RAM. Esptool flashed
and verified only TSwift `979604` / `44:1B:F6:97:96:04` on COM157. Read-only
post-reset evidence reported `tdeck-dev-local`, channel 11, 19/19 mesh sends
with zero failures, valid local GPS time, and healthy PSRAM, keyboard, touch,
ES7210, GPS, heap, and PSRAM probes. No fixture command or mutation was sent.

## 2026-08-27 -- Ben + Codex -- TSwift promoted to permanent Bridge OS shell bar

Ben accepted the compact activity ribbon as a useful control signal but asked
to replace the overlay with real shell chrome. Bridge OS now permanently owns
the top 26 pixels on every LVGL screen. The left cell carries app identity, the
center carries idle app/network status or a scrolling activity ribbon, and a
fixed right-side clock and Stop button appear while this T-Deck owns a stream
or tracked program lease. Keeping the changing clock out of the text prevents
it from restarting the scroll each second. Permanent streams say
`LOCAL ... until STOP` and count up; expiring program leases count down. Fresh
non-self T-Deck, PUCA, CoreS3, or
unknown publishers remain passive warnings, and local-plus-foreign overlap is
red.

Removed duplicated per-screen title/status objects and migrated the launcher
plus all 13 implemented apps to the y>=26 content contract. LED Studio's class
picker and swatches and Wildfire CA's dense controls were explicitly reflowed;
Fleet and Health moved their former top-right controls to bottom action rows.
The bar never appears or disappears, so starting or stopping activity cannot
shift app content or cover a title, picker, or action button. Fleet detail
charger labels and Fleet View charger-phase filters remain included; Health's
existing VBAT/CHG toggle remains unchanged in behavior.

The complete native Bridge OS suite and build-wrapper contract passed. The
locked development build produced a 1,565,056-byte binary with SHA-256
`fcb0c499e749474e5c49128dcdc6245b613298a89e7b189cde7f0d80f2fc9810`;
the linker reports 49 percent flash and 59 percent global RAM. Esptool flashed
and verified only camp-labelled TSwift `979604` / `44:1B:F6:97:96:04` on
COM157. Read-only post-reset checks passed PSRAM, keyboard, touch, ES7210, and
GPS probes and reported `tdeck-dev-local`, channel 11, 15 live / 19 seen peers,
and active mesh/time publication. No fixture command, OTA, profile/lifecycle
change, reboot request, or NVS mutation was sent. Cross-app touch and conflict
behavior remains for Ben's physical visual acceptance.

## 2026-08-27 -- Ben + Codex -- TSwift banner moved off app action rows

Physical acceptance immediately rejected the first persistent-banner layout:
after LED Studio Green, its full-width bottom strip covered the app's
Solid/Stop/Back controls. Auditing every Bridge OS screen confirmed the bottom
row is universally operational and the top-right also contains controls in
several apps. The common safe region is the top-left title/status area before
the nearest interactive control at x=130.

Replaced the bottom strip with a 122x26 top-left control pill. For local
activity the entire pill is Stop, with stream mode plus explicit `n=` target
count or program countdown in a scrolling label. Foreign-only status is still
informational/non-clickable, and local-plus-foreign conflict remains red. The
pill occupies x=2..124 and y=0..26, leaving LED Studio's x=130 class selector
and y=200 Solid/Stop/Back row untouched.

The full native suite passed. The locked warm build produced a 1,586,544-byte
binary with SHA-256
`aaf2b9ae2a0d111ca69def7a04bee52af0d03cab999f12308338c4cef6335b64`;
the linker reports 50 percent flash and 59 percent global RAM. Esptool flashed
only exact TSwift `979604` / `44:1B:F6:97:96:04` on COM157 and verified every
region. Post-reset serial reported `tdeck-dev-local`, channel 11, live fleet
receive, 14/14 sends with zero failures, and healthy heap/PSRAM. No serial
command or fixture mutation was sent. COM157 was closed for the operator's
immediate LED Studio recheck.

## 2026-08-27 -- Ben + Codex -- Banner build flashed to camp-labelled TSwift

Recorded the camp label `TSwift` for the second LCD T-Deck Plus, exact MAC
`44:1B:F6:97:96:04`, short ID `979604`. USB enumeration independently exposed
that full MAC on COM157; the only remaining dashboard process owned primary
T-Deck COM152, so the exact upload target was unambiguous and free.

Rebuilt through the healthy locked development cache and USB-flashed only
TSwift with the banner and Fleet charge-filter image. The application is
1,586,688 bytes with SHA-256
`f97af709e6ea6bf234d323c53366384880c5fa734ee4a8203ac12bbe32ae45f3`;
esptool identified the same full MAC and verified every written region.

Post-reset serial reported `fw=tdeck-dev-local`, ID `979604`, channel 11, live
mesh receive, 19/19 successful sends with zero failures, roughly 7.7 MB free
PSRAM, and a valid local GPS time source. The local `probe` check passed 8 MB
PSRAM, keyboard, GT911 touch, GPS at 38400 baud, and ES7210 at 0x40. Only local
read-only `help` and `probe` commands were entered; no fixture command, OTA,
profile/lifecycle change, reboot request, or NVS mutation was sent. The monitor
was closed cleanly. Banner touch/expiry/conflict behavior and Fleet charge
filter canaries remain queued for physical acceptance.

## 2026-08-27 -- Ben + Codex -- Global control banner and charge filters built only

Added a shell-level T-Deck control activity banner that stays on LVGL's top
layer across apps. Local direct streams remain labelled `until STOP`; local
program leases show a countdown, and one touch stops the stream and releases
the tracked program without returning to its originating app. The passive side
uses the existing packet header and lease fields to identify fresh non-self
T-Deck, PUCA, CoreS3, or unknown controller activity by exact short ID. It adds
no wire format, discovery packet, or polling traffic.

Kept the already-present Health VBAT/CHG toggle unchanged. Existing Fleet detail
charger decoding now uses the operator labels `CHARGING_CC`, `CHARGING_CV`,
`TOP-OFF`, `DONE/OFF`, and `FAULT`; Fleet View adds filters for those phases plus
unknown and off air. The generated Health roster was reconciled with the
pre-existing local Bidoof registry update (6 Ah, downlight) so its source digest
contract remains exact.

The complete native Bridge OS suite passes, including new publisher freshness,
self-suppression, program lease/release/expiry, and charger-filter tests. A
locked no-upload development build completed at 1,586,688 bytes with SHA-256
`f97af709e6ea6bf234d323c53366384880c5fa734ee4a8203ac12bbe32ae45f3`;
the linker reports 50 percent flash and 59 percent global RAM. Because the build
laptop was issuing a fleet OTA, this session deliberately opened no serial port,
flashed no T-Deck, and sent no mesh, OTA, profile, lifecycle, reboot, or NVS
command. Hardware acceptance remains queued until the OTA writer is finished.

## 2026-08-27 -- Ben + Codex -- Second T-Deck Plus updated from origin

Fast-forwarded the working branch from `ec5e6c6` to current `origin/main`
`027f725a44d57c86a170d4ffba4d9817d1724aec`, preserving the existing local
LOG, TODO, registry, USB-rescue, and camera work. The two append-heavy journal
conflicts were reconciled by retaining the newer upstream record together with
the later local camera and Bidoof updates.

Positively identified the connected old handheld as LCD T-Deck Plus
`44:1B:F6:97:96:04` / short ID `979604` on COM157. It was running
`tdeck-0.1.0`; preflight showed healthy 8 MB PSRAM, keyboard, GT911 touch,
ES7210 at 0x40, GPS at 38400 baud, channel-11 mesh traffic, and retained local
credentials. No fixture or PUCA serial device was present on the selected
port.

The complete native Bridge OS suite and build-wrapper contract passed. Built
current source through the healthy locked warm cache and USB-flashed only
`979604`. The 1,583,456-byte `tdeck-dev-local` binary has SHA-256
`12d43afdf80d483e3340ddf66b554d8878d13a71586917b40c4ad3fe0405d80c`;
esptool verified every written region. Post-reset serial confirmed the same
identity, retained SSID/API/model/channel/day-mode settings, healthy onboard
peripherals, live mesh receive, 19/19 sends with zero failures, and firmware
`tdeck-dev-local`. No fixture command, OTA, profile/channel persistence, or
fixture NVS mutation was sent.

## 2026-08-27 -- Ben + Codex -- Current-source fleet OTA promotion and timing

Codex was the sole writer through exact T-Deck `8EB508` on COM7. Source commit
`d6c923427b6dc0b27ed3d054b457856c084dc0c1` was clean. The T-Deck native suite
passed and its warm-cache `tdeck-dev-local` image was flashed and boot-checked;
the 1,583,456-byte binary has SHA-256
`d22712f7f383968645cb4c381b7ad6abb17257a4c34d16bf9519a1578b8e2b06`.
This folds in the Fleet VBAT/signed-current row, human-readable detail states,
firmware/program filters, DAY/NITE UI, Health VBAT/charge swatch toggle and
clipped legend, charge-state detail, and ADR 0065 best-effort operator knocks.

Built one fresh immutable production fixture artifact from that same commit:
`fx-260827-1254f04-p`, 1,207,376 bytes, SHA-256
`2f9a93344e172b023ee8df473b7c747b26f38dc0ec5353f6efd00d50ec45f4af`.
The full native fixture suite, release build, embedded revision, canonical
recipe, manifest, build options, credential semantics, and clean-source checks
passed. Hawkeye `9F2664` then passed exact-target canary job `F774B62C`: upload
ACK, fresh software-reset mesh rejoin, exact revision beyond the 25-second
pending-verify gate, and FIELD profile, in 53.844 seconds.

The promoted rollout named 103 ordinary-cadence fixtures and a separate six-ID
PROTECT cohort. All 109 post-canary targets accepted the exact binary without an
automatic retry; together with Hawkeye, the final dashboard roster reports all
110 intended fixtures on `fx-260827-1254f04-p` and all 110 in FIELD profile.
The only observed commission fixture is Clank `F2BF60`, which was deliberately
excluded at 0.94 V. Also excluded were protected development canary Akuma
`9E668C`, targeted-maintenance exception Bidoof `9F26D8`, unsafe Tidus `F40424`,
uncommissioned `F402A8`, and off-air protected Magic Wand `F40344`.

The strict ADR 0040 completion ledger accepted 107/110 targets. Gambit
`F2BCF4`, Kabuto `F2BEE4`, and Zubat `F2B7DC` now report the exact new revision,
but their upload jobs did not observe a sufficiently fresh post-upload heartbeat
through the pending-verify contract. No accepted upload was retried. F2BEE4
entered at 3.063 V and returned to PROTECT; F2B7DC entered at 3.227 V with weak
input and later exposed exact-revision 32.096-second uptime, but the cached row
was already outside the verifier's freshness window. Treat these three as
installed-but-unproven until a later fresh exact-revision wake is recorded.

Measured from the first 103-target ordinary job at 22:16:47Z through PROTECT
cleanup at 23:01:56Z, the field rollout took 45 minutes 9 seconds rather than
the predicted roughly 10 minutes. The first ordinary job itself took 9 minutes
49.5 seconds, but sparse maintenance association found only 54/103 endpoints
and verified 53. Across the six post-canary upload waves, all 109 HTTP uploads
ACKed; their upload windows totaled 5 minutes 11 seconds, with per-fixture ACKs
10.43-29.22 seconds (median 14.41 seconds). The new six-way uploader was thus
faster than the runbook's predicted 7-9-minute upload component. Repeated
150-second discovery windows, one fail-closed cohort where a found endpoint
vanished before fresh maintenance preflight, and full verifier timeouts on
sleeping holdouts consumed the speedup.

The state machine's safety behavior held: explicit rosters, complete 1.3-second
maintenance cycles, acknowledged FREEZE before uploads, safe deferral of missing
targets, no duplicate write after upload ACK, exact-revision/pending-window
checks, automatic endpoint resume, and exclusive job ledgers. Follow up by
making partial discovery survive a later endpoint loss, aligning the 5-second
verification freshness threshold with the T-Deck's 10-second full-census serial
cadence (or forwarding proof immediately), and carrying verification across a
full PROTECT wake without weakening ADR 0040.

## 2026-08-27 -- Ben + Codex -- Operator knocks bypass energy qualification

Ben clarified the sparse Knocker result: the control plane should request a
knock from every receiver that hears it. An empty capacitor yielding weak or no
motion is acceptable; DAY_ACTIVE, >=150 mA solar input, and FULL tier must not
pre-filter a deliberate operator request. ADR 0065 supersedes the operator-
strike portions of ADR 0030/0038/0041/0060 while retaining their energy policy for
autonomous program-generated knocks.

Added a pure strike-origin policy and routed exact-target and deduplicated fleet
radio events through the operator path. Received operator traffic now reaches
`solenoidStrike()` regardless of lifecycle, solar, supply-good, or tier. The
persisted arm, rest interval, pulse clamp, D7 collision check, durable load-
armed marker, timer/failsafe, maintenance exclusion, event dedupe, one-pending
bound, and late-drop remain. Native CA/choreography still calls the autonomous
path and retains the renewable/power gate. T-Deck and agent wording now says
attempt/mechanism gate rather than promising a physical impact.

Also fixed the Health page key reported below the physical display: caption and
legend are explicit 310 x 17 pixel clipped rows inside the 320 x 240 viewport,
and both VBAT/CHG legends are shortened to one line.

The complete fixture and T-Deck native suites pass, including the new four-case
strike-origin regression. A production-conditional field fixture development
build passed at 1,207,049 bytes (36 percent) flash and 68,692 bytes (20 percent)
RAM; its 1,207,344-byte `dev-local` binary has SHA-256
`d81285ed81931f5ca3feed4ba36a5038d1e6a1b240abf4d0dcd874886f384f1a`.
The T-Deck development build passed at 1,583,307 bytes (50 percent) flash and
194,856 bytes (59 percent) RAM; its 1,583,456-byte binary has SHA-256
`d22712f7f383968645cb4c381b7ad6abb17257a4c34d16bf9519a1578b8e2b06`.
Neither development image was flashed or OTA-deployed.

## 2026-08-27 -- Ben + Codex -- Sparse Knocker roll is not the live count

Ben observed 92 live fixtures but only sparse audible results from a Knocker
targeted roll. A subsequent read-only 22-second T-Deck serial census captured
106 unique peers, 83 of them under the app's 5-second freshness threshold. Of
the fresh rows whose full lifecycle fields were available, 18 reported
DAY_CHARGE, four reported DAY_ACTIVE, and one reported COMMISSION. Only four
had affirmative FIELD + DAY_ACTIVE + FULL + good input >=150 mA evidence:
`9E5B44`, `9F2664`, `9F266C`, and `F401CC`. Sixty fresh serial rows lacked the
full lifecycle/firmware tail, so they could not be counted as strike-ready.
No fixture command was sent during this diagnosis.

Knocker currently equates `fresh` with rollout membership. It snapshots every
real ID heard within five seconds, regardless of class, full-state evidence,
DAY_ACTIVE, tier, solar input, or physical mallet presence. A 92-target roll at
80 ms per target takes about 7.36 seconds, so an ordinary short-wake peer may
already be asleep when its sorted turn arrives even though it was `live` when
the modal opened. Every receiver then independently refuses a field strike
unless it is DAY_ACTIVE with >=150 mA input and FULL tier; a fixture without
physical mallet hardware is silent regardless. The displayed live count is
therefore send fanout, not expected audible knocks. Add explicit fresh,
mallet-class, state-known, and strike-ready counts/filtering; retain the
multicast modes for the updated fleet when simultaneous awake-peer delivery is
desired.

## 2026-08-27 -- Ben + Codex -- Hawkeye passed ADR 0064 OTA canary

Declared exact T-Deck `8EB508` as the sole writer and exact fixture `9F2664`
(Hawkeye) as the only target. Fresh preflight showed the battery-backed field
fixture on `fx-260826-51d1fe1-p`, 3.571 V battery, +317 mA battery current,
6.137 V / 244 mA input, FIELD profile, DAY_ACTIVE lifecycle, FULL tier, DARK
program, and no protect latch. OTA job `192D6A8B` used retained artifact
`fx-260827-db0cb73-p`, source `8c0e577ed05b32453aeeefa239819920551e7a6d`,
and binary SHA-256
`e30c42802f34966ca5c959539d20bf9c737786c935dc81c3934a3ff638386b5c`.
No other fixture was targeted.

The shared-WiFi maintenance endpoint was found at `192.168.1.254`, fresh
maintenance power preflight passed at 3.570 V battery and 6.026 V / 264 mA
input, and the 1,207,408-byte upload returned its reboot ACK. The host then saw
the exact new revision at 8.649 s uptime and did not accept the canary until a
fresh software-reset heartbeat at 28.521 s uptime had survived the 25-second
pending-verify gate. Profile audit confirmed FIELD; no commission correction or
NVS write was needed. The job completed with one verified, zero deferred, and
zero failed targets. The generated job and upload ledgers are retained under
`ops/bench/data/Nevada City/`.

Post-verify read-only telemetry reported 3.567-3.573 V, +312 to +323 mA IBAT,
6.157-6.232 V / 240-242 mA input, charge status 2 (`CHARGING_CV`), FULL tier,
and LEDs off. At 79.741 s uptime Hawkeye completed the sustained-solar probe
and returned to DAY_ACTIVE. OTA reboot cleared the prior volatile DARK lease,
so its program returned to IDLE. This proves artifact identity, power
ride-through, corrected positive-sun telemetry, solar-active transition, and
the profile audit. It does not yet prove the no-surplus 120-second sleep/wake
path because Hawkeye's input remained above the 150 mA DAY_ACTIVE threshold.
Hold fleet promotion until a low-input wake proves the validity transition and
ordinary sleep behavior.

## 2026-08-27 -- Ben + Codex -- ADR 0064 immutable fixture candidate retained

Committed the complete short-wake power-truth change as clean source
`8c0e577ed05b32453aeeefa239819920551e7a6d`, then generated the canonical
production recipe with field profile, channel 11, LFP, 300 mA precharge,
120-second day sleep, 12,000 ms listen grace, ADR 0039 basic-listener
capability, and the local `party-in-the-woods-v1` credential set. Recipe
SHA-256 is
`db0cb732abe988e6c0e8e691d5c11ccabc92cf127d99691db95fe10542838463`.

The fresh retained build completed normally with the actual production
conditional paths. Candidate `fx-260827-db0cb73-p` uses 1,207,113 bytes (36
percent) of flash and 68,692 bytes (20 percent) of RAM. Its 1,207,408-byte
`fixture.ino.bin` has SHA-256
`e30c42802f34966ca5c959539d20bf9c737786c935dc81c3934a3ff638386b5c`.
`build.options.json`, embedded identity, binary digest, immutable manifest, and
the locally installed PowerFeather SDK source-tree digest all agree. The
manifest marks this `p` artifact `candidate_canary_pending`.

No fixture was commanded, rebooted, profile-mutated, or OTA-flashed. ADR 0064
requires one explicit battery-backed field canary before widening, and ADR 0040
still requires Ben to name the exact short-MAC target roster and sole writer.
A subsequent 18-second read-only T-Deck snapshot captured 230 peer lines / 115
unique peers: 113 field, one persisted commission (`F2BF60` Clank), and one
profile-unknown (`F40424` Tidus). No correction was sent. These are the two
already-known unsafe battery exceptions (about 0.86 V and near-zero/missing,
respectively), so they remain outside ordinary OTA/profile mutation until their
battery paths are repaired or separately proven stable.

## 2026-08-27 -- Ben + Codex -- Short-wake charge truth and Health charge view built

Investigated why most idle daytime fixtures reported battery current around
zero while fixtures held awake by a dark command showed strong charge. The
initial full heartbeat is a boot announcement before lifecycle initialization;
fixture power initialization disables charging until the 6-second battery
guard. The installed 120-second sleep / 3-second listen recipe could therefore
return to deep sleep before charging was re-enabled. Independently, the
MAX17260 Current register can remain a hibernate-era sample for 5.625 seconds,
so a successful early I2C read did not prove a fresh zero. This reopens the
3-second cadence as unsafe for solar-health visibility and charge continuity.

ADR 0064 appends `power_sample_flags` as heartbeat tail 17. Old firmware current
is now explicitly unverified. New firmware trusts IBAT only after 12 seconds,
immediately sends a corrected full heartbeat on that transition, and blocks
ordinary day sleep until the sample is valid or a bounded 15-second gauge-fault
fail-open expires. A live program lease also blocks day sleep for its duration,
so a long Blackout lease cannot be lost through a timer reboot. The full wire
packet is now 193 bytes; the fixture RX buffer was raised to the ESP-NOW
250-byte limit and canonical layout tests pin the append-only offset.

Bridge OS now latches BQ state across short heartbeats, displays signed IBAT
only with the new validity proof, derives human charge phases
`CHARGING_CC`/`CHARGING_CV`/`TOP_OFF`/`NOT_CHARGING/DONE` plus disabled/fault/
unknown/off-air, and lets Health toggle its stable swatches between VBAT and
CHG. Fleet and Health detail use readable class/lifecycle/tier/program/charge
names. The launcher and apps now separate Wake Fleet, Blackout, and Deep sleep.
Wake Fleet is the former repeated dark day-baseline campaign; Blackout is the
releasable LED-off/radio-awake lease.

The OTA host now audits verified runtime profiles. With explicit
`--fix-commission-profile`, it sends persisted FIELD only to exact verified
commission targets and requires a fresh expected-revision field confirmation;
unknown evidence fails closed. Repeated RF profile copies are deduplicated in
fixture firmware so one command cannot multiply NVS writes.

Fixture native tests, the complete T-Deck native suite, and 12 OTA host tests
pass. The field-recipe fixture development build uses 1,197,161 bytes (35
percent flash) and 68,692 bytes (20 percent RAM); its 1,197,456-byte binary has
SHA-256
`129fe62ad4c831b6e7a42ec9aa30eae9a69e769bf1cf98e6a90ec6cd485d472c`.
It is `dev-local`, lacks maintenance WiFi credentials, and was not flashed to
any fixture.

The T-Deck build uses 1,583,199 bytes (50 percent flash) and 194,856 bytes (59
percent RAM). Its 1,583,344-byte binary has SHA-256
`3bc13ca8a8bfeb60aaa31d349721b3760522dc1600f3913dd145400fe1ef905c`.
Preflight read exact COM7 identity `44:1B:F6:8E:B5:08`; only T-Deck `8EB508` was
USB-flashed, esptool verified every written region, and post-reset serial showed
live `8EB508` channel-11 time traffic. No fixture command, OTA, profile write,
or reboot request was sent. Next is one clean immutable credentialed fixture
artifact and a named battery-backed short-wake canary before any exact-roster
fleet OTA.

## 2026-08-27 -- Ben + Codex -- Combined current Bridge OS flashed on 8EB508

Ben requested the new Fleet build on the primary T-Deck and asked whether it
also contained the other recent Bridge upgrades. The previously retained Fleet
binary was older than the current source tree, so it was not reused. Recorded
source digest
`f8b93cb792aee9edd7b7271f7748243f662aca4027038e2e1b2d1a00adc4adbd`
before the build and matched it afterward. The combined source includes the
persistent high-contrast DAY/NIGHT presentation and Fleet quick toggle, the
compact Fleet VBAT/signed-IBAT row, IDLE-versus-unknown state fix, readable
detail names, program/firmware rollout filters, the exact-target Default app,
and the current semantic-white planner.

The complete native T-Deck suite passed. Built and USB-flashed only exact
T-Deck `8EB508` (`44:1B:F6:8E:B5:08`) on COM7; COM6 remained the separately
identified PUCA CP2102N and was never opened. The application uses 1,581,023
bytes (50 percent) of flash and 164,888 bytes (50 percent) of global RAM. The
1,581,168-byte `tdeck-dev-local` binary has SHA-256
`c87b2805feb8bd95c0d6c9ae3022baaa40079483bca652de6c33f738c0e69e7e`.
Upload verified every written region. Three raw `read-flash` attempts stopped
after 4-44 KB with USB serial corruption at 921600, 460800, and 115200 baud;
the independent `verify-flash` operation then compared all 1,581,168
application bytes in place and passed with a matching digest.

After the verification reset, `8EB508` reported `tdeck-dev-local`, channel 11,
`display=day`, saved night backlight 59, mesh up with live receive, 14/14 sends
and zero failures, healthy 8 MB PSRAM/keyboard/touch/ES7210/GPS probes, about
68.7 KB free heap, and about 7.72 MB free PSRAM. Only local read-only
`show`/`probe`/`mem` and emitter on/off diagnostics were used; no fixture
command, profile change, reboot request, OTA request, or NVS mutation was sent.
Physical checks of the new Fleet layout/filter scrolling and Default/semantic-
white behavior remain in TODO.

## 2026-08-27 -- Ben + Codex -- PUCA safe bootstrap and exact-target OTA accepted

Onboarded from and fast-forwarded to `origin/main` at `ec5e6c6`, preserving the
existing uncommitted Bridge and PUCA work through a named safety stash. Reviewed
the incoming T-Deck cache-schema-2 optimization: Arduino owns only
`build/dev-cache`, wrapper recipe/interruption state now lives in sibling
`dev-cache.state`, and the compile-free build-wrapper contract passed locally.
No T-Deck rebuild or flash was required for this acceptance run.

Built the credentialed PUCA Original Edition `puca-bridge-0.5.0-dev` bootstrap
with the explicit 4 MB default dual-app partition layout. The retained
application is 1,024,128 bytes, SHA-256
`1e90f6f1731a622b11274fa91abbc6eeebb17c35abe90bd86337c915cb99e8da`.
The received CP2102N automatic-reset circuit repeatedly booted normally instead
of entering the ROM downloader. The visible onboard button proved to be GPIO36,
not BOOT. With stable Pod20/USB power, holding the accessible `RST` header pin to
`GND`, asserting DTR/download, releasing `RST`, and using esptool
`--before no_reset` entered ROM download; all four flash regions completed hash
verification. A normal jumper was used -- never an ammeter-mode meter or a
short to `VIN`/`VDD`.

Fourteen consecutive post-bootstrap samples reported DJ selected but
`active=0`, `bootarmed=0`, controls locked, healthy codec, and `frames=0`; only
the intended PUCA heartbeat counter advanced. Primary Bridge OS `8EB508` saw
fresh channel-11 `A4EB10` heartbeats with exact revision
`puca-bridge-0.5.0-dev`.

The host then sent only exact command `UA4EB10`. PUCA joined shared maintenance
WiFi, exposed identity-matching telemetry, accepted the exact retained
1,024,128-byte image, rebooted with software reset, rejoined the Bridge with
reset uptime/sequence, and survived the 25 s pending-verify gate. A later
ordinary USB-induced reset again reported SAFE-IDLE, locked controls, codec
ready, and zero direct frames. The OTA result is in
`ops/bench/data/ca/2026-08-27-ota-results.jsonl`. Routine enclosed OTA is now
accepted; paw-held DJ/setup controls, `/resume`/timeout, broadcast rejection,
softAP absence, forced rollback, and visible audio/light acceptance remain open.
## 2026-08-27 -- Ben + Codex -- Local Windows Arducam viewer added

Added `ops/camera/`, a dependency-free localhost USB UVC viewer launched as a
Microsoft Edge app window. It prefers an Arducam/USB Camera source, requests
320x240 through 1920x1080 modes, reports the accepted mode, saves PNG snapshots
and WebM recordings, supports mirror/fullscreen shortcuts, and exposes the
camera controls Windows publishes to Chromium. A guarded launcher reuses one
hidden localhost server; a companion script stops only its recorded matching
process. Video stays local.

The attached device enumerated as USB camera `0C45:6366` with video and audio
interfaces. LilyGO's T-Deck schematic confirms that its ESP32-S3 USB data pins
are present but the Type-C port has sink/device CC pull-downs, VBUS input, and
no host 5 V source. The future T-Deck path is therefore a channel-11 WiFi
320x240 JPEG/MJPEG viewer fed by a laptop or Pi, not a passive OTG cable.

The likely Arducam B0205/B0506 day/night family is designed to leave its IR LED
boards connected: a photoresistor automatically switches the 850 nm LEDs and
mechanical IR-cut filter. Disconnect them, with USB power removed first, only
for visible-only dim-room tests, reflection troubleshooting, or reduced power.
HTML JavaScript, PowerShell launcher/stop parsing, ASCII, diff checks, localhost
serving, and the complete no-permission UI passed. The Edge app is open; final
live-video acceptance awaits the one-time user camera-permission click.

## 2026-08-27 -- Ben + Codex -- Bidoof USB rescue and A/B verification completed

Declared the single state-changing target as Bidoof `9F26D8` on COM156 and
used immutable production artifact `fx-260826-51d1fe1-p` from clean commit
`64264b2c60cbff9a9423642cb6cfee93e4272636`. Its 1,206,784-byte binary freshly
matched manifest SHA-256
`57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`.
No competing flash or OTA process was active.

The generic Arduino uploader failed closed before any write because this
archived artifact retains the application binary but not the companion
bootloader/partition binaries expected by the full-image upload recipe. Bidoof
remained on the old image with uninterrupted uptime and guard stage FULL. The
documented ESP32 application-only path then wrote only the existing app0 address
`0x10000`; esptool positively identified full MAC `D8:85:AC:9F:26:D8`, wrote
1,206,784 bytes, and verified the on-device data hash. The partition table and
NVS were preserved.

The production boot reported the exact revision, healthy field profile, channel
11, good input/charge, downlight class, and healthy MSA311/TMF8820. An explicit
serial `F1` persisted field profile. Local maintenance then joined
`Party In The Woods`, exposed exact identity at `192.168.1.75`, and accepted the
same immutable binary into app1. Fresh serial evidence showed `pending_verify`
at 12.9 and 17.0 seconds, followed by the 20-second network self-test PASS,
`ota_state=valid` at 21.0 seconds, and continued exact-revision stability through
93.9 seconds. Final telemetry was VBAT 3.451 V / +673 mA, qualified 4.637 V input,
no BQ fault, field/DAY_ACTIVE, channel-11 ESP-NOW 114/0 sends, guard FULL, and
healthy MSA311/TMF8820.

The original no-endpoint failure is closed: Bidoof now has working maintenance
WiFi and a validated production image. A T-Deck-targeted maintenance command
could not be rechecked because its COM152 bridge was physically absent; the
dashboard correctly reported disconnected. That one test remains useful only to
exclude a secondary ESP-NOW receive fault. Evidence is in
`ops/bench/data/usb/2026-08-27-bidoof-usb-rescue.jsonl`.

## 2026-08-27 -- Ben + Codex -- Bidoof USB diagnosis found WiFi-less same-version image

The newly attached Espressif native-USB device on COM156 positively identified
as Bidoof `9F26D8` / `D8:85:AC:9F:26:D8`. Read-only serial telemetry showed old
`fx-260816-otafix1-b` on app0 with pending verify false, persisted commission
profile, channel 11, guard stage FULL, and healthy PowerFeather, battery, charger,
radio uplink, LED rail, MSA311, and TMF8820. Settled samples measured about
3.43 V / +613 to +621 mA at the battery and 4.63 V / 610 to 632 mA good input,
with no BQ fault. ESP-NOW status reported 41 successful sends and zero failures.

A transient local USB `u` maintenance test supplied the missing discriminator.
The fixture entered the maintenance transition but printed
`no wifi_secrets.h -> cannot OTA`, immediately resumed ESP-NOW, and returned to
healthy COMMS. No flash, NVS/profile/channel write, manual reset, or persistent
mutation was performed. The accepted immutable `fx-260816-otafix1-b` artifact
had demonstrably working compiled-in maintenance WiFi on its F40364 canary, so
Bidoof is running different bytes under the same historical revision string.
This fully explains why supervised targeted-maintenance campaigns never found
an identity-matching endpoint; those observations do not prove a per-unit
ESP-NOW downlink fault. A secondary receive fault remains untested until the
current exact production artifact is USB-installed and targeted maintenance is
rechecked. Preserve NVS, then verify field profile and reboot ride-through as
already planned.

## 2026-08-27 -- Ben + Codex -- T-Deck warm-build cache boundary repaired and proven

Before changing the wrapper, fetched GitHub and confirmed the clean local HEAD
matched current `origin/main` at `d60d1ca`; the second laptop still needed its
own fetch, which Ben acknowledged before work continued.

Fixed the cache failure exposed by the day-mode upload. Cache schema 2 now
treats `firmware/tdeck_bridge/build/dev-cache` as exclusively Arduino-owned.
The wrapper's recipe hash/detail and interruption marker live in sibling
`dev-cache.state`, where Arduino's source-graph cleanup cannot delete them.
Clean removes both cache and state; recovery quarantines both plus any lock;
the existing atomic single-writer and fail-closed interruption behavior remain.
Schema 2 also recognizes a legacy schema-1 `.build-in-progress` marker and
requires quarantine instead of silently reseeding an interrupted old cache.

Added a compile-free wrapper regression with a fake Arduino CLI. Its first
compile deletes every entry, including dotfiles, from the Arduino build path;
the second invocation must retain the external recipe, report `DEV_CACHE_HIT`,
reuse the simulated LVGL object, and leave no lock or in-progress marker. The
clean path must then remove both directories. The complete native Bridge suite
passes with this regression.

Real-toolchain acceptance used one uninterrupted schema-2 seed followed by two
unchanged builds. The seed took 3,051 s (50m51s); the warm runs took 143 s and
145 s (2m23s and 2m25s), about 21x faster. No `.o` or `.a` timestamp changed
during the warm runs. All produced the same 1,579,040-byte application with SHA-256
`d4eb675e4c0423310325d9c35d8613c985d75edaea29833cfc71cb9c3b67f3db`,
50% flash use, and 44% global RAM use. The build was not flashed. Independent
cold-build byte nondeterminism remains a separate explicit TODO.

## 2026-08-27 -- Ben + Codex -- Rotated-power cohort updated; cadence holdbacks closed

After Ben rotated the USB power cables onto dark lanterns, a read-only 379 s
census found eight ordinary old-firmware candidates with safe power evidence.
The exact immutable target remained `fx-260826-51d1fe1-p` (1,206,784 bytes,
SHA-256 `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`).
Protected Akuma `9E668C`, protected magic-wand Thor `F40344`, and unsafe 0 V
Shuckle `F4031C` were excluded.

The first eight-target 360 s campaign failed closed before upload because
Bidoof `9F26D8`, Cynder `F4042C`, and Meowth `9F266C` did not expose maintenance
endpoints. A follow-up exact five-target batch uploaded Loki `9E5AD4`, Chunli
`9F2714`, Pacman `F40174`, Ponyta `F2B7DC`, and Rikku `9F26B0`; all five supplied
fresh exact-revision heartbeats and survived the 25 s pending-verify gate. The
apparent Cynder/Meowth failure was then correctly identified as cadence: both
had reported `LedTier::PROTECT`, so a separate 930 s PROTECT campaign caught,
uploaded, and pending-verify-proved both. This closes all six fixtures left in
the original 120 s / 3 s cadence rollout defer list.

An authoritative maintenance telemetry audit found Loki, Chunli, Pacman, and
Rikku persisted as `field`, while Ponyta remained `commission`; exact targeted
`FF2B7DC:1:1` changed and persisted Ponyta to field, and a subsequent full mesh
heartbeat reported `profile=1`. Cynder and Meowth reported field immediately
before OTA, and OTA preserves NVS profile. No named nonzero retained peer now
reports commission. A stale `000000/profile=0` record matches Shuckle's old
revision, panic reset, 0 V telemetry, and RSSI and is treated as a transient
identity record rather than a separately addressable fixture.

Bidoof remains the known `fx-260816-otafix1-b` targeted-maintenance downlink
exception: it is continuously fresh on channel 11 with about 3.40 V battery and
strong USB input, but two supervised campaigns never found an identity-matching
WiFi endpoint. Windows exposes only T-Deck COM152; the rotated fixture cables
provide power, not USB data. Bidoof therefore requires a real data-cable USB
rescue. No AP fallback was used.

The failed all-or-none campaign also exposed a cleanup edge case: when
`discover_batch()` raises for missing required targets, its locally found
endpoint map is not returned to `main()`, so the finalizer cannot request
`/resume` for already discovered peers. The bridge roster still freezes
correctly and no upload occurs, but the host should retain incremental discovery
state across the exception so endpoint cleanup is complete.

## 2026-08-27 -- Ben + Codex -- Bridge day mode flashed; dev-cache defect isolated

Ben found the dark Bridge OS presentation too dim in early-morning sun. The
T-Deck now has a persistent Bridge-wide display mode that defaults to day for
existing and new settings. Day mode uses the full 255 backlight and explicit
high-contrast light palettes in the launcher, Fleet, Health, and RF Diagnostics;
night mode restores the saved night-backlight level (59 on primary T-Deck
`8EB508`). Fleet has a quick `DAY`/`NITE` toggle, and Settings exposes the same
mode plus a night-backlight slider. SunTest returns to the active display mode.
The serial console reports the mode and accepts `set display day|night`; `set bl`
now changes the stored night level.

The complete native Bridge suite passed. The ESP32-S3 application is 1,579,040
bytes, using 50% flash and 44% global RAM. It was flashed through the guarded
wrapper to exact T-Deck `8EB508` (`44:1B:F6:8E:B5:08`, COM152); every write
region verified, and a complete application-region readback matched SHA-256
`473510ba76ec5ee9ce47e76575556ac0a7783c78445d913548473e0b3d4b819a`.
After reset it reported `display=day night_bl=59`, rejoined mesh channel 11,
received fresh traffic, reported `sendfail=0`, and passed peripheral and memory
probes. No fixture command or mutation was sent.

This build also exposed a defect in the T-Deck development cache. The first
compile entered as a cache hit, but Arduino rebuilt its dependency graph and
cleaned the build directory, deleting `.dev-cache-recipe.*` metadata that the
wrapper incorrectly stores inside that disposable directory. The immediate
upload invocation therefore failed closed with `DEV_CACHE_RESET reason=missing`
and performed a second full build. Two same-source, same-size full builds also
had different application hashes (`2cabfda9...` then the flashed/read-back
`473510ba...`), so dev builds are not currently byte-reproducible. Repair and
regression coverage are recorded in `TODO.md`; current cache safety behavior is
preserved until that work is done.

## 2026-08-27 -- Ben + Codex -- Fleet table contrast hotfix flashed on 8EB508

Ben's first physical Fleet check found black table text on the dark cell
backgrounds for most live fixtures. The custom Fleet draw hook was styling the
cell-fill task, then returning before LVGL's separate label task; the label
therefore inherited the default black table text. Fleet now sets label contrast
explicitly for every table cell: near-white for headers and live rows, and a
muted light grey for retained off-air rows.

The complete native Bridge suite passed. The corrected `tdeck-dev-local`
application is 1,576,768 bytes with SHA-256
`43cfda9c42e2c73126d232584c4a72717d4b118331dcbe96bf3ab108153f65e2`,
using 50% flash and 44% global RAM. It was flashed through the guarded wrapper
to the exact primary T-Deck `8EB508` (`44:1B:F6:8E:B5:08`, COM152); every write
region verified, and a complete application-region readback matched that hash
exactly. After the readback reset, the Bridge rejoined mesh channel 11, received
fresh fixture traffic, reported `sendfail=0`, and passed peripheral and memory
probes. No fixture command or mutation was sent. Physical contrast confirmation
and the remaining Fleet acceptance checks stay open in `TODO.md`.
## 2026-08-27 -- Ben + Codex -- Fleet truth labels, battery current, and rollout filters

Onboarded from the repo and traced the T-Deck Fleet report in which only a few
fixtures showed `dir` while most showed `idle`. The fixture runtime does not
persist DIRECT through a protection lockout: the lease expires after direct
frames stop, and a reboot/deep-sleep reconstructs runtime state. The actual UI
bug was that each sparse short heartbeat cleared `hasFixtureState`, after which
Fleet rendered the default numeric program value 0 as `idle`. Full-heartbeat
program/profile/lifecycle/tier state is now latched across short heartbeats with
its own evidence timestamp. A sender reboot or sequence restart clears cached
program and firmware evidence until that new boot emits a full heartbeat, so
old state cannot masquerade as current.

Reworked the compact Fleet row around the field-useful values: callsign/class,
`inf`/age, raw VBAT, signed IBAT (`+` charge, `-` discharge), and program. RSSI,
PDR, and advisory gauge SOC remain in detail. An explicit `idle` now means a
reported program 0; `?` means no program evidence. Detail uses human-readable
class, profile, lifecycle, tier, program, and network-mode names such as
`perimeter`, `FIELD`, `DAY_CHARGE`, `PROTECT`, `DIRECT`, and `COMMS`, and also
shows state age, battery/input current, and input-good state.

Extended the scrollable View screen with program filters plus firmware known,
unknown, exact-reference, and not-reference/unknown filters. The firmware
reference picker is populated from up to 16 exact revisions retained by the
census; a compact 32-bit equality fingerprint rides cached census rows so the
many 192-row consumers do not duplicate 24-byte strings. The non-match cohort
deliberately includes missing revision evidence for rollout triage; host-side
fresh-revision and pending-verify gates remain the authority for OTA success.

The complete native T-Deck suite passes, including regressions for IDLE versus
unknown, full-state retention across short heartbeats, reboot invalidation, and
firmware rollout filters. The unflashed `tdeck-dev-local` build uses 1,578,783
bytes (50 percent) of flash and 164,880 bytes (50 percent) of global RAM. Its
1,578,928-byte binary has SHA-256
`d7e544c435b9bc9bbfc26488f985ea06666224cc9cc354f6fa2a4aa2af25c599`.
No hardware was flashed; primary T-Deck `8EB508` still carries the prior
verified image documented below.

## 2026-08-27 -- Ben + Codex -- PUCA safe boot and exact-target shared-WiFi OTA source

Implemented ADR 0063 for the one-off Original Edition PUCA `A4EB10`. The
`0.5.0-dev` source now boots SAFE-IDLE after every reset and emits no
`NB_DIRECT_FRAME` unless the capacitive paw is held continuously for 1.2 seconds
during boot. A successful hold arms line input + DJ (the prior CLASSIC per-slot
look) and opens the existing 20-second setup window; the live cycle is now DJ,
HEARTBEAT, EMBER, HUE. Locked touches still report status only. An OTA/software
reboot deliberately returns dark and requires a later physical paw-held boot to
publish.

Added a PUCA tail-7 heartbeat carrying `puca-bridge-0.5.0-dev`, so Bridge OS can
census and target the publisher without any audio bridge mistaking it for a
fixture. PUCA accepts only an exact `NB_TARGET_ENTER_MAINT` for its own short ID;
the all-zero fleet target is test-covered and ignored. Maintenance stops
publishing without a blackout frame, leaves channel-11 ESP-NOW, joins the shared
maintenance WiFi, and exposes the fixture-compatible `/telemetry`, `/update`,
and `/resume` endpoints. It never creates a softAP. Resume, startup failure, and
the 10-minute timeout return to dark COMMS. The default 4 MB dual-app partition
layout and deferred 20-second codec/I2S/network/NVS self-test provide A/B
rollback; a forced-failure build flag supports the rollback drill.

Extended the build wrapper with explicit default OTA partitions, `--ota`,
`--wifi-source`, and `--ota-fail-selftest`. Updated PUCA hardware/firmware docs,
the Bridge OS field manual, TODO, and added the ADR. The documented end-to-end
path reuses `field_cycle_ota.py`: Bridge OS sends `UA4EB10`, host discovery
identity-matches the PUCA endpoint, waits out the 35-second command tail,
uploads the PUCA binary, then requires fresh exact-revision rejoin evidence and
25-second pending-verify survival.

All 103 native PUCA checks pass. A clean ESP32-PICO-D4 build on explicit
`PartitionScheme=default` uses 1,013,704 bytes (77 percent) of the 1,310,720-byte
app slot and 62,504 bytes of RAM; binary is 1,013,856 bytes, SHA-256
`1ff4f5aa9baf168fe8d6866d92f841b21406a0ad29cc8281901e8f595c674e58`.
This is deliberately only a compile-check: the checkout has no local
`wifi_secrets.h`, so that image would refuse OTA. No hardware was flashed.
Windows also exposed only COM3 / PowerFeather `F2BE20` (`VID_303A`), not the
PUCA CP2102N (`VID_10C4`, previously COM154), so the wrong ESP32 was explicitly
excluded. Next is to re-enumerate the PUCA, provide the shared-WiFi header via
`--wifi-source`, build/USB-flash that credentialed bootstrap, then execute ADR
0063's safe-boot, exact-target OTA, pending-verify, and rollback hardware gates.

## 2026-08-27 -- Ben + Codex -- One plugged canopy class probe verified; second identity still unknown

Onboarded from the repo and investigated Ben's report that two USB-powered
physical canopy lights appeared as uplights while a separate PUCA bench was
active. Preserved the in-progress PUCA source and did not touch PUCA hardware.
Windows exposed only one Espressif USB data device: COM3 / Sonic `F2BE20`
(`68:EE:8F:F2:BE:20`), a registered outer-ring downlight. The other powered
canopy was not data-enumerated on this host, so its exact identity remains
unknown from host evidence.

Opening COM3 with DTR and RTS disabled nevertheless caused the ESP32-S3 native
USB path to report `USB_UART_CHIP_RESET`; no other fixture port was opened after
that. Sonic rebooted cleanly on valid production artifact
`fx-260826-51d1fe1-p`, retained commission/listener, 15 Ah, 2 A, channel 11,
and reported no class mismatch. Its cold boot ID probe found TMF8820 + BMP581
(`sensor_bits=5`) and selected `downlight`; the later MSA driver probe reported
the MSA present, although the early telemetry sample preceded its first healthy
read. This proves the visible listener light is not itself a sensor-health gate
and rules out a missing-TMF/uplight classification for Sonic on this boot.

Ben clarified that the suspected misclassification came from T-Deck LED
Studio's white swatch rendering equal RGB rather than the dedicated W die. The
current planner does use dedicated W for a census class of downlight, but it
also deliberately maps class `unknown` to RGB white. LED Studio consumes the
raw census class and does not use the embedded registry-role fallback that the
new Fleet view uses. Therefore RGB white proves only that the T-Deck did not
have `downlight` latched for that target at planning time; it does not by itself
prove the fixture probed as uplight. Sonic's exact boot evidence is the stronger
current result. Fixtures send a full class-bearing heartbeat at boot and every
5 seconds in commission, but a missed initial full frame can leave a briefly
fresh/unknown target after its short heartbeat arrives.

Ben then ruled out both PUCA and LED Studio as sources for the three illuminated
bench fixtures at about 09:00. The lifecycle trace identifies two remaining
paths. Sonic is explicitly in persisted commission/listener, which never
day-sleeps and deliberately shows a ready beacon regardless of time. A field-
profile fixture without valid scheduled UTC instead falls back to the bare-peer
heuristic: 30 continuous minutes without a good >=20 mA input becomes dusk and
starts autonomous `NIGHT_SHOW`; five minutes of useful input ends that show.
Distributed UTC is RAM-only, expires 30 minutes after the last accepted source,
and is lost immediately on reboot, so a battery-only fixture stranded from the
fleet in bench shade can enter a false night. The autonomous Greenberg-Hastings
CA can look like random light activity. USB normally supplies affirmative day
evidence (Sonic reported about 70 mA), so the USB-powered fixtures remaining lit
for more than five minutes point to commission/listener rather than false dusk;
the unpowered-by-USB fixture could be either. Fleet detail exposes the decisive
state directly: lifecycle 3 is `NIGHT_SHOW`, while 4 is `COMMISSION`.

The initial diagnosis performed no flash, OTA, profile/channel persistence, NVS
mutation, fixture command, or PUCA operation. A follow-up host recheck after more
than five minutes still exposed only COM3/Sonic; no T-Deck and neither other
fixture enumerated as a data device. Elapsed time in shade does not satisfy the
five-minute dawn gate: it specifically requires continuous good input at >=20
mA.

Ben then explicitly authorized the exact `F2BE20` profile repair and the known
USB-open reset risk. With this laptop declared the sole configuration writer,
the guarded transaction identity-checked COM3's USB serial as
`68:EE:8F:F2:BE:20`, sent local `F1`, received `profile -> field`, and verified
with a bare `F` query returning `profile=field`. The live lifecycle immediately
started its daytime solar probe on 358 mA input. No image, channel, or other
fixture was changed.

The same short serial window captured six successive 40 ms `capboard SW1`
strikes, numbered 27 through 32; the host sent no strike command. Ben then
confirmed that the 433 MHz handheld transmitter had been used intermittently
that morning. The receiver and physical SW1 intentionally share the external
D7 path and therefore the same log label, so this is expected transmitter
activity rather than a profile-write side effect or capboard fault. Field
profile deliberately does not suppress that physical/receiver authority path.
The serial port was closed after verification.

Ben next connected the other prior RGB-white canary. USB enumeration identified
Bidoof `9F26D8` on COM4 and, six seconds later, registered outer-ring downlight
Leia `F40384` on COM5. With Ben authorizing the possible USB-open reset, a
read-only COM5 telemetry query proved Leia is running `fx-260826-51d1fe1-p`,
classifies `downlight` with `class_ovr=0` and no mismatch, and has healthy
MSA311, TMF8820, and BMP581. The TMF reported 85,330 successful reads with zero
errors or recoveries. This rules out a current STEMMA/class-probe failure as the
cause of Leia's prior LED Studio RGB-white result and supports the already
traced T-Deck raw-census `unknown` -> RGB semantic-white gap.

The same telemetry exposed two independent current-state facts. Leia remained
in persisted commission/listener (`life_state=4`), so it showed its daytime
ready beacon after commands stopped. It was also actively receiving direct
frames: over the observation interval `direct_seen` rose from 110 to 1,702 and
`direct_matched` from 110 to 716 while program remained 3 (`DIRECT`). Ben then
clarified that PUCA was connected and believed to be publishing. Fixture
telemetry does not retain the direct sender ID, so this observation is attributed
to the declared PUCA bench context and is not evidence of a lingering T-Deck
LED Studio stream. The existing Bridge OS Back-versus-Stop semantics remain
documented separately.

Ben explicitly authorized Leia's exact profile repair. With this laptop the
sole fixture configuration writer, COM5 identity was rechecked as `F40384`,
local `F1` received `profile -> field`, a bare `F` query returned
`profile=field`, and complete telemetry independently confirmed field profile,
`life_state=1` (`DAY_CHARGE`), 328 mA good input, and LED rail off. The live
class remained downlight with healthy TMF. PUCA and Bidoof/COM4 were untouched;
no fixture image or other NVS field changed.

## 2026-08-27 -- Ben + Codex -- Filterable Fleet Bridge OS flashed and read back on 8EB508

Declared the primary T-Deck `8EB508` (`44:1B:F6:8E:B5:08`, COM152) the sole
writer and flashed the already verified `tdeck-dev-local` Fleet image through
the Bridge build wrapper. The 1,576,688-byte application has SHA-256
`703b71a038530029dbd12b0cd072bc589b2d06dc83b12d9c3d4e6ca7073ea622`.
Esptool verified every written region, and a subsequent 1,576,688-byte readback
of the application region matched that SHA-256 exactly. The temporary readback
was removed after verification.

After both the flash and readback reset, the Bridge rejoined mesh channel 11 as
`8EB508` on `tdeck-dev-local`, received fresh fixture traffic, and reported
`sendok=19`, `sendfail=0`. PSRAM, keyboard, touch, codec, and GPS probes passed;
the final memory sample showed 85,744 bytes minimum free heap and 7,718,884
bytes minimum free PSRAM. Only read-only `show`, `probe`, and `mem` console
queries were used for acceptance. No fixture command, OTA, reboot, profile,
lifecycle, or fixture-NVS mutation was sent. Physical screen layout, dropdown,
stable-scroll, detail/back, 192-row memory, and named filtered-identify canary
acceptance remain open in `TODO.md`.

## 2026-08-27 -- Ben + Codex -- Stable filterable T-Deck Fleet view implemented

Onboarded from the repo and traced the Fleet app's scroll jumps to its two-
second live-first re-sort by heartbeat age and RSSI. Replaced that implicit
dynamic order with a native-tested Fleet view model. The default is now the
complete 144-entry production registry plus fresh unexpected peers, sorted
stably by callsign. Registry fixtures retain grey off-air rows, the old 64-row
UI cap is gone, and fixture identity plus scroll context survive refreshes and
detail round trips. The table now exposes raw VBAT instead of advisory SOC;
detail retains both.

Added a View screen with independent roster/seen/live scope, downlight/
perimeter/uplight/chandelier/unknown class filters, good/near-low/low/off-air/
invalid-VBAT filters, and callsign, short-ID, voltage-low/high, recent, or RSSI
sorting. Stable callsign is the default; age and signal are explicitly dynamic
operator choices. Live class telemetry stays authoritative, with registry-role
fallback only for absent or not-yet-full-heartbeat rows.

Added confirmed identify of the current filtered cohort. The button snapshots
only fresh visible IDs, reports the exact count with cancel focused, then walks
those exact targets at 80 ms cadence with 30-second green blinks. It does not
broadcast to hidden/off-air rows and changes no packet, OTA, reboot, profile,
lifecycle, or fixture-NVS contract. Sleep/Dark and Release remain together
behind the Fleet Power entry point.

The complete native Bridge suite passes, including the new stable-roster,
scope, class, battery-band, voltage-sort, and role-fallback tests. A first
embedded pass caught and fixed Arduino's `LOW` GPIO macro colliding with the
battery enum. The clean `tdeck-dev-local` build then passed at 1,576,688 bytes,
SHA-256
`703b71a038530029dbd12b0cd072bc589b2d06dc83b12d9c3d4e6ca7073ea622`,
using 50% flash and 44% global RAM. Documentation is updated. Nothing was
flashed; physical layout/input, stable-scroll, memory-watermark, and named-
canary filtered-identify acceptance remain in `TODO.md`.

## 2026-08-27 -- Ben + Codex -- USB-powered recovery updated 20 fixtures; low-VBAT lane passed

Declared T-Deck `8EB508` / channel 11 the sole writer and recovered the connected
old-firmware cohort with immutable fixture artifact `fx-260826-51d1fe1-p`
(1,206,784 bytes, SHA-256
`57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`).
The expected 24 USB fixtures appeared gradually as 23, 24, and finally 25 exact
USB-powered peers. Twenty were uploaded in staged canary/ordinary/low-VBAT
batches; every one produced a fresh exact-revision heartbeat and survived the
25-second pending-verify gate. Three already-current fixtures were not
reflashed, Thor `F40344` retained its protected net-bench image, and Clank
`F2BF60` was deliberately refused at 0.86 V. Final accounting was 23/25 on the
production artifact, one protected magic wand, and one unsafe low-cell defer.

The 2.2-2.5 V recovery lane passed on real hardware. Daxter `9D7884` was the
one-device canary at 2.324 V, followed by Daisy `F40308` and Dratini `F4035C`.
All three survived OTA, proved an attached battery, enabled guarded charging,
reported field profile and no BQ fault, and showed the expected USB input-current
step. Daxter and Daisy rose to about 2.61 V and graduated to recovery state 4;
Dratini rose from 2.342 V to about 2.46 V and remained actively recovering at
the final sample. The MAX17260 current channel remained asleep at these low
voltages, but BQ state, input current, presence evidence, and rapid VBAT rise
proved charge flow. Tidus `F40424` already had current firmware but still
reported about 0.01 V/recovery refused and needs a battery-path inspection.

The first canary attempts failed closed before upload and exposed two
control-plane bugs. A 100+ peer full snapshot every second permanently exceeded
115200-baud capacity, while the host requested status four times per second and
amplified the backlog. T-Deck snapshots now emit every 10 seconds, host status
requests are bounded to once per 10 seconds with a 30-second acknowledgement
window, and the newly immediate acknowledgement exposed/fixed one ledger
`phase` name collision. Added exact-target-only serial profile command
`F<ID>:<0|1>:<0|1>` (profile/persist), refused for all-zero targets; hardware-
smoked a nonpersistent field command on Gambit. Post-OTA audits found every
sampled fixture already in field profile, so no persistent mutation was needed.

Final exact T-Deck dev-local image on `8EB508` is 1,573,072 bytes, SHA-256
`f42b4a1d40b151e07de1f569fd9cf50a095bf270f3d68547b450953d54ef4d51`.
The full embedded build, all 15 native test binaries, and 29 host OTA/dashboard
tests passed. Live bridge evidence ended on channel 11 at 7,888 successful sends,
zero failures, with the last campaign frozen. Full target/result/ledger detail is
in `docs/tests/USB_POWERED_FIXTURE_RECOVERY_2026-08-27.md`.

## 2026-08-26 -- Ben + Codex -- CoreS3 Milestone A cadence passed on hardware

Flashed only exact CoreS3 `4D5DB0` / `80:45:6B:4D:5D:B0` with the fresh
Module Audio/channel-11 `r6` build. The 1,189,072-byte binary SHA-256 is
`4AB7DB1D7F09526CD82A6E6FE7B22C5455F5DAA951C3956AED8ABB10A6F2E6C2`;
all upload region hashes verified. The full Audio screen transfer initially
limited cadence to about 15 Hz, so active Audio now transfers only its 304 x 96
spectrogram at 25 Hz, updates small meters at 5 Hz, and limits the full USB peer
table to once per 5 seconds while retaining 1 Hz compact timing telemetry.

On the awake fleet, a 65-second Ambient run achieved 24.802 Hz analysis,
24.799 Hz display, and 9.999 Hz direct-frame publishing with zero audio read
failures, radio send failures, or RX queue drops. A preceding 70-second Module
TRS run achieved
24.020/24.006/10.000 Hz; its one radio send failure leaves the 30-minute
zero-failure soak open. Both source handoffs performed an explicit black frame,
and Audio was paused/black at the end. No fixture firmware, OTA, flash, NVS,
profile, or lifecycle state changed. Physical Aux photodiode and Ambient video
sound-to-photon measurements remain open.

## 2026-08-26 -- Ben + Codex -- Live CA color and perimeter-silence diagnosis

Onboarded from the repo and inspected the already-running read-only dashboard
on T-Deck `8EB508` / `COM152`; no fixture command, OTA, profile change, reboot,
or NVS write was sent. At 20:37 PDT the bridge was healthy on channel 11 but
had restarted or been power-cycled about five minutes earlier and reported zero
outgoing frames, sends, or failures. Thus no LED Studio stream was active; a
direct stream disappears from fixtures after about three seconds of silence,
while their autonomous GH-CA continues independently.

The red/orange apparent CA island is primarily the intended class tint, not
proof of a disconnected component. Autonomous GH-CA uses hue 30 for perimeter
HEX fixtures and hue 160 for the rest of the fleet; neighbor excitation crosses
classes without copying hue. Awake perimeter peers Magmar `F2BDFC` and Swablu
`F2BE70` both reported class 2, LED rail on, one warm pixel at roughly
`25,17,0`, and usable bidirectional mesh evidence. Their battery readings were
3.170 V / 8 percent advisory SOC and 3.228 V / 34 percent respectively. The
RSSI-selected strongest-eight, three-second-fresh neighbor view can still make a
local cluster look islanded, especially before a pinned spatial map exists.

The perimeter cohort nevertheless has a real power/recovery problem. Current or
recent known-perimeter evidence included `F403DC` at 2.848 V with its LED rail
off, stale `F2BF74` / `F3FD88` near 2.25 V, `F401DC` near 3.10 V with tier 3 and
rail off, and `F40314` near 3.22 V but still tier 3/rail off. Some protected
wakes report class unknown and are therefore excluded by LED Studio's perimeter
filter even while briefly reachable; local power vetoes would still refuse
light output. Ben corrected the repository's pre-QC inventory state: all 24
perimeter fixtures were QC'd, dead cells were replaced, and none should now be
batteryless. The slot-to-MAC map and registry role coverage remain incomplete;
silence now needs to be separated into low charge, solar/charge-path failure,
identity/firmware trouble, or another hardware fault using fresh voltage and
current evidence. More precisely, QC rejected every installed cell below
3.10 V and replaced it with a factory-fresh cell individually verified above
3.20 V; the replacement cohort was not merely populated with marginal old
cells.

After LED Studio was reactivated, T-Deck `8EB508` transmitted continuously at
about 27 packets/s with successful send callbacks and zero failures. This rules
against a bridge publishing failure as the explanation for only 2 of 24
perimeters lighting. Under that load, `F2BDFC` sagged toward 3.10 V and `F2BE70`
toward 3.20 V. The former is at the LEDs-off/protect boundary and the latter has
little reserve. The current LED Studio run adds load and should be stopped when
not needed, but ten minutes near 180 mA is only about 30 mAh -- far too little
to take a healthy 6 Ah cell from charged to PROTECT.

The depletion also predates the current LED Studio run and the late-day tests.
Thirteen fixtures were already undiscoverable/deferred during the production
rollout, most with prior PROTECT evidence. The new fixture sleep audit shows the
updated cohort using automatic 120-second DAY_CHARGE sleeps, while the current
T-Deck action audit contains no retained Sleep, Dark, or Force action. Today's
207-second PUCA soak and short audio/CA tests are also too small to explain the
class-wide loss. The strongest working hypothesis is therefore accumulated
negative energy balance in the 6 Ah / 2 W perimeter class, caused by inadequate
recent harvest or a shared panel/orientation/cabling/charge-path problem. There
is no historical daytime logger for this interval, so the exact harvest failure
is not yet proven. In the next strong-sun window, leave artistic LED streams
off and capture supply voltage/current plus battery current on several named
perimeters before and after inspecting their P126 panels and connections.

Follow-up review identified the perimeter VL53L5CX as a plausible contributor
but not yet a proven root cause. Every awake perimeter starts the sensor and
ranges continuously from the fixture's perspective at 4x4 / 5 Hz, regardless
of whether the current artistic program uses ToF. Deep-sleep entry explicitly
cuts the complete VSQT sensor rail, so it should add essentially nothing during
the 120-second sleep interval. The ULD documents autonomous mode as its default,
but fixture firmware does not explicitly set or read back the ranging mode or
integration time. ST specifies about 19 mW (roughly 6 mA at 3.3 V) for autonomous
4x4 / 5 Hz / 5 ms, versus 313 mW (roughly 95 mA) in continuous mode. The intended
case is too small to explain 22 of 24 by itself; an unexpected continuous-mode
case would materially worsen the small-battery nightly budget. A named-perimeter
current A/B with identical radio/LED state, plus mode/integration readback, is
required before ruling this axis in or out.

Ben then connected USB to two physically dark perimeter positions. The two USB
identities were `F40344` (Thor) and `F4035C` (Dratini). Thor reported healthy
4.68 V input and about +495 mA battery charge, although the registry currently
describes it as the portable Magic Wand rather than a perimeter; reconcile that
physical assignment. Native USB identified Dratini on COM155. An exact-target
telemetry query restarted that unit once through the USB CDC connection but
made no firmware or configuration change. It booted old
`fx-260816-prtrel1-b`, self-classified as perimeter, and reported durable
PROTECT stage 4, a real but deeply depleted 2.324 V / 6 Ah LFP, good 4.883 V
USB input at 72 mA, no BQ fault, but `battery_present=false`,
`charging_enabled=false`, and 0 mA battery current. Thus Dratini is not being
rescued merely by leaving USB attached. This is the known pre-ADR-0042 firmware
gap: below the old 2.5 V plausible-cell threshold the image refuses charging;
later fleet images add the guarded 2.20-2.50 V recovery lane and verified
300 mA precharge. Dratini needs the explicit supervised USB recovery/flash path
with post-flash positive-current and voltage-rise evidence, not an OTA retry.

The clarified QC threshold makes a firmware-mediated post-QC collapse credible.
Perimeter Sneasel `F403DC` is especially useful evidence: it was 2.296 V before
QC and therefore had to receive one of the >3.20 V replacement cells, but the
dashboard now retains a 2.755 V heartbeat from `fx-260818-05ed4b3-b` with
`profile=0` (commission), about -166 mA, no input, and PROTECT. Dratini's exact
USB telemetry likewise reports commission profile. Commission intentionally
keeps ESP-NOW RX awake; the measured LEDs-dark battery-only floor is about
126-144 mA, or 3.0-3.5 Ah/day before the listener pixel and ToF. At roughly
160-200 mA total, a fixture spends 3.8-4.8 Ah/day. A >3.20 V open-circuit LFP
check proves a real, non-dead cell but does not prove a full 6 Ah charge because
of the flat LFP plateau. A field-default OTA image also does not overwrite an
already persisted commission profile. The likely chain is therefore: a
partially charged factory cell was installed, an old persisted commission
posture spent most or all of its reserve faster than the small P126 could
replace it, and old pre-recovery firmware could then either charge at the old
30 mA precharge rate or, below 2.5 V, refuse charging altogether. This chain is
directly demonstrated on one perimeter and consistent with another, but is not
yet proven for all silent perimeter slots.

Ben then connected two physically dark canopy fixtures by USB. Windows
enumerated only Loki `9E5AD4` / `D8:85:AC:9E:5A:D4` on COM105; no second
Espressif serial, JTAG, bootloader, or failed USB device appeared. Opening
Loki's USB CDC for read-only telemetry restarted it once but made no firmware,
profile, NVS, or other configuration write. Exact telemetry identified
`fx-260818-f80f315-b`, 6 Ah LFP, downlight class, persisted commission profile,
PROTECT stage 4, rail off, and the verified 300 mA precharge setting. With USB
attached it held clean supply and no BQ fault, charged at about +293 mA, and
rose from 2.851 V to 2.868 V over 93 seconds. Loki is therefore deeply depleted
but is successfully recovering on its existing image; keep it attached. The
second canopy remains electrically unidentified and should be connected alone
through Loki's proven data cable/host port to distinguish a cable/port problem
from failure to boot/enumerate on USB.

## 2026-08-26 -- Ben + Codex -- Predictable job-scoped fleet OTA implemented

Implemented ADR 0062's conservative path from the 120/3 rollout post-mortem.
The T-Deck now owns a bounded 160-target exact-maintenance roster instead of one
overwritten target. It schedules at 10 ms per target (1.3 s for 130 fixtures),
uses explicit job-scoped begin/add/freeze/status commands, preserves legacy
`U<ID>`, and emits structured `nb-maint` evidence through the dashboard. Native
campaign tests pass 452 checks including capacity, idempotence, wraparound,
expiry, wrong-job refusal, round-robin timing, and immediate freeze.

Refactored `fleet_dashboard_ota.py` into PLAN -> PREFLIGHT -> GATHER -> DISCOVER
-> FREEZE -> UPLOAD -> VERIFY -> CLEANUP. The host derives a 150 s ordinary or
930 s PROTECT gather, refuses insufficient deadlines, requires a fresh matching
freeze ACK before upload, removes the fixed 38 s tail, separates HTTP results
from promotion, reconciles responsive endpoints before any retry, requests
`/resume` during cleanup, uses independent firmware evidence age, and exclusive-
creates one append-only per-target job ledger. Dashboard and host tests pass.

Recorded deferred multicast/mesh-distribution and pre-stage/activate designs in
`docs/design/FLEET_OTA_FUTURE_SPEED_OPTIONS.md`; shared-WiFi unicast remains the
production transport. Added the 10-minute runbook.

Later in the same session, exact T-Deck `8EB508` on COM152 was flashed with the
complete `dev-local` image (1,572,560 bytes, SHA-256
`96f5bad803c7af646d64c2a9d3e75c42ca1a2eaa50aaac99d79c7830985f3281`).
The restarted dashboard positively reported that bridge on channel 11. A
collision-checked synthetic 130-ID roster reported a 1,300 ms cycle, dispatched
348 packets with 348 successful callbacks and zero failures, retained the same
86-peer census, and stayed exactly at dispatch 348 for one second after FREEZE.
Legacy `UF00001` still parsed as one-target `U<F00001>`, not the new lowercase
freeze command; its fake campaign was explicitly frozen afterward. The source
and live bridge scheduler are validated. A clean immutable bridge promotion,
small real-fixture interruption batch, and timed full fleet pass remain open.

## 2026-08-26 -- Ben + Codex -- PUCA standalone heartbeat baseline flashed and fleet-soaked

Installed the official Silicon Labs CP2102N Windows driver and positively
identified the received PUCA on `COM154` as ESP32-PICO-D4 revision 1.0 with
4 MB embedded flash, MAC `4C:75:25:A4:EB:10`, and USB identity
`USB\\VID_10C4&PID_EA60\\0EC45B486617EC1183509E9D47486EB0`. The S3 bench
device on `COM152` was explicitly excluded. Before the first Resonance flash,
captured an exact 4,194,304-byte full-flash recovery image with SHA-256
`c4f67d01dc1c001e1e6342f7b3b604f77b5f6f701ee4962b3bb94dd5e3d0bfcf`.

Changed the no-laptop runtime to boot in HEARTBEAT + line input with controls
LOCKED. A normal capacitive-paw touch now displays status only, so a DJ or
performer cannot accidentally stop or change the show. A continuous 1.2 s paw
hold at boot opens a 20 s setup window; short touches cycle HEARTBEAT, CLASSIC,
EMBER, and HUE, while a long hold confirms and locks. OFF is deliberately absent
from the paw cycle. The bottom LED encodes input with long pulses and mode with
short pulses. Native coverage for gestures, setup timing, LED timing, waveform
following, full-census chunking, and peer identity reached 85 passing checks.

The first flashed candidate exposed a stale `fixture-*`-only filter: transmit
counters stopped once current ADR 0040 `fx-*` heartbeats had identified every
peer. Aligned PUCA with the already-tested CoreS3 eligibility rule (`fx-*`,
legacy `fixture-*`, and `dev-local`) and added an eligible-fixture count to
status. Built and flashed the corrected `0.4.1-dev` candidate from the unique
`puca-bridge-20260826-standalone-heartbeat-v041-c2` directory. Its binary is
964,752 bytes, SHA-256
`e8ec74680564f96f10c2f6e87b37eb807b9d9ba3b355ccf41c72f8301c4984b6`;
all flash write/read-back hashes verified.

The powered Pod20 booted with steady rails, codec and stereo I2S ready, line
input, HEARTBEAT, channel 11, and LOCKED controls. Ben's normal paw test printed
status only and did not change mode. During a 207 s full-census soak the bridge
grew to 70+ eligible fixtures and 8,318 successful send callbacks with zero send
failures, audio read failures, receive drops, codec I2C errors, or clipped
blocks. This proves the local publisher and more-than-18-fixture packet path;
the performer's exact DG1022Z waveform, visible receiver response/stale fallback,
full knob sweeps, boot-hold setup feel, intended-placement RF/PDR, mixed-output
fidelity, and multi-hour soak remain open.

Left the powered bridge in a deliberate non-publishing service state so silent
HEARTBEAT frames do not own the fleet while no generator is connected. USB key
`A` sent one final eight-chunk black census, status changed to `active=0`, and
the transmit counters remained fixed. This state is intentionally not persisted;
the next power cycle restores active HEARTBEAT + line input + LOCKED controls.

## 2026-08-26 -- Ben + Codex -- 120/3 production rollout reached 85/98; OTA post-mortem recorded

Rolled immutable fixture artifact `fx-260826-51d1fe1-p` (1,206,784 bytes,
SHA-256 `57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`)
from source commit `64264b2c60cbff9a9423642cb6cfee93e4272636` to 85 of the
98 fixtures on the prior production revision. Every promoted fixture produced
a fresh exact-revision mesh heartbeat and survived the 25 s host pending-image
gate. Stage-1 fixtures `9F2638`, `9F26BC`, and `F4019C` also proved the new
cadence end to end: after their 10-minute software-boot grace they slept near
600 s uptime and returned from an approximately 120 s timer sleep with
`reset_reason=deepsleep` and the exact new revision.

Thirteen production fixtures remained undiscovered and were not flashed:
`9F266C, 9F26B0, 9F2714, F2B7DC, F2BCF4, F2BDD4, F2BF7C, F3FD28, F40174,
F401DC, F402A8, F40314, F4042C`. Their last VBAT range was 3.101-3.539 V;
most full heartbeats reported PROTECT. The field PROTECT cadence is 900 s sleep
with only an 8 s wake grace, so the late-day five-minute pass could not span one
cadence. They are deferred for a strong-sun recovery/gather window. Temporary
fleet-dark gathering was explicitly released; special, dev-local, recovery,
prototype, and Magic Wand revisions were not selected.

The rollout exposed a bridge/tool contract bug: T-Deck stores only one sustained
exact-target maintenance campaign, while the host batch tool sends many targets
as if each campaign persists. Rapid lists therefore overwrite earlier targets.
It also exposed the difference between recently heard and positively held
awake, dashboard full-field revision lag, HTTP timeout despite successful OTA,
and a gather/verification race that re-caught three new-image fixtures in
maintenance; exact endpoint telemetry plus `/resume` recovered them without a
second flash. The full evidence ledger, root-cause analysis, deterministic
pacing math, and proposed PLAN -> PREFLIGHT -> GATHER -> DISCOVER -> FREEZE ->
UPLOAD -> VERIFY -> CLEANUP redesign are in
`docs/tests/FLEET_OTA_120X3_ROLLOUT_POSTMORTEM_2026-08-26.md`.

## 2026-08-26 -- Ben + Codex -- 120/3 fleet artifact built; rollout waiting on T-Deck USB

Promoted the validated Akuma cadence recipe into clean source commit
`64264b2c60cbff9a9423642cb6cfee93e4272636`, containing only fixture firmware,
tests, and ADRs 0059/0060. The complete fixture native suite passed, including
172 behavior checks, 63 packet-layout checks, 37 fail-closed serial-mutation
checks, and 13 sleep-audit checks. A first embedded build was interrupted by a
concurrent field-laptop compile; its exact lingering compiler PIDs were stopped
and its partial directory was never reused. The fresh exclusive build completed
normally with the required flash/RAM summary.

The immutable production artifact is `fx-260826-51d1fe1-p`: 1,206,784 bytes,
binary SHA-256
`57306019dbf93a1d0cf950f25b9f557d9a0a68663621a7ce4579aba01dea1261`,
recipe SHA-256
`51d1fe14bcde00e05bca6174bf9c9317d14c1dcaa3568bdcd62c0a1e368e8812`.
Its manifest is clean-source variant `p`, field profile, channel 11, standard
listener capability, 120 s DAY_CHARGE sleep, and 3000 ms timer-wake listen.
The copied canonical artifact and recipe hashes were independently rechecked.

Declared stage 1 as healthy exact targets `9F2638` (Lucas), `9F26BC` (Pinsir),
and `F4019C` (Treecko). Their dry-run preflight passed at 3.335-3.515 V with
live input. The first live maintenance POST returned HTTP 500 because the
T-Deck had disappeared from COM152 and the dashboard was disconnected. Its
recorded last command remained the earlier Akuma test, proving no stage-1
maintenance command was accepted; discovery and upload never started. Resume
only after physical T-Deck USB reconnection and a fresh `8EB508` bridge
heartbeat, then require exact revision through the 25 s pending-verify gate
before widening. Full callout:
`ops/bench/data/ca/20260826-120x3-cadence-rollout-callout.txt`.

## 2026-08-26 -- Ben + Codex -- CoreS3 Milestone A latency scheduler implemented

Implemented the CoreS3-only first slice of the sound-to-photon plan without
changing the fixture wire contract or fixture source. Audio analysis, legacy
direct-frame publishing, the spectrogram display, and bridge status now have
independent phase-locked deadlines. Late work advances from the prior deadline,
skips stale periods without catch-up bursts, and records achieved rate, minimum
and maximum interval, skipped periods, maximum lateness, and maximum capture,
FFT, packetization, display, and total-loop blocking time. This removes the
spectral path's permanent 40 ms / 100 ms quantization to 120 ms (8.3 Hz) while
keeping the installed-fleet publisher target at 10 Hz.

Calibration now measures two seconds across contiguous successful microphone
captures rather than assuming a fixed number of loop iterations. A capture gap
over 200 ms restarts that clock. Decoupling the publisher exposed a stale-data
safety edge: an I2S failure could otherwise replay the last bright feature. A
stale input now blocks publishing and emits one explicit black frame before the
fixture's ordinary fallback. The Audio screen reports achieved FFT/TX rates,
and the one-second USB `audio-timing` line carries the full scheduler evidence.

Added the durable physical-measurement record at
`docs/tests/AUDIO_REACTIVITY_LATENCY_BASELINE_2026-08.md`: Aux uses a common-clock
electrical input edge and fixture photodiode, Ambient uses a 240 fps visible-clap
cross-check, and both require median/p95/max, settle/release, and cohort skew.
The native CoreS3 suite passes 167 checks, including phase regression,
overrun/skip, millis wrap, cadence math, successful-time calibration, stale
capture freshness, FFT/envelope, and app-model coverage.

No device was flashed and no mesh artistic frame or fixture command was sent
during implementation. Several CoreS3 build attempts were abandoned when
unrelated PowerFeather compiles took the shared Arduino path. The final fresh,
exclusive Module Audio/channel-11 `r4` build passed at 1,187,435 sketch bytes
(37 percent) and 100,320 bytes dynamic memory (30 percent). Its 1,187,584-byte
binary SHA-256 is
`8FE579C3B83B8481776D0028635C133031D94EDD5B3A3C9E1077F8FBA65724FF`;
the build options confirm the CoreS3 board, channel 11, and Module Audio flag.
Exact CoreS3 `4D5DB0` / `80:45:6B:4D:5D:B0` then reappeared on `COM43`; its
pre-flash boot reported `cores3-os-0.1.2-dev`, channel 11, and Module Audio
ready. Physical cadence and sound-to-photon evidence remain open.

## 2026-08-26 -- Ben + Codex -- DG1022Z PUCA heartbeat source built

Added a dedicated HEARTBEAT look to the partially proven PUCA performance bridge
without changing the fleet packet contract or any fixture source. USB key `H`
now selects the WM8978 line input and HEARTBEAT in one step. Unlike the ambient
audio path, this mode follows raw stereo block peak with no quiet-room
calibration, so a continuously repeating generator waveform is not learned away
as noise. A fast peak follower preserves narrow pulses within the existing
100 ms capture / approximately 10 Hz direct-frame ceiling, and KNOB2 caps a
shared deep-red RGB pulse. Status output adds `wave` beside raw peak/clipping
evidence. Native coverage grew to 39 passing checks for peak attack/release,
sensitivity, and heartbeat color.

A clean `esp32:esp32:pico32` build completed at 963,248 bytes with SHA-256
`365bd7d61e67897c8080c9089f5c2176e41f099664c2090b7e42a9bef66a68d9`.
This is a development compile-check, not an ADR 0040 promoted artifact; nothing
was flashed or transmitted.

The Rigol DG1000Z manual establishes that DG1022Z has only two independent
arbitrary outputs. Its two rear Sync connectors can provide TTL repetition
timing, but an arbitrary waveform produces a 50 percent duty square rather than
a copy of its internal shape; the current AC-coupled peak input may detect both
square-wave edges. Exact waveform following therefore needs an
analog monitor copy; a passive BNC T is an acceptable high-impedance no-human
bench tap but is not acceptable evidence for a body-connected ceremony. Added
`docs/howto/DG1022Z_PUCA_HEARTBEAT.md` with the initial gain staging, clipping
checks, and a hard requirement for a qualified, separately isolated monitor
boundary before PUCA, Eurorack, USB, inverter, or Tree grounds approach the
human-connected circuit. Hardware line-in, waveform/light fidelity, RF, and
isolation acceptance remain open.

## 2026-08-26 -- Ben + Codex -- 120/3 energy decision moved to theory gate

Ben confirmed there is no INA instrumentation at the Burning Man camp, so the
Akuma cadence decision cannot depend on an unavailable empirical current test.
Replaced that field blocker with an explicit break-even model. Let `b` be the
fixed boot/setup energy expressed as the equivalent number of seconds at the
ordinary awake/listening power. Comparing full cycle active fractions,
`(b+3)/(120+b+3)` for the canary and `(b+15)/(300+b+15)` for production, gives
an exact break-even at `b=5 s`. The 120/3 recipe uses less awake energy when
`b<5 s`, the same at 5 s, and more when `b>5 s`.

Akuma's passive T-Deck evidence placed a post-deep-sleep heartbeat at about
1.1 s uptime, after board/sensor/radio initialization. Even treating that boot
interval as 2-3x the steady awake power gives roughly 2.2-3.3 awake-equivalent
seconds, still below the 5-second boundary. At `b=1.1 s`, the idealized awake
fractions are about 3.3 percent for 120/3 versus 5.1 percent for 300/15, about
35 percent lower; at a conservative `b=3 s`, 120/3 is still about 16 percent
lower. Sleep current is already known to be sub-mA and does not reverse this
comparison. Therefore the field recommendation is to treat 120/3 as likely an
energy improvement, not merely a responsiveness improvement, while retaining
the honest uncertainty of a theory-based estimate. Promote by staged fleet
observation of census/reachability rather than an INA gate that cannot be run
onsite.

## 2026-08-26 -- Ben + Codex -- Akuma 120/3 maintenance catch passed 3/3

Removed only Akuma `9E668C` from USB and left the T-Deck on `COM152`. Turning
the panel face-down on reflective playa was not enough: live Akuma heartbeats
continued to report about 4.648 V / 538 mA, well above the ADR 0060 150 mA
surplus threshold. Adding opaque plastic reduced measured input first to 42 mA
and then 0 mA / supply-not-good. That immediate physical-to-telemetry transition,
together with the burned-in full MAC inventory, confirmed the handled fixture
was Akuma even though the requested visible identify did not light it.

The identify miss exposed a real operator-parity issue rather than an RF miss.
T-Deck Fleet UI identify explicitly requests a green blinking production-light
frame, but serial quick command `i9E668C:10` calls the legacy color-0 default,
which drives only the PowerFeather status LED hidden inside the enclosure. The
field manual currently describes the serial example as green; align or clearly
separate those commands before relying on laptop identify in the field.

After the low-input hysteresis and the existing software-boot grace expired,
Akuma entered the experimental 120-second day-charge sleep cadence. Sustained
exact-target `U9E668C` campaigns caught three clean timer wakes in 86.4, 100.9,
and 94.3 seconds (3/3; mean 93.9 s, maximum 100.9 s). HTTP telemetry became
available at fixture uptimes 3.746, 4.319, and 4.380 seconds. Each catch reported
`reset_reason=deepsleep`, `last_sleep_reason=day-charge`, `last_sleep_s=120`,
`day_sleep_s=120`, and `wake_listen_ms=3000`. Because WiFi association and HTTP
startup happen after the ESP-NOW command is accepted, the roughly 4-second HTTP
times are compatible with and prove command receipt inside the 3-second radio
window.

Two apparent early repeats were deliberately excluded: one caught the 10-minute
non-deep-sleep boot grace after a same-binary OTA, and one caught a still-active
35-second T-Deck maintenance tail. The clean trials waited out each RF tail,
called `/resume`, confirmed the maintenance endpoint remained closed, and then
started from a real timer sleep. After the third pass, `/resume` again succeeded
and the endpoint remained closed for 12 seconds, returning Akuma to natural
cadence. No other fixture ID was commanded or flashed; fleet cadence remains
300/15 and the fleet still runs its prior immutable image.

The 3-second window is therefore a promising command-reachability setting on
this canary, but it is not yet an energy result. A 120-second sleeper wakes 2.5x
as often as a 300-second sleeper, so fixed boot/sensor/radio cost can erase the
listen-time savings. Use the INA logger to compare complete cycle charge before
fleet promotion. The session also showed temporally contradictory T-Deck and
direct same-revision records while modes were changing; exact cause remains
unproven, so keep the verifier fail-closed and reproduce with synchronized raw
serial, USB, and HTTP evidence rather than labeling it a cache bug prematurely.

## 2026-08-26 -- Ben + Codex -- Sound-to-photon latency development plan

Made reactive latency and cross-fixture skew explicit artistic requirements and
added `docs/projects/LOW_LATENCY_AUDIO_REACTIVITY_DEV_PLAN.md`. The plan separates
an immediate CoreS3-only path from a gated future fixture path. The immediate
work measures true sound-to-photon median/p95/max, fixes the spectral source's
nominal 10 Hz publisher quantizing to 8.3 Hz, decouples capture/analysis/TX/UI,
adds an 8 ms transient lane beside the rolling 512-sample FFT, and measures how
far faster direct publishing can improve the existing 10 Hz fixture latch.

The future path is conditional on those measurements. It proposes one compact
fleet-size-independent audio-feature broadcast, local audio mapping, and an
event-driven/50 Hz fixture latch only while a fresh Audio lease is active. It
includes old/new mixed-fleet capability handling, fast missing-stream decay,
native wire/lease tests, physical HEX/RGBW USB canaries, 130/150-node RF gates,
LFP soak, and ADR 0040 staged promotion. No firmware was changed, built, flashed,
or transmitted as part of the planning session; no fixture OTA was authorized.

## 2026-08-26 -- Ben + Codex -- CoreS3 spectral Audio experiment source-built

Kept frequency analysis inside the existing CoreS3 Audio app rather than adding
a third launcher app (ADR 0061). Audio now analyzes a 512-sample Hann-windowed
FFT at a nominal 25 Hz while retaining the proven 10 Hz `NB_DIRECT_FRAME` fleet
cap. The on-device view is a roughly three-second, 24-row log-frequency
spectrogram with live 60-250 Hz bass, 250-2000 Hz mid, and 2000-8000 Hz high
meters. Existing CLASSIC, EMBER, HUECYCLE, and PULSE remain; BANDS RGB, BANDS
SPLIT, and TIMBRE HUE add common-color, stable fixture-third, and spectral-
centroid responses without a packet or fixture firmware change.

Separated capture/analysis/display cadence from fleet TX, added independent
adaptive AGC per band, cached the Hann window and log-row bin map, and preserved
the Audio app's program-release, source-handoff, black-frame, pause/exit, and
stale-fallback contracts. Corrected Module Audio analysis to collapse complete
interleaved L/R frames to mono; treating the stereo sequence as one waveform
can manufacture a false high-frequency rail. A Windows slash/backslash include
identity failure in the first cold build duplicated the M5GFX type graph. The
Module build now includes M5Unified only through M5Module_Audio, while built-in
and Cambium variants include it directly.

The native CoreS3 suite passes 118 checks, including synthesized 125 Hz, 1 kHz,
and 5 kHz inputs, 44.1 kHz Aux bin mapping, centroid ordering, stereo-to-mono
conversion, and band color planning. The final fresh channel-11 Module Audio
build passes at 1,185,799 sketch bytes (37 percent) and 100,120 bytes of dynamic
memory (30 percent). Binary
`cores3-spectrum-module-20260826-r3/cores3_bridge.ino.bin` is 1,185,952 bytes
with SHA-256
`94B2AB4BE005F4CE682B92859029BE43D85D9A270A67F0E9494D8654F7C1731C`;
`build.options.json` confirms `esp32:esp32:m5stack_cores3`, channel 11, and
`CORES3_AUDIO_MODULE=1`.

No device was flashed. Exact CoreS3 `4D5DB0` was not attached; the only USB
serial devices were fixture `9E668C` and T-Deck `8EB508`. No fixture command,
OTA, NVS write, profile/lifecycle change, or mesh artistic frame was sent.
Hardware validation remains the measured Ambient/Aux spectrogram cadence,
band-dominant material across the three new modes, touch/input handoff, and
pause/app-exit fallback on a declared awake cohort.

## 2026-08-26 -- Ben + Codex -- Akuma 120/3 cadence canary installed; fleet unchanged

Kept the production field cadence at 300 seconds asleep / 15 seconds listening
and made both values explicit build-recipe controls. `build.sh` now validates
`--day-sleep-s` and `--wake-listen-ms`, includes them in the recipe fingerprint,
and fixture USB telemetry reports the compiled values. The complete fixture
native suite passes, including lifecycle, presence, sleep-audit, serial-mutation,
packet-layout, power, and sensor coverage. The host OTA verifier has five new
same-revision evidence tests and now requires a pre/post uptime or sequence
reset for `dev-local` or another unchanged revision, plus a fresh matching
heartbeat beyond the 25-second pending-verify window.

Built only the Akuma experiment with a 120-second ordinary field sleep and a
3,000 ms post-initialization listen window. The locked development-cache build
completed normally at 1,205,920 binary bytes, SHA-256
`7fca2ae63b3c840c31400d269f382cf0222a66e2e5bc0d839a9b8e7297d3283c`.
It contains the current fixture tree, including ADR 0059 sleep provenance, the
conditional solar wake, hardened addressed serial sleep, and repeated Color
Virus strains. Production defaults and every non-Akuma fixture remained
unchanged.

The addressed maintenance/OTA path named only Akuma `9E668C`; it discovered the
identity-matching endpoint at `192.168.1.126` and uploaded the exact binary. The
new verifier then correctly failed closed because the host dashboard continued
to timestamp a cached pre-reboot T-Deck census record instead of showing a reset
transition. Direct no-reset USB telemetry independently proved the new app:
`reset_reason=software`, `ota_state=valid`, pending verify false, `app0`, field
profile, `day_sleep_s=120`, `wake_listen_ms=3000`, healthy FULL/DAY_ACTIVE at
99 percent SOC. This also exposed a separate dashboard freshness bug: repeated
serialization of a cached T-Deck peer must not refresh host evidence age.

Natural cadence and command-catch validation remain pending physical removal of
Akuma's USB/good-solar input; otherwise ADR 0060 intentionally keeps it awake.
Measure several 120/3 cycles and full wake energy before considering fleet
promotion. The earlier 99-fixture Color Virus artifact predates ADR 0059, and
the provenance-capable T-Deck build was not flashed, so do not infer fleet or
handheld provenance from the Akuma result.

## 2026-08-26 -- Ben + Codex -- USB sleep parser hardened and proven on Akuma

Replaced the fixture's fail-open one-byte serial sleep command with a bounded
platform-independent mutation parser. Timer sleep now accepts only a complete
newline-terminated `!S<short-mac>:<seconds>` envelope for the receiving fixture,
with a canonical explicit duration from 1 through 65,535 seconds. Bare `S`,
`S1`, missing/zero/overflow durations, wrong IDs, partial lines, nonprintable
bytes, repeated sentinels, and overlong input cannot call the sleep handler. The
commissioning helper's one-second TMF sensor-domain reset now uses the targeted
form. The radio sleep packet contract is unchanged.

The complete fixture native suite passes, including 37 new parser checks and a
250,000-byte deterministic junk stream; the 16 host dashboard tests and Python
syntax check pass. The field-profile ESP32 build completed through the locked
development cache at 1,205,856 binary bytes, SHA-256
`54A3118B10BB9A9FD58815429AC4470599B71EE621DF9B73AE717A001A354AB8`.

Closed-lid OTA targeted only downlight Akuma `9E668C`. Direct USB telemetry
proved the new image valid in `app1`, pending verification cleared, software
reset, both downlight sensors healthy, and DAY_ACTIVE on measured external
input. A representative legacy/ROM-style stream containing bare `S`, `S1`, and
`S10` caused no reset and no new immediate sleep audit. The exact
`!S9E668C:10` command then produced a timer wake with `reset_reason=deepsleep`,
`last_sleep_reason=serial`, `last_sleep_s=10`, restarted uptime, and subsequent
`life_state=2` / FULL power on about 4.64 V and 1.10 A input. No other fixture
was flashed, rebooted, or commanded.

The run also exposed a separate verification bug: because both canary binaries
report `dev-local`, `field_cycle_ota.py` accepted matching cached dashboard
state before proving a post-job reboot. Direct USB later supplied the real
software-reset/app1/valid evidence, but the automatic result was not sufficient
under ADR 0040. The helper must correlate a post-job reset/sequence transition
and fail closed when a mutable same-revision image cannot be distinguished.

## 2026-08-26 -- Ben + Codex -- Closed-lid solar canary exposes USB sleep parser blocker

Used exact downlight Akuma `9E668C` as the only fixture canary. It was healthy
at 3.388 V with both expected downlight sensors. The existing field image woke
for only a few seconds, so no lid or reset-button access was used. The standard
sleeping-peer maintenance helper repeatedly targeted Akuma, caught its timed
wake at `192.168.1.126`, validated the responding fixture ID, and OTA-uploaded
the existing 1,205,312-byte `dev-local` artifact with SHA-256
`04498751D88E90EBC01712D28C6A5D7FDB93F0E13527FDBA3EB9E7282D9AF232`.
Akuma survived the pending-verify window and freshly rejoined with the expected
identity. No other fixture was flashed or rebooted.

Hardware then observed the conditional input path reach `life_state=2` / FULL
power after the 60-second confirmation and remain continuously available on
about 4.64 V external input. A later audited timer wake reported `deepsleep`,
restarted uptime, and again remained awake while input current was about 1.2 A.
This validates the positive conditional-solar path on one canary, but does not
yet cover weak/tapered input, threshold hysteresis, or repeated catch rate.

The same session found a release-blocking serial safety regression. Fixture
`handleSerial()` is byte-oriented; receiving uppercase `S` calls
`readSerialUint(80, 65535)`, and the no-digit result deliberately defaults to a
21,600-second sleep. It requires neither a complete line nor the fixture ID and
has no input arm/quarantine. Outbound ROM boot text does not normally feed RX,
but a host probe, echo, or corrupted inbound stream containing `S` can park the
device; the existing ModemManager TODO is relevant defense in depth, not a
parser guarantee. Do not promote this development image beyond Akuma until
sleep requires a bounded, newline-framed, explicitly targeted command and junk
input is fuzz-tested fail-closed. Also observed that the host dashboard's
legacy `P<id>:<seconds>` serial command is accepted by the dashboard but absent
from T-Deck serial quick commands, so it reports queue success without putting
the target to sleep; reconcile or explicitly reject that combination.

## 2026-08-26 -- Ben + Codex -- Conditional solar probe repairs daytime wake

Live daytime census evidence showed field fixtures repeatedly waking from deep
sleep for about 15 seconds, then returning to the 300-second cadence. Source
review found that `DAY_ACTIVE` required 60 continuous seconds of surplus while
the lifecycle and timer reset on every deep-sleep boot, making the transition
unreachable once cadence had begun.

ADR 0060 keeps the ordinary 300/15 energy posture but lets measured good input
at or above the existing 150 mA strike-surplus threshold hold only that wake
open. Sixty continuous seconds at FULL tier enters `DAY_ACTIVE`; a transient
clears the probe and permits immediate sleep after the original grace. Battery
voltage alone no longer counts as surplus. Active fixtures use a 100 mA remain-
awake threshold and return to `DAY_CHARGE` after 300 continuous seconds below
it; strikes still require FULL tier and at least 150 mA at the instant of use.
Night/day transitions clear candidate history, and a live commission-to-field
profile change now reinitializes the lifecycle instead of potentially remaining
in `LIFE_COMMISSION` until reboot. The probe is RAM-only and adds no wire, RTC,
or NVS state.

The complete fixture native suite passes, including 172 behavior checks and all
power, schedule, sleep-audit, packet, presence, and sensor tests. The cached
field-profile ESP32 build passes at 1,205,312 binary bytes with SHA-256
`04498751d88e90ebc01712d28c6a5d7fdb93f0e13527fdba3eb9e7282d9af232`.
This is a development-cache compile check, not an ADR 0040 fleet artifact. No
hardware was flashed and no fixture command was sent. Named-canary current and
transition validation remains open before fleet promotion.

## 2026-08-25 -- Ben + Codex -- Durable action and sleep provenance

Implemented ADR 0059 after the post-OTA fleet-silence diagnosis showed that a
generic `deepsleep` reset reason could not distinguish low-battery PROTECT from
an operator sleep. T-Deck Bridge OS now retains a checksummed four-action NVS
ring for fleet Sleep, Dark, Release, and lifecycle overrides. Each record is
allocated with the exact outgoing mesh sequence and saved before RF, with fresh
GPS UTC when available and bridge uptime as the fallback. Availability-reducing
commands fail closed if their audit cannot persist; restorative Release remains
allowed so storage failure cannot strand a reachable fleet.

Fixtures now record every imminent timer sleep in RTC slow memory and name the
immediate cause on the next true deep-sleep wake. Rare operator-sleep receipts
and first entry into PROTECT are additionally persisted in NVS; recurring
day-charge and PROTECT sleeps do not write flash. A validated operator sleep is
refused if its NVS receipt cannot be saved. Append-only heartbeat tail 16 brings
the full heartbeat from 160 to exactly 192 bytes and exposes immediate cause,
source ID/sequence, retained command receipt, and last PROTECT-entry voltage.
USB JSON retains the fuller records. T-Deck census/serial output, the host
dashboard, and the JSONL logger parse and display the new evidence while legacy
lines remain valid.

The Sleep / Dark app now opens on reversible Dark for 10 minutes instead of
low-power sleep for one hour. Its screen shows the newest retained action, and
serial `show` prints all four. Fixture native tests pass in full; the complete
T-Deck native suite passes in an isolated temporary tree regenerated from the
already-edited fleet registry; all 16 dashboard parser/policy tests and Python
syntax checks pass. Development builds also pass: fixture 1,205,056 bytes,
SHA-256 `c0851404de6ab9917f2cc32ed5004d074edcb997e72a043fd6c6a7f639abeeb8`;
T-Deck 1,570,560 bytes, SHA-256
`2ad8d57bf8fdd4285484b395f964cd41861eae6c16ba39089828c18c70dcd094`.
No hardware was flashed and no fixture command was sent. Isolated hardware
validation of reboot survival, sequence correlation, automatic sleep causes,
and NVS-failure refusal remains open.

## 2026-08-26 -- Ben + Codex -- Burning Man Windows build laptop bootstrap

Bootstrapped the new Windows build laptop from `main` at `20c26c6` without
flashing any hardware. Installed user-scoped Scoop plus Git 2.55.0, Arduino CLI
1.5.1, GCC 15.2.0, Make 4.4.1, Python 3.12.10, Node 22.23.2/npm 10.9.8,
GitHub CLI 2.98.0, ripgrep, jq, and Playwright Chromium. Persisted the tool
paths for fresh terminals and configured Git for LF worktrees so registry hash
generation remains deterministic on Windows.

Installed the recorded firmware baseline: ESP32 Arduino core 3.3.7,
PowerFeather SDK 2.1.0, Adafruit BusIO 1.17.4, MSA301 1.1.4, NeoPixel 1.15.5,
Unified Sensor 1.1.15, SparkFun Qwiic TMF882X 1.0.2, LVGL 9.5.0, LovyanGFX
1.2.24, M5Unified 0.2.21, and M5GFX 0.2.28. The optional M5Module Audio
library is pinned at `d8649a4863fea4a47d690781eb330f7c28a434b3`.

All fixture, TDeck, and CoreS3 native suites pass. A channel-11 commission
fixture cold cache seed produced 1,185,792 bytes / SHA-256
`534256be106000ca0826ff683c4054cab19b2f8236834a7d7f79ea039a233377`;
its warm rebuild was 18 seconds with an explicit cache hit. The TDeck cold seed
was 12m34s and produced 1,552,672 bytes / SHA-256
`73c249bd206b6d51b2f30193764b3a0a18262be59b0b9a8150748e1aa7a89f27`;
its warm rebuild was 48 seconds. The standalone channel-11 CoreS3 build took
5m38s and produced 1,150,128 bytes / SHA-256
`d6f3ab999b19fae43eefd02a16c5407cc03c0eb6adfd6e8ffd3bd97e9c8a0de2`.

The app production build and all 44 unit-test files pass (367 passed, 4
intentionally skipped). Playwright Chromium passed the unplugged BLINK scenario
and a 121-control audit; the complete 43-case serial visual acceptance campaign
was intentionally stopped after those smoke cases because several cases carry
9-15 minute budgets. Python scientific/serial imports pass and the locator suite
passes 34/34. Bench tests pass 26/27; the one repository-level failure is a
stale assertion expecting 141 callsigns while the current generated production
health roster correctly contains 144. GitHub CLI still needs `gh auth login`,
field Wi-Fi secrets remain intentionally absent, and no USB boards were present.
`npm ci` also reports 10 dependency advisories (3 moderate, 5 high, 2 critical);
no lockfile-changing automatic audit fix was applied.

## 2026-08-25 -- Ben + Codex -- CoreS3 Audio CA takeover and fx-fleet fix

The first unplugged all-awake Audio test showed `PUBLISHING` on CoreS3 while the
fixtures remained in CA. Two independent causes were proven. Fixture arbitration
correctly gives an explicit CA/Contagion/Dark program lease precedence over
direct-frame micro-leases, and CoreS3 Audio did not release that hidden prior
lease. Its full-heartbeat target filter also accepted only obsolete `fixture-*`
firmware names, so it counted the current ADR 0040 `fx-*` fleet as live on screen
while silently omitting those peers from direct frames.

Audio Start now sends the existing fleet `NB_PROGRAM_SET` release before its
first direct frame. This is RAM-only and changes no fixture firmware, NVS,
profile, lifecycle, or autonomous default. The Audio selector now native-tests
current `fx-*`, older `fixture-*`, and fixture-cache `dev-local` identities while
still excluding identified legacy net-bench/bridge peers. ADR 0058 records the
ownership and selection behavior. The native Audio/app suite passes 42 checks.

Built and USB-flashed exact CoreS3 `4D5DB0` (`80:45:6B:4D:5D:B0`) with the
channel-11 Module Audio `cores3-os-0.1.2-dev` artifact: 1,168,352 bytes, SHA-256
`FDDAC35CA9778D1698763F77FAABA88A5FBB56A8167C1D24EE6E0701F1742C65`.
Esptool independently reconfirmed the hardware MAC and the app-region readback
matched that SHA byte-for-byte. Hardware then selected Builtin Dual Mic,
completed calibration, and published four fleet chunks per 10 Hz tick with zero
send/read failures. The separate T-Deck `8EB508` dashboard observed fresh
fixtures transition from CA/autonomous programs to program 3 Direct, including
five simultaneous fresh full-heartbeat confirmations. The Ambient publisher was
left running for Ben's spoken visual test, which he immediately confirmed works
really well. Pause/app-exit fallback remains to be observed.

## 2026-08-25 -- Ben + Codex -- CoreS3 USB loss identified as intentional standalone use

Ben clarified that the CoreS3 USB disappearance during the all-fixture Audio run
was intentional: he unplugged it to operate the new app from the CoreS3 battery
and touchscreen. This is the designed no-laptop workflow, not evidence of a
cable, host, or CoreS3 power fault. The earlier 444-frame/zero-failure evidence
remains valid only for the USB-observed Aux portion; unplugging ended host
visibility, not necessarily the on-device publisher.

For spoken local testing, select `AMBIENT MIC`, leave two seconds of ordinary
quiet for calibration, then speak. Every fixture fresh in the five-second live
window is addressed; Ben currently reports 14 live, while sleeping fixtures are
expected to remain absent until they wake. Pause or leave Audio to release the
publisher when finished.

## 2026-08-25 -- Ben + Codex -- All-fixture audio run interrupted by CoreS3 USB loss

Started the installed CoreS3 Audio publisher without OTA, fixture flashing, or
persistent fleet mutation. The exact `4D5DB0` Bridge OS entered Aux publishing,
completed calibration, and reached 444 accepted direct-frame packets in about
60 seconds with zero send failures and zero audio read failures. Two unique
duty-cycled fixtures became fresh during that short window; the intended run
was scheduled for 390 seconds so it would span the full 300-second sleeper
cadence. Aux had RMS 0 throughout, so it was successfully publishing a dark
level but had no connected/live analog signal.

At about 60 seconds the serial operation was canceled and CoreS3 disappeared
from USB/PnP entirely. The test harness attempted its Pause cleanup, but the
device was already unreachable, so paused state could not be confirmed. The
USB identity remained absent through another 60-second watch. If the CoreS3
lost power, fixture direct mode expired through the normal roughly three-second
stale fallback. If it remained alive on its internal battery, the screen is the
authority and an operator must tap Pause or power it off before reconnecting.
The active Ambient handoff and full-cadence fleet coverage were not reached and
must be rerun after the USB/power interruption is understood.

## 2026-08-25 -- Ben + Codex -- CoreS3 Ambient/Aux runtime input selector

Added a third Audio-app control so one Module Audio Bridge OS artifact can cycle
between `AMBIENT MIC` (the CoreS3 dual microphones) and `AUX INPUT` (Module
Audio LINE/MIC) without reflashing. The footer is now Start/Pause, Input, and
Look; optional USB `N` invokes the same Input action for headless diagnostics.
A handoff sends an explicit zero frame when publishing, pauses sampling, starts
the requested hardware path, resets envelope/noise calibration, and resumes
only if the stream was previously active. Failure preserves the prior working
input. Module LEDs are green for Aux and dark blue for Ambient.

Hardware exposed a useful reset edge case: one intermediate build saw Module
Audio late after a USB reset and correctly fell back to the built-in mics, but
had treated Aux as permanently unavailable. Separated build capability from
current readiness. A missing module controller may now be retried later from
Input; a detected I2S/codec initialization failure is not repeatedly retried
because the upstream library exposes no teardown and instead requires a full
power cycle. ADR 0057 records this lifecycle.

Built and USB-flashed exact CoreS3 `4D5DB0` (`80:45:6B:4D:5D:B0`) with the
channel-11 Module Audio r6 binary: 1,168,208 bytes, SHA-256
`01F99A5167C5DE01C6FC75BD5781F4F8AB337D2095B3870122858909B206EE12`.
The flasher independently confirmed the full MAC and verified every region.
Fresh boot reported Aux ready with Audio paused. The common Input action then
passed Aux -> Ambient -> Aux on real hardware; every state remained ready with
`active=0`, `frames=0`, and `readfail=0`. No direct frame or fixture-control
command was sent. Native audio/app tests pass 31 checks. Physical touch and
active-publisher behavior remain for named-canary acceptance.

## 2026-08-25 -- Ben + Codex -- Repeated Color Virus strains promoted to 99 fixtures

Color Virus now treats a re-armed local ToF gesture as a new ordered strain,
even when the source is already infected. The source chooses a visibly different
random hue, increments the newest visible strain sequence, and propagates that
strain through already-infected neighbors. Serial-newer generations win; equal
generations converge on the larger hue bucket; older/equal-losing strains cannot
bounce back. Every accepted replacement remains one infection edge for the
selected light/knock output. Epidemic recovery behavior is unchanged. Native
fixture tests now pass 147 behavior checks, and the full fixture, T-Deck, and
host dashboard suites pass. ADR 0056 records the ordering contract.

Committed the source as `c46e21f35073aea6ee2724a306d95dc603139afd` and
built immutable production revision `fx-260826-024e508-p` from a detached clean
worktree. The 1,199,792-byte binary has SHA-256
`c5bc1eb08ebff3df5cd2edd1c4932f3f4096678056caa0287876b4db2c289168`;
its recipe SHA-256 is
`024e508eefae86876e18325e4e7b45aae4e8d09dfacd8227de42bf91dd3f5523`.

The live T-Deck census observed 102 fixture IDs. Protected Magic Wand `F40344`
was intentionally excluded because its dedicated image must not be overwritten;
the other 101 were named explicitly. Ninety-eight accepted the first OTA batch,
and `F2B7DC` accepted the same immutable image on its next maintenance wake. All
99 uploaded fixtures supplied fresh exact-revision evidence beyond the 25-second
pending-verify gate. Five PROTECT sleepers (`9F266C`, `9F2714`, `9F2724`,
`F2BCF4`, and `F2BDD4`) required a later deep-sleep wake because the first
420-second verifier window was shorter than their 900-second sleep cadence; each
then reported the exact new revision with reset reason `deepsleep`, so no rollback
occurred.

`9E5AD4` (about 3.02 V, PROTECT) and the already documented downlink exception
`9F26D8` remained responsive on their known-good old images but did not expose an
identity-matching maintenance endpoint during the original batch or one bounded
retry. No upload was attempted on either. Do not loop radio retries: recover
`9E5AD4` on supervised power and bench-diagnose `9F26D8`'s targeted-maintenance
receive path. The exact scope and evidence paths are recorded in
`ops/bench/data/ca/20260826-contagion-fleet-ota-callout.txt`.

## 2026-08-25 -- Ben + Codex -- CoreS3 Bridge OS flashed and passively verified

Identified the USB-attached CoreS3 by its own serial status as short ID
`4D5DB0`, then independently confirmed full MAC `80:45:6B:4D:5D:B0` during
flash. Declared that exact device on observed `COM43` as the sole flash target;
the COM number remains incidental. The old image was an active EMBER audio
publisher, so the first Bridge OS flash also returned the unit to the new safe
launcher posture with Audio paused.

The first Module Audio build booted safely with `active=0` and `frames=0`, but
reported its input as `FAILED`. A temporary built-in-mic flash restored a ready
input while the failure was diagnosed. M5Unified only installs the CoreS3
internal-mic pin/callback configuration when `internal_mic` is enabled; the
Module Audio build had disabled it, which also made the documented missing-
module fallback impossible. Changed the ordinary Bridge OS initialization to
configure, but not start, the internal microphones. Module Audio still gets
first choice in `setupAudioInput()` and the built-in input remains a real
fallback.

Rebuilt and installed the corrected Module Audio image. The exact binary is
1,167,184 bytes with SHA-256
`280407DC5EEA741FCD2B7528BF32A4C71776CE6CF70428184E99F16B0C1EB464`.
The flasher verified every written region, and a fresh boot reported
`fw=cores3-os-0.1.0-dev`, channel 11, `MODULE TRS ready=1`, `active=0`,
`frames=0`, no send failures, and no receive-queue drops. Passive ESP-NOW peer
traffic and read-only USB `nb-*` status queries also passed. No direct frame or
fixture-control command was sent. Physical launcher/touch checks and deliberate
RODE/canary-fixture publishing remain open so an unattended live fleet is not
changed merely to prove the UI.

## 2026-08-25 -- Ben + Codex -- PUCA bridge scale/safety build and document reconciliation

Onboarded through the canonical repository docs and reviewed the PUCA ADR,
hardware record, audio-ingest research, firmware target, and the recovered
2026-08-20/21 Claude session findings. Reconciled stale statements that still
said no PUCA firmware existed or that it had never touched hardware: the earlier
mono build did prove WM8978 initialization and onboard-MEMS audio, while the
current build remains only partially hardware-verified.

Promoted the PUCA source from the original 18-fixture proof-of-concept ceiling
to a full-census development publisher. The sorted live fixture list now chunks
across the canonical 18-entry `NB_DIRECT_FRAME` limit; 130 live fixtures plan as
eight packets per roughly 10 Hz update, with stable global color slots. Added
stereo L/R capture and in-place averaging so either input channel works, raw
peak/clipping counters for gain staging, codec/I2S readiness gates that refuse
unsafe publishing, and a startup paw debouncer that baselines the settled input
instead of manufacturing a mode-change edge. The packet contract remains the
single shared `firmware/fixture/src/core/packet.h`; no new wire type was added.

Added a platform-independent PUCA core test target covering full-fleet chunk
planning, stereo mixing, clipping statistics, and touch startup/debounce. All 28
PUCA checks pass; the shared CoreS3 audio/app suite also passes 28 checks. The
fresh `esp32:esp32:pico32` 3.3.7 compile check passes at 962,576 sketch bytes and
60,496 bytes of global data. Binary `puca-review-20260826-r4/puca_bridge.ino.bin`
is 962,720 bytes with SHA-256
`1d7d6df1d32bc253ff9451a4dca32bae2aaca2a2795410934a18cb172874dc19`.
This is explicitly a compile-check binary, not an ADR 0040 promoted artifact.

Updated the root/firmware indexes, hardware record, PUCA README, research note,
field manual, and TODO with the honest partial-proof boundary and recovered
Eurorack v0.5 wiring/power/input findings. No firmware was flashed: the only
enumerated USB devices were native Espressif USB/JTAG devices belonging to the
T-Deck/CoreS3-class bench hardware, not the Original Edition ESP32-PICO-D4 PUCA.
Next is an explicit powered-Pod20 acceptance pass on the exact PUCA: identity,
current stereo build, controls, RODE line path and clipping, one fixture plus
stale fallback, more than 18 fixtures, mixed HEX/RGBW, closed-case PDR, and soak.

## 2026-08-25 -- Ben + Codex -- Contagion, perimeter palm seed, and old-fleet knock fanout

Kept Greenberg-Hastings wildfire in CA Studio and added a separate Contagion
fixture program/app for Color Virus and Epidemic (ADR 0055). Program 5 carries
susceptible/infected/immune state plus propagating hue, accepts only fresh
program-5 neighbors, and supports light, native knock, or both output. Color
Virus persists; Epidemic recovers and can be reinfected. Manual seed is exact-
target, repeated RF copies are de-duplicated, and one infected edge requests at
most one 40 ms strike through the existing local gates. Production perimeter
nodes are sensor/relays only; only downlight-class nodes request mallet pulses.

Expanded VL53L5CX USB telemetry to report fresh reads, raw-target/valid/near
zone counts, and 16 nearest valid distances. A synchronized cloudy-evening test
on exact perimeter canary `Magmar [F2BDFC]` separated a stable clear 0/16 from a
flat palm held 5-10 cm above the sensor at sustained 15-16/16 valid near zones,
roughly 60-93 mm. Touching/complete cover can settle to the same no-return state
as clear space, so the reliable contract is a one-shot approach/palm gesture,
not continuous covered-state detection. The gate requires >=4 near zones for
two fresh reports and four clear reports to re-arm. Direct sun and final
installed geometry remain unqualified.

Added an explicit `legacy fleet roll` Contagion output for the deployed fleet
that does not yet understand program 5. One selected source infection edge is
de-duplicated on the T-Deck and starts the already-proven addressed 40 ms strike
roll, filtered to fresh downlights. It expires with the 10-minute lease and Stop
disables it; it is deliberately distinct from native knocks to prevent a future
updated fleet from receiving duplicate intent.

The full fixture native suite passed (including 131 behavior, 312 presence, and
37 VL53 grid checks), as did the full T-Deck native suite including source-edge
deduplication and downlight-only planning. Exact fixture `68:EE:8F:F2:BD:FC`
was USB-flashed with a 1,199,472-byte development binary, SHA-256
`45679a01b2c5e03d6fb53534f724e7951f2038f558fac8ab112de1d19305b347`.
Exact T-Deck `44:1B:F6:8E:B5:08` was USB-flashed with a 1,565,840-byte
development binary, SHA-256
`f95218ac6526a48b841afc4c92d342007c123ae4d9bea39f13ad8424069ae298`.
Post-flash checks showed both on channel 11, Magmar as a healthy perimeter
fixture with zero strikes, and the T-Deck receiving the live fleet. A final
120-second watch ended before the operator confirmed Start: Magmar remained in
program 4, the adapter never armed, and no strike command was sent. Complete
the sensor-edge-to-one-roll/held-dedup/Stop check next; no mass OTA was done.

Later operator evidence confirmed the first live Contagion/ToF light edge:
hovering a hand turned Magmar blue. It did not visibly spread because Magmar was
the only nearby program-5 fixture; the deployed old-image fleet rejects that
program. A second hover did not change the color because Color Virus deliberately
keeps an infected node infected until Stop/restart/expiry. Whether a new local
gesture should introduce a new color strain remains an artistic decision.

Reworked the Contagion source picker after the full census made scrolling to
Magmar laborious. Known fixtures now sort alphabetically by callsign, unknowns
follow by short ID, and a one-line physical-keyboard filter narrows the synced
dropdown by callsign substring or short ID (for example `mag` or `F2BD`). Native
sorting/filter tests and the complete T-Deck suite passed. Exact T-Deck
`44:1B:F6:8E:B5:08` was USB-flashed with the 1,566,944-byte development binary,
SHA-256
`4da7f7034d288f5e4a5dcfc383343323aaba77d70292a453c82455f5d32cfcbc`.
Post-flash serial evidence showed `8EB508` healthy on channel 11 with live mesh
traffic and Magmar still visible over RF. The legacy knock roll remains
unfired; the operator deferred the solar-surplus test after daylight faded.

## 2026-08-25 -- Ben + Codex -- Knocker compatibility diagnosis and named picker

Ben reported that Knocker's original targeted roll still produced physical
strikes while the newer fleet choices were silent. The deployed fixture image
predates `NB_EVENT_SOLENOID_STRIKE`: targeted roll uses the older addressed
`NB_TARGET_SOLENOID` path, while broadcast-now and sync +1.0 s use the new event
receiver that old images intentionally ignore. The same artifact boundary
applies to the newer CA knock output. Source review found no contradictory
builder/scheduler path; an isolated explicitly armed fixture running the
matching image is still the required hardware proof.

Changed the Knocker single-fixture dropdown to show the permanent registry
callsign plus authoritative short MAC, with a short-MAC fallback for an
unexpected fresh fixture. Enlarged the bounded option buffer for the full
192-entry census. The complete T-Deck native suite passed, including its
144-entry generated-registry check; the complete fixture native suite also
passed, including the strike-event scheduler. The cached embedded T-Deck build
passed at 1,558,144 binary bytes, SHA-256
`52265b2ac4ee871ad4c8f893bb7d02714b32d2795e669b43c1c8cc837b68b00d`.
No device was flashed, no mesh command was sent, and no fixture state changed.

## 2026-08-25 -- Ben + Codex -- Optional ToF-seeded light/knock CA

Added ADR 0053 and an off-by-default `ToF seed` control to Bridge OS Wildfire
CA. GH params byte 6 now lets either light or knock CA accept one debounced local
TMF presence edge. Sensor-verified downlights can originate the excitation;
every other class still relays normal neighbor CA state. The fixture consumes a
TMF report once through the existing hardened ADR 0044 gate and routes its
rising edge to either CA or the listener color wipe, never both. The separate
wipe remains suppressed during a program lease, and all light/power/solenoid
safety gates retain authority.

Corrected `spark /256 = 0`: a leased CA now truly has no spontaneous sparks, so
a presence/neighbor-only run is possible. Internal autonomous CA explicitly
requests its historical default `2/256` rate, so field and selected commission
fallback behavior do not silently change.

Recorded why the August rig felt less responsive than earlier bench demos. The
rig path intentionally added 90-report per-zone learning, a 300 mm closer delta,
three consecutive confident hits, and four clear reports; the earlier presence
dashboard and LED Studio reactions evaluated fresh anomalies much more
immediately. Close ground and adjacent optical emitters remain plausible noise
sources, but the retained run has no per-zone trace that proves crosstalk. The
TODO now requires an isolated-vs-group A/B and treats the broom pass as a stress
case rather than the human-presence acceptance test.

The complete fixture native suite passed, including 106 behavior checks and 299
presence-gate checks. The complete T-Deck native suite passed. Commission/basic-
listener fixture development build passed at 1,196,768 binary bytes, SHA-256
`5a4c71ebc663899ee45f59b6f89094c3135231aa15366f31d6fa924863916c27`.
The T-Deck development build passed at 1,555,776 binary bytes, SHA-256
`18f7ab16b43e22ef429d003116c47cbaa5edba320c24f18dd2380812e43e7633`.
No device was flashed, no mesh command was sent, and no actuator was exercised.
Named-canary optical/CA validation remains open in `TODO.md`.

## 2026-08-25 -- Ben + Codex -- CoreS3 wireless two-app Bridge OS

Onboarded from the repository and replaced the ordinary CoreS3 normal-vs-audio
artifact split with one touch-first, battery-capable Bridge OS image (ADR 0054).
It boots to a launcher with two explicit apps. Listener is a read-only mobile
adaptation of the bench dashboard: 24 stable short-ID-sorted fixtures per page,
class-shaped/raw-VBAT/freshness/reported-color glyphs, and touch-through radio,
power, class/program, sensor/recovery, LED-output, and firmware detail. Audio
retains the proven Module Audio/RODE and built-in-mic inputs but now has local
start/pause and look controls. Entering it starts a fresh calibration; pausing or
leaving it sends zero and stops the publisher so an invisible background stream
cannot fight another artistic controller. USB `nb-*` dashboard/logger output and
the existing serial grammar remain available but are not app dependencies.

Removed the old 18-fixture audio ceiling without changing the wire contract.
Each 10 Hz wave now walks the complete sorted fresh fixture census in canonical
18-entry `NB_DIRECT_FRAME` chunks, preserving stable CLASSIC color slots across
chunks. Cambium remains a separate binary-only artifact and has no launcher.
The old `--audio` switch is a compatibility alias; `--audio-module` now selects
only the external hardware layer for the same two-app image.

The expanded CoreS3 native suite passes 28 checks. Fresh compile checks pass for
all three supported targets: built-in Bridge OS is 1,133,168 binary bytes,
SHA-256 `e1a59af9fd992ed1459dfc6c639bdf9dc47047296585a6f0cf4c4970ab95cfd4`;
Module Audio Bridge OS is 1,167,184 bytes, SHA-256
`299698e46c3691915ae69759e11383b85adc20d818d72d7ca99e52769cb623f5`;
Cambium is 1,087,888 bytes, SHA-256
`ddabcd9e39cec1f027259ccc4fb09a0317b4ebd6b8affe172a52c38a76170d10`.
These are compile-check outputs, not flashed deployment artifacts. No device was
flashed, no mesh command or direct frame was sent, and no fixture state changed.
Exact CoreS3 `E39F1C` launcher/listener/audio, mixed-fleet, USB compatibility,
and unplugged-runtime validation remains open in `TODO.md`.

## 2026-08-25 -- Ben + Codex -- Selectable commission default and RGB-white fix

Added ADR 0052 and a runtime/NVS-stable commission no-command selector with
three local fallbacks: the existing class-aware ready beacon, autonomous
light-only GH wildfire, and strict rails-off dark. The missing/invalid value
remains listener, active direct/program leases retain authority until release
or expiry, and field profile ignores the setting. Type 29 stays reserved for
the queued sensor report; new packet type 30 carries an exact-target
`NB_COMMISSION_DEFAULT` setting with an optional persist flag. Fixtures reject
the all-zero target, suppress repeated RF copies before an NVS write, and expose
the selected value in USB telemetry.

Bridge OS now has a fourteenth launcher tile, Default. It defaults to one named
fresh fixture, offers RAM-only or persisted lifetime, and confirms every
change. `ALL: targeted fresh` is a deterministic short-ID walk, not a broadcast
NVS mutation. Autonomous commission CA is deliberately light-only; knock CA
remains an explicit bounded CA Studio lease. Strict dark returns false from the
fixture render seam so it cuts the physical LED rail rather than holding an
all-zero powered frame.

Fixed LED Studio white semantics. Its W-only swatch is now planned per fresh
fixture class: known downlights retain dedicated W, while perimeter, uplight,
chandelier, and not-yet-classified fixtures receive equal RGB with W clear.
This makes the deployed three-channel RGB uplights illuminate white and applies
the same safe rule to future RGB chandelier fixtures without sacrificing clean
downlight white.

The fixture native suite passed, including the 57-check packet-layout test. The
T-Deck native suite passed, including class-aware white and deterministic target
planning. Commission/basic-listener fixture development build passed at
1,196,688 binary bytes, SHA-256
`fea5218d6c8eec0b004338a3412375653750079b3b62b4b12625076079d5e329`.
The T-Deck development build passed at 1,555,504 binary bytes, SHA-256
`d050760e69733b9527036527c0191b6dd9231ab5fef3f6bd31e4d1a5c5c262e3`.
No device was flashed, no mesh command was sent, and no fixture NVS was changed.
Named mixed-hardware/default-persistence validation remains open in `TODO.md`.

## 2026-08-25 -- Ben + Codex -- Daytime knock wildfire added to Bridge OS CA

Onboarded from the repository and changed CA Studio from a generic fixture-
program picker into a wildfire-specific operator surface with `lights` and
`knocks` outputs. Removed `idle`, `bridge`, `direct`, and `dark` from that
picker: those are internal fallback/show-frame/direct-frame/dark roles, not CA
algorithms, and their real controls already live elsewhere in Bridge OS.

GH params byte 5 now selects sound-only knock output. A quiescent-to-excited CA
edge emits one 40 ms `ProgramOutputs` strike request, while the LED frame is
suppressed and the rail remains off. ESP32 glue routes the request through the
same lifecycle/power permission and solenoid arm/rest/maintenance/load-marker/
timer/failsafe mechanisms as every other strike. Production day-state CA now
consumes and publishes neighbor state, including edge bursts, so the wildfire
can actually propagate while daytime solar surplus permits knocks. Fixtures
without solarnoids remain graph participants but cannot actuate.

The algorithm is deliberately not ToF-dependent: spontaneous sparks and fresh
excited neighbors seed it. The separate ToF presence-wave path remains
opportunistic and is suppressed during a CA lease, avoiding any dependency on
unqualified direct-sun optical performance. Same-program parameter leases now
reapply without the old release/re-lease light blip, and release/expiry resets a
leased GH knock configuration to autonomous light defaults even though both use
program ID 1.

The complete fixture native suite passed. Development builds passed for the
fixture (1,195,152-byte binary, SHA-256
`4f1489d5ad87834f9992b3d502854ecd446ffc239a7dcc25aafffbd04345b364`)
and T-Deck (1,553,008-byte binary, SHA-256
`9309bf915b3b6f556610a0938bcd0d3f3272bb99ae0288ef6f87d06fbb986382`).
The T-Deck native wrapper stopped before its C++ tests because the pre-existing
Panther registry edit has not regenerated the embedded Health-roster hash; the
CA/T-Deck embedded compile itself passed. No device was flashed and no mesh
command or physical strike was sent. Named daylight cohort validation remains
open.

## 2026-08-25 -- Ben + Codex -- Panther exact USB recovery and 15 Ah recommission

Declared one firmware writer and targeted only Panther `9E5A84`
(`D8:85:AC:9E:5A:84`) on COM52. Reused accepted immutable pack-out artifact
`fx-260818-f80f315-b` from clean commit
`29ebe2b5949bc52b03690986ea8ecbdcbecf4a65`: 1,177,264 app bytes, SHA-256
`0f1119c6ba80f2280db2c04f478a59b6be0c407edf6c95c62248f89af90ad638`.
The final preflight had FULL guard, 3.328 V replacement LFP at +558 mA,
qualified 4.617 V / 672 mA USB input, no BQ fault, and healthy
MSA311/TMF8820/BMP581.

The first guarded commissioning attempt stopped before writing because this
retained 2026-08-18 artifact directory had preserved only the exact app binary,
manifest, identity header, build options, and checksum; Arduino's uploader also
expected missing standard bootloader/partition companion files. Its flash-ID
probe reset the old image but changed no flash bytes. Panther returned healthy
and charging. Rather than rebuild the artifact, modify its immutable directory,
erase NVS, or substitute newer companion bytes, wrote only the manifest-verified
app to its canonical `app0` address `0x10000`. Panther's already validated
bootloader, A/B partition table, OTA data, and NVS were preserved. Esptool
verified the written data hash and hard-reset the board.

Fresh serial telemetry reported exact `fx-260818-f80f315-b`, app0, pending false,
FULL guard, the ADR 0051 reset semantics, 300 mA precharge readback, universal
solenoid policy armed with the gate off, channel 11, commission profile, and all
three downlight sensors healthy. An exact-target capacity write corrected stale
6,000 mAh NVS to 15,000 mAh and produced a clean software reboot. The
identity-matched `Party In The Woods` maintenance endpoint came up at
`192.168.1.70`; resume returned to channel-11 ESP-NOW with zero comms-init
failures. Final fresh evidence at 44.1 seconds uptime showed 3.378 V / +721.6 mA
battery, 4.625 V / 662 mA good input, no BQ fault, FULL power, TMF 246 reads / 0
errors, healthy MSA/BMP, and no pending OTA verify. The registry is restored to
`commissioned` with the exact artifact identity. The updater's pre-write failure
and successful app-only recovery remain as separate append-only JSONL evidence.

## 2026-08-25 -- Ben + Codex -- Panther replacement battery/reset restored charge

Ben confirmed that Panther `9E5A84` had already received its replacement
battery and then pressed RESET once. Read-only COM52 telemetry after that reset
showed normal application firmware, not ROM download mode. The boot cleared BQ
EN_HIZ (`REG0x16 0xB0 -> 0xA0`): input became good at 4.617-4.625 V and
664-670 mA, while the replacement LFP charged at roughly +537 to +736 mA with
no BQ fault. All expected outer-downlight sensors initialized: MSA311,
TMF8820, and BMP581 were present; TMF reached 757 reads with zero errors.

The old `fx-260816-otafix1-b` boot guard initially counted the physical reset as
another interruption and moved FULL -> DIM (`guard_stage=2`, `power_tier=1`).
With qualified USB charge it recovered and persisted FULL by about 65 seconds;
at 101.7 seconds it remained FULL, battery 3.368 V / +600 mA, input 4.625 V /
670 mA, LED rail on, and OTA pending false. Do not press RESET again while this
old image remains installed: repeated unexpected resets are the known path
toward durable PROTECT. Bootloader rescue is not indicated. Remaining service
is an identity-gated USB firmware/configuration pass because the fixture still
reports the old bench image and stale 6,000 mAh capacity instead of its 15,000
mAh downlight configuration. No command, reset, flash, maintenance entry, NVS
write, or mesh mutation was sent by Codex.

## 2026-08-25 -- Ben + Codex -- Panther USB diagnosis; app alive, input held HIZ

Onboarded from the repository and read-only diagnosed the single attached
Espressif native-USB device on `COM52` as registered outer-ring downlight Panther
`9E5A84` (`D8:85:AC:9E:5A:84`). The application answered repeated no-reset USB
telemetry, so the board is not stranded in the ROM bootloader. It remained on
known old `fx-260816-otafix1-b`, `app0`, pending-verify false, channel 11, and
commission profile. No reset, flash, maintenance entry, NVS write, LED command,
or mesh mutation was sent.

Six samples across about 18 seconds were stable: VBAT 3.179-3.181 V, 11 percent
advisory SOC, battery current -142.1 to -149.2 mA, USB/input sense 4.994-4.998 V,
zero input current, `supply_good=false`, no BQ fault, and BQ REG0x16 `0xB0`
(EN_HIZ set). The fixture therefore was not charging despite the attached USB.
Telemetry also showed the stale 6,000 mAh NVS capacity instead of the registered
15,000 mAh downlight cell, no MSA311/TMF8820/BMP581 presence, LED rail off, and
`guard_stage=FULL`; this is not a durable-PROTECT park. Repo history already
places `9E5A84` in the playa-unpack battery-swap cohort and records its known
targeted-maintenance/downlink failure. Recovery order is therefore physical
battery/input-path service with proven positive charge first, then an
identity-gated USB install of one inspected immutable fixture artifact and the
full downlight/capacity/sensor/mesh acceptance gate. Do not spend a bootloader
or erase-NVS operation on the present evidence.

## 2026-08-25 -- Ben + Codex -- Bridge OS flashed; schedule posture clarified

Built and USB-flashed the integrated Bridge OS development image to T-Deck
`8EB508` on `COM152`. The 1,552,672-byte binary has SHA-256
`26200e59595b32d85d5edd2935e6da37dba2913b96f5433f5fcf0402a33d41dc`;
esptool verified every written segment. Post-reset serial evidence showed the
bridge back on channel 11, receiving fleet traffic, and publishing a valid
direct-GPS `NB_TIME_QUALITY` observation.

Live full heartbeats still report `profile=0`, `life=4`, `program=4`: the fleet
is in commission/listener posture. Visible low-level light is therefore the
commission ready beacon, not autonomous dusk-to-dawn behavior. Schedule
Auto/Day Dark/Night Show only selects the field lifecycle baseline; it does not
promote commission fixtures to the persistent field profile. No fixture
profile or other fixture NVS value was changed in this session.

## 2026-08-24 -- Codex -- All-work integration verification

Consolidated the laptop's remaining useful local branches for the requested
`origin/main` push. Patterns and RF branches were already patch-equivalent to
the integrated Bridge OS. Recovered the false-PROTECT safety work as ADR 0051,
including its adversarial-audit fixes; restored the retained fleet OTA evidence
and 144-entry Health roster; preserved the superseded fleet-sleep field record
without reopening its retired serial commands; and included the concurrent RF
contrast / Claude census paging commit.

The complete fixture native suite passes. A sequential compile-only fixture
development build also passes at 1,194,896 bytes, SHA-256
`2c076b0de0d9783bf4d734a5324aecd0010968dfe40f40e219004885e0a35c3c`.
The complete T-Deck native suite passes with 144 registry fixtures, and its
sequential compile-only development build passes at 1,552,672 bytes, SHA-256
`26200e59595b32d85d5edd2935e6da37dba2913b96f5433f5fcf0402a33d41dc`.
No binary was flashed; no fixture command, OTA, maintenance request, strike,
sleep request, or network mutation was sent.

## 2026-08-24 -- Codex -- Preserved superseded fleet-sleep field record

The all-work audit also found local branch `codex/tdeck-transport-sleep`
(`97d839e`, documentation `937164a`). On 2026-08-22 it built and flashed
T-Deck `8EB508` artifact `tdeck-260822-97d839e-sleep`, 1,498,752 bytes,
SHA-256
`f808acdeeabc1ca1ab4f5955df6f0a196a524cbcc1b81085ed918f1ad565282b`.
The bridge confirmed an eight-copy 20-hour `NB_TRANSPORT_SLEEP` broadcast and
then a 65,535-second legacy fallback for older fixtures. The host suspended
during the planned census check, so transmission is bridge-confirmed but final
fleet silence was not observed from the host.

Did not transplant that branch's `Q<hours>` / `S<seconds>` serial commands into
current source: ADR 0048 deliberately superseded that control surface with the
on-device confirmation rail and keeps fleet sleep out of the serial CLI and
Claude tools. Current source also already uses the shared `core/version.h`, so
the old branch's split-version TODO is resolved. This entry preserves its field
evidence without reopening a retired, less-guarded operator path.

## 2026-08-24 -- Codex -- Recovered retained fleet-wave evidence

Audited every local worktree before the all-work push and found the unmerged
2026-08-18 `fx-260818-f80f315-b` OTA reconciliation on commit `04481c8`.
Merged its 84 verified wave rows into the current registry field-by-field,
without overwriting the later 2026-08-25 observations. This restores exact
revision, SHA-256, OTA status, and verification times for 76 existing rows and
adds three real fixtures that had fallen out of the canonical CSV despite
appearing throughout the saved RSSI evidence: `F2BDFC`, `F402A4`, and `F40348`.

The three restored fixtures use the `68:EE:8F` batch OUI inferred from their
neighboring PowerFeather IDs plus their observed six-digit short IDs; physical
confirmation remains queued. Assigned the next reserved callsigns
`Magmar [F2BDFC]`, `Magneto [F402A4]`, and `Marill [F40348]`. The generated
Bridge Health source roster now contains 144 fixtures; the last documented
field flash still embeds 141 and will treat these three as foreign-live until
the next named T-Deck build/flash. No fixture, bridge, or network state changed.

## 2026-08-24 -- Ben + Codex -- RF contrast and complete Claude census paging

Made RF Diagnostics independent of the inherited LVGL theme: the screen now
uses explicit white primary text, pale-green strongest-link text, amber
weakest-link text, and white button labels against its dark background.

Removed Claude's stale 64-row census prefilter. The tool now examines all 192
tracked census slots while retaining the intentional 24-row result budget for
the 4 KB tool-result buffer. Responses report matched/returned counts and
provide `next_offset` for explicit pagination. Updated the tool schema, app
roadmap, and field manual to match. The complete T-Deck native suite and an
embedded development build pass; the binary is 1,552,448 bytes with SHA-256
`1005934c0338e4b4ad74aac76670b8fd5fd46935b64c98f5f8cfe89a1acc8952`.
No device was flashed because this combined T-Deck image also exposes the new
Knocker multicast/scheduled controls before matching fixture support has been
canary-deployed.

## 2026-08-24 -- Ben + Codex -- Selectable multicast and synchronized Knocker

Extended the existing Knocker picker without replacing its deterministic
targeted roll. It now offers `ALL: broadcast now` for asynchronous one-to-many
fire and `ALL: sync +1.0s` for a short-future shared deadline. Both actions
retain the confirmation rail; single-device addressed strikes remain available.

Reused the canonical packed `NbEvent` rather than adding another protocol
header or changing layout. New kind `NB_EVENT_SOLENOID_STRIKE` carries a 32-bit
dedupe ID, bounded pulse, and existing `fire_in_ms`. The T-Deck repeats one
logical event six times for RF reliability and decrements the remaining delay
on later copies so all copies point at the original bridge deadline. Fixtures
timestamp receipt in the ESP-NOW callback, remember recent IDs, arm at most one
pending event, clamp 5-300 ms, cap initial scheduling at five seconds, and
refuse a strike more than 250 ms late. Lifecycle, power, arm, rest, maintenance,
and solenoid mechanism gates are evaluated at actual fire time. Older firmware
ignores the new event kind.

The complete fixture and T-Deck native suites pass, including new pure event
builder/scheduler tests. Sequential development builds also pass: fixture
1,193,376 bytes, SHA-256
`aa1d639046b2e7d9431b1eae94f33d90a77995e85581f34010b53addda942ebd`;
T-Deck 1,552,000 bytes, SHA-256
`2b6e968009341e73a62cd23ccdb651682d276b3019debb0c4f7b4509a19725c2`.
No device was flashed and no strike, mesh command, maintenance request, or OTA
was sent. Matching fixture support must be isolated and validated before this
T-Deck UI is exposed on the live bench.

## 2026-08-24 -- Ben + Codex -- Two RTC anchors commissioned against live GPS

Added read-only Bridge time-quality diagnostics plus an exact-target RTC
commissioning path. T-Deck `8EB508` now reports received GPS/RTC quality,
validity, age, uncertainty, and live GPS disagreement to the host dashboard.
The final diagnostic development binary is 1,551,088 bytes with SHA-256
`326023943434559bba5abf4de9a4aabb5ad269c2966d9a6d9d46065b20455711`.
The complete T-Deck native suite and 22 host dashboard/OTA/RTC policy tests
pass. Commit `055a58c` contains the diagnostics.

Fixture commit `7963816` adds native-tested DS3231 calendar conversion, a
guarded maintenance-only `/rtc` endpoint, explicit UTC write/readback, RTC
validity telemetry, and `ops/bench/rtc_commission.py`. The endpoint requires
the exact six-digit fixture ID, a bounded 10-digit UTC value, and the literal
`SET_RTC_UTC`; the host tool additionally requires active maintenance, exact
firmware, DS3231 presence, safe power, and a fresh valid T-Deck GPS source.
The fixture native suite passes, including 32 RTC calendar checks.

Built immutable field/listener canary `fx-260825-9ef9d64-b` once from clean
commit `7963816bf8576f193a4be15bb62f9fb51aa8c219`. Its canonical recipe SHA-256
is `9ef9d64e50d3e3b006ad3ab5f677534860a3e25c62f072cf5a2c441ff77532c2`;
the 1,192,832-byte binary SHA-256 is
`2fc29583a526a995e8bf35c4d4f2b5db3e32c45052a094aa44611baa1f27f9b9`.
The manifest records channel 11, field profile, basic-listener posture, LFP,
and 300 mA precharge.

With one declared operator, OTA-updated only Navi `9F0E7C`, then only Zorua
`9F26C0`. Both used their installed LFPs, identity-matched at
`192.168.1.128` and `192.168.1.78`, rejoined on the exact expected revision,
and survived the 25-second pending-verify gate with software-reset evidence,
downlight class, and recovery state 0. Both DS3231s were present but correctly
reported invalid/OSF before commissioning. Exact-target writes read back valid
within 2 seconds of GPS for Navi and 1 second for Zorua. After returning to
mesh service, three independent 10-second RTC broadcasts from each were valid
and date-valid: Navi differed from live GPS by 659/639/624 ms, and Zorua by
662/669/656 ms. A final simultaneous snapshot remained fresh at 655 ms and
644 ms disagreement respectively.

Zorua's first PowerShell `/resume` request returned HTTP 200 but kept the
connection/session in maintenance; this initially looked like missing RTC
traffic. Reissuing `/resume` with an explicit connection close restored mesh
service, after which all repeat checks passed. Treat closed-session resume plus
fresh heartbeat as the operator acceptance, not the HTTP response alone. No
other fixture was flashed or commissioned. The commissioning tool now forces
that connection close and requires three new exact-revision, valid/date-valid,
GPS-aligned RTC mesh observations before it reports completion. Thor/Magic Wand
`F40344` was never targeted. Remaining qualification is compressed dusk/dawn,
a real overnight, RTC holdover/drift/backup energy, and the unresolved anchor
inventory.

## 2026-08-24 -- Ben + Codex -- Navi RTC-anchor schedule canary OTA

Declared one OTA operator and selected only RTC anchor Navi `9F0E7C`. Reused
the immutable `fx-260825-d374034-b` fixture artifact from clean source commit
`c544bf6bdfee15e7b1fcf25db350e8bee6b1bde7`; its 1,190,240-byte binary again
matched manifest SHA-256
`b0722b5fe965f16c0b0d95f7fe3b8b905799702343fd0ad2c5f9fcbeaadf5009`.
The guarded preflight saw a fresh Navi heartbeat on
`fx-260818-f80f315-b`, 3.307 V battery, and no external supply. The installed
LFP satisfied the OTA ride-through policy.

The shared-WiFi maintenance path identity-matched only Navi at
`192.168.1.128`, uploaded the exact retained binary, and received its reboot
acknowledgement in 10.54 seconds. Navi rejoined on the exact expected revision
and remained fresh beyond the 25-second pending-verify gate: the acceptance
sample had 27.796 seconds uptime, software reset, recovery state 0, and no
button intervention. A later fresh heartbeat at 135.683 seconds still reported
the new revision, 3.308 V battery, and sensor bits 33, so its DS3231 remained
detected. Zorua `9F26C0` and every other fixture were left untouched.

This closes the image/OTA/rejoin and RTC-hardware-presence portion of the first
anchor canary. It does not yet prove RTC validity or disagreement handling: the
current dashboard heartbeat view does not expose `NB_TIME_QUALITY` observations
or the fixture's selected UTC source. Hold the second RTC canary until that
wire-level/time-selection evidence or the compressed schedule test is captured.

## 2026-08-24 -- Codex -- Bridge field manual PDF export

Exported the illustrated Markdown manual to the print-ready, 20-page US Letter
PDF `output/pdf/Resonance_Bridge_Field_Manual.pdf`. The PDF has a designed
cover, embedded role and launcher visuals, two-page linked contents, PDF outline
bookmarks, repeating headers/footers, page numbers, styled callouts, compact
tables, code blocks, and a two-column source index. After the seven-fixture
registry reconciliation landed, refreshed both the manual and PDF to the
141-entry roster and current T-Deck artifact identity. Final size is 351,519 bytes;
SHA-256 is
`826fa0e75c22430bad7a8e0861d4b220be0dd863e501289e80a89402e8b4e065`.

Reopened the final file with independent PDF parsers, confirmed all 20 pages and
all major sections, rendered every page through Poppler, and visually inspected
the complete contact sheets plus full-size samples. The first draft's sparse
chapter-transition pages were removed before final export. Temporary builders,
rasterized figures, and page renders were deleted after QA; the PDF and the
Markdown/SVG sources remain. No firmware, hardware, bridge, fixture state, or
network configuration changed.

## 2026-08-24 -- Ben + Codex -- Seven live fixtures reconciled and named

A passive T-Deck census found seven fresh field fixture IDs that appeared as raw
hex because they had never been entered in `ops/fleet/registry.csv`: `9E5B44`,
`9F26D8`, `F2B900`, `F2BFE0`, `F40308`, `F40314`, and `F4035C`. Historical logs
already knew each ID, so this was a canonical-data omission rather than stale
Bridge firmware. Four report the current field image, two the prior recovery
image, and only `9F26D8` the deliberately retained older image. Callsigns are
bridge-side aliases, so no fixture OTA was needed.

Added all seven as commissioned identities while leaving unobserved role,
capacity, and placement fields unresolved. Assigned seven previously reserved
permanent callsigns: `Astro [9E5B44]`, `Bidoof [9F26D8]`, `Cammy [F2B900]`,
`Coco [F2BFE0]`, `Daisy [F40308]`, `Dixie [F40314]`, and
`Dratini [F4035C]`. The production-health roster is now 141 identities with 19
reserved names remaining. The complete Bridge native suite and all 17 host
dashboard/OTA-policy tests pass at commit `fca9e73`.

Built and USB-flashed only exact T-Deck `8EB508` (`44:1B:F6:8E:B5:08`) on
`COM152`. The 1,550,224-byte binary SHA-256 is
`3026593615bd58304c2a6b8893bf4f92cd8f9f92211f9222a5a28517fedf6e32`.
Esptool verified every region. Post-flash probes passed PSRAM, keyboard, touch,
ES7210, and GPS; the bridge rejoined channel 11 with zero send failures and saw
fresh heartbeats from all seven reconciled IDs. No fixture command, maintenance
request, fixture OTA, or Magic Wand OTA was sent.

## 2026-08-24 -- Codex -- Illustrated bridge field manual

Added `docs/howto/BRIDGE_OS_FIELD_MANUAL.md` as the friendly living operator and
IT guide for the T-Deck Bridge OS, CoreS3 dashboard/audio modes, and the pending
PUCA performance-audio bridge. It is grounded in the current launcher and app
source, acceptance records, ADRs 0035-0037 and 0047-0050, and the existing camp
network, CoreS3 audio, and artifact-handoff runbooks. It separates working,
canary, and planned capabilities; explains control precedence, live-versus-seen,
Dark versus Sleep, stream ownership, callsign/MAC identity, channel guard,
provisioning, app-by-app operation, symptom-driven troubleshooting, field
recipes, and shift handoff cards.

Added two diffable SVG guide figures: a source-derived 320 x 240 launcher map and
a three-device role diagram. Linked the manual from the root, T-Deck, CoreS3,
and PUCA READMEs. Queued real device-photo capture during existing named-canary
acceptance work. Documentation review also exposed that T-Deck's one-token
serial WiFi parser cannot enter the spaced `Party In The Woods` SSID; the manual
warns against the unsafe command shape and `TODO.md` now tracks a quoted/escaped
provisioning fix. No firmware, hardware, bridge, fixture state, or network
configuration changed.

## 2026-08-24 -- Ben + Codex -- Callsign Bridge OS flashed to T-Deck

USB-flashed the final callsign- and Magic-Wand-aware Bridge OS image to exact
T-Deck `8EB508` (`44:1B:F6:8E:B5:08`) on `COM152`. The immutable observed binary
is 1,549,728 bytes with SHA-256
`b416c9642ceb1808dce66e84c21224cfce32d1d5685044fe81487eb35047a6eb`.
Esptool verified every written region and reset the board.

Live post-flash checks found 8 MB PSRAM, keyboard, touch, ES7210 audio codec,
and GPS at 38,400 baud. The bridge rejoined ESP-NOW channel 11 as master
`8EB508`; frame and successful-send counters advanced with zero send failures.
No fixture-control command, maintenance request, or fixture OTA was sent. The
Magic Wand remains on its known-good `.1` image because callsign display and
protected-role recognition do not require fixture firmware changes. Physical
callsign UI and named-command acceptance remain open in `TODO.md`.

## 2026-08-24 -- Ben + Codex -- Bridge OS callsigns joined with Magic Wand

Committed the completed callsign work, then merged current `main` so Bridge OS
and the host dashboard share Steve's accepted Magic Wand record. The generated
T-Deck registry now identifies `Thor [F40344]` as the protected 15 Ah
`magic_wand` role while retaining short MAC as the only wire, flash, and OTA
identity. The merge commit is `be15acd`.

All 17 dashboard/callsign/wand-OTA policy tests, the complete T-Deck native
suite, and the complete fixture native suite pass. The final combined T-Deck
build is 1,549,728 bytes with SHA-256
`b416c9642ceb1808dce66e84c21224cfce32d1d5685044fe81487eb35047a6eb`.
It is compile-only because T-Deck `8EB508` was no longer attached over USB.

The dedicated Magic Wand `.2` source also compiles with the exact peer/channel
11/740-pixel/sensor/15 Ah LFP/500 mA/4.6 V recipe: 1,057,913 bytes sketch flash
and 136,564 bytes static RAM. No wand OTA is required for callsigns or protected
role integration. Per ADR 0050, the working installed `.1` remains in place
until the playa WiFi profile and one immutable dedicated artifact are ready for
an explicit sole-target `F40344` promotion with fresh post-pending-verify proof.
No USB flash, OTA, maintenance request, or fixture-control command was sent.

## 2026-08-24 -- Ben + Codex -- Fixture callsigns mapped and integrated

Added the canonical `ops/fleet/callsigns.csv` table with 134 permanent
production-health fixture assignments and 26 unassigned spares. The curated
pool contains 160 unique, case-insensitive ASCII names, each 3-7 characters,
biased toward games and Pokemon with short, familiar film, animation, and hero
names mixed in. The checked-in table is now authoritative; its deterministic
shuffle seed is retained only as provenance, not as a runtime assignment rule.

Short MAC remains the immutable machine identity for ESP-NOW, OTA, USB flash,
artifact manifests, persistence, and all other safety-critical operations.
Callsigns are display and command-entry aliases. Operator-facing confirmations
show both forms, for example `Ponyta [F2B7DC]`, and unknown or non-production
peers continue to fall back to their short MAC.

Integrated callsigns into the host network dashboard, the T-Deck Fleet and
Health detail surfaces, and Claude's Bridge tools. Claude tools now accept an
exact callsign or short MAC while resolving and transmitting the same three-byte
fixture ID. Dense fleet/health overviews retain compact cells to avoid crowding.
The generated Bridge registry now embeds and validates the callsign table and
pins both source digests.

The host dashboard's 13 tests and the complete T-Deck native suite pass. A
compile-only ESP32-S3 dev-cache build succeeded at 1,549,587 bytes of sketch
flash and 102,992 bytes of static RAM; the 1,549,728-byte binary SHA-256 is
`0acd54ef3a2f6356ca43b50029f70833524d86a86a9109c792a42a95b3eb8e8a`.
This work did not flash a device or send any fleet command. Deliberate T-Deck
display and named-command canary validation remains open in TODO.

## 2026-08-24 -- Ben + Codex -- Patterns v1 and RF Diagnostics integrated

Completed the first parallel Bridge OS app batch from isolated feature lanes.
Patterns v1 adds deterministic Wash, Chase, Wave, and Twinkle modes; five
palettes; speed/intensity controls; class and stable short-ID cohort filters;
and final per-fixture RGBW planning. It shares the existing bounded 8 Hz
direct-frame service with LED Studio under one replaceable stream owner. The
pure pattern model passes 190 native checks. Microphone/audio reactivity remains
a separate v2 feature.

RF Diagnostics adds a read-only summary and valid-frame-tail view. It separates
production-roster unobserved fixtures from foreign live IDs, labels observation
coverage honestly, deterministically ranks fresh strongest/weakest peers, and
surfaces RX drops, TX counters, WiFi/AP/mesh/channel-guard state, and recent
frames. It adds no wire type and sends no mesh packet. Its pure RF model and UI
integration pass the complete native suite.

Replaced launcher string dispatch with direct app callbacks, then integrated
both apps through the shared lane. The full T-Deck native suite passes, including
Health, NMEA time, direct-frame, Patterns, and RF coverage. The combined
ESP32-S3 build uses 1,542,295 bytes of sketch flash and 102,992 bytes of static
RAM. USB-flashed its 1,542,448-byte binary to exact T-Deck `8EB508`
(`44:1B:F6:8E:B5:08`) on `COM152`; SHA-256 is
`705119167e51ae8dff399a6c46cfd442b1610d14d0acb5d8a470c63461242b46`.
Esptool verified the upload. Live probes found PSRAM, keyboard, touch, ES7210,
and GPS; the bridge rejoined mesh channel 11 with advancing frames and 94/94 TX
success. No lighting, sleep, knock, maintenance, OTA, or other fixture-control
command was sent. Ben reports the preceding Health/Schedule physical smoke test
looked good; explicit Patterns/RF acceptance checks remain in TODO.

A concurrent callsign workstream modified the Health aggregate shape before its
generated roster was refreshed. The first local rebuild therefore misassigned
the old role string to the new callsign field. The freshness test caught the
partial integration after flash. Rebuilt from clean checkpoint `3e7ed36` in an
isolated release worktree that excluded every uncommitted callsign file,
reflashed the corrected image above, and repeated the live verification. The
temporary image was not used to send fixture controls and is no longer running.

## 2026-08-24 -- Ben + Codex -- First UTC/schedule fixture canary OTA

Checkpointed the integrated Bridge OS and fixture source at clean commit
`c544bf6bdfee15e7b1fcf25db350e8bee6b1bde7`. From that exact source, built the
immutable bench/canary artifact `fx-260825-d374034-b` (UTC commit date, recipe
SHA-256 `d37403418522644bbbe7163cb21108fd3612f88ad1021d64c1e4615199cb41f8`).
The fresh field-default, channel-11, basic-listener image is 1,190,240 bytes;
binary SHA-256 is
`b0722b5fe965f16c0b0d95f7fe3b8b905799702343fd0ad2c5f9fcbeaadf5009`.
Its new artifact directory contains the exact binary, build options, identity
header, canonical recipe, manifest, and independently checked hash file.

The first two maintenance discovery attempts correctly stopped before upload.
They exposed a host compatibility seam: the dashboard wrote raw commands with
no terminator because the legacy bridge consumes a byte-oriented alphabet,
while Bridge OS consumes complete lines. An interrupted dashboard wrapper had
also left its old Python child owning `COM152`, so the apparent restart still
used the old behavior. Stopped only the two verified COM152 dashboard children,
changed dashboard writes to one newline-terminated command each, and added a
regression proving batched `U...`/release writes cannot concatenate. All 11
dashboard tests pass. Legacy bridge parsers ignore the harmless newline.

Direct serial then proved Bridge OS acknowledged exact-target
`UF2BE08`. A single fresh dashboard process found fixture `F2BE08` at
`192.168.1.224`; pre-upload telemetry matched the target and reported the old
`fx-260818-f80f315-b` revision with 3.351 V battery. OTA uploaded only the exact
immutable binary and rebooted to comms. The fixture returned a fresh heartbeat
on `fx-260825-d374034-b`; at 42 seconds uptime, beyond the 20-second
pending-verify window, another fresh heartbeat still reported that revision,
software reset, 3.351 V battery, and 4.664 V good supply. No other fixture was
uploaded. Existing NVS remains authoritative; this canary was commission
profile before OTA, so the artifact's field compile default did not silently
change its runtime profile.

## 2026-08-24 -- Ben + Codex -- Integrated Bridge OS health/time image and OTA bridge

Reviewed the concurrently added T-Deck Health app and RTC/GPS schedule path as
one shared source tree. Both native suites pass. The review found one fixture
lifecycle interaction before deployment: a valid scheduled/forced day correctly
suppressed dusk, but also bypassed the normal solar-surplus transition from
`DAY_CHARGE` to `DAY_ACTIVE`, which would have disabled daytime Knocker use.
The non-night path now preserves charge/surplus transitions while suppressing
only dusk entry. A regression covers scheduled day, surplus activation, strike
permission, forced night, and return to scheduled day; the fixture suite now
passes 89 lifecycle checks plus all packet, time, schedule, and subsystem tests.

Built and USB-flashed the final merged Bridge OS image to explicit T-Deck Plus
`8EB508` (`44:1B:F6:8E:B5:08`) on `COM152`. The image is 1,511,504 bytes with
SHA-256 `e208ad2be85681e4149de9f8c6890017f57c4b4342d76a3589e31be8c037e893`.
Esptool verified every region. The board rejoined as mesh master `8EB508` on
channel 11, transmitted successfully, and populated live production-registry
health data. Health and Schedule are now available for Ben's physical UI test.

Added the minimum bridge seam needed for safe OTA from the T-Deck: serial
`U<6-hex-ID>` sustains `NB_TARGET_ENTER_MAINT` for 35 seconds without blocking
the UI or census. It has no broadcast form, rejects malformed and all-zero
targets, and serializes sends through the existing single-writer mutex. The live
CLI advertised the command and refused `U000000`. No fixture maintenance or OTA
command was sent during these checks. Next is a clean source checkpoint,
immutable bench/canary fixture artifact, and one explicit battery-backed canary
OTA with fresh post-pending-verify evidence.

## 2026-08-24 -- Codex -- T-Deck fleet Health app

Added a separate read-only Health app to Resonance Bridge OS without changing
fixture firmware, the packet contract, mesh TX, stream ownership, or the
concurrent Schedule work. The app embeds a generated, test-pinned snapshot of
the 134 production-health PowerFeathers in `ops/fleet/registry.csv` (126
commissioned plus 8 commission-failed), excluding quarantined, bench-only,
merely enumerated/demo, and bridge hardware. Registry tiles stay in stable
short-ID order; unexpected fresh census IDs append with a cyan border.

All normal entries fit on one non-scrolling 320x240 grid. Raw reported VBAT,
not flaky learned SOC, drives the requested bands: green above 3.20 V, yellow
above 3.10 V through 3.20 V, and red at or below 3.10 V. Grey means the
registry fixture is not currently fresh/on-air. A separate blue state prevents
a live but invalid battery sample from masquerading as either low or offline.
Touch or trackball selection opens read-only voltage/current, age, RF/PDR,
supply, advisory SOC, class/lifecycle/program, sensor signature, firmware, and
registry details.

The pure merge/band model, threshold edges, stable roster ordering, live-foreign
append, stale-foreign omission, stale-detail retention, and generated-registry
freshness all pass the complete T-Deck native suite. During embedded validation,
Arduino's global `LOW` macro was found to collide with the initial enum name;
the shared source now consistently uses `LOW_BATTERY`. LVGL float formatting is
disabled, so voltage labels were also changed to integer millivolt formatting.

The deliberately interrupted development-cache compile was stopped at its
exact Arduino/Xtensa processes and quarantined through the wrapper as
`dev-cache.quarantine.20260825T010409Z-220`; it was not resumed. The final exact
source then passed a fresh, unique-path compile at
`firmware/tdeck_bridge/build/health-compile-20260824-r2/`: 1,510,767 bytes of
sketch flash (48 percent), 91,512 bytes static RAM (27 percent), and a
1,510,912-byte binary with SHA-256
`e11b0b76cd06dfa092bbbe43c9200fb517692179cf4808bce255ee55df560fe8`.
This was compile-only: no T-Deck was flashed and no fleet command was sent.
Physical grid/touch/trackball/detail and repeated-navigation memory validation
on named T-Deck `8EB508` remain open in TODO.

## 2026-08-24 -- Ben + Codex -- UTC civil-twilight schedule and field overrides

Activated the existing `NB_TIME_QUALITY` wire seam rather than creating a
second protocol. The T-Deck now checksum-validates active GPS RMC date/time and
publishes a quality-tagged UTC anchor every two seconds. Fixtures with DS3231
anchors read UTC at 100 kHz without writing the clock, refuse oscillator-stop
or malformed calendar state, and publish ten-second holdover observations.
Fixture SAM-M8Q I2C acquisition remains a qualification item; the already
hardware-verified T-Deck GPS is the initial absolute source.

Added a bounded, native-tested eight-source UTC selector. Direct GPS, bridge,
or RTC time can stand alone; peer time requires two agreeing reports. Selection
prefers vote count then GPS/bridge/RTC/peer quality, never moves accepted time
backward, rejects a source more than five minutes from accepted time, and
expires 30 minutes after the last accepted observation. Fixtures calculate
solar elevation locally for Black Rock City and use civil twilight (`-6 deg`)
as the deterministic field-profile day/night gate. The existing panel-current
heuristic regains authority when UTC is unavailable.

Added the T-Deck Time / Schedule app with confirmed Auto, Day Dark, and Night
Show field baselines. Overrides remain RAM-only and repeat for six minutes so a
300-second field sleeper gets a complete opportunity to hear them. Artistic
direct/program leases can light the tree during scheduled day; dark and timer
sleep can suppress scheduled night; local battery/boot/solenoid vetoes remain
authoritative. Commission profile deliberately retains ADR 0039's always-awake
listener behavior.

Corrected the daytime reachability gate: only a valid operator command now
starts the ten-minute awake hold. Peer heartbeats, choreography, events, and
time quality no longer keep an otherwise sleeping fleet awake forever. ADR
0049 keeps 300 s sleep / 15 s listen as the production default and records a
future selectable 60/8 build-week posture. From measured 126-144 mA dark-awake
and sub-mA rails-off endpoints, estimated averages are about 15-18 mA for 60/8
and 6-8 mA for 300/15; external-INA measurements including boot overhead remain
open.

The complete fixture native suite passed, including 19 new time/schedule
checks. A coherent field/basic-listener development build completed at
1,190,208 bytes with SHA-256
`af336b08f212d736bec863bd4d60b03f3fbb8c0e351cb30d3aa678424960bf99`.
The T-Deck native suite also passed. During final shared-source compilation, a
concurrently added Health app exposed Arduino's `LOW` macro collision; the
shared fix renames that enum member to `LOW_BATTERY`. Its owning task completed
a clean unique-path build of the final quiet shared source: 1,510,896-byte
binary, SHA-256
`bb812c2db2e440e476e537b566892a1435ba616ec083c58395718b45c77472eb`.
No T-Deck or fixture was flashed and no mesh command was sent in this work.

## 2026-08-24 -- Ben + Codex -- Bridge OS P0 fleet-scale hardening

Ben field-smoke-tested LED Studio, Sleep / Dark, Knocker, and CA Studio on the
night of 2026-08-23/24 and reported that all behaved as designed. The pass also
exposed misleading Knocker `knock all` behavior: it selected only the first 32
fresh census rows, whose heartbeat arrival order varied, then issued one per-ID
targeted request every 300 ms. Fixture-side daytime, surplus, and power-tier
gates can independently refuse a request, so the UI must not equate a sent
request with a physical strike.

Hardened the shared Bridge OS baseline. Knocker now plans the complete fresh
192-entry census in deterministic short-ID order, uses an explicit 80 ms
targeted rollout, reports request progress rather than claiming strikes, and
labels the action as non-synchronized. Added pure/native Knocker coverage with a
130-fixture case. True simultaneous fire remains separate work behind the
RTC/GPS service and a fixture scheduled-strike event seam.

Serialized all packet emission across UI, Claude, streaming, lifecycle resend,
and time-service callers with one mutex-backed TX boundary, including entire
repeated bursts. Added locked census accessors for the cross-task Claude and
stream paths. Extracted direct-frame planning into a pure module with full
192-peer selection, freshness/class filtering, deterministic ordering, dim and
blink behavior, and 18-entry chunk tests.

The complete T-Deck native suite passed: build-wrapper contract, battery,
census, chat log, direct-frame planner, Knocker planner, NMEA time, shared
packet include/layout, and SSE parser. A fresh wrapper-owned `--dev-cache`
firmware build also passed: 1,504,355 bytes sketch flash (47 percent), 79,960
bytes static RAM (24 percent), 1,504,496-byte binary, SHA-256
`c793487d25d4c7a93553d78126b9bb2f49a88ae8b58a7edffcc09cdb51ceeb80`.
No wire type changed. No USB flash or fleet command was issued; revised Knocker
hardware validation remains open.

## 2026-08-24 -- Codex -- Bridge OS app triage and parallel roadmap

Onboarded from the repository and reconciled the older handheld proposal against
the implemented Bridge OS. Added `firmware/tdeck_bridge/APP_ROADMAP.md` as the
working app priority order. P0 is validation and shared-platform hardening:
named-canary LED/Sleep checks, removal of misleading fleet-size caps (especially
the 32-node knock-all queue), serialized mesh TX, locked census reads, native
stream-planner coverage, measured handheld runtime, and authenticated commands
before trusted event use.

The first parallel app batch is RF Diagnostics, Sensors Health from existing
heartbeat data, and deterministic/manual Patterns. Active RSSI Locate and ES7210
audio follow; full sensor snapshots, synchronized knocks, CA-to-strike, and voice
remain dependency-bound. The roadmap gives separate feature-file ownership and
reserves launcher/shared-service integration, artifact identity, and flashing to
one integration lane. It explicitly leaves the separate RTC/GPS dusk-to-dawn
workstream authoritative and untouched.

Corrected the root README's obsolete claim that the bridge was unbuilt and no
hardware was on hand, and pointed the T-Deck README at the new roadmap. No
firmware, hardware, fleet command, build, or flash changed in this triage.

## 2026-08-24 -- Ben + Codex -- Locked T-Deck development cache ported

Reviewed the accepted fixture `--dev-cache` implementation and applied the
same bounded mechanism to `firmware/tdeck_bridge/build.sh` without changing the
fresh-build default. The T-Deck cache has an atomic single-writer directory
lock, a recipe fingerprint covering the full FQBN, flags, Arduino CLI, ESP32
platform, LVGL, and LovyanGFX, an in-progress marker, fail-closed stale-state
handling, and explicit quarantine recovery. Cached images report
`tdeck-dev-local`; `--dev-cache` is rejected with `--build-path`, and retained
paths must be new or empty. A compile-free build-wrapper contract now runs with
the native suite; all tests passed.

A deliberate pre-compiler interruption exercise left the expected marker and
lock. Its pause expired during harness cleanup and briefly spawned one Arduino
process; the exact wrapper, Arduino process, and Xtensa child were stopped, no
build process remained, and the wrapper quarantined the partial cache as
designed. The earlier interrupted `tdeck-ledsleep-20260824-field2` directory
also remains untrusted and was not reused.

The first real cache seed then completed cleanly in about 55 minutes under
heavy host load. A warm no-op cache hit completed in about 76 seconds with an
identical 1,500,176-byte binary and SHA-256
`90fd45e257a92d61c94ae1cd87e23fa7383ee2cd009916f673dc219ac57cbae5`.
Later host scheduling made the pre-flash warm check slower, so the speedup is
real but T-Deck timing is not yet stable. No cache-corruption signature
appeared; lock and marker both cleared after success.

With Ben's T-Deck explicitly on USB, the same cached binary was flashed only
to `8EB508` (`44:1B:F6:8E:B5:08`) on `COM152`. Esptool verified every region.
The controlled reboot reported `tdeck-bridge dev-local` and
`fw=tdeck-dev-local`; PSRAM, keyboard, touch, ES7210, GPS, LVGL, memory, and
live fleet receive all passed (1,447 peer lines in 16 seconds), with no panic.
No lighting, dark, or sleep command was sent.

## 2026-08-24 -- Ben + Codex -- T-Deck LED Studio and night-rest controls

Onboarded to the Burning Man field state and completed the existing T-Deck
Zones direction as a field-facing LED Studio. It now offers labelled colors,
client-side dim, class filters, and a 1 Hz solid/blink toggle. Direct-frame
waves use the full 192-entry census instead of silently limiting control to 64
fixtures; blink edges use the existing hard-cut flag. No wire type changed.

Added a local Sleep / Dark app with 10-minute, 1-hour, 4-hour, 8-hour, and
12-hour choices. Dark is an expiring hard-cut program lease with the radio
awake. Low-power sleep reuses `NB_SLEEP_FOR`, cuts both rails and the radio,
cannot be cancelled while sleeping, and resumes normally at timer wake. Both
actions stop any suspended LED stream and require the on-device confirmation
rail showing live/seen counts. Sleep remains absent from the Claude tools and
serial CLI. ADR 0048 records the narrow exception to ADR 0037/0047.

The T-Deck native suite and the complete fixture native suite passed. The final
fresh build reports `tdeck-0.2.0-field1`: sketch use 1,500,023 bytes (47
percent), static RAM 71,984 bytes (21 percent), binary 1,500,176 bytes, SHA-256
`ffc274ebdb936e487e8c81551f27ccd69a8667bcf84b82760781ef022706c1bd`.
That exact image was then USB-flashed to T-Deck `8EB508` on `COM152`; esptool
verified every written region. The board clean-booted as `0.2.0-field1` with
8 MB PSRAM, keyboard, touch, GPS, LVGL, stable memory, and live fleet receive
(1,072 peer lines observed in 12 seconds). No mesh command was sent.

Validation exposed one identity defect: `nb_emit.cpp` independently defaulted
its machine line to `tdeck-0.1.0` even while the authoritative boot banner
correctly said `0.2.0-field1`. Current source centralizes both surfaces in
`src/core/version.h` and bumps the next retained identity to
`0.2.0-field2`. Its attempted fresh build was interrupted and is untrusted; it
was not flashed. Physical UI and canary action validation remain in TODO.

## 2026-08-24 -- Ben + Codex -- Locked fixture dev cache adopted; cleanup incident recovered

Executed the gated build-acceleration handoff on branch
`codex/build-acceleration-plan`. The unmodified wrapper measured 187.2 and
217.7 seconds for fresh builds, 11.4-13.1 seconds for retained no-ops, and 12.5
seconds for a real harmless leaf-source edit. Implemented opt-in
`./build.sh --dev-cache`: one atomic owner lock, bounded waiter, deterministic
recipe fingerprint, `dev-local` identity, build-in-progress marker, explicit
healthy clean and interrupted-cache quarantine, validated `--jobs`, and hard
rejection with OTA, immutable revisions, or artifact directories. Normal fresh
build behavior remains the default and passed again in 177.3 seconds.

Host-only safety evidence passed: all native tests; five same-recipe contention
pairs; commission/field serialization plus recipe invalidation; hard-kill
refusal/quarantine and clean cold recovery; dead-PID lock refusal; normal
compiler-error lock/marker cleanup and corrected warm reuse; dev identity in
build options/binary; fresh-path independence; and no known Arduino cache
corruption signature. Protected warm builds were about 14-18 seconds versus the
187.2-second fresh median. No hardware was flashed and no OTA was attempted.
Full evidence is
`docs/tests/FIRMWARE_BUILD_ACCELERATION_SMOKE_2026-08-22.md`.

The laptop suspended during an explicit `jobs=1` cold trial, leaving an orphaned
Arduino process and `/tmp/fixture-build.Ptr2aR`; exact PIDs were stopped and the
temporary directory was abandoned. Ben later aborted a `jobs=0` trial; its live
marker/lock were preserved and the entire cache was quarantined. Neither timing
is counted, and job-count tuning remains open.

Cleanup then exposed an operator error: despite a dry run saying it would remove
`firmware/fixture/build/`, `git clean -fdX -- .../cache-proof` was executed and
deleted the whole ignored fixture-build directory. Tracked source was untouched.
Twelve immutable artifacts had exact copies in the untouched `basic-listener`
worktree; their contract files were restored and every binary re-hashed against
its manifest, including `fx-260818-f80f315-b`, `fx-260818-05ed4b3-b`, and
`fx-260817-ec7f28d-b`. The filesystem copy of bench-only
`fx-260819-7afe0a6-b` was not found; its recorded exact bytes may be recoverable
later from prototypes `9E5AF0`/`9E5AB8`, but the revision must never be rebuilt
or reused.
## 2026-08-24 -- Ben + Codex -- Integrated Steve's NeoHex Magic Wand

Fetched and merged Steve's two-commit `codex/NeoHex-Magic-Wand` branch onto the
current `main` in an isolated worktree, leaving Ben's dirty build-acceleration
checkout untouched. Preserved `main`'s newer registry safety history and
resolved the only textual conflict by changing only `F40344` to its dedicated
15 Ah `magic_wand` record. The handoff now includes exact wiring, installed
artifact evidence, the 740-pixel renderer, standalone commissioning sketch,
and explicitly provisional pattern-control notes.

ADR 0050 makes `F40344` / `68:EE:8F:F4:03:44` the permanent one-off identity.
The fleet batch OTA helper reads the registry and refuses this protected role
unless the operator repeats `--allow-special-target F40344`; even then it must
be the sole target. Removed one duplicate no-solenoid stub introduced only by
the semantic merge of two independently added fixes.

Four host policy tests, Python syntax checks, registry CSV validation, and an
uncached `net_bench --role peer --channel 11 --magic-wand` compile passed. The
compile used the intended 15 Ah LFP / 500 mA / 4.6 V recipe and finished at
1,051,397 bytes flash (31 percent) and 136,564 bytes static RAM (41 percent).
This was compile-only: no USB flash, OTA, or live fleet command was issued. The
working `.1` wand image remains installed pending a dedicated immutable build
and current post-pending-verify acceptance.

## 2026-08-21 -- Ben + Codex -- Hardened worksite bridge and Friday-midnight fleet pack

Fetched `origin/main` and traced the Aug 20 USB shutdown incident to a stray bare
`m` accepted by the CoreS3 bridge as 5.5 V. Ported the explicit-digits-only guard
to `firmware/net_bench`, committed it as
`d082968b43ba8783759b0f45b39f9f09a8671658`, and built the immutable worksite
PowerFeather bridge artifact under
`firmware/net_bench/build/worksite-pf-bridge-20260821-r4/`. Its binary is
1,022,048 bytes with SHA-256
`777658ea0031c5117ec31a47e420992a0484622f048e8f97d6e11ff819f402f1`.
Flashed only bridge `9F2684` (`D8:85:AC:9F:26:84`) and verified the exact flash
hash plus a stable cold boot on channel 11. A second bare PowerFeather, `F40380`,
was read only and left unflashed. Preserved NVS reads from both boards found no
persisted `maint_v10`, so neither was one of the 5.5 V incident victims.

With Ben's explicit approval, broadcast `m46`; the hardened bridge reported
`broadcast SET_MAINTAIN 4.6 V`, four successful sends, and no send failures. A
fresh 14-peer census split into seven healthy/charging fixtures and seven
low-battery fixtures. The battery-swap cohort for playa unpack is `9E5A84`,
`F2BCF0`, `F2BF60`, `F3FCAC`, `F402A8`, `F403DC`, and `F4043C`; observed VBAT
was about 2.30-3.06 V, with `F403DC` radio-silent at 2.296 V by pack time.

Targeted OTA used the already accepted immutable pack-out artifact
`fx-260818-f80f315-b` (1,177,264 bytes, SHA-256
`0f1119c6ba80f2280db2c04f478a59b6be0c407edf6c95c62248f89af90ad638`).
Six healthy legacy fixtures completed exact-revision rejoin and the >=25-second
pending-verify gate: `9E5AC8`, `9E5AD4`, `9E5AE0`, `9F0E54`, `F40424`, and
`F4042C`. Healthy `9F26D8` repeatedly ignored its targeted maintenance command;
no upload was attempted, so it remains on known-good `fx-260816-otafix1-b`.
All seven low-battery fixtures were deliberately excluded from OTA.

Ben then explicitly approved sleeping the fleet to Saturday 2026-08-22 00:00
PDT (Friday midnight). A broadcast followed by fresh-peer addressed passes put
all 13 radio-reachable fixtures on recomputed timers for that same wall-clock
target. The final quiet check found no heartbeat newer than 36 seconds; the six
continuously awake fixtures had been quiet for about 16 minutes. `F403DC` could
not receive a command because it was already radio-silent/critically low and is
effectively off. The USB bridge remains awake for observation; it was not
included in the fixture sleep commands.

## 2026-08-20 -- Ben + Claude -- INCIDENT: fleet-wide SET_MAINTAIN 5.5 V at 22:24:22 PDT; two USB peers down; root cause + fixes

Two battery-less USB bench peers went quiet at 22:24:22 PDT (05:24:22Z) and
were found with VINDPM persisted at 5.5 V — above USB's 5 V, so the charge
input collapses at every boot (bootloader-level recovery needed). Forensics:

- **The packet did not come from the T-Deck** — Bridge OS has no
  NB_SET_MAINTAIN sender anywhere (tool surface and CLI exclude
  maintain/capacity/sleep by design), and its transcripted TX that night was
  identify-only (the one fleet-wide lease attempt timed out UNSENT).
- **The landmine:** the CoreS3 bridge's serial `m` command with NO digits
  cycled presets starting at **55 (5.5 V)** — one stray byte = fleet-wide
  5.5 V broadcast. Solar-bench heritage (5.5 V is sane for panel VINDPM,
  lethal on USB).
- **The trigger window:** the timestamp is seconds after the CoreS3
  re-enumerated from the overnight audio-build flash (artifact 05:23:49Z).
  The flashing agent's serial bytes were reconstructed from its transcript:
  exactly `t`, `I`, `M`x2, `A`x4 — no `m` (case-sensitive switch). Leading
  suspect: **ModemManager** (active on the Ubuntu PC) probing the freshly
  enumerated port; its probe bytes land in the bridge's terminator-less
  parser. Unconfirmed from the user journal; check with
  `sudo journalctl -u ModemManager --since "2026-08-19 22:23"`.
- **Fixture-side aggravator:** `applyMaintainV10` persists to NVS BEFORE
  applying, so the bad value survives reboot.

Fixes: CoreS3 bare-`m` DEFUSED (explicit digits required; built, flash
pending reconnection); corrective `m46` broadcast queued for when the CoreS3
returns (battery-backed fixtures that heard the 5.5 V are running with a
collapsed charge input and draining); TODO entries added for the fixture
apply-then-confirm pattern and the ModemManager udev blacklist. Recovery for
the two dead peers: boot them from an attached battery or a >5.5 V bench
supply, then `m46` by radio or serial — gentler than an NVS erase. This
incident is also a concrete argument for the tracked authenticated-command
work: any byte on any serial port currently commands the fleet.

## 2026-08-20 -- Ben + Claude (overnight) -- Bridge OS M2-M4: LVGL shell, Fleet app, streaming Claude client, agent tool loop — all hardware-verified

Overnight autonomous session (Ben asleep; questions queued in the morning
report). ADR 0047 written — Bridge OS is now the platform of record.

**M2 (LVGL shell + apps).** LVGL 9.5 on hand-configured LovyanGFX; launcher
grid (12 tiles), status bar, confirm rail. Fleet app: live census table
(reported-LED color chip per row — ADR 0043 "reported color is truth" — plus
class letter, age, EWMA, PDR, SoC, named program), tap/trackball row → node
detail with targeted identify, fleet dark/release behind the confirm modal
(origin-tagged, focus lands on cancel). Settings app (backlight/channel;
secrets stay on the serial CLI). Trackball became a context-aware keypad:
left/right focus-step, up/down row-jump on the grid / row-scroll in tables,
edit-mode pulses = arrow keys (slider tuning). Fixed en route: launcher-screen
deletion crash (status timer wrote a freed widget → panic-reboot), table
tap-select cleared before CLICKED, cell padding wrapping 3-char headers.
Touch verdict (Ben): touch-first UI; trackball stays first-class for
gloves/dust.

**M3 (Claude client).** Raw TLS with embedded GTS trust anchors
(`anthropic_root_ca.h`, chain inspected same day), manual chunked-transfer
decode into a pure native-tested SSE parser, 12-turn PSRAM chat log,
`thinking: disabled` + effort low, SNTP-before-TLS gate, offline queue with
capped backoff (verified: submit while `wifi off` → amber "queued" → delivered
on rejoin). First live response 2.8 s round-trip; TLS heap transient ~55-65 KB
(single-flight); census PDR unaffected during streaming. Chat app: streaming
scrollback, QWERTY input line, thinking/queued/mesh-silent states.

**M4 (agent tool loop).** Six tools exactly (ADR 0037 §6) via `tool_schema.h`;
tool turns stored as raw content-array turns; iteration cap 8; every mesh
effect through `mesh_tx`; fleet-wide `set_program` blocks on the cross-task
confirm rail. All three acceptance tests green on hardware:
- "Which fixtures are quiet?" → mesh_census → "No quiet fixtures — all 3
  observed nodes reported within 60 s (3 of ~130, not the full fleet)".
- "Make fixture 9E5AF0 blink green" → identify → physical fixture blinked.
- "Lease dark to the whole fleet" with nobody at the device → 30 s confirm
  timeout → "Denied — not sent. No fixtures were changed."
Two live-fire bugs the model itself helped diagnose: jsonFindString rejected
model-authored `"id": "..."` (space after colon) and truncated 6-hex ids
(unescape headroom vs 8-byte buffer); census tool JSON had a leading comma.
All fixed with pinned native regression tests (5/5 suites green).

Fleet-side observations during the night: census organically grew to 4 devices
(9E5A** fixture-firmware feathers + 9F26F8/9F2690 net-bench-era peers whose
full 15-tail heartbeats round-tripped the emitters perfectly, BQ registers,
lux and MPPT fields included).

**M5 (partial, same night).** Zones (class-targeted solid colors via a
single-writer 8 Hz NB_DIRECT_FRAME streamer — `stream_svc`; client-side dim;
stream survives app-switch, stop is explicit), Knocker (single strike +
knock-all as a 300 ms-spaced timer queue behind the confirm rail; synced
schedules stubbed pending ADR 0031), CA Studio (program leases + GH-CA
params[0..4] sliders; apply = release-then-re-lease workaround for the
fixture params gap). All flashed, boot-stable, heap steady. Patterns + the
ES7210 mic HAL deferred (riskiest unverified hardware path — not an
unattended-overnight job).

**Extra credit (same night, via subagents).** CoreS3 (4D5DB0, /dev/ttyACM1)
audio-reactive build upgraded to four visual modes — CLASSIC per-slot R/G/B,
EMBER warm-white, HUECYCLE (20 s shared hue), PULSE (beat-transient flash
over a dim floor) — tap or `M` cycles, `A` toggles; flashed + serial-verified,
left ON in CLASSIC; fixture-side look unverified (no fixture-firmware peers
were live during the check). Docs updated. PUCA PoC written UNVERIFIED at
`firmware/puca_bridge/` (compiles clean on pico32, 73% flash): WM8978 +
16 kHz I2S → shared cores3 envelope → 10 Hz NB_DIRECT_FRAME on ch 11;
KNOB1 = log sensitivity, KNOB2 = brightness ceiling / hue, paw touch = mode
cycle; pin table with provenance from github.com/ohmic-net/puca_dsp. The PUCA
did NOT enumerate on USB (cable/power question queued for Ben); nothing
flashed.

Next: Patterns + mic (M5 tail), whisperd voice + Sensors/Locate/RF Survey
(M6), fixture-side TODO items to unblock live CA knobs without the blip.

## 2026-08-19 -- Ben + Claude -- Bridge OS begins: T-Deck M0 bring-up complete, M1 mesh core live

ADR 0037 re-prioritized into active development, expanded to **"Resonance Bridge
OS"** — an app-launcher handheld on the T-Deck Plus (Claude terminal, fleet
health, Hue-style zones, knocker, pattern generator + audio reactivity, CA
studio, plus stubs for locate/cambium/sync-knock). Plan approved (LVGL 9 on
LovyanGFX; voice = laptop whisperd behind a swappable STT interface, cloud
later; fixture gaps documented as TODOs rather than limiting the handheld;
all four headline apps are v1 targets). New target: `firmware/tdeck_bridge/`.

**M0 (bring-up) — done except the passive battery-runtime number:**

- Dev bench is the Ubuntu PC (`/dev/ttyACM0`, 303a:1001), arduino-cli + esp32
  3.3.7 + LovyanGFX 1.2.24. FQBN pinned: generic `esp32s3`,
  `FlashMode=qio,FlashSize=16M,PSRAM=opi` (FN16R8), `app3M_fat9M_16MB`.
- Probes all green on first boot: 8 MB OPI PSRAM, keyboard (0x55), GT911,
  **ES7210 mic at 0x40** (audio reactivity unblocked), **GPS NMEA @ 38400**.
- **Coexistence proven:** ONLINE on BubbyNet (2.4 GHz is already channel 11)
  while mesh frames flow — the one-radio premise of the whole device.
- **Channel guard demoed live** (mesh ch temporarily 6 vs AP ch 11): Wi-Fi
  dropped, mesh kept, mismatch displayed, restored cleanly. Guard never
  auto-retries a wrong-channel AP.
- Direct-sun verdict (after removing the shipped screen film — a glare
  confound): usable in full sun tilted off-normal; **UI minimum text size 2**
  (M2 constraint). Trackball map a=UP b=RIGHT c=DOWN d=LEFT.
- Provisioning is NVS-only over a serial CLI (`set wifi/key/model/channel`);
  no secret compiled or committed.

**M1 (mesh core) — functional, soak in progress:**

- Census ported from cores3_bridge (all 15 `NB_HAS_HB_FIELD` tails,
  reboot/seq-restart accounting) into pure `src/core/census.cpp` with native
  tests, plus: RSSI EWMA (α=1/8), windowed PDR, eviction with 6 dB hysteresis
  (cores3 wedged silently at 192), class latching across hb-short, and an
  observation ledger so duty-cycled listening reads "unobserved", never
  "quiet" (ADR 0037 §11).
- `nb-master`/`nb-peer`/`nb-scanap`/`nb-rssi` emitters are byte-compatible:
  30/30 + 60/60 captured lines parse against `net_bench_dashboard.py`'s own
  regexes. `packet.h` included from the fixture tree (never forked), golden
  sizes pinned in native tests.
- TX path live via `mesh_tx` (burst 4x/5 ms broadcast, 6x/8 ms targeted):
  WAN-down quick commands `i/I/K/B/b/t` on the CLI. Identify-all was received
  by the bench feathers (their hb now reports our downlink at `dlrssi=-19`,
  `dlpdr=1.000`). Strikes require a real target id — broadcast strike is
  refused in both the handheld and the fixture.
- Fixed en route: send-callback registration was lost on every ESP-NOW
  re-init (sendok stayed 0); accounting moved into `espnow_link`.
- Fixture-side gaps Bridge OS exposes are now six explicit items under
  `TODO.md` → Firmware track (params re-lease no-op at `runtime.cpp:56`,
  inert `NbShowFrame.bright/beat_phase/energy`, `NB_SENSOR_REPORT`,
  `fire_in_ms` + strike event kind, CA→strike seam, `NB_NEIGHBOR_SET`
  persistence).

Next: 1 h heap soak completes M1 → M2 (LVGL shell, launcher, confirm rail,
TxService, Fleet + Settings apps) → M3 Claude client. Bridge OS ADR to be
written with M2. Battery runtime: `bat_mv` rides the 10 s `nb-mem` line; run
the T-Deck unplugged for an evening and read the log.

## 2026-08-19 -- Ben + Codex -- Prototype peers flashed for T-Deck channel-11 testing

Promoted early-header prototypes `9E5AF0` (`D8:85:AC:9E:5A:F0`, COM151)
and `9E5AB8` (`D8:85:AC:9E:5A:B8`, COM4) to the same immutable modern fixture
image for local T-Deck discovery work. Added a guarded `--fw-rev` build option
and compile-time version-token stringification, committed as
`50243afe30610ffffffd18fac5686761c59dd6c6`, so new ADR 0040 identities can be
injected without reusing the ambiguous legacy `.4` name. All 382 native fixture
checks passed.

Built bench artifact `fx-260819-7afe0a6-b` from a clean detached worktree with
commission profile, ESP-NOW channel 11, strict-dark commission idle behavior,
and the `party-in-the-woods-v1` credential set. The accepted binary is 1,170,736
bytes with SHA-256
`95e8d74727089c9bc309ae66109c2f26c1cb7cb7888d84c8fe90158f8bc9fcbc`;
its manifest and build options are preserved under
`firmware/fixture/build/fx-260819-7afe0a6-b/`. An earlier compile failed before
producing a binary because the Windows Arduino command line preserved string
escape backslashes; that failed directory was retained separately and was never
flashed or reused.

Flashed `9E5AF0` first while `9E5AB8` still ran its historical channel-11 serial
bridge. Over 18 seconds B8 printed 18 reports for F0 at about -19 to -20 dBm,
zero packet gaps, and PDR 1.0000, proving real ESP-NOW transmission and reception.
Then flashed B8 from the exact same binary. Both final serial gates pass with the
expected revision, `profile=commission`, `channel=11`, `espnow_up=true`, inferred
no-sensor class `chandelier`, `pf_ready=true`, good USB supply, no battery,
charging disabled, 6,000 mAh capacity, 2,000 mA charge ceiling, and
`ota_pending_verify=false`. Evidence is in
`ops/fleet/bringup/2026-08-19-ca-usb-prototype-{f0,b8}.jsonl`.

The registry marks both `bench_only` because their soldered headers are wrong;
they are local prototype peers, not fleet installation candidates. The fixtures
need no infrastructure WiFi for this test. Next bring up the Beryl's dedicated
2.4 GHz SSID at fixed channel 11 / HT20 / WPA2, verify the actual channel over
the air, then associate the T-Deck and confirm it continues to see both short
MACs while internet access is active.

## 2026-08-19 -- Ben + Codex -- Early-header prototypes pass bench-board preflight

Identified the two USB-connected early prototypes as `9E5AB8`
(`D8:85:AC:9E:5A:B8`, COM4) and `9E5AF0` (`D8:85:AC:9E:5A:F0`, COM151).
These are heavily exercised historical bench boards, not members of either recent
quarantine cohort. Ben reports that they carry the wrong soldered headers for
production fixtures, so they remain local bench hardware.

Both passed a fresh read-only `esptool flash-id` preflight: ESP32-S3 QFN56 rev
0.2, embedded 2 MB PSRAM, valid `20:4017` external-flash JEDEC identity, and 8 MB
detected flash. Both hard-reset back into their existing applications and returned
PowerFeather telemetry with `pf_ready=true` and good USB supply. `9E5AB8` runs
`net-bench-2026-07-08.1` as a channel-11 serial-bridge master; with charging off
and no battery, its battery-side gauge is unpowered while the regulator and charger
still respond. `9E5AF0` runs `power-bench-2026-06-29.1`; its charger, fuel gauge,
and regulator all respond with no telemetry errors.

Both are good candidates for modern local test firmware, but neither was flashed
in this pass. The local tree has no immutable manifest for the observed
`fx-260816-prtrel1-b` image, and the worktree is dirty with unrelated work. Follow
ADR 0040 for any replacement: select or build one clean named bench artifact,
record its exact SHA-256, and target only `9E5AB8` and `9E5AF0`. Until then, do not
attach a battery to `9E5AF0`: its legacy power-bench image automatically enables
charging at 2,000 mA. No firmware, NVS, or persisted board setting was changed.

## 2026-08-19 -- Ben + Codex -- Shed boards matched prior flash-failure quarantine

The two suspicious PowerFeathers brought from Ben's shed identified as `F402B4`
(`68:EE:8F:F4:02:B4`, COM20) and `F402F4` (`68:EE:8F:F4:02:F4`, COM25).
These are the same two boards quarantined on 2026-08-06, not additional members
of the uncertain worksite reverse-polarity cohort. A fresh USB-only serial capture
reconfirmed that both remain in ROM boot loops repeatedly reporting
`invalid header: 0xffffff07`; neither starts application firmware. This matches
the preserved invalid/unreadable external-flash diagnosis. Keep both quarantined
and retire/e-waste them. No battery was attached and no firmware was built,
flashed, erased, or changed.

## 2026-08-19 -- Ben + Codex -- Entire uncertain reverse-polarity cohort quarantined

Ben clarified that the exact members of the four-board batch exposed to the
reverse-polarity assembly error are not known. Quarantine therefore applies to
the whole physical cohort, not only to boards that fail a USB boot. The third
identified board is `9E5AA0` (`D8:85:AC:9E:5A:A0`, COM111). It still enumerated,
booted `fx-260816-prtrel1-b`, initialized the PowerFeather SDK, and reported no
battery, charging disabled, 4.859 V / 68 mA USB supply, a responsive BQ25628E,
and no current charger fault bits. Those are USB-only liveness observations and
do not clear an uncertain reverse-battery exposure.

The fourth physical board did not enumerate as an Espressif USB device, so its
MAC could not be read; a cable or connection fault remains possible. Physically
label it with the same `NO BATTERY - REVERSE-POLARITY COHORT` quarantine and do
not reconnect a cell merely to identify or test it. Known quarantined identities
are now `F40330`, `9E5B24`, and `9E5AA0`, plus that one unidentified physical
unit. No firmware was built, flashed, or changed.

## 2026-08-19 -- Ben + Codex -- Reverse-battery PowerFeathers retired from fleet

Identified the two USB-connected suspect PowerFeather V2 boards as `F40330`
(`68:EE:8F:F4:03:30`, COM18) and `9E5B24` (`D8:85:AC:9E:5B:24`, COM71).
Both still enumerate as ESP32-S3 USB JTAG/serial devices and boot their existing
firmware. USB-only telemetry showed successful PowerFeather SDK initialization,
no battery present, charging disabled, and normal-looking USB supply telemetry;
`F40330` also reported a responsive BQ25628E with no current fault bits.

This does not qualify either board for reuse. They were exposed to a reversed LFP
connection with observed smoke. The V2 schematic connects the battery path to the
BQ25628E and MAX17260 without a reverse-polarity isolation stage, while both ICs
limit battery-pin negative voltage to -0.3 V absolute maximum. Retire both boards
from the production fleet and do not reconnect a battery. They may be kept only as
clearly marked USB-only forensic/dev specimens; otherwise send them to electronics
recycling rather than ordinary trash. No firmware was built, flashed, or changed.

## 2026-08-04 -- Ben + Claude -- ~20 ohm coil "target" retired: provenance traced to the 07-05 design doc, premise doesn't hold

Ben questioned where the "~20 ohm preferred winding" figure came from (it had been
carried forward as an open want in session memory). Provenance traced:
`docs/research/STRIKER_DESIGN_2026-07-05.html` section 1, which claimed the
~6 ohm/1 A and ~20 ohm/0.3 A winding variants are "wound to the same ampere-turns
at rated voltage, so they deliver the same force but very different current draw
and cap droop," and preferred the 300 mA winding for one-third the droop on the
then-current single 10,000 uF cap.

Two problems, so the target is RETIRED (errata note added to the design doc):

1. **The equal-force premise was wrong at a fixed rail.** Same frame = fixed
   winding window, so R grows as turns squared and force at a given drive voltage
   tracks dissipated power (F prop (NI)^2 prop V^2/R). A 20 ohm winding on this
   frame at the 6 V VDC-tap of the July-5 design would have delivered ~30% of the
   6 ohm winding's force -- likely unable to move the paddle at all, given the
   later rig finding that even the ~6 W-class units barely move without o-ring
   pre-plunge. "Same ampere-turns" is only true at each winding's own rated
   voltage (20 ohm is effectively the 12 V winding: 12^2/20 ~= 6^2/6 ~= 6 W).
2. **The droop rationale is obsolete.** It was never about refill time or
   rapid-fire; it was droop-per-strike (force retention through the 25 ms pulse
   and knocks-per-charge within a 3-4 strike chirp) against a single 10,000 uF
   cap. The bank is now 2-3x 22,000 uF and capboard v2.0 boosts the rail to
   ~12 V: the measured ~3.8 ohm 0730B draws ~3.2 A there and droops only ~1.2 V
   per 25 ms strike from 66 mF. Comfortable.

Under the overvolt strategy (whack prop V^2/R at the fixed 12 V rail), LOWER
resistance is now strictly better for strike energy; the only reasons to derate
are XH pin current (VBOOST is a single pin at ~3.2 A pulsed vs the 3 A continuous
series rating -- fine at 25 ms duty) and bank droop, both comfortable. What
survives from the design doc: meter every unit on arrival; trust measured ohms
over the badge (the "6 V 1 A" 0730B fleet leader measures ~3.8 ohm).

---

## 2026-08-19 -- Steve + Codex -- NeoHex-Magic-Wand playa handoff

Completed Steve's one-off truncated-icosahedron wand: 20 M5Stack NeoHex boards,
740 WS2812 pixels, four separately fused 5.1 V injection zones at Hex 1/6/11/16,
and one continuous GPIO10 data/GND chain with +5 V isolated at each five-board
boundary. The protected Gotion 33140 15 Ah LFP feeds both the PowerFeather V2
and Pololu U3V70F5. MSA311 and BMP581 share Wire1 at the required 100 kHz.

Battery-only validation passed full RGB fills, a white chase through all 20
boards and all three isolated-power boundaries, automatic default-pattern boot,
and thermal checks. The commissioned low-light pattern moves a red five-board
row bottom-to-top every 0.4 seconds over orange/yellow/green/blue backgrounds.

Added the standalone commissioning sketch and a `net_bench --magic-wand` peer
role with 740-pixel RMT rendering, maintenance blanking, telemetry, fixed LFP
defaults, and sensor diagnostics. The deployed `.1` binary on fixture `F40344`
is SHA-256
`2617A33C47FE526AC01840149F091812DCDE37723D52C7281F07B7B273FFAB0B`.
The shared-WiFi upload/reboot path worked and returned post-reboot telemetry;
because that session predated the current artifact/pending-verify contract, it
is recorded as transport/reboot validation rather than full fleet OTA
promotion. Organized the complete Ben handoff under
`docs/projects/NeoHex-Magic-Wand/` and ported `.2` source to current `main`.

## 2026-08-18 -- Ben + Claude -- ADR 0051: no more false PROTECT latches

Closed the three remaining paths by which a fixture could acquire a durable
PROTECT latch without a genuinely collapsing battery (the "panel before
battery" family from the risk register).

One: the floating-BAT window. A cell-less BAT node held up by a powered
charger can float anywhere in 2.5-3.05 V and used to trip the immediate
protect floor with a durable NVS write. Now the PROTECT *posture* (rails off,
park) still applies instantly, but the durable write waits for battery
corroboration: recent >=30 mA charge/discharge current, an on-demand
rate-limited SLUAB31A BQ presence test (charging-enable state restored on a
REAL verdict, never on EMPTY), a recovery-lane detection, or battery-only
operation. An uncorroborated park stays awake on verified external power --
the false positive requires a powered charger by construction -- and a fresh
EMPTY verdict vetoes batteryPresent() for 60 s, cleared instantly by real
current so installing a cell mid-session is never locked out.

Two: power-ordering escalation. POWERON is an unexpected reset class, so two
benign power interruptions from a stored FULL stage used to walk FULL -> DIM
-> PROTECT with no load ever energized (9F26F8's 31 poweron resets). A new
durable `load_arm` NVS marker is written before the LED rail or solenoid gate
can energize (an unpersistable marker refuses the load, mirroring the
stage-persist doctrine) and cleared by allLoadsOff, a 60 s all-loads-quiet
debounce, and every boot; bootGuardDecide escalates only when it was set.
Collapse loops under load still escalate identically because the marker
persists through the resets they cause. Write-on-change keeps NVS wear at a
handful of writes per day.

Three: the recovery-lane self-latch. An active ADR 0042 low-VBAT lane made
batteryPresent() true at ~2.3 V, so the ladder persisted PROTECT during the
rescue; worse, a field-profile wake (8 s grace) could sleep 900 s with
charging disabled if the guard's 5 s retry cadence missed the window. The
lane now freezes the ladder outright (batt_valid excludes an active
recovery), and the freeze-plus-supply-good rule keeps the fixture awake for
the rescue's duration in any profile.

ADR 0051 records the decisions; the rescue handoff notes that post-0051
images cannot false-latch from bare boards (X remains for pre-0051 latches).
Native coverage: the boot-guard Phase-4 matrix re-run armed and disarmed, and
a corroboration matrix on the policy (uncorroborated entry/hold/resolve,
corroborated entry). The bench battery/panel/USB ordering matrix is still
owed before the marker counts as qualified -- TODO updated.

## 2026-08-18 -- Ben + Claude -- ADR 0051 adversarial audit fixes

Second adversarial audit round (four lenses, thirty agents, two refuters per
finding, none refuted) then broke the first implementation properly: the
deferred persist resolved on the ABSENCE of the defer flag, which a freeze
tick also produces -- so the EMPTY presence verdict itself would have durably
latched PROTECT on a proven-empty node seconds after boot, the exact brick
the ADR forbids. Also caught: the load-armed gate had silently dropped
escalation for loads-off collapse loops (bare radio / sensor bring-up), so
the never-wired ADR 0028 rule-4 loop breaker is now real (streak of 3
unexpected resets escalates even disarmed; healthy minute clears); the
presence test could read a stale ADC conversion and call a floating node
REAL (now measures under the asserted 30 mA discharge and polls the one-shot
up to 150 ms); zero margin at the 2.20 V lane floor refused real cells
(detection floor now 2000 mV); chargingGuardTick could charge a proven-empty
node (now honors the veto) while a REAL verdict now enables charging
outright; the battery-only corroboration clause certified floating nodes
during dusk/dawn supply flicker (removed -- discharge current covers true
battery-only); the release clean-reboot is gated on the persist succeeding;
the commission override survives the persist-failure fallback; verdict
windows self-expire. All fixes native-tested (16 suites, 792 checks) and the
full image recompiled. ADR 0051's Amendment section has the complete list
including accepted residuals for the bench matrix.

## 2026-08-18 -- Ben + Claude -- Gamma scrubbed from the codebase

Ben's call: gamma correction introduces too many problems and the render
doctrine is linear anyway (led_driver's "direct linear 8-bit levels" comment is
now the whole story). The `resGamma8` dim-floor table salvaged this morning is
deleted along with its tests -- the 1..23 -> 0 dead zone it existed to fix is a
gamma artifact, so with gamma gone there is no floor-to-zero problem to solve.
Stale "pre-gamma"/"post-gamma" comments in `fixture_context.h` and `packet.h`
now say linear/post-cap. TODO's ambient-dimming item is closed with the
decision recorded: if sub-24 granularity is ever needed, use temporal
dithering, not gamma. Recovery path if ever wanted: commit 9260ae3.

## 2026-08-18 -- Ben + Claude -- Branch convergence and ADR 0046 charge-knee ladder

Git housekeeping first. `codex/deep-recovery-canary` (with the Jimmy chandelier
tester commit) fast-forwarded into `main` and pushed: the 300 mA IPRECHG write,
the ADR 0042 low-VBAT recovery lane, transport sleep, the basic-listener
posture, and artifact-identity injection are now trunk. `codex/basic-listener`,
`codex/puca-performance-audio-bridge`, and `codex/pre-origin-sync-20260811`
deleted locally (all content-contained in main; local tags `backup/*` and
`archive/*` preserve every prior tip). The Lighting-Controller worktree
fast-forwarded 202 commits; its tip fixes the same Arduino-ESP32 3.x RMT-detach
family one layer deeper (`setPin` after `begin`). `codex/commissioning-mode`
turned out to be content-superseded by main -- its dashboard, class-probe, and
heartbeat-tail work all evolved further on the canary line, and its ADR
0039-0041 files are byte-identical -- so the planned rebase was dropped. The
one genuine salvage was `resGamma8`, the dim-floor gamma table, now in
`firmware/fixture/src/core/gamma.h` unwired (commissioning renders stay direct
linear). The branch itself stays alive only as codex's active RSSI worktree;
retire it once that work lands on main. Known wart carried forward: two ADRs
share number 0041 (STEMMA classification and universal recovery/solenoid
defaults), matching the existing 0032 duplicate.

Then ADR 0046 on `codex/charge-knee-thresholds`: the default ladder rises to
dim 3.15 / off 3.10 / protect 3.05 V load-compensated (release floor 3.25 V),
LED ramp guards and dashboard battery bands in lockstep. Rationale: park above
the BQ25628E VBAT_LOWV precharge knee (believed ~3.00 V, unmeasured) so every
morning starts in fast charge -- the ~4% of pack below the old floor takes ~2 h
to re-earn at 300 mA precharge and ~19 h at the 30 mA POR default, which is the
death-spiral arithmetic we lived through. All 15 native suites pass (748
checks); the power-policy test voltages shifted +150 mV with every relative
assertion intact. Open: bench-measure VBAT_LOWV in Oakland (TODO has the
procedure), then revisit the exact values per the ADR's REVISIT clause.

Adversarial audit round (three independent reviewers over the threshold diff,
the supersession claim, and the repo topology; topology came back clean) caught
five lockstep misses, all fixed in the follow-up commit: the live PROTECT-rescue
procedures (USB rescue handoff, fixture README) still promised release at
3.10 V; the power_policy.h ladder diagram still drew 3.00/2.95/2.90; the status
LED's 3.00 V floor rung understated severity across the new bottom of the
ladder (now 3.15); stale per-unit NVS overrides could interleave with the
raised defaults and silently invert the ladder -- `powerConfigSanitize()` now
enforces dim > off > protect at boot with native tests; and the net_bench
field-cycle emulation plus its OTA config pusher still ran the old ladder while
the shared dashboard had already moved (bench rigs would have kept parking
below the knee -- exactly the behavior fixtures no longer exhibit). The audit
also recovered the resGamma8 dim-floor unit tests the gamma.h salvage had
dropped (back in test_geometry_pattern.cpp), and left two deliberate notes:
`RES_MAINT_MIN_LFP_MV` 3200 now sits below the 3250 release floor (advisory
only; absolute-vs-ladder-relative decision deferred to the REVISIT pass), and
ADR 0023 is back-annotated as amended.

## 2026-08-18 -- Ben + Codex -- RSSI-only point-cloud extraction tested on Nevada City rigs

Analyzed the `basic-listener` field capture at commit `c322562` without using
its rig note, a roster, fixture classes, ToF, CAD, known positions, or path-loss
calibration. Added `ops/locate/locate_rssi_cloud.py`, which median-aggregates the
directed EWMA observations, fits reporter-local ordinal near/far constraints with
a learned transmitter bias, and link-holdout-tests 1D through 5D before emitting
a dimensionless 3D cloud. Added three native tests and the full result record in
`docs/tests/RSSI_ONLY_POINT_CLOUD_2026-08-18.md`.

The 25,154 rows reduce to 4,558 directed pairs across 96 devices and 48
reporters. RSSI contains real topology: held-out RMSE improves from 9.99 dB for
radio biases alone to 7.30 dB in 2D and 6.77 dB in 3D. It does not identify a
physical 3D point cloud. The reporter-local rank score barely changes from 2D
to 3D (0.642 -> 0.650), then improves further in 4D/5D (0.691/0.718), and the
3D cloud does not expose the rectangular rigs. Early/late stability is high but
comes from repeated, nearly unchanged on-device EWMAs rather than independent
surveys. The derived JSON remains useful for topology visualization, outlier
detection, and initialization; it has no defensible meters, vertical, north,
handedness, or origin.

Queued the stronger RSSI-only capture: all devices report, true window medians
and expected counts, orientation churn, and an independent repeat. Physical
coordinates still require an external scale/gauge fact and should continue
through the existing CAD/ToF/beacon pipeline.

## 2026-08-17 -- Ben + Codex -- Three standalone chandelier testers flashed

Inspected the clean `codex/deep-recovery-canary` worktree through its Aug 17
transport-sleep, fleet-recovery, and RSSI-capture activity before starting this
bench change. Windows USB identities and an esptool flash-ID preflight separated
the three requested PowerFeathers from the attached CoreS3: COM12 / `F40380`,
COM31 / `F40358`, and COM103 / `9E5A70` were ESP32-S3 rev 0.2 boards with 8 MB
flash and 2 MB PSRAM; COM43 / bridge `4D5DB0` was excluded from every flash
command.

Extended `chandelier_chain_bench` with a standalone demo mode for Jimmy's
chandelier mockup: nine homogeneous RGB pixels on GPIO10/A0 and the switchable
3V3 rail render a slow rainbow glow with a moving wipe. A debounced press of the
PowerFeather USER/BOOT button toggles an explicit all-off frame plus the physical
LED rail, then restores the rail and animation on the next press. Removed the
sketch's post-`show()` `pinMode(GPIO10)` calls so Arduino-ESP32 3.x does not
detach NeoPixel RMT during an off/on cycle. The older 34-second RGB/RGBW
diagnostic ladder remains available as the default build mode.

Built once from source commit `c322562` plus targeted-test patch SHA-256
`50a99d8176e8c58398941302fe3de8991e7bb6872e701c19ebc61cefb627e530`.
The immutable non-fleetable revision is `fx-260818-926d4c2-t`: 361,792 bytes,
binary SHA-256
`cf048f95e036488abbc6cf64d9b77e1224796cc79d74c76513b6cdcf1ea615ee`.
Its ignored artifact directory contains the binary, exact build options,
manifest, identity header, and checksum. Recipe: RGB, nine pixels, 88/255
requested and applied brightness, conservative 800 mA LED budget, 6,000 mAh LFP
profile, no WiFi or mesh.

Sequential USB uploads to only `F40380`, `F40358`, and `9E5A70` verified every
written region. Fresh JSON from each reported the exact revision, its expected
MAC-derived ID, `pixel_type=RGB`, `pixels=9`, `demo_mode=true`,
`pattern=rainbow-glow-wipe`, `pf_ready=true`, and the 3V3 rail on. All three were
battery-absent on USB, so charging correctly remained off. Serial `o`/`p` cycles
then proved rail-off and rail-on recovery on all three without a reset; this is
the same power path called by the physical button. No LED strand was attached
during this flash session, so Jimmy's first nine-module hookup remains the
visual/button check and production current/voltage/thermal qualification remains
open. Append-only evidence is
`ops/bench/data/usb/20260818-0457-jimmy-chandelier-testers.jsonl`.

## 2026-08-17 -- Ben + Codex -- Pack-out transport sleep, RSSI matrix, and red-fixture recovery

Built immutable fixture revision `fx-260818-f80f315-b` once from clean commit
`29ebe2b5949bc52b03690986ea8ecbdcbecf4a65`. The 1,177,264-byte binary
SHA-256 is
`0f1119c6ba80f2280db2c04f478a59b6be0c407edf6c95c62248f89af90ad638`;
the credential-bearing binary, manifest, and checksum remain in the ignored
local artifact directory `firmware/fixture/build/fx-260818-f80f315-b/`.
Battery-backed canary `F2BE70` and the explicitly qualified fleet waves produced
fresh exact-revision evidence through the pending-verify gate. Eighty-four of
97 observed identities ultimately held the exact image. The remaining 13 were
old, intermittent, low, or unable to enter maintenance and were not subjected
to looping retries.

CoreS3 bridge `4D5DB0` alone was updated to
`cores3-bridge-2026-08-17.2`. Its exact 1,103,968-byte binary SHA-256 is
`e3747cbf0844418cb890ec85957e13d78e7b169ae0a817c0a9c3dd62b237b1aa`;
the bridge artifact also remains ignored locally. A 140-second Nevada City rig
survey recorded 25,154 pair observations from 48 reporters hearing 96
transmitters, covering 4,558 unique directed pairs. The append-only raw capture
is `ops/locate/data/field/20260818-0300-nevada-city-rig-rssi.jsonl`. Rows are
ranked neighbor-table EWMA observations with `n=1`, not censoring-corrected
window medians; offline planar/grid recovery remains a separate analysis task.

At about 20:11 PDT, all 84 exact-revision fixtures accepted `Q99` and vanished
from fresh telemetry. Their target timer wake is about 23:11 PDT Friday, shortly
before the hoped-for Saturday playa unload; timed wake retains the LED-dark
latch until a valid program command such as bridge `b`. Ten then-reachable old
fixtures received the legacy `S65535` fallback, which lasts only about 18.2
hours. Old or radio-silent fixtures that missed that short broadcast remain
holdbacks. The transport command was already in flight when Ben's pause message
arrived; subsequent state changes were held for explicit authorization, and a
single physical RESET was confirmed as the no-flash wake path. Charge-only USB
does not reset a sleeping ESP32, while the autonomous BQ25628E solar/USB charger
continues to charge during transport sleep.

The field team physically woke a small test cohort. Five exact-revision fixtures
reported blue output: `9E5954`, `9F0E30`, `9F0E5C`, `9F26E4`, and `F40174`.
A visually red sixth fixture proved over direct USB to be previously unobserved
`9F2720` on `fx-260816-otafix1-b`, not the low dashboard identity first inferred.
Exact-artifact USB commissioning passed on `COM150`; it now reports the current
revision, healthy TMF8820/MSA311 canopy sensors, about 3.4 V battery, qualified
4.6 V USB input, no charger fault, and the canopy warm-white default. Its
persisted capacity remains 6,000 mAh despite downlight classification, so the
physical cell must be checked before changing that setting. All six awake test
fixtures were finally verified on valid USB input; five were net charging and
`9F26E4` was approximately energy-neutral with USB carrying its live load.

## 2026-08-17 -- Ben + Codex -- Transport sleep and full-roster RSSI capture

Implemented ADR 0045 for the Nevada City pack-out. New append-only protocol
types provide a 32-bit, rails-off transport timer and a bounded RSSI survey.
Transport wake is automatic and restores radio/telemetry while an RTC-retained
latch keeps LED output electrically dark; a valid program command, including
the bridge's bare `b` release, clears the latch without opening a fixture.

The survey expands the neighbor cache from 24 strongest peers to the complete
160-device design envelope. During an explicit `L[seconds]` window only, each
updated fixture reports its full fresh heard roster in 16-entry fragments about
every 20 seconds. The bridge emits directed `nb-rssi` rows and the new
exclusive-create `ops/locate/rssi_capture.py` logger records canonical-shaped
JSONL for offline grid-recovery experiments. These first observations are RSSI
EWMA samples, not censoring-corrected window medians, and must be analyzed as
feasibility data rather than production coordinates.

All fixture native tests (including golden packet layouts and a survey/pinned
map separation check), dashboard command-validation tests, Python compilation,
and throwaway fixture/CoreS3 builds pass. The enlarged roster adds about 5.9 KB
of fixture global memory; the final compile still leaves 259,580 bytes of dynamic
memory and uses 35 percent of flash. Immutable build, bridge flash, OTA rollout,
live capture, and the final transport command remain to be recorded separately.

## 2026-08-17 -- Ben + Codex -- Small-fixes OTA rollout and dusk USB rescue

Built immutable fixture revision `fx-260818-05ed4b3-b` once from clean source
commit `e09f46fd052be1eac18d357ba4f2664d1b9168b8`. The exact variant-b recipe
SHA-256 is
`05ed4b3fcc7f448672f0b72afaa81ac097bafe0f6c10fd0bfce315798540132f`.
The 1,176,000-byte binary SHA-256 is
`2986a0294827ef6be970d2ffe50066c885f3107f139f8601d5054d797467e1db`;
the credential-bearing binary, manifest, and checksum remain in the ignored
local directory `firmware/fixture/build/fx-260818-05ed4b3-b/`.

Battery-backed canary `F40174` accepted the artifact, produced a fresh exact
revision heartbeat after its software reboot, and remained alive beyond the
20-second pending-verify gate. A live blackout test then confirmed the new
explicit dark-lease path cuts the physical LED rail. The 56-target main wave
and three-target safe tail each passed the same exact-revision and pending-gate
checks. In total, 60 of 60 attempted named fixtures verified; no low-voltage,
active-recovery, downlink-anomalous, or slot-anomalous fixture was included.
Post-rollout telemetry showed all 60 on the exact revision with the LED rail off
and zero lit pixels under `B3600`. Bridge `4D5DB0` was not flashed. Immutable OTA
evidence is in `ops/bench/data/ca/20260818-010236-fleet-ota-results.jsonl`,
`20260818-011015-fleet-ota-results.jsonl`, and
`20260818-011454-fleet-ota-results.jsonl`.

The new read-only anchor inventory found SAM-M8Q GPS on `F2BDB4` and DS3231 RTC
on `9F0E7C` and `9F26C0`. This discovers three of the eight purchased anchor
boards; it is not evidence that the other five are absent because the OTA
deliberately held back the low, anomalous, and silent population.

For dusk battery triage, Ben used inverse visual search on the 74-fixture large
rig and attached charge-only USB to every fixture that remained dark. Nine low
fixtures then reported positive battery current: `F402A4`, `F401DC`, `9F0E5C`,
`F2BDD4`, `9E5B34`, `F2BF7C`, `F3FD28`, `9F2714`, and `9E5A94`. Four very-low
fixtures saw external USB but accepted at most 1 mA: `F2BDFC`, `F2B900`,
`F40314`, and known holdback `9E5B44`; treat these as battery/charge-path bench
candidates. The small rig ran out of USB cables, so some dark perimeter
fixtures remain unrescued. Their silence must not yet be called a dead battery
or a failed charge attempt.

## 2026-08-17 -- Ben + Codex -- Dusk triage and small-fixes OTA candidate

Implemented the queued fixture fixes without changing the ESP-NOW packet size.
An active `PROG_COMMISSION_DARK` lease is now distinguishable from the unleased
commission fallback, so the basic-listener build returns no render frame and
actually cuts the LED rail. Accepted bridge program leases and direct-frame
microleases retire the latched presence-wave display, and autonomous presence
events are suppressed while explicit authority is active. This prevents a prior
wave from reappearing after a blackout lease expires.

Extended the existing sensor-signature byte append-only: bit 4 is a read-only
ACK inventory probe for the SparkFun SAM-M8Q at `0x42`, and bit 5 is the Adafruit
DS3231 at `0x68`. Neither bit changes physical class inference. The dashboard
spells out both devices and adds compact G/R anchor badges to fixture tiles. It
also tracks the age of the last rich firmware-identity report independently of
ordinary short-heartbeat age, closing the misleading cached-revision display.
All fixture-native tests and all nine dashboard tests pass. This entry records
source readiness only; the immutable artifact and hardware rollout are recorded
separately after build and verification.

The field visual census is now 74 canopy fixtures, 8 installed trunk fixtures,
and 24 perimeter fixtures, including one intentionally batteryless perimeter;
roughly 20 additional trunk fixtures are boxed without batteries. At one
reconciliation point the dashboard had 99 identities, 93 heard within five
minutes, against 105 fixtures expected powered, leaving roughly 12 installed
units not participating. Known silent/dead identities included batteryless
`F2BCF4`, critical old-image `9E5B44` and `F2BDFC`, effectively batteryless
`F403F0`, and noncritical silent `F4031C` and `F40414`; an exact physical slot
map remains the required way to find the rest.

At cloudy dusk, addressed 255-second locate commands were sent to the onboard
PowerFeather status LED only for 15 low-battery identities. The lantern LED path
correctly remained power-vetoed. The first physical USB-charge queue was
`9F0E5C`, `9E5B34`, `F2BF7C`, `F3FCAC`, `9F2714`, `F3FD28`, `F40424`, and
`9E5A94`; more deeply depleted but already silent fixtures must be found from
the physical census. No low-voltage fixture was authorized for this OTA.

Committed the day's generated USB inventory/commission evidence and reconciled
the registry for repaired `F2BE70`, `F2BF74`, `F3FD88`, `F40424`, `F4043C`, and
`F403DC`. `F2BCF4` remains firmware-commissioned but intentionally batteryless
after its cell collapsed below 1 V. `F40414` remains a failed Board.init case.
The cell involved in the `F2BE70` wrapper-scrape spark/smoke incident is recorded
as never reusable; its replacement MCU/power path later passed batteryless USB
checks with no BQ fault and a healthy reversed sensor daisy chain.

## 2026-08-17 -- Ben + Codex -- Dark-awake fleet test exposed the radio floor

Added a RAM-only fleet-dark lease to the normal CoreS3 bridge. `B<seconds>`
broadcasts `NB_PROGRAM_SET` for `PROG_COMMISSION_DARK` with a hard cut and a
bounded 1-65,535 second TTL; lowercase `b` releases the lease. The command does
not change fixture profile, lifecycle, sleep state, or NVS. The dashboard exposes
one-hour dark and explicit release buttons and validates the same serial grammar.
This uses program support already present in the fixture image, so no fixture OTA
was needed.

Built and USB-flashed only bridge `4D5DB0` with normal channel-11 revision
`cores3-bridge-2026-08-17.1`. The exact 1,103,040-byte binary at
`firmware/cores3_bridge/build/cores3-bridge-20260817-darklease-r1/` has SHA-256
`f4b76098d031a4166620e5319ebd754ebd7932abdf38df7f4343ef3529b80613`.
Upload verification passed and the restarted dashboard required the new revision
before any fleet command was sent.

A one-hour `B3600` lease took every updated fixture with returned LED telemetry
to zero lit pixels while leaving its radio and existing commission profile awake.
The three fresh old-firmware peers (`9E5A84`, `9E5B44`, and `9F26D8`) do not
report LED output and cannot prove or consume this fixture-era program lease.

The matched daylight comparison used per-fixture median battery current. Among
59 updated fixtures that were definitely lit in the 15-second baseline and
definitely dark in the 20-second post-command window, the median improvement was
72 mA per fixture. Their summed medians moved from -2,154 mA to +1,742 mA, a
3,896 mA fleet swing (roughly 12.5 W at 3.2 V). For the 42 fixtures with stable
external input in both windows, summed battery current moved from -811 mA to
+2,097 mA; 27 additional fixtures crossed into net charging. Sun variation is a
remaining caveat, but the paired windows were under three minutes apart and the
stable-input subset showed the same result.

With LEDs dark, the three continuously awake battery-only fixtures still drew
about 126-144 mA. That directly confirms the earlier roughly 168 mA always-on
ESP-NOW floor: radio receive dominates after the light load is removed. Merely
changing heartbeat transmit cadence from 1 Hz to 0.2 Hz should therefore save
little while the receiver and the separate 1 Hz choreography keepalive remain
awake. The existing command-only 0.2 Hz field profile also enables five-minute
day-charge deep sleep and autonomous lifecycle, so it is not an isolated cadence
test. The dark lease remains active for one hour unless explicitly released.

## 2026-08-16 -- Ben + Codex -- Presence wipe field demo and overnight fleet sleep

Built the first presence-wave image as immutable revision
`fx-260817-9ef4324-b` from commit
`cc28553751dfb2b8585ed69fd2bfc148c61c3cdd`; its 1,175,440-byte binary had
SHA-256
`233465aee349d019787fe77f5ceea51a477df59edfba472d8acd189c27139076`.
It proved the event transport but was deliberately superseded after the hanging
rig's occlusion made a fixed-distance TMF trigger produce overlapping origins.
The superseding artifact is `fx-260817-ec7f28d-b`, built once from commit
`e70cb86774dee3d298d5c964499a2e034e66cb1f` with recipe SHA-256
`ec7f28d9ae77e292c763f39b2586e6d15b3a4cde04dc50cb75fd7ceaf294549b`.
Its 1,175,648-byte binary SHA-256 is
`1598f5506e4541e4f5c6efdd8693a3959510c9ed1f3467db4bc8bf874b40f2b7`.
The credential-bearing binary and manifest remain in the ignored local artifact
directory `firmware/fixture/build/fx-260817-ec7f28d-b/`.

The hardened TMF detector keeps a closest-background value for each of nine
channels, learns the installed scene for 90 confident reports, and requires one
channel to move at least 300 mm closer for three consecutive confident reports.
Four clear reports re-arm it. This makes a stationary close rig member part of
the per-zone background instead of a permanent presence. A randomized short
origin delay lets a fixture cancel its local origin when it hears another wave,
reducing simultaneous starts.

Each accepted presence event chooses a new color, persists it locally, and
forwards the addressed event to the two strongest recent wave-capable neighbors
that are not already in the event ledger. The 150-hop budget and event ledger
stop loops. Bridge identify/tag leases and local power protection still override
the demonstration. Ben and the field team confirmed that triggering was a little
difficult but the propagation across the tightly hung grid was visually clean;
telemetry separately observed 60 of 63 and 57 of 63 fresh hardened fixtures
change during two short windows. The team captured video of the successful
demonstration.

`F40364` accepted the hardened image as canary and survived the pending-verify
window. The explicit 73-peer remainder batch discovered 71, returned 70 upload
ACKs, and produced fresh exact-revision acceptance evidence for all 70 after
the verification gate. Together with the canary, 71 fixtures accepted the
hardened artifact. Three members of the prior 74-fixture accepted cohort remain
on earlier images: `9E5AE0` timed out on OTA and was parked for one hour on
`fx-260817-9ef4324-b`; low/intermittent `9E5B34` was not discovered and also
holds that eager image; low/intermittent `9F2638` was not discovered and remains
on `fx-260817-29ac840-b`. Retries stopped. These are in addition to the earlier
12 observed holdbacks already documented below.

The dashboard now issues bedtime as individual addressed commands rather than
the old ambiguous broadcast. At 23:16 PDT it accepted an eight-hour sleep batch
for all 65 fixtures then fresh on the bridge: 62 hardened fixtures and three old
`fx-260816-otafix1-b` fixtures. All 18 fresh fixtures reporting external input
were included. The live lit count fell to zero; subsequent fresh packets were
rail-off/deep-sleep heartbeats. Low/intermittent peers not awake for the batch
were already in their protective sleep cadence, and `9E5AE0` was already parked.
The BQ25628E charger remains autonomous while the controller, radio, sensor rail,
and LED rail sleep, so USB charging continues and solar charging can resume in
daylight. The addressed cohort is scheduled to wake at about 07:16 PDT.

## 2026-08-16 -- Ben + Codex -- Self-identifying recovery image accepted on 74 observed fixtures

Built one immutable fleet artifact from clean commit
`35d1a1e152e7e2570a2f66c619248d2a9ad227ee`: revision
`fx-260817-29ac840-b`, 1,173,632 bytes, binary SHA-256
`be48bdd8961e0277d2830ca54e8775c19834129ad8e522586cd292826f314fb8`,
and recipe SHA-256
`29ac84054b4f6eeff156433918b95f297256f5570dbefc2a03e981c8063547dd`.
The manifest is under the ignored local artifact directory
`firmware/fixture/build/fx-260817-29ac840-b/`; the binary is not committed
because the local WiFi credentials are compiled into it. Both attached CoreS3
bridges, `4D5DB0` on COM43 and `E39F1C` on COM40, were flashed with the same
clean bridge build and verified as `cores3-bridge-2026-08-16.1` on channel 11.

Healthy canary `F40364` survived pending verification, self-identified as canopy
from sensor bits 5, and reported one warm-white pixel at 128. Low-VBAT USB
canary `9F268C` entered recovery state 2 after a 2,388 mV BQ presence result,
kept its LED rail off, and charged from a qualified roughly 4.9 V input. The
remaining qualified low-USB cohort `9E5A5C`, `F2BDB0`, `F2BF8C`, and `F402A4`
also accepted the image; the prior exact-target canary `F2BFE0` returned to the
fleet image. `9E5B44` twice booted the new image but safely rolled back at the
20-second self-test, so retries stopped. A healthy-fixture tag check then changed
only `F40364` from its reported warm-white default to green 128 and back again,
proving the dashboard checkbox, addressed lease, fixture override, and returned
LED-state bar end to end. No strike was requested during rollout.

The first live class census exposed 11 powered fixtures as no-sensor/class 4,
even though no chandelier fixture is currently powered: `9E5AB0`, `9F0E30`,
`9F26B4`, `F2BE1C`, `F2BE3C`, `F2BE6C`, `F2BEF4`, `F2BF90`, `F3FD50`,
`F402A8`, and `F40310`. An addressed 1-second deep-sleep reboot on `9E5AB0`
held VSQT off through sleep and ran the verified boot rail cycle/re-probe, but
still returned sensor bits 0/class 4. A second addressed 10-second off interval
also returned zero. Further fleet resets stopped; these fixtures need physical
STEMMA reseating/inspection, followed by the same addressed re-probe. The
dashboard's sun/plug icons remain as a useful class convention, not an
authoritative electrical USB-vs-panel measurement; Ben prefers to retain them.

The exact accepted set at the final 86-ID dashboard census was 74 fixtures:
`9E5A58`, `9E5A5C`, `9E5A74`, `9E5A88`, `9E5AB0`, `9E5AC8`, `9E5AD4`,
`9E5AE0`, `9E5AE4`, `9E5B04`, `9E5B10`, `9E5B14`, `9E5B18`, `9E5B34`,
`9E5B48`, `9E5B68`, `9E5B8C`, `9E668C`, `9F0E30`, `9F0E54`, `9F0E5C`,
`9F0E7C`, `9F2638`, `9F2664`, `9F266C`, `9F268C`, `9F26AC`, `9F26B0`,
`9F26B4`, `9F26BC`, `9F26C0`, `9F26C4`, `9F26D4`, `9F26E4`, `9F26E8`,
`9F2714`, `9F2718`, `9F2724`, `9F2738`, `9F275C`, `F2B7DC`, `F2BDB0`,
`F2BDB4`, `F2BE0C`, `F2BE1C`, `F2BE20`, `F2BE3C`, `F2BE48`, `F2BE60`,
`F2BE6C`, `F2BE94`, `F2BEA4`, `F2BEE4`, `F2BEF4`, `F2BF54`, `F2BF5C`,
`F2BF8C`, `F2BF90`, `F2BFE0`, `F3FC90`, `F3FD50`, `F3FD60`, `F401A8`,
`F40254`, `F40268`, `F402A4`, `F402A8`, `F402C4`, `F40310`, `F40350`,
`F40364`, `F40384`, `F403F0`, and `F4042C`. The last eight sleeping peers
proved acceptance by subsequently booting the exact revision with reset reason
`deepsleep`; pending images are explicitly forbidden to sleep.

Twelve observed IDs remain deliberately old. Rollback exceptions `9E5B44` and
`F40424` need bench diagnosis. `9E5A84`, `9F26D8`, `F2BCF4`, and old
`F3FD88` never exposed an identity-matching maintenance endpoint through their
bounded attempts, so no upload was made. Low/no-external-power fixtures
`F2BE08`, `F3FD28`, `F401DC`, `F40308`, `F40314`, and `F4035C` were refused
before OTA. This is 74 of 86 fixtures seen by this bridge, not a claim about
unseen members of the planned roughly 130-fixture production fleet.

Added `ops/bench/fleet_dashboard_ota.py` for future explicit batches. It hashes
the named image, validates every short MAC and power sample, performs one shared
maintenance discovery scan, verifies each HTTP identity before parallel upload,
and requires fresh exact-revision evidence beyond the pending-verify gate. Its
full-cadence mode can collect named sleepers while deferring, rather than
guessing, any target not discovered. Two direct-batch acknowledgement records
are committed under `ops/bench/data/ca/`; the remaining generated uploader logs
stay bench-local under the existing raw-OTA ignore rule.

## 2026-08-16 -- Ben + Codex -- Fleet recovery, self-identifying dashboard, and class-aware listener implemented

Merged the deep-recovery and self-identifying dashboard branches onto the
current listener baseline, then implemented the fleet candidate Ben requested.
The common fixture image now uses the BQ25628E's documented 30 mA BAT-discharge
presence test before enabling any 2.2-2.5 V recovery. It requires strong USB,
clean faults/current, and verified precharge configuration; a proven cell is
capped at 100 mA with loads parked until it holds at least 2.55 V for 60 seconds,
then the normal persisted charge cap is restored. Recovery state and the BQ ADC
test voltage are append-only heartbeat telemetry.

The same heartbeat now reports the raw STEMMA signature plus the already
implemented sensor-derived class and mismatch guard. CoreS3 and the dashboard
carry the fields end to end. Dashboard glyphs use circle/hex/triangle/diamond
for canopy/perimeter/trunk/chandelier, their thin bar uses the fixture's actual
post-cap RGBW output, and each tile has a persistent checkbox that issues a
renewable, addressed, steady-green tag at linear 128. Tags remain bounded leases
and local power vetoes still win.

Ben refined the listener defaults before rollout: canopy/downlight uses its
dedicated warm-white die at linear 128; 37-pixel perimeter HEX uses red at
linear 16; RGB trunk/uplight remains red at linear 128. Native fixture tests
pass all 430 checks, dashboard tests pass 6/6, and both the PowerFeather fixture
and normal CoreS3 bridge compile successfully. The dashboard was restarted from
this worktree and visually checked in the in-app browser; a first CSS pass found
and fixed the global button minimum-height overriding the small tag checkbox.
No fleet OTA is claimed by this entry; immutable artifact creation and canary
promotion follow from the clean commit.

## 2026-08-16 -- Ben + Codex -- Target-locked deep-LFP recovery canary passed OTA and controlled charge

Ben authorized one supervised OTA experiment to recover an installed LFP that
the older fixture image would not charge below its 2.5 V plausible-cell guard.
Firmware now has a test-only, exact-short-MAC recovery posture: it requires a
fresh qualified supply, a 2.25-4.4 V cell reading, near-zero pre-enable battery
current, no BQ fault, and an exact 16-bit precharge-register readback before
enabling charge or accepting a pending OTA image. The posture holds precharge at
50 mA, imposes a 100 mA absolute charge ceiling, disables LED and sensor rails,
clamps D7/GPIO37 low, and refuses strikes. Native tests pass all 404 checks.

The first immutable test artifact, `fx-260816-e5ca3a0-t`, was locked to
`F401DC`, but no maintenance command or upload was sent: a guarded preflight
found that fixture's telemetry stale and `supply_good=false`. The field team
later identified an accidental cable swap. Ben authorized continuing with an
already powered replacement canary rather than changing course again.

The successful immutable artifact is `fx-260816-625fab1-t`, built from clean
source commit `dc0234a580a2d83bb03025b1e884a571cdf1a173` and authorized only
for `F2BFE0`. Its 1,171,296-byte binary has SHA-256
`a22aa230ebc310924b2b9ce0e14e2b0784d422b7a6ba24761a39525ace52e178`;
the recipe SHA-256 is
`625fab1445bbfd0b569b8d646c8ebd67a221db813c02f6081852603a459787e3`.
Immediately before OTA, `F2BFE0` reported a fresh 2.410 V cell, 0 mA battery
current, and qualified 4.918 V / 132 mA USB input. One declared Ben/Codex writer
uploaded only to that exact MAC. The fixture rejoined on the exact revision with
a software-reset reason, survived the full 20-second A/B pending-verify window,
and remained stable through more than two minutes of fresh heartbeats. Observed
charge held at 52-55 mA while VBAT rose from 2.411 V to 2.446 V and USB remained
4.918-4.922 V. No rollback, reboot, LED activity, or solenoid strike was seen.
Physical supervision remains required, and this `t` artifact is not fleetable.

## 2026-08-16 -- Ben + Codex -- Five dark critical fixtures recovered on supervised USB

Ben connected five non-red critical-battery lanterns to USB power on Elliot's
Mac mini while the Mac remained power-only and Ben/Codex retained sole OTA
writer ownership. The exact five recovered IDs were `9F26B0`, `9F26E8`,
`F2B7DC`, `9F266C`, and `9F2724`. Each received the existing immutable
`fx-260816-19c6bbb-b` binary (SHA-256
`a018550e90f27f1d08a08e998294fb73ba97dc5367152c2c048b5f6adeb2f395`)
through an identity-verified, sequential shared-WiFi upload. All five produced
fresh exact-revision heartbeats after the 20-second A/B pending-verify window;
no solenoid strike was sent.

`9F26B0` and `9F26E8` first reached roughly 3.05 V with strong positive input.
The deeper trio entered maintenance at 2.796 V (`F2B7DC`), 2.782 V (`9F266C`),
and 2.728 V (`9F2724`) while a stable roughly 4.8 V USB source supported the
installed LFP. `F2B7DC`'s first post-discovery HTTP connection missed before an
upload began; direct telemetry proved the old valid image and stable power, and
one identity-verified retry succeeded. After the universal image landed, the
trio's observed battery current rose from roughly 28-34 mA to +240 mA, +240 mA,
and +295 mA respectively, confirming that the 300 mA precharge policy was active
under the available USB budget. They should remain connected until voltage has
recovered comfortably above the critical range. This raises the universal
artifact's accepted observed fleet count from 32 to 37. Evidence is appended to
`ops/bench/data/ca/2026-08-16-ota-results.jsonl`.

## 2026-08-16 -- Ben + Codex -- Universal recovery and solenoid artifact deployed to 32 visible fixtures

Ben made the 300 mA BQ25628E precharge configuration and solenoid capability
universal defaults for every PowerFeather fixture image. ADR 0041 records the
decision. Solenoid policy v1 deliberately migrates the historical absent/off NVS
state to enabled once, then preserves any later explicit runtime disarm. Armed
idle remains D7/GPIO37 INPUT/high-Z, preserving the rev-1 433 MHz receiver/manual
path; a Feather without a capboard has no connected load. Strikes remain
addressed or deliberate local actions under all existing lifecycle,
solar-surplus, battery, pulse, rest, maintenance, and failsafe gates. The USER
button path remains available. `--canopy-solenoid` is now a deprecated no-op,
while `--solenoid-test` remains bench-only. All 392 native checks pass.

The immutable fleet artifact is `fx-260816-19c6bbb-b`, built from clean source
commit `bab1ea3f56f584fdde5f416a2087320e5b7a3357`. Its 1,170,736-byte binary has
SHA-256
`a018550e90f27f1d08a08e998294fb73ba97dc5367152c2c048b5f6adeb2f395`;
the recipe SHA-256 is
`19c6bbbc2e8c328846d6357308f117d51a8842373e790fbb752019f8ae9bd769`.
The manifest under `firmware/fixture/build/fx-260816-19c6bbb-b/` records channel
11, commission/basic-listener, 300 mA precharge, universal solenoid policy v1,
and no solenoid test override. Canary `F40384` directly reported the exact image,
`app1`, OTA `valid`, pending false, REG0x10 `0x00F0` / 300 mA, no BQ fault,
solenoid enabled, gate off, and zero strikes/failsafes after the A/B window.

One declared Ben/Codex writer then deployed that exact binary to 31 more devices.
All 32 accepted targets produced fresh exact-revision heartbeats after the
20-second pending-verify window: `F40384`, `9E5A74`, `9E5AB0`, `9E5B8C`,
`9E668C`, `9F0E54`, `9F0E5C`, `9F2638`, `9F2664`, `9F26AC`, `9F26BC`,
`9F26E4`, `9F275C`, `F2BE0C`, `F2BE1C`, `F2BE3C`, `F2BE48`, `F2BEE4`,
`F2BEF4`, `F3FD50`, `F40268`, `F40310`, `F40364`, `F2BF90`, `F2BF5C`,
`F3FC90`, `F2BF54`, `9E5A88`, `9F26C4`, `9E5B34`, `9F2718`, and
`9F2738`. The last five became eligible only after fresh telemetry showed safe
ride-through voltage or strong positive charging. No solenoid strike was sent.
The registry now carries the exact artifact for all 32; nine previously missing
live IDs were added with role/capacity deliberately unreconciled rather than
guessed.

The rollout did not bypass safety or identity gates. `9E5A84` and `9F26D8`
continued to send healthy uplinks but did not enter maintenance during separate
90-second sustained targeted discoveries, so no upload was attempted. `F2BE6C`
returned HTTP 500 on a sequential retry and remained safely on
`fx-260816-prtrel1-b`. Known-anomalous `F2BE20` timed out during its sequential
upload, task-watchdog reset, and remained safely on `fx-260816-8ea551a-b`.
Low or inadequately supplied fixtures `9E5A5C`, `9E5B18`, `9F26B0`, `9F26E8`,
`9F2724`, `F2B7DC`, `F2BDB0`, `F2BEA4`, and intermittently visible `9F266C`
were not updated; they still need proven USB/solar recovery before reusing this
exact artifact. Elliot's Mac mini connections remained power-only under the
declared single-writer rule. Evidence is append-only in
`ops/bench/data/ca/2026-08-16-universal-default-wave1.jsonl` and
`ops/bench/data/ca/2026-08-16-ota-results.jsonl`.

## 2026-08-16 -- Ben + Codex -- 300 mA precharge recovery validated and deployed

Ben selected 300 mA as the production BQ25628E precharge limit after the live
fleet showed the charger's 30 mA POR setting leaving 2.73-2.89 V LFP fixtures
near energy-neutral despite valid solar. Firmware now performs a two-byte
little-endian, reserved-bit-preserving read/modify/write of REG0x10, exposes the
target, decoded readback, raw register, and match result in maintenance
telemetry, and refuses to mark a pending OTA image valid unless the requested
value reads back. Trickle charge below 2.25 V, input DPM, thermal protection,
the roughly 3.0 V fast-charge transition, and the 2 A normal charge ceiling are
unchanged. All 381 native checks pass.

The first canary artifact, `fx-260816-2cdf1ab-b` (SHA-256
`1d16390f95e979c1cdced5bdf6aa9602ec3700c55220d9c19a09ac8295140ded`),
was safely rejected by the t+20 s self-test and automatically rolled back on
`9F26B0`. That retired image used an 8-bit transaction against TI's 16-bit
REG0x10 and was not reused. The corrected immutable artifact is
`fx-260816-8ea551a-b`, built from clean commit
`45c568db15c161c190f2088f9886e87b68b1c9c2`; its 1,170,528-byte binary has
SHA-256
`6e836c22634b597d052ff6dd40159c3282d107455fa416ac446dbf78bc2ddf92`.
The manifest is under `firmware/fixture/build/fx-260816-8ea551a-b/` and records
the exact 300 mA, channel-11, commission/basic-listener, canopy-solenoid recipe.

Corrected canary `9F26B0` directly reported REG0x10 `0x00F0`, decoded 300 mA,
OTA `valid`, no BQ fault, and solenoid gate off with zero strikes. Its measured
battery charge increased from 31-34 mA to 302-306 mA, panel input from about
0.47 W to about 1.5 W, and VBAT from 2.795 V to 2.803 V during the observed
window. Second low-voltage canary `F2BEA4` held 299-302 mA and about 1.54-1.58 W
after the A/B window, with VBAT moving from 2.895 V to about 2.935 V.

One Ben/Codex OTA writer then deployed the same binary to 17 more reachable
canopy targets: `F2BEE4`, `F3FC90`, `F2BE0C`, `F40384`, `9F0E54`, `9E668C`,
`F40364`, `9F275C`, `F2BF54`, `F2BF5C`, `9E5B8C`, `F40268`, `9F26E4`,
`9F26AC`, `F2BE48`, `9F26BC`, and `F2BE20`. All survived the verification
window; direct telemetry resolved `F2BF54`'s cached dashboard identity and
showed exact/valid plus 300 mA readback. `F2BE20`, handled separately because
of its prior slot anomaly, remained exact for 129 seconds and directly reported
`app0`, `valid`, REG0x10 `0x00F0`, no fault, and zero strikes. This makes the
300 mA artifact 19/27 across the exact canopy roster while preserving the
earlier 21/27 solenoid enablement.

Four additional installed, solar-fed critical fixtures from the earlier fleet
rollout were included because they showed the same precharge bottleneck:
`9E5A88`, `9E5B18`, `9F26C4`, and `9F26E8`. All four survived A/B verification.
The first three held about 299-307 mA and roughly 1.5 W input; `9F26E8` reached
308 mA when its panel was available but continued to track rapidly fluctuating
sun/input power. Its direct telemetry showed the exact valid image, 300 mA
readback, and no BQ fault.

No OTA was attempted on canopy fixtures `9F266C` (2.770 V), `F2B7DC`
(2.801 V), or `9F2724` (2.693 V) because all had zero panel input and were
discharging. `9E5A84` remained healthy on uplink but failed two targeted
maintenance bursts and a bounded 60-second discovery, so no upload occurred.
`9E5A94`, `F2BDB0`, `F2BE8C`, and `F2BF8C` remained offline. Non-roster
critical fixture `9E5A5C` was about 2.49 V and was deliberately excluded: the
firmware does not enable charging until the gauge reports a plausible cell
above 2.5 V, so it needs supervised external recovery first. Upload evidence is
in `ops/bench/data/ca/2026-08-16-ota-results.jsonl`,
`2026-08-16-precharge300-batch1.jsonl`, and
`2026-08-16-precharge300-critical-batch.jsonl`.

## 2026-08-16 -- Ben + Codex -- Canopy solenoid image built and rolled to 21 fixtures

Built the immutable `fx-260816-cef34a4-b` fixture artifact from clean source
commit `55c2302a401ef291bbb64a07999b413b60edb045`. The binary is 1,169,408 bytes,
SHA-256
`80b6086b7b6496a318d644b22c5ef9c42f9491b190e295036a546b873186ae38`;
its manifest is under `firmware/fixture/build/fx-260816-cef34a4-b/`. The image
uses the basic-listener commissioning posture and arms D7/GPIO37 solenoid
strikes under the normal lifecycle, power, rest-time, and OTA safety gates. It
does not enable the solenoid test override. Idle D7 remains input/high-Z, which
preserves the rev1 RX480E receiver and SW1 path; the local USER-button bounded
strike remains available. No strike was requested during rollout, and direct
canary telemetry showed the gate off and zero strikes.

One declared Ben/Codex writer targeted the exact 27 installed downlights over
shared `Party In The Woods` WiFi. Twenty-one fixtures have fresh exact-revision
heartbeats after the 20-second A/B verification window: `F2BEE4`, `F3FC90`,
`9F26B0`, `F2BE0C`, `F40384`, `9F0E54`, `9E668C`, `F2BEA4`, `F40364`,
`9F275C`, `F2BF54`, `F2BF5C`, `9E5B8C`, `F40268`, `9F26E4`, `9F26AC`,
`F2BE48`, `9F26BC`, `F2B7DC`, `9F266C`, and `F2BE20`. The sleeper/low-VBAT
updates were done sequentially with installed LFP ride-through; `F2B7DC` was
the low-voltage canary and remained stable after OTA. Registry rows now carry
the exact artifact identity. Upload evidence is append-only in
`ops/bench/data/ca/2026-08-16-canopy-solenoid-ota-batch1.jsonl` and
`ops/bench/data/ca/2026-08-16-ota-results.jsonl`.

`F2BE20` accepted this image twice, reported `app1`, `valid`, pending false,
and solenoid enabled, but after the first later power-state reboot direct
telemetry showed the old `app0` image again. The second OTA remained the exact
image through an 11-minute soak; an attempted five-second sleep command was not
received, so the deep-sleep boot case remains open even though current state is
counted. The remaining non-updated targets are `9E5A84` (healthy uplink but no downlink
command acceptance), `9F2724` (2.72 V, no usable panel source), and four units
not seen during the multi-hour bridge session: `9E5A94`, `F2BDB0`, `F2BE8C`,
and `F2BF8C`.

The live charging split was also characterized. Six low fixtures at
2.73-2.89 V showed valid roughly 6 V panel input but a tightly clustered
31-36 mA net battery charge and 0.42-0.52 W input, matching the previously
observed BQ25628E low-VBAT/precharge regime. Healthy roughly 3.3 V fixtures
accepted about 2.7-3.3 W and 500-750 mA. This is charger regulation, not the
LFP cell refusing current. By contrast, `9F2724`, `9F266C`, and `F2B7DC`
reported zero panel power; that is a separate source/shade/connection condition.
Direct `9F26B0` telemetry confirmed 2.751 V, +33.9 mA, 6.015 V / 78 mA input,
2 A ICHG configured, charge enabled, and no BQ fault. The BQ25628E IPRECHG
register POR is 30 mA and is programmable from 10-310 mA; the current
PowerFeather SDK does not expose it. Candidate mitigation is a direct-I2C,
readback-verified 250 mA precharge canary while preserving the fixed trickle,
input-DPM, thermal, and roughly 3.0 V fast-charge transition protections.

## 2026-08-16 -- Ben + Codex -- Final four unassigned bare boards OTA-bootstrapped

The final four known unassigned bare PowerFeathers (`9F2678`, `9E5AC8`,
`9F26B4`, and `9F2708`) passed the normal battery-absent/VDC-absent path without
rescue steps. All four passed exact `fx-260816-prtrel1-b`, 8 MB flash / 2 MB
physical PSRAM, 6 Ah / 2 A provisional configuration,
`battery_present=false`, `class_ovr=0`, channel-11 ESP-NOW, and
`Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-unassigned-bare-final4.jsonl`.

This closes Ben's current loose/unassigned bare-board pile at 54 OTA-ready
canopy/downlight-or-extra candidates across five ten-board batches plus this
final four. Their roles remain deliberately blank and 6 Ah is provisional; set
15 Ah before battery-backed validation for any unit assigned to a large
downlight enclosure. Possible forgotten boards already inside enclosures remain
a separate installed-battery rescue/census task and must not use bare-board `X`.

## 2026-08-16 -- Ben + Codex -- Fifth ten-board canopy/extra batch OTA-bootstrapped

Ten more battery-absent/VDC-absent PowerFeathers were prepared as tentative
canopy/downlight fixtures or extras: seven new boards (`9E5A58`, `9F2780`,
`9E5B04`, `9F0E7C`, `9E5B10`, `9E5AE0`, and `9E5B00`) and three reused July
fixtures (`F401A8`, `F40350`, and `F40254`). `F40350` appeared briefly and then
slept before initial batch selection, leaving seven factory-fresh units to pass
the first normal parallel path.

The bridge's `..-` onboard-status identify pattern isolated all three silent
boards. They were rescue-uploaded together in ROM download mode, then Ben reset
all ten because the three physical positions were no longer certain. The seven
completed units simply rebooted unchanged; telemetry confirmed the three targets
were running the exact image in battery-absent durable PROTECT. Their recorded
three-board retry safely cleared all latches through guarded `X`. Final result:
10/10 unique boards passed exact `fx-260816-prtrel1-b`, 8 MB flash / 2 MB
physical PSRAM, 6 Ah / 2 A provisional configuration,
`battery_present=false`, `class_ovr=0`, channel-11 ESP-NOW, and
`Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-canopy-candidates-batch5.jsonl`; all roles remain
unassigned pending final canopy/extra allocation.

## 2026-08-16 -- Ben + Codex -- Fourth ten-board canopy/extra batch OTA-bootstrapped

Ten more battery-absent/VDC-absent PowerFeathers were prepared as tentative
canopy/downlight fixtures or extras: seven new boards (`9F26D0`, `9E5B14`,
`9F26A4`, `9F26D4`, `9E5B68`, `9E5AF4`, and `9F0E30`) and three reused July
fixtures (`F402C4`, `F403F0`, and initially silent `F2BE94`). The first nine
visible units passed the normal parallel path; `F402C4` and `F403F0` safely
cleared battery-absent durable PROTECT through guarded `X`.

The bridge's `..-` onboard-status identify pattern isolated `F2BE94`, which
required ROM download mode. Its first exact upload succeeded but the application
remained in ROM mode; the append-only evidence preserves that no-telemetry
attempt. A normal RESET booted the uploaded image and the recorded retry cleared
its battery-absent PROTECT latch. The final unique-board result is 10/10 for
exact `fx-260816-prtrel1-b`, 8 MB flash / 2 MB physical PSRAM, 6 Ah / 2 A
provisional configuration, `battery_present=false`, `class_ovr=0`, channel-11
ESP-NOW, and `Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-canopy-candidates-batch4.jsonl`; all roles remain
unassigned pending final canopy/extra allocation.

## 2026-08-16 -- Ben + Codex -- Third ten-board canopy/extra batch OTA-bootstrapped

Ten more battery-absent/VDC-absent PowerFeathers were prepared as tentative
canopy/downlight fixtures or extras: nine new boards (`9E5A9C`, `9E5954`,
`F3FD60`, `9E5B48`, `9F26C0`, `9E5AA0`, `9E5B98`, `9E5AE4`, and `9F2714`)
plus reused July fixture `F40330`. The nine new boards passed the normal parallel
path. `F40330` never appeared under ordinary reset; the bridge's onboard-status
`..-` identify pattern separated the nine completed boards from the silent unit,
which then enumerated in ROM download mode.

The first `F40330` rescue upload succeeded but the application remained in ROM
mode, so the append-only evidence retains the resulting no-telemetry attempt. A
normal RESET booted the uploaded image, exposed its battery-absent durable
PROTECT state, and the recorded retry safely cleared it through the guarded `X`
path. The final unique-board result is 10/10 for exact
`fx-260816-prtrel1-b`, 8 MB flash / 2 MB physical PSRAM, 6 Ah / 2 A provisional
configuration, `battery_present=false`, `class_ovr=0`, channel-11 ESP-NOW, and
`Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-canopy-candidates-batch3.jsonl`; all roles remain
unassigned pending final canopy/extra allocation.

## 2026-08-16 -- Ben + Codex -- Published USB rescue handoff for Elliot's bench

Added `docs/howto/FIXTURE_USB_RESCUE_HANDOFF.md` as a self-contained agent
runbook for Elliot's parallel USB flashing. It pins the basic-listener branch,
version, channel/profile/feature flags, local `Party In The Woods` credentials,
build-once workflow, explicit-port batch commands, role/capacity separation,
and the per-fixture evidence gate. The failure ladder distinguishes factory-fresh
boards, transient sleeping USB ports, ROM download mode, bare-board `X`, and the
installed-battery durable-PROTECT path. It explicitly requires a 90-second powered
post-flash hold and proof of qualified release/automatic clean reboot rather than
treating upload or OTA-endpoint success alone as proof that PROTECT cleared.

## 2026-08-16 -- Ben + Codex -- Second ten-board canopy-candidate batch OTA-bootstrapped

Ten more battery-absent/VDC-absent PowerFeathers intended as likely inner-ring
canopy/downlight candidates received the exact `fx-260816-prtrel1-b` artifact:
`9E5AD4`, `F2BE70`, `F40174`, `F40358`, `F4042C`, `F2BE60`, `F2BE38`,
`F40424`, `F2BDB4`, and `F4031C`. Only the new `9E5AD4` remained continuously
visible; the other nine carried sleeping July fixture images. A reset-synchronized
USB watcher ignored the CoreS3 bridge and the already-complete new board, then
rescue-uploaded the exact artifact to all nine transient ports on first sight.

The normal recorded commissioning path subsequently passed all 10/10: 8 MB flash
/ 2 MB PSRAM preflight, exact upload, guarded bare-board PROTECT clear,
`battery_present=false`, 6 Ah / 2 A provisional configuration, channel-11
ESP-NOW, and `Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-canopy-candidates-batch2.jsonl`. All remain
unassigned with `class_ovr=0`; the sensorless `chandelier` fallback is not a
fixture allocation. Set 15 Ah before battery-backed validation if any enter a
large downlight enclosure.

## 2026-08-16 -- Ben + Codex -- Ten unassigned canopy candidates OTA-bootstrapped

Ten battery-absent/VDC-absent PowerFeathers intended as likely inner-ring canopy
lights received the exact `fx-260816-prtrel1-b` basic-listener artifact. Five
were new (`9F2684`, `9E5AD8`, `9F2688`, `9E5A70`, and `9D7884`) and five carried
the July field image (`F40380`, former fallback bridge `F2BED4`, `F2BDC0`,
`F402D0`, and initially invisible `F4044C`). The old image slept too quickly for
normal batch selection, so a MAC-targeted watcher ignored the five completed
boards and rescue-uploaded only the five old targets as Ben reset all ten. The
normal recorded commissioning path then reran on all five. Each old board's
durable bare-board PROTECT state cleared through the guarded `X` path.

All ten unique boards passed 8 MB flash / 2 MB PSRAM preflight, exact upload,
`battery_present=false`, 6 Ah / 2 A provisional configuration, channel-11
ESP-NOW, and `Party In The Woods` OTA endpoint verification. `9E5A70` had one
empty HTTP response after a good USB flash/serial check; its immediate isolated
retry passed and both events remain in the append-only evidence at
`ops/bench/data/usb/2026-08-16-canopy-candidates-batch1.jsonl`. All report
`class_ovr=0`; the sensorless bare-board probe currently falls back to
`chandelier`, which is not an allocation. Registry roles remain blank and note
the likely canopy/downlight use plus the required 15 Ah change if these become
large-enclosure fixtures. `F2BED4` is no longer reserved as a serial bridge.

## 2026-08-16 -- Ben + Codex -- Twenty-four 6 Ah trunk-light Feathers allocated and OTA-bootstrapped

The final four bare boards (`9F0E4C`, `9E5B5C`, `9F2648`, and `9E5AB0`) passed
the same battery-absent/VDC-absent commissioning gate as the preceding 10 + 10:
8 MB flash / 2 MB PSRAM, exact `fx-260816-prtrel1-b` upload, guarded bare-board
release, `battery_present=false`, 6 Ah / 2 A LFP configuration, channel-11
ESP-NOW, and `Party In The Woods` OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-trunk-allocation-batch3.jsonl`.

Ben allocated all three batches -- 10 + 10 + 4 = 24 PowerFeathers -- to the
assembly team's trunk-light pool. All 24 registry rows now use `uplight`, the
firmware/schema compatibility spelling for trunk light, with 6 Ah capacity and
installation position TBD. The NVS class override remains AUTO because these
were sensorless bare-board flashes; the assembled sensor stack can still drive
normal class probing. This is an electronics handoff allocation and does not by
itself change the still-open final installed trunk-light count/layout.

## 2026-08-16 -- Ben + Codex -- Second ten-board OTA bootstrap batch; trunk candidates

A second nominal ten-board, battery-absent/VDC-absent batch received the exact
`fx-260816-prtrel1-b` basic-listener artifact. Nine factory-fresh boards
(`F402B8`, `F2BEF4`, `F2B8DC`, `F40438`, `F2BE6C`, `F3FC8C`, `F2BF90`,
`F2BCE0`, and `F401CC`) enumerated normally and passed in parallel. The apparent
tenth factory-fresh board blinked red at 1 Hz but presented no USB device to
Windows. BOOT/download mode identified it as previously commissioned `9F2694`,
not a new board; the first bootloader window vanished before preflight and wrote
nothing, while the repeated window completed the exact upload successfully.

All ten unique boards ultimately passed flash/PSRAM preflight, exact upload,
guarded bare-board PROTECT clear where needed, `battery_present=false`, 6 Ah LFP
configuration, channel-11 ESP-NOW, and `Party In The Woods` shared-WiFi OTA
endpoint verification. The append-only evidence intentionally retains both the
no-write `9F2694` preflight failure and its later PASS in
`ops/bench/data/usb/2026-08-16-factory-trunk-candidates-batch2.jsonl`. Ben expects
these boards will probably become 6 Ah trunk lights; registry roles remain blank
until enclosure/sensor assignment is final.

## 2026-08-16 -- Ben + Codex -- Ten factory-fresh boards receive basic-listener OTA bootstrap

Ten new PowerFeather V2 boards (`F3FC9C`, `F4019C`, `F2BE3C`, `F2BEB4`,
`F40310`, `F2BD00`, `F3FD50`, `F2BE64`, `F2BE10`, and `F2BE1C`) were
USB-commissioned as a single battery-absent/VDC-absent batch. The explicitly
selected `COM77` through `COM86` ports excluded the attached CoreS3 bridge on
`COM43`. Every board passed 8 MB flash / 2 MB PSRAM preflight, exact upload of
`fx-260816-prtrel1-b` (SHA-256
`6305E9713CE1DD3FDDD37C66FEF4FAB84FB1D6782AEBD8723BB12E989243297E`),
guarded bare-board PROTECT clear, serial verification with
`battery_present=false`, channel-11 ESP-NOW bring-up, and serialized
`Party In The Woods` shared-WiFi OTA endpoint verification. Evidence is in
`ops/bench/data/usb/2026-08-16-factory10-basic-listener.jsonl`.

These are OTA-bootstrap boards rather than assigned fixtures: role is blank and
the 6 Ah LFP capacity is a conservative provisional default. At assembly, set
15 Ah for a large-enclosure/downlight board or retain 6 Ah for a small-enclosure
board, then connect battery before panel/VDC. This USB-only endpoint check does
not qualify OTA reboot ride-through; perform actual OTA/A-B tests only with the
production battery installed or another separately proven stable supply.

## 2026-08-16 -- Ben + Codex -- Factory-fresh rev-2 fixtures commissioned; physical-reset rescue corrected

Three physically dark rev-2-capboard downlights were initially suspected of a
VDC/D7 boot interaction. Read-only ROM checks instead showed healthy ESP32-S3
silicon on `9F266C` and healthy 8 MB JEDEC flash on `9F26B0`. Ben then recalled
that these builds had never received their mandatory initial USB application
flash. This fully explained why physical RESET produced no fixture light or
application serial output: there was no fixture application to run.

Factory-fresh `9F26B0` received the exact prebuilt
`fx-260816-prtrel1-b` artifact through native USB with no physical RESET/BOOT.
The full bootloader, partition table, boot app, and fixture application writes
all passed hash verification. Its 15 Ah profile, good USB supply, charging,
channel-11 ESP-NOW, downlight class, TMF8820, MSA311, LED rail, and Ben-observed
steady red all passed; its shared-WiFi endpoint was not checked before it was
disconnected. `9F266C` and `9F2724` then passed the repeatable two-board
`fleet_usb_bringup.py` path: physical flash/PSRAM preflight, parallel exact
artifact upload, 15 Ah configuration, TMF/MSA downlight checks, and serialized
`Party In The Woods` maintenance endpoint checks. Both resumed to COMMS with
ESP-NOW and LED rail on; Ben visually confirmed all three fixtures steady red.
The evidence is in
`ops/bench/data/usb/2026-08-16-rev2-darkwads-*.jsonl`; the fleet registry now
contains all three. This proves normal first flash through the enclosure rescue
USB can keep the lid closed; physical buttons are contingency-only.

The batch tool also no longer inventories every attached Espressif native-USB
device during a port-scoped commission run. CoreS3 and PowerFeather share that
VID/PID, so the old ordering could add the attached `4D5DB0` CoreS3 desk bridge
as a PowerFeather even though only `COM75 COM76` were selected. Commissioning
now adds only the selected ports; the bridge registry row is correctly labeled
CoreS3 / `serial_bridge` and remains explicitly excluded from fixture flashing.

A fourth connected fixture, `F2BF5C`, exposed the separate old-image recovery
bug with unusually clear telemetry. It ran `fx-260816-otafix1-b` with good
4.609 V supply, no charger fault, approximately +340 mA battery charge, and
ESP-NOW up. Without any connector movement it climbed PROTECT -> LEDS_OFF ->
DIM -> FULL, persisting stages 3 -> 2 -> 1, while `led_rail_on=false` and all
sensors remained uninitialized because that boot's in-RAM `park` flag never
cleared. A deliberate software reboot at recovered FULL returned with
`reset_reason=software`, `guard_interrupted=false`, LED rail on, and all three
outer-ring sensors initialized; Ben confirmed steady red.

This corrects the old rescue advice. Physical RESET is not a reliable
`otafix1-b` release: from persisted LEDS_OFF or DIM it is intentionally treated
as an unexpected reset and can return to PROTECT. Disconnecting/reconnecting
the rev-2 three-pin branch was incidentally changing reset/power sequencing,
not proving that low VDC overrode USB. Prefer USB-installing `prtrel1-b`, whose
automatic clean software reboot occurs immediately after qualified PROTECT
release. A true stage-4 hardware canary of that automatic path is still owed
before fleet OTA.

## 2026-08-16 -- Ben + Codex -- Fleet OTA reached 32/33; USB PROTECT recovery isolated and automated

After Elliot stopped the competing OTA writer, shared-WiFi maintenance completed
on `F2BE0C`, `9E5A84`, `9F275C`, `F2BE20`, `F2BE48`, `F2BEE4`, and the newly
classified lighting peer `9F26D8`. The accepted `fx-260816-otafix1-b` image now
runs on 31 of the original 32 eligible fixtures, or 32 of 33 when `9F26D8` is
included. Converted solarnoid `9E5B8C` remains deliberately excluded. The sole
known exception is `F3FD88`, still on `fixture-2026-08-06.5`: it hears the
maintenance hail and returns to ESP-NOW, but reports maintenance-start failure
and never joins `Party In The Woods`. That pre-SSID-migration image likely has
obsolete WiFi credentials and requires USB rescue. The append-only OTA record
now contains 31 successful upload ACKs. All local maintenance/OTA processes
were explicitly stopped afterward; PowerShell wrappers must be checked for
surviving child Python processes, because terminating only the visible wrapper
can leave maintenance hails running.

A USB investigation of a physically dark fixture identified COM67 as `F2BEA4`,
not a never-seen board. It already ran the accepted image and had healthy USB,
battery, charger, ESP-NOW, and sensors, but booted from durable stage 4 with
`park=1`. With its LFP installed and USB supply good, 60 seconds of sustained
qualified charging correctly persisted the release to `STAGE_LEDS_OFF`. The
old image nevertheless retained the in-RAM park flag for that boot. One ordinary
RESET then booted unparked, initialized the three downlight sensors and LED
profile, climbed the power ladder, and returned to the steady red listener.
`RESET-BOOT-RESET` was neither required nor appropriate.

The current-image rescue runbook is therefore: keep the battery installed,
connect USB, allow at least 60 seconds of healthy positive charging, then press
ordinary RESET once. Use BOOT/download mode only when normal USB CDC or the flash
tool cannot connect. To remove the extra manual reset, candidate
`fx-260816-prtrel1-b` now performs an automatic clean software reboot immediately
after the qualified release is durably persisted. A clean reboot is intentional:
the parked boot skipped the sensor-domain cold start, class probe, sensor init,
and LED profile, so merely clearing `park` in RAM would run partially initialized
hardware. All 368 native checks pass. The channel-11 commission/basic-listener
artifact is `firmware/fixture/build/fx-260816-prtrel1-b/fixture.ino.bin`
(1,169,424 bytes), SHA-256
`6305e9713ce1dd3fddd37c66fef4fab84fb1d6782aebd8723bb12e989243297e`.
Hardware canary and a real persisted-PROTECT release remain required before
fleet OTA of this follow-up.

## 2026-08-16 -- Ben + Codex -- Basic-listener fleet OTA reached 25/32; paused on competing OTA writer

Published the hardened supervised-listener firmware as commit `545b459` on
`origin/codex/basic-listener`. The deployed artifact is
`firmware/fixture/build/fx-260816-otafix1-b/fixture.ino.bin` (1,169,328 bytes),
SHA-256 `2e9946e6cabff669d48385f487c0a004b1713b05f006e51c0669f6cc25359f36`.
The rollout scope was 32 previously seen fixture IDs; converted solarnoid
`9E5B8C` was deliberately excluded from the light-only image. Never-seen roster
entry `9F26BC` was not counted.

Parallel shared-WiFi OTA plus a continuous maintenance hail caught awake and
PROTECT-cycling fixtures. Twenty-five IDs now report `fx-260816-otafix1-b`:
`9E5A5C`, `9E5A88`, `9E5A94`, `9E5B18`, `9E5B44`, `9E668C`, `9F0E54`,
`9F2664`, `9F2680`, `9F26AC`, `9F26C4`, `9F26E4`, `9F26E8`, `9F2720`,
`F2B7DC`, `F2BDB0`, `F2BE08`, `F2BEA4`, `F2BF54`, `F2BF5C`, `F2BF8C`,
`F3FC90`, `F40268`, `F40364`, and `F40384`. The append-only result record is
`ops/bench/data/ca/2026-08-15-otafix1-batch-live.jsonl`: 24 unique upload
ACKs, all recovered without a button. `F40364` was already the accepted canary;
`F2BF54` was updated by the same artifact through the wake catcher before its
per-upload row was written. Direct HTTP and/or subsequent Cambium heartbeats
confirmed the target revision; sampled maintenance boots reported OTA state
`valid`.

The fleet-wide visual proof passed. A 60-second broadcast-identify command made
the updated fixtures blink blue and expiry returned them to the steady red
listener beacon. A preceding rapid sequence of per-fixture direct frames was
not visually observed across the fleet even though bridge TX counters advanced
without failures. The single-fixture direct path had already passed on `F40364`;
bulk direct-frame timing/addressing remains a separate follow-up and was not
treated as proof.

The rollout was stopped after proving a competing OTA writer on the shared LAN.
`F2BE0C` first reported the accepted `fx-260816-otafix1-b`, then changed to
Elliot's `fixture-2026-08-15.7`. This cannot be A/B rollback because its
pre-update image was `.4`, not `.7`. During the final maintenance hail,
`9E5A84` likewise changed directly from old `.2` to `.7`. Newly observed,
unrostered `9F26D8` also reports `.7` and must be classified before inclusion.
The maintenance endpoints have no writer authentication, so exposing fixtures
on `Party In The Woods` lets another laptop with a pending OTA job overwrite
them. All local catcher processes were stopped, every responding fixture was
sent `/resume`, and repeated radio resume commands returned the fleet to COMMS.
No local OTA job remains active.

Seven of the original 32 remain off the target image: `.7` now runs on
`F2BE0C` and `9E5A84`; old images remain on `9F275C`, `F3FD88`, `F2BE20`,
`F2BE48`, and `F2BEE4`. Resume only after the other Cambium/OTA daemon is
stopped or otherwise isolated. This is an OTA-writer coordination problem,
not a reason to add control-frame leasing; ordinary lighting remains
deliberately leaseless.

## 2026-08-15 -- Ben + Codex -- Mode-aware OTA verification passed good-image and forced-rollback canary gates

Fixed the maintenance-mode rollback found immediately below. The deferred OTA
self-test now validates the network path owned by the active mode: COMMS requires
ESP-NOW up plus at least one completed send; MAINT requires an associated WiFi
station plus the active OTA HTTP server. The predicate lives in platform-neutral
`ota_verify_policy` and has a six-case native transition test. Telemetry now
reports `ota_partition`, `ota_address`, and string `ota_state` in addition to the
legacy pending boolean, so an identical-version rollback cannot hide behind the
firmware string. The full native suite passes 368 checks with zero failures.

Built channel-11 commission/basic-listener good artifact
`firmware/fixture/build/fx-260816-otafix1-b/fixture.ino.bin` (1,169,328 bytes),
SHA-256 `2e9946e6cabff669d48385f487c0a004b1713b05f006e51c0669f6cc25359f36`.
OTA from the last-good `railoff-b` image to canary `F40364` proved the complete
acceptance transition on `app1`: `pending_verify` at 17.7, 18.8, and 19.8 seconds,
then `valid` at 20.9 seconds with no reboot through 30.2 seconds. This occurred
in MAINT with ESP-NOW intentionally down, proving the new mode-aware predicate.
The LED rail stayed on and the installed LFP held about 3.34 V; USB/VBUS remained
present at about 4.65 V for maintenance access.

Then built distinct forced-failure artifact
`firmware/fixture/build/fx-260816-otafail-b/fixture.ino.bin` (1,168,816 bytes),
SHA-256 `b4c0f42e1fe7b42c5040894854f9b1bb1398e78d7ac5cb28bac7fa722735ac1f`,
with `RES_OTA_FAIL_SELFTEST=1`. It ran on `app0` as `pending_verify` at 19.85
seconds, deliberately rejected itself, and rebooted. Direct telemetry then
proved recovery to the accepted `fx-260816-otafix1-b` on `app1`, state `valid`,
with ESP-NOW up, rail on, healthy battery/power/sensors, and no pending verify.
Cambium independently showed `F40364` online in COMMS with the accepted revision.
The source version was restored to `otafix1-b` after building the deliberately
bad artifact. This closes the four-gate canary sequence; a battery-only/no-VBUS
field-path OTA remains a separate useful rehearsal, not a prerequisite for this
maintenance-mode bug fix.

## 2026-08-15 -- Ben + Codex -- Basic-listener canary passed control and rail gates; good OTA rolled back in maintenance

Canary `F40364` passed the supervised basic-listener checks. Targeted Cambium
direct frames produced black, green, blue, and dedicated-white output, and the
fixture returned to steady red after the three-second stale-command window.
Bridge delivery reported 77 successes and zero failures. A diagnostic hole was
found before the rail-cycle gate: `L0` only disabled the smoke render, so the
basic listener's red fallback immediately kept the rail powered. `L0` now sets a
RAM-only forced-off override and cuts the rail; `L1` clears it and resumes the
smoke render. Hardware telemetry and Ben's visual checks proved rail off, then
rail on with white breathing still working after the cycle. A reset cleared the
override and returned directly to steady red with no boot salute.

All 362 native checks pass. The exact channel-11 commission/basic-listener
artifact is `firmware/fixture/build/fx-260816-railoff-b/fixture.ino.bin`
(1,168,672 bytes), SHA-256
`81841e4839c0342b9af7b444005075070cb76600675977d8430452559e54bca2`.
USB flashing to the positively identified COM46 / `F40364` verified every
segment. Post-flash telemetry showed the expected revision, healthy battery and
USB supply, all three downlight sensors healthy, ESP-NOW up on channel 11, and
the LED rail on.

The good-image OTA gate exposed a deterministic verifier bug. With USB present,
the OTA reboot correctly entered maintenance mode on `Party In The Woods` at
`192.168.1.148`; maintenance disables ESP-NOW. Telemetry showed the new slot
`PENDING_VERIFY` at uptimes 17.7, 18.8, and 19.8 seconds. At the 20-second
self-test the fixture software-reset, then returned with pending false. This was
a rollback: `ota_verify.cpp::selfTest()` unconditionally requires
`espNowUp() && espNowSendOk() > 0`, an impossible condition while the valid
maintenance-mode boot deliberately has ESP-NOW down. The battery remained about
3.33 V and USB about 4.65 V, so this was not a ride-through failure. Because the
test intentionally OTA'd the identical binary, the version string stayed the
same across both slots and would have hidden the rollback without the pending
state, uptime, and reset-reason trace. Make OTA verification mode-aware and add
running-partition identity to telemetry before declaring the A/B gate passed.

## 2026-08-15 -- Ben + Codex -- Basic linear listener artifact built; hardware canary pending

After full-bright boot salutes were followed by apparently dark fixtures on the
rig, inspection found that the intended red listener level 24 was gamma-mapped to
wire value 1. Ben called for a supervised return to basics. The optional listener
posture now has exactly one autonomous visual state: steady red at level 128. A
bridge lease overrides it and stale direct control returns to red within three
seconds. Removed gamma correction from the physical LED output and removed the
boot salute, supply-dependent RGB carousel, identity pop, and local ToF color
reaction. Channel values are now linear 0..255; 128 is the dim reference and 255
is the 8-bit bright endpoint. Hard battery, rail, solenoid, and OTA rollback
protection remains unchanged.

Renamed the opt-in build posture to `--basic-listener`; the old
`--quiet-autonomy` spelling is a compatibility alias for the same minimal code.
The `tools/ops` artifact parser now recognizes both legacy manual revisions and
the new `fx-YYMMDD-<recipe7>-<class>` form. Native firmware validation passes 362
checks with zero failures. Built channel-11 commission artifact
`firmware/fixture/build/fx-260816-f2bb4cd-b/fixture.ino.bin` (1,168,528 bytes),
SHA-256 `c792d8c28e8a9c57a0e19455394a5b19c161030cdcf016d741001b672797f965`.

The first USB attempt exposed a target-identification failure. Windows arrival
time was incorrectly treated as device identity and COM43 / `4D5DB0` was flashed
with the fixture artifact. Its repeated `Board.init()` and rail-pad failures were
correct safety behavior because COM43 is the CoreS3 desk bridge, not a
PowerFeather. The unchanged `RESONANCE BRIDGE` screen was a stale framebuffer:
the fixture image never initialized or cleared the CoreS3 display. Historical
logs showed that this hardware had once been the Module Audio bridge, but the
current-session record and display posture established that it had since been
repurposed as the Cambium binary bridge. Rebuilt and restored the current
channel-11 Cambium artifact; USB hash verification passed and `cambium doctor`
then proved `cores3-cb-0.1`, MAC `80:45:6B:4D:5D:B0`, channel 11, plus 14 live
fixture heartbeats.

The actual lantern was identified by a physical-reset/uptime correlation and
live telemetry as COM46 / `F40364`, then USB-flashed with the basic listener.
Every flash segment verified. Fresh serial telemetry proved the exact revision,
PowerFeather ready, battery present and charging, ESP-NOW up on channel 11, LED
rail on, all MSA311/TMF8820/BMP581 sensors healthy, and
`ota_pending_verify=false`. Human confirmation of the steady linear-red idle
state passed after USB was unplugged. A targeted Cambium color-identify then
proved bridge control: after waiting for COM43's status handshake, the bridge
radio-success counter advanced from 0 to 1 with zero failures and only `F40364`
changed from red to solid blue. Future multi-device USB work must identify
targets by live firmware/hardware identity or a physical-reset uptime
correlation, never COM arrival time alone.

The first targeted-blue attempt also exposed a separate host-side Cambium bug.
The older one-shot ops CLI starts its asynchronous serial connection and sends
immediately; `SerialCobsTransport` deliberately drops while disconnected, but
the CLI still prints a success-looking line. Holding one connection open until
the bridge STATUS handshake made the same targeted command work. Fix the CLI to
gate one-shot mutations on bridge readiness and report a dropped/not-ready send
instead of claiming success. Fixed this against Elliot's latest
`lighting/dev-mode-endpoints` branch and published as
`origin/codex/serial-ready-gate`. The first fix (`f1cb699`) correctly required a
fresh STATUS, adopted the real bridge ID before building the packet, and waited
for the USB writer to drain, but hardware proved writer-drain alone was not
enough: closing COM43 could still interrupt/reset the CoreS3 before its ESP-NOW
callback. The complete fix (`078071c`) holds the port until STATUS reports an
incremented `tx_ok` or `tx_fail`, fails loudly on a missing/failed radio outcome,
and only then prints success or closes. The full current Cambium suite passes
349 tests with 1 skipped. Live proof passed: the committed one-shot CLI reported
radio success and Ben confirmed only `F40364` changed from red to solid blue.
## 2026-08-16 -- Ben + Codex -- STEMMA class identity corrected and dashboard tail retained

Ben clarified the physical fleet signatures: TMF8820/TMF8821-family means
canopy/downlight; otherwise VL53L5CX means perimeter; otherwise MSA311 means
trunk/uplight; otherwise no STEMMA sensors means chandelier. Chandelier power is
currently absent, so no live chandelier classification is expected. Recorded the
contract in ADR 0041 and updated the canonical system table and onboarding docs.
BMP581 is explicitly environmental-only and no longer classifies an uplight; ADR
0034 already assigns it to the outer 24 downlights.

Corrected the fixture decision table and its sensor-death guard. A remembered
downlight/perimeter that loses its ToF but still sees MSA311 now keeps its prior
class and raises `class_mismatch`, and any remembered sensored fixture that loses
all class sensors behaves the same way. New MSA311-only fixtures learn uplight;
new no-sensor fixtures learn chandelier. A lone BMP581 runs the safe chandelier
profile for that boot but is flagged and not persisted. Explicit overrides still
win while reporting disagreement.

Also fixed the dashboard parser so a short heartbeat cannot erase class and LED
render fields from the latest rich heartbeat. All 382 native fixture checks and
six dashboard tests pass, as does Python bytecode compilation. Restarted the live
COM43 dashboard with the parser fix; it saw 47 cached / 43 fresh peers at the
post-restart check, but zero published a class. Their rounded-square glyphs are
therefore accurate unknown/legacy state until a named fixture artifact carrying
the class tail is separately built, validated, and intentionally deployed under
ADR 0040. No fixture firmware was built, flashed, or OTA'd in this change.

## 2026-08-16 -- Ben + Codex -- Fleet strike, class glyphs, and independent LED color bar

Enhanced `ops/bench/net_bench_dashboard.py` without changing the mesh wire format or
rebuilding firmware. With `All` selected, the D7 control now queues one individually
addressed `K<id>:<ms>` command for every fresh fixture after an explicit confirmation.
The server rechecks freshness, rejects broadcast-like targets and out-of-range pulses,
deduplicates IDs, caps the batch at 192, and serializes writes with an 80 ms gap. This
preserves the targeted-strike contract; fixtures with `sol_en=0` ignore the request and
the local lifecycle, power, pulse, rest-time, and mechanism gates still decide whether
an enabled solarnoid fires.

The fleet glyph now uses the already-reported fixture class (normally from the Stemma
probe, with an override available): circle for canopy/downlight, hexagon for
perimeter, triangle for trunk/uplight, diamond for
chandelier, and rounded square for unknown/legacy telemetry. Replaced the ambiguous
battery-colored nub/bottom glow with a dedicated top LED-output bar. The bar uses the
reported post-cap/post-gamma RGBW output, shows dark for a reported-off light, and uses
a hatch when render telemetry is unavailable; battery health remains confined to the
center fill and outline.

Five Python regression checks and Python bytecode compilation pass. Browser QA against
synthetic peers verified all shapes, independent red/blue/orange/purple LED bars,
enabled all- and single-target strike controls, and zero console errors. The strike
endpoint was exercised only against a visual fixture server with no serial handle; no
real bridge or fixture received a strike.

## 2026-08-16 -- Ben + Codex -- Two CoreS3 dashboard bridges cloned and Elliot handoff packaged

Ben declared his existing CoreS3 `4D5DB0` as the source and authorized USB
flashing the other two attached bridges to the same normal dashboard firmware.
Read immutable chip MACs before writing: COM40 was
`44:1B:F6:E3:9F:1C` / `E39F1C` and was running Cambium binary mode; COM133 was
`44:1B:F6:E3:9A:34` / `E39A34` and emitted no dashboard text. Reused the exact
already-built `dashboard-tail-20260816-r1` artifact installed on `4D5DB0` rather
than rebuilding: 1,102,288 bytes, SHA-256
`b912a88281300038aa81ba9991c931bba2e9eec214582500b902db1d928e706d`,
FQBN `esp32:esp32:m5stack_cores3`, flags `-DNB_CHANNEL=11`.

Sequential uploads to only `E39F1C` and `E39A34` verified every written region.
Fresh post-reset serial evidence from both reported
`cores3-bridge-2026-08-15.1`, their expected bridge IDs, channel 11, and live
`nb-peer` fleet rows. A separate dashboard smoke on COM40 served the state API,
identified bridge `E39F1C`, and parsed the live fleet; it was then stopped. No
fixture, fixture NVS/profile, or OTA state was changed. The existing COM43
dashboard process remains waiting for `4D5DB0`; COM43 physically disappeared
from Windows after the bridge work and will reconnect automatically when the
device is present again.

Packaged an Elliot-ready dashboard handoff at
`firmware/cores3_bridge/build/elliot-fleet-dashboard-20260816-clean.zip`. It
contains the single-file Python dashboard, parser regression test, pyserial
requirement, platform-neutral start instructions, exact matching bridge binary,
build options, and binary checksum. The clean ZIP SHA-256 is
`00897a65f84880a69070133a975665cd9469a2c4b2c5732a513898aee2e4f456`;
all three parser checks pass. Per the project coordination rule, no external
message was sent to Elliot; Ben can relay this package and the prepared launch
note.

Ben then selected Git as the durable Elliot handoff. Committed the dashboard and
matching wire/fixture/CoreS3 telemetry implementation as `15a318c` on
`codex/commissioning-mode`; the operator docs and session record follow in the
next commit on the same branch. Generated bridge binaries remain ignored rather
than being added to source history. Elliot's agent can build the normal bridge
with `firmware/cores3_bridge/build.sh --channel 11` and a unique named build path.

## 2026-08-16 -- Ben + Codex -- CoreS3 restored to serial bridge and fleet dashboard launched

Ben authorized replacing the only attached ESP32's Cambium binary image with the
normal channel-11 CoreS3 serial bridge so the new fleet dashboard could run. The
explicit target was COM43 / USB serial and chip MAC `80:45:6B:4D:5D:B0`. Uploaded
the already-compiled `dashboard-tail-20260816-r1` artifact without rebuilding:
1,102,288 bytes, SHA-256
`b912a88281300038aa81ba9991c931bba2e9eec214582500b902db1d928e706d`,
FQBN `esp32:esp32:m5stack_cores3`, flags `-DNB_CHANNEL=11`. Esptool verified every
written region and hard-reset the bridge on COM43.

Post-flash fresh evidence showed `cores3-bridge-2026-08-15.1`, bridge ID `4D5DB0`,
channel 11, and live ASCII `nb-peer` rows. Launched
`ops/bench/net_bench_dashboard.py` against COM43 on localhost port 8765 and opened
it in the Codex browser. The first API check saw 20 fixtures; the opened grid had
grown to 26 fixtures (12 healthy, 9 attention, 5 silent, 17 with input) as sleeping
peers woke. No fixture was flashed and no fixture/NVS/profile state was mutated.

## 2026-08-16 -- Ben + Codex -- Fleet-at-a-glance ESP-NOW dashboard

Reworked `ops/bench/net_bench_dashboard.py` from a solar-bench-first console into
a fleet-health landing view while preserving the existing controls, plots, table,
and raw serial console under collapsed detailed diagnostics. Each fixture is now a
compact composite glyph: an ADR 0023 load-compensated battery fill, live input icon,
actual rendered light-color foot, adaptive heartbeat fade, and a two-digit MAC
suffix. Colliding suffixes become deterministic compact labels such as `DC-1` and
`DC-2`. Selection reveals the full ID and exact voltage/current/link/output state.

The freshness model follows the declared firmware posture rather than applying one
timeout to every fixture: commission expects the 1 Hz heartbeat; normal field peers
expect 5 s; sleeping day-charge and PROTECT peers receive their known 315 s and
900 s windows. Panel-loss highlighting requires daylight consensus from comparable
panel-bearing peers, so night is not painted as fleet failure. Charger telemetry
does not yet prove USB vs panel universally; the UI labels the input honestly and
uses fixture class only to choose a sun or external-power glyph. A TODO records the
remaining explicit source-discrimination qualification.

Extended the canonical append-only `NbHeartbeat` tail with fixture class, LED-rail
state, actual post-cap/post-gamma rendered RGBW average, and lit-pixel count. Fixture
commission builds send this full truth every 5 s without bloating the 1 Hz short
heartbeat; field builds retain the 60 s full cadence. The CoreS3 bridge length-gates
and publishes both the existing lifecycle tail and the new render tail. No second
packet definition was introduced.

Parser compatibility tests, embedded JavaScript syntax checks, 371 fixture native
checks, and 11 CoreS3 audio checks pass. Fresh sequential Arduino builds also pass
for the channel-11 commission fixture (35 percent flash, 18 percent RAM) and CoreS3
bridge (35 percent flash, 25 percent RAM). Desktop and 360 px browser QA passed; the
grid stayed readable with 60 simulated fixtures, late/silent/critical/input states,
colored output, and suffix collisions. No fixture was flashed and no shared firmware
artifact was published in this source/build-only session.

## 2026-08-15 -- Ben + Codex -- Listener posture accepted; artifact identity and shared-bench handoff defined

Peer review of Elliot's pushed `Lighting-Controller` branch established that
`b047986` contains Ben's full `d4b1405` commissioning hardening and the exact
NeoPixel/RMT rail-cycle fix. Only the fixture build wrapper and behavior glue
differ afterward. Elliot's additions intentionally change the assembly posture:
no-command fallback is low red, a subdued MAC-derived pulse aids identity, and
a fresh/confident close ToF target shows the fixture's signature color. Ben
accepts the low-red and basic ToF behavior as the normal commission-listener
experience. ADR 0039 supersedes only ADR 0038's no-command-dark default; strict
commission-dark remains an explicit rail-cycle diagnostic, and no-command still
means no autonomous show.

Corrected the apparent `.4` rollback. Read-only partition inspection had shown
two valid OTA slots and later user coordination confirmed that Elliot's separate
bridge accidentally OTA'd Ben's attached `9E5A94`. It was not evidence that
Ben's `.4` failed A/B verification. The incident exposed a separate release
problem: Ben's strict image and Elliot's listener/presence image both advertised
`fixture-2026-08-15.4` despite different source, flags, toolchains, and binary
hashes. That legacy counter is retired for new shared builds.

ADR 0040 and `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md` now define generated
`fx-YYMMDD-<recipe7>-<variant>` revisions, immutable manifests, exact binary
SHA-256, explicit target-MAC callouts, and single-operator ownership for OTA/NVS
mutations while allowing intentionally leaseless live color control. They also
record that Cambium's current OTA completion can accept a cached 30-second
`online` state before any fresh rejoin or the fixture's 20-second A/B decision;
fresh expected-version evidence is required before calling an update complete.

Elliot currently owns the boot-salute firmware change, so this session did not
edit fixture source. The docs distinguish an automatic USB-only liveness salute
from a host-triggered final completion salute after artifact, identity,
profile/channel, power, class/sensor, and pending-verify checks. Firmware core
tests passed 368 checks; Elliot's controller passed 367 tests with 4 skipped;
Cambium passed 343 with 1 skipped, but the quiet ESP32 glue and OTA job runner
still lack direct coverage.

## 2026-08-15 -- Ben + Codex -- NeoPixel RMT rail-cycle bug isolated and fixed; `.4` canary reverted before acceptance

The new fixture image repeatedly reported a healthy LED rail and active smoke
render while two known-good 4 W RGBW modules remained dark. Physical elimination
was conclusive on fixture `9E5A94`: the PowerFeather locator identified the exact
unit, the RGBW connector measured 3.3 V from V+ to GND under load, and swapping the
module did not change the symptom. A temporary USB diagnostic then powered the
rail and held A0/GPIO10 statically HIGH; the external module lit red and shut off
when the probe pulled data LOW and cut the rail. Power, ground, module, connector,
signal conductor, and GPIO10 were therefore all functional.

Root cause was the rail-off fail-safe sequence interacting with Arduino-ESP32 3.x
and Adafruit_NeoPixel. After `show()` first claims GPIO10 through RMT, fixture
`ledRailOff()` called `pinMode(GPIO10, OUTPUT)` to park data LOW. Arduino-ESP32
3.x's peripheral manager clears the previous pin bus in `pinMode()`, detaching RMT.
Adafruit_NeoPixel retains a private static `rmtPin == 10`, so the next `show()` on
the same pin skips `rmtInit()` and sends into a detached peripheral. Static GPIO
HIGH still works, exactly matching the red-probe result. This can affect any of
Elliot's or Ben's sketches that mix NeoPixel `show()` with later `pinMode()` calls
on the same data pin, especially around switchable LED-rail cycling.

Fixed `fixture` by leaving RMT attached after its first `show()`. The all-off frame
is sent before rail cut and RMT's end-of-transmission level remains LOW; plain
GPIO parking is used only before RMT has ever claimed the pin. The temporary
static-HIGH diagnostic was removed from the production image. The exact fix is in
`led_driver.cpp`, and the general rule is now prominent in
`firmware/POWERFEATHER_NOTES.md`.

All 368 native checks pass. Named channel-11 commission artifact
`firmware/fixture/build/commission-rmt-railfix-20260815-r1/fixture.ino.bin`
(`fixture-2026-08-15.4`) is 1,170,320 bytes, SHA-256
`e4b0efaff0dcd93b3c36ab6e12dd5a1c21b45be1ad4e5269c381eb600c78de2a`.
USB flashing to `9E5A94` verified every segment hash and the board initially
reported `.4`, but before the two-cycle visual test could be accepted it booted
the older `fixture-2026-08-10.2` A/B slot and resumed matched red direct frames.
Therefore the source-level RMT diagnosis is strong and the fix is published for
review, but the hardware regression remains open: hold `.4` past pending-verify,
then prove `L1` breathe -> `L0` rail cut -> `L1` breathe again.

## 2026-08-15 -- Ben + Codex -- Bridge-authoritative commissioning firmware built; canary pending

After deployment began exposing the cost of long sleeps, autonomous fallback,
and a PROTECT state that could require manual intervention, split the fixture's
existing runtime profile into two operator-legible postures without changing its
wire/NVS values. `PROFILE_DEV` (0) is now **commission**: bridge-authoritative,
continuously reachable during ordinary operation, no inferred dusk/dawn or
autonomous choreography, and hard dark with the LED rail off when the bridge
lease/direct frames expire. `PROFILE_PROD` (1) is now **field** and retains the
autonomous solar/energy behavior. Local battery/thermal/actuator safety remains
authoritative in both modes. The architectural contract and rollout gates are
recorded in ADR 0038.

Fixed the immediate PROTECT release deadlock: the core policy now remains awake
while the compound recovery condition accumulates its required 60 seconds.
Commission mode also stays awake on a verified good external supply; a truly
critical battery-only peer uses a 60-second retry rather than the field
15-minute cadence. This makes PROTECT recoverable without pretending software
can revive a board with no usable power. A powered PROTECT fixture should still
timer-wake and announce at roughly 15-minute intervals under the old field
image, so a peer absent after a full roughly 16-minute census is more consistent
with BMS cutoff/no power/range than with PROTECT alone.

Added `PROG_COMMISSION_DARK`, live profile-fallback switching, hard-cut direct
frame semantics, stale-frame dark fallback, and suppression of peer autonomous
choreography TX in commission mode. Added persistent normal-CoreS3 controls:
`F0`/`F1` for broadcast commission/field and `F<id>:0|1` for a targeted peer.
User-facing serial/config/telemetry labels now say commission/field while the
stable numeric values remain 0/1.

Verification is software-complete but hardware deployment is deliberately not:
368 native fixture checks pass. Named channel-11 commission artifact
`commission-rescue-20260815-r2/fixture.ino.bin` built at 1,169,040 bytes, 34%
flash / 18% RAM, SHA-256
`a0dc8334a3f8025acee125782c18fdd7af700014163d5c2f39eda0e749527fd1`.
The matching normal bridge artifact `commission-controls-20260815-r1` also
builds (35% flash / 25% RAM). At this checkpoint neither artifact had been
flashed; the later `.2` through `.4` USB canary work is recorded immediately
above. The attached CoreS3 remains in Cambium binary mode. Next action is one
battery-backed canary, then four/five, then the 24-unit fleet only after the ADR
0038 gates pass.

## 2026-08-15 -- Ben + Claude -- Camp network pinned to channel 11; Claude mesh bridge recorded as direction only

Codified a chat-drafted design brief for a Claude-backed handheld mesh bridge
after reconciling it against the actual firmware. The brief's central insight
holds and was worth capturing: the ESP32-S3 has one 2.4 GHz radio, so WiFi STA
and ESP-NOW must share a channel, and in STA mode the access point picks it.
Every bridge built so far dodges this by never associating -- `cores3_bridge`
stays unassociated by design, and fixtures in OTA maintenance have already left
ESP-NOW. Any device that wants the mesh and the internet at once cannot dodge it,
and an auto-channel AP would silently deafen it with no error and no log line.

**ADR 0036 (accepted, router ordered but not received):** the camp AP is pinned
to channel 11, HT20, WPA2-PSK, on a dedicated 2.4 GHz SSID, fed by Starlink in
bypass mode through a GL.iNet Beryl AX. Any Resonance device that associates
while using ESP-NOW must read the actual channel after association and, on
mismatch, **drop WiFi and keep the mesh**. The original brief had this inverted
(refuse to start mesh TX/RX); the mesh is the primary function and Claude is the
enhancement, so the priority is the other way around. Maintenance-mode fixtures
are explicitly exempt. Added `docs/howto/CAMP_NETWORK_SETUP.md` with the home
rehearsal, field checklist, and troubleshooting table -- the Starlink bypass
switch needs a factory reset to undo, so it is rehearsed at home, not improvised
in dust.

**ADR 0037 (proposed only):** the handheld itself. No hardware procured, no
firmware written, no board chosen; post-2026-event unless Ben re-prioritizes.
Six corrections to the brief are recorded in
`docs/research/CLAUDE_MESH_BRIDGE_DESIGN_2026-08-15.md` section 9 so the
reasoning is not lost. The two that matter most: the brief's "task one" of
extracting a shared `mesh_protocol.h` is **already done** -- `packet.h` is the
platform-independent contract with golden layout pins that 24 commissioned
fixtures and all host tooling parse, so a second header would fork the fleet
contract; and the proposed promiscuous-mode sniffer is largely redundant, since
all fleet traffic is broadcast (targeting is a 3-byte `target_id` inside the
payload) and `esp_now_recv_info_t.rx_ctrl->rssi` already gives per-packet RSSI
on the ordinary receive callback that `cores3_bridge` already uses for 192
peers. Also corrected: no group addressing exists on the wire (the brief's
`north/south/east/west/canopy` is invented -- real addressing is a 3-byte short
ID with `00:00:00` for all, plus four self-detected classes), the fleet is about
130 fixtures not 150, the solenoid clamp is 5-300 ms with a 40 ms default and
80 ms coil rest rather than "20-30 ms", pure ESP-IDF would fork the arduino-cli
toolchain for one device, and `claude-sonnet-4-6` is still valid but
previous-generation. Two embedded API gotchas were added that the brief missed:
thinking is on by default on current models and `max_tokens` caps thinking plus
response text together, so a 1024-token budget can truncate mid-answer.

The unauthenticated-ESP-NOW problem is recorded as inherited, not new -- it is
the same open item already tracked against the Atom clicker work, and a lanyard
device that can command the fleet widens that surface rather than creating a
second one.

**Hardware facts corrected same session, from Ben.** Starlink is Gen 3 + Gen 4
(possibly all Gen 4), so Ethernet is built in and no Starlink Ethernet Adapter is
needed -- that was the only lead-time-sensitive item in ADR 0036 and it is now
closed. The GL-MT3000 is ordered. Handheld hardware is **already on hand**:
2x LilyGO T-Deck and 1x M5Stack Cardputer ADV, so ADR 0037's "no hardware
procured" framing was wrong and is corrected; board choice is settled as T-Deck
first (two units means a spare, plus the larger display and better keyboard),
Cardputer ADV port after, behind the display/input HAL.

One verification worth recording because it cuts both ways: LilyGO's **T-Deck
Pro** is a genuinely different device -- 3.1 in e-paper, CST328 touch, TCA8418
keypad controller -- and had the units been Pros, streamed chat would have needed
paragraph-boundary repaints instead of per-delta rendering. Ben confirmed the
units are the **LCD** T-Deck, so the original brief's board table is correct as
written and no UI redesign is required. The Pro/LCD distinction is now recorded
in `AGENTS.md`, the ADR, and the brief so nobody ports from the wrong driver set
after reading a spec page.

**Variant resolved: the T-Decks are Plus.** Confirmed parts: ESP32-S3FN16R8,
8 MB PSRAM / 16 MB flash, 2.8 in ST7789 IPS 320x240 with GT911 capacitive touch,
BlackBerry keyboard on an ESP32-C3 aux MCU over I2C, trackball, SX1262 LoRa as
standard, a GPS receiver, and a bundled 2000 mAh battery. Two consequences worth
recording beyond the spec list:

*Runtime is now a real design constraint.* The Plus is untethered, but this repo
already measured an always-on ESP-NOW peer at roughly 168 mA / 0.55 W on an
ESP32-S3, radio-RX-dominated (LOG 2026-06-08 -- the finding that made deep sleep
mandatory for fixtures). A handheld is by design an always-on receiver plus a
backlit IPS panel plus periodic TLS, so continuous census will not last a night
on 2000 mAh. Added as a milestone 0 measurement alongside the sun-readability
check. The design consequence: duty-cycling the radio buys runtime but costs
census completeness, so the census must distinguish "this node was quiet" from
"I was not listening" rather than letting the two look identical.

*The GPS is a genuine ADR 0031 adjacency, deliberately not adopted.*
`NB_TIME_QUALITY` (type 20) is already defined and parse-stubbed in `packet.h`
with a `source` field that includes `bridge`, so a battery-powered handheld that
knows UTC could act as a walking time anchor beside the four purchased SAM-M8Q
and four DS3231 anchors. Recorded as an opportunity to evaluate after the core
device works, and explicitly excluded from ADR 0031's production path -- a show
clock that depends on someone carrying a handheld is not a clock. The SX1262
LoRa is out of scope entirely; the fleet link remains ESP-NOW on channel 11.

The real display risk on this hardware is not refresh rate but direct-sun
readability of a 2.8 in IPS through sunglasses, now a milestone 0 check rather
than a milestone 5 acceptance criterion -- the cheapest available falsification
of the whole concept.

No firmware, hardware, or fleet state changed. `AGENTS.md` gains the channel
rule and a one-wire-contract gotcha; `TODO.md` gains a camp-network section.

## 2026-08-14 -- Ben + Codex -- CoreS3 audio-reactive operator guide

Added `docs/howto/CORES3_AUDIO_REACTIVE.md` as the durable operating guide for
the accepted CoreS3 + Module Audio + Rode VideoMic NTG setup. It records the
selector-B and LINE/MIC hookup, recommended mic settings, all relevant Rode
controls, bridge display interpretation, adaptive two-second calibration,
channel-11 and lifecycle requirements, three-second autonomous fallback, the
three accepted perimeter fixture IDs, tuning recipes, troubleshooting, and the
2026-08-06 hardware baseline. Linked the guide from the root and bridge READMEs.
No firmware, persisted fixture state, or architectural decision changed.

## 2026-08-13 -- Ben + Codex -- PUCA performance-audio bridge documented on arrival

Recorded the newly arrived performance-audio setup so `PUCA` is no longer
conversation-only context. The primary source bridge is an Ohmic PUCA DSP
Original Edition on its 6 HP Eurorack expansion, installed in a powered 4ms
Pod20 and paired with the RODE VideoMic NTG + WS11. The already-owned CoreS3 +
Module Audio stacks remain the independent fallback and the only currently
implemented audio-reactive bridge.

Added ADR 0035, glossary/agent/top-level pointers, the procurement note, and the
detailed hardware/bring-up record at
`hardware/puca-audio-bridge/README.md`. The record separates received hardware
from unfinished firmware: the factory oscillator/effect image is not Resonance
firmware. First light should reuse the proven 10 Hz `NB_DIRECT_FRAME` path on
channel 11 plus the fixture's three-second stale fallback; raw-audio transport
and a new feature-packet contract are explicitly out of the initial milestone.
Open work now includes Original-edition/ribbon/control checks, exact RODE input
and gain calibration, a PUCA-specific firmware target, mixed-fleet proof, and
closed-Pod20 RF/PDR and soak validation.

## 2026-08-11 -- Ben + Codex -- Origin synchronized; printable gobo baseline pending Steve source

Fetched origin and reconciled the eleven incoming Cambium, channel-migration, and
audio-reactive commits with the newer local production-bench commit. Rebased the
local commit over `origin/main` at `9312973`; preserved its pre-rebase state on
`codex/pre-origin-sync-20260811`. The combined native verification passed all 295
fixture checks and all 11 CoreS3 audio-reactive checks.

Audited all reachable Git objects and remote branches for Steve's successfully
printed "Resonance Tree" gobo. No native CAD, mesh, or vector source for that gobo
has landed yet. The only current gobo geometry remains the two earlier 50 mm
bamboo-leaf SVG prototypes; their approximately 0.9-1.1 mm stem-slot note is not a
validated structural limit.

Recorded the production calibration rule in `enclosure/gobo-templates/README.md`
and the follow-up in `TODO.md`: once Steve's source lands, separately measure the
thinnest connected solid web, narrowest surviving aperture, and Z thickness, and
record the printer process variables. The proven solid-web width will become the
empirical lower bound for generated patterns after a calibration coupon establishes
the production margin. No numeric production threshold was locked in this session.

## 2026-08-11 -- Ben + Codex -- Outer-ring 24 hardware and firmware record locked

Recorded the completed first production group as 24 **large-enclosure outer-ring
downlights**. Every unit has the TMF8820 -> MSA311 -> BMP581 sensor chain, a
rev-1 capboard, and the boosted cable retrofit: the external approximately 12 V
boost is spliced into the capboard input VDC/GND pair. All 24 received the locked
`fixture-2026-08-10.1` Party maintenance image through their final rescue-USB
pass and passed the class, three-sensor, and live maintenance gates. This image
is OTA-capable; four of the 24 were subsequently updated over the air to
`fixture-2026-08-10.2` during the TMF recovery validation. This distinction
keeps the installed firmware capability separate from the transport used for
the original 24-board flash.

Corrected the fleet registry's two stale commissioning failures. `F40268` had
recovered after a reset alone, with no cable movement, and `F40384` passed after
the first STEMMA connection was reseated; both are now recorded as commissioned
Party peers on `.1` with healthy TMF8820, MSA311, and BMP581 gates.

The rev-1 boosted receiver path is not the rev-2 overvoltage fault. Rev 1 uses a
correctly routed AMS1117-5.0: pin 3 accepts boosted VDC and pin 2/tab supplies
5 V to the RX480E. This is a linear regulator, not a buck, so it burns the
approximately 12-to-5 V difference as heat. Ben reports the populated rev-1
433 MHz receiver paths working normally. The rev-2 failure came from changing
to an HT7550-1 without changing the old net-to-pin assignment; that reversed
VIN/VOUT and produced the measured 11.76 V receiver rail. A representative
rev-1 P5V and extended-run regulator temperature check remains open before lid
closure.

## 2026-08-11 -- Ben + Codex -- Chandelier chain current cap corrected

Added the `chandelier_chain_bench` diagnostic for RGBW or RGB daisy chains on
the PowerFeather switchable 3V3 rail. Four RGBW modules ran cleanly through
rainbow, channel fills, address bars, chase, and a modeled 896 mA full stress
after the battery connector was reseated. Bare USB had previously browned out
at the high setting, so battery ride-through/source quality was a confounder in
that observation.

Seven lensed RGB modules then appeared visually clean at static white, but the
initial 220 mA/module model was provisional. Rechecking the June 11 measurement
showed 256.5 mA at full RGB, so version `.5` uses 257 mA/module, an 800 mA
default LED budget, and a hard 900 mA supervised-bench maximum. Seven-RGB white
is therefore capped at 113/255 (about 797 mA), not the earlier 149/255. The
software cap is required: the PowerFeather's 1 A rating is shared by the board,
3V3 header, and VSQT rail, and regulator saturation is neither a controlled nor
a safe current limiter. Both conditional builds compile cleanly. The seven-RGB
artifact is 361,344 bytes with SHA-256
`27AEB65DAA5EBC28D063551F37CA372C9D0E655757B0D23EA1592E816C3FFA1F`;
the four-RGBW artifact is 361,392 bytes with SHA-256
`99964B6E8686703D3DADFAF362CA2B5E5F98DBA860980D88EDE8581596945196`.
A direct current/voltage/thermal run remains necessary before treating seven
modules as production-qualified.

## 2026-08-10 -- Ben + Codex -- TMF hardening shipped; four-node OTA passed with battery ride-through

Implemented and hardware-tested the TMF boot/recovery hardening prompted by
`F40268`'s reset-only recovery. Production `fixture-2026-08-10.2` now performs a
verified 100 ms VSQT off/on cycle before every non-parked class probe. The
PowerFeather SDK setter is retried and physical GPIO14 is read back, preventing
warm/OTA resets from inheriting the RTC-held sensor rail state. Three consecutive
failed TMF measurement cycles escalate once per boot from cheap stop/start to a
full shared-domain power cycle and SparkFun driver reconstruction, forcing TMF
init/open, firmware upload, application-mode switch, configuration, and ranging
start. MSA311 and BMP581 are reinitialized after the shared-rail reset. Persistent
faults remain degraded rather than flapping the rail; telemetry adds
`tmf_domain_resets`. Wire1 remains 100 kHz.

Added platform-independent recovery-policy coverage; the native suite passes 262
checks. `fleet_usb_bringup.py` now recognizes the diagnostic distinction: when a
downlight reports TMF present but zero reads, commissioning performs one `S1`
timed-sleep/VSQT reset and retests. An absent TMF ID skips the retry and still
points to the module/cable/power path.

Built exactly once as
`firmware/fixture/build/production-party-bmp-tmf-recovery-20260810-r1/fixture.ino.bin`
(1,166,656 bytes), SHA-256
`C818BB4B2D507184C3C274D192A756C6E819A9CBF442C61B9928E5DA2DC1BCC1`.
Inspection confirmed channel 11, prod defaults, `Party In The Woods`, no retired
SSIDs, and no test-only flags. Parallel OTA delivered the exact artifact to
`9F26BC`, `9E5A84`, `9E5B8C`, and `F2B7DC`. The first three passed `.2` rollback
validation and all three sensor gates immediately.

The closing fleet check later showed `reset_reason=brownout` on both bare-USB
fixtures (`9F26BC` and `9E5A84`) while each retained valid `.2` and clean sensors;
the battery-backed `9E5B8C` and `F2B7DC` remained on software-reset boots. Treat
an installed LFP or another proven stable supply as required ride-through for a
meaningful OTA/A-B test, even when USB remains attached for service/data.

`F2B7DC` supplied the real failure injection: before OTA, `.1` reported TMF
present with zero reads and 713 accumulated errors/recoveries. Its first upload
acknowledged but bare USB power dropped during the pending-verify window, so A/B
correctly rolled back to `.1`; a later retry lost USB before upload, and a reseat
alone still produced hard power cycles. Adding its LFP provided ride-through.
The final OTA then booted `.2` with reset reason software, cancelled rollback,
and reached 298 TMF reads with zero errors/recoveries/domain resets plus healthy
MSA311/BMP581 and a live Party maintenance endpoint. This distinguishes the TMF
state fix from the separate bare-USB instability and validates the real
battery-installed field OTA path. Evidence is in
`ops/bench/data/ca/2026-08-10-nc-downlight-tmf-hardening-ota-final4.jsonl` and
`ops/bench/data/ca/2026-08-10-nc-downlight-tmf-hardening-ota-F2B7DC-battery.jsonl`;
the two failed/retried traces are retained beside them.

## 2026-08-10 -- Ben + Codex -- Party production reflash complete (24/24); battery-installed rescue USB passed

Completed the final outer-ring batch: `F2B7DC`, `9F26BC`, `9E5A84`, and
`9E5B8C`. The first three passed the locked `fixture-2026-08-10.1` artifact,
15 Ah / 2 A / 4.6 V configuration, downlight class, MSA311/TMF8820/BMP581
sensor gate, and live `Party In The Woods` endpoint immediately.

`9E5B8C` initially tripped only the commissioner's default no-battery guard.
Live telemetry confirmed a real installed LFP at 3.358 V, charging normally at
about 152 mA from 4.665 V USB with no charger faults. At Ben's request, the same
locked artifact was then deliberately reflashed again through the fixture's
physical rescue USB port with battery-present mode explicitly allowed. Flash-ID
preflight, exact upload/hash verification, reboot, battery-aware telemetry,
all three sensors, and the Party maintenance endpoint passed. This replaced the
temporary solenoid-test image; final production telemetry has
`solenoid_enabled=false`.

All 24 BMP-equipped outer-ring downlights now run the single locked production
artifact with the corrected Party credential. Evidence:
`ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-05-final4.jsonl`.
The artifact remains
`firmware/fixture/build/production-party-bmp-20260810-r1/fixture.ino.bin`,
SHA-256 `F7A75F222497879899A8578FC3ECE1C84B9DA3F6370BBAA427DB31493FDDC7A1`.

## 2026-08-10 -- Ben + Codex -- Party production reflash batch 4 complete (20/24)

USB-reflashed outer-ring downlights `F40364`, `9E5A94`, `F2BE48`, `F2BE20`,
and `F2BF8C` from the locked `fixture-2026-08-10.1` artifact without rebuilding.
All five passed ESP32-S3/8 MB flash/2 MB PSRAM preflight, exact upload, the
15 Ah / 2 A / 4.6 V production configuration, downlight classification,
MSA311/TMF8820/BMP581 sensor gates, and live `Party In The Woods` maintenance
endpoints on the first attempt. The production credential reflash is now 20/24.
Evidence: `ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-04.jsonl`.

## 2026-08-10 -- Ben + Codex -- CORRECTION: F40268 recovered from reset only, not a cable reseat

Ben did not touch `F40268`'s STEMMA cable; he only pressed PowerFeather reset.
Before reset, the TMF passed ID/class detection and `begin()` but produced zero
measurements while accumulating 22 timeout/recovery cycles; MSA311 and BMP581
remained healthy. After reset, the same untouched hardware reached 223 TMF reads
with zero errors/recoveries and passed WiFi verification. This is evidence for a
TMF firmware-load/measurement-start recovery gap, not a physical connection fault.
The earlier batch-3 wording that says the cable was reseated is incorrect.

Operationally, distinguish `tmf8820_present=false` (electrical/module/contact
suspect) from `present=true`, zero reads, and increasing errors/recoveries
(reset/reinitialize once before touching hardware). Firmware follow-up: give
VSQT a verified off/on power cycle before the boot class probe and add one bounded
strong TMF reinitialization after repeated ranging failures; never flap the rail
indefinitely. The shared power-management bus remains fixed at 100 kHz.

## 2026-08-10 -- Ben + Codex -- Party reflash batch 3 complete; targeted bridge locator added (15/24)

USB-reflashed outer-ring downlights `F3FC90`, `F40268`, `F2BF54`, `9E668C`,
and `9F0E54` from the locked `fixture-2026-08-10.1` artifact without rebuilding.
Four passed immediately. `F40268` detected its TMF8820 but initially accumulated
22 failed reads/recoveries with zero good samples; after physically locating the
exact Feather and reseating its first STEMMA connection, it passed with 223 TMF
reads, zero errors/recoveries, healthy MSA311/BMP581, and a live
`Party In The Woods` endpoint. The production credential reflash is now 15/24.
Evidence: `ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-03.jsonl`
and `ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-03-F40268-cable-recheck.jsonl`.

Added exact-ID locator commands to the CoreS3 bench bridge:
`i<fixture-id>:<seconds>` targets one peer for 1-255 seconds while bare `i`
retains next-peer cycling. The dashboard safe-command filter accepts only the
bounded exact form. Built and USB-flashed COM40 with
`cores3-bridge-2026-08-10.1`; the 1,101,520-byte artifact is
`firmware/cores3_bridge/build/nc-cores3-bridge-target-identify-20260810-r1/cores3_bridge.ino.bin`,
SHA-256 `1D8ED02B6F5408AB7FA68C96664412A1D43BA3A4065EB099A48E0033839FCFEA`.
The dashboard reconnected and reported the new version. A targeted
`iF40268:255` produced the Feather's `..-` red-user-LED beacon; `iF40268:1`
then ended it after repair. This is now the preferred physical locator and does
not depend on LED harnesses or rail-switch success.

## 2026-08-10 -- Ben + Codex -- Party production reflash batch 2 complete (10/24)

USB-reflashed outer-ring downlights `9F275C`, `F2BE0C`, `9F26E4`, `F2BE8C`,
and `F2BF5C` from the locked `fixture-2026-08-10.1` artifact without rebuilding.
All five passed ESP32-S3/8 MB flash/2 MB PSRAM preflight, exact upload, the
15 Ah / 2 A / 4.6 V production configuration, downlight classification,
MSA311/TMF8820/BMP581 sensor gates, and live `Party In The Woods` maintenance
endpoints on the first attempt. The production credential reflash is now 10/24.
Evidence: `ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-02.jsonl`.

## 2026-08-10 -- Ben + Codex -- Party production reflash batch 1 complete (5/24)

Built and locked the corrected production fixture image
`fixture-2026-08-10.1` for channel 11 and the exact case-sensitive
`Party In The Woods` maintenance SSID. The artifact is
`firmware/fixture/build/production-party-bmp-20260810-r1/fixture.ino.bin`
(1,165,424 bytes), SHA-256
`F7A75F222497879899A8578FC3ECE1C84B9DA3F6370BBAA427DB31493FDDC7A1`.
Binary inspection found `Party In The Woods` and neither `WonkyHouse` nor
`BubbyNet`; the build has no forced/test solenoid flags. Native coverage remains
238 checks, zero failures.

USB-reflashed and verified outer-ring downlights `F2BEE4`, `9F26AC`, `F2BEA4`,
`F40384`, and `F2BDB0`. All five now pass the 15 Ah / 2 A / 4.6 V production
configuration, downlight class, MSA311/TMF8820/BMP581 sensor gate, and live
shared-WiFi maintenance endpoint. `F40384` initially lost its TMF while the
downstream MSA311/BMP581 stayed healthy; reseating its first STEMMA connection
and resetting restored 318 TMF reads with zero errors/recoveries. Evidence is
`ops/bench/data/ca/2026-08-10-nc-downlight-party-reflash-batch-01.jsonl`.

Bring-up correction: PowerFeather's GPIO4-controlled LED/header 3V3 rail and
the separate VSQT/STEMMA-QT rail must not be conflated. The existing timed-sleep
command cleanly cuts both; sleeping only the target for a bounded window is the
current no-reflash physical locator for a sensor-chain fault. A reset then
re-enables VSQT and performs a clean class/sensor initialization.

## 2026-08-09 -- Ben + Codex -- Reduced-access Atom solenoid clicker flashed

For 2026 camp operation, the preferred local manual-control direction is now a
small set of reduced-access Atom Matrix mini-bridges distributed to campmates,
rather than prioritizing the optional 433 MHz receiver retrofit. The receiver
candidate remains open for later. A clicker can use the pressable 5x5 face as
its sole control and leave fixture firmware as the strike safety authority.

Added `firmware/atom_clicker/`, a deliberately narrow ESP-NOW sender. Its only
fleet transmission is `NB_TARGET_SOLENOID`; target ID, channel, and pulse are
compile-time settings. It has no WiFi, OTA, serial command parser, maintenance,
configuration, sleep, or show controls. It requires a released face button,
debounces 35 ms, rate-limits presses to one per second, and repeats the same
packet six times over 40 ms. The fixture's active-pulse/rest guards collapse
that RF burst to one bounded strike. A center pixel reports radio/target state;
the amber face means sent, explicitly not actuator-acknowledged.

COM42 identified as the purchased original Atom Matrix: ESP32-PICO-D4, node
`54AD9C` (MAC `14:08:08:54:AD:9C`). The audited channel-11/target-`9E5B8C`/40 ms
artifact is
`firmware/atom_clicker/build/nc-atom-clicker-9e5b8c-r1/atom_clicker.ino.bin`,
914,608 bytes, SHA-256
`647B9C9262E0F8967865F9ACA890CF718C676ECE006A8A0E94C60E421111DAC2`.
USB upload hash-verified every region, and the boot report confirmed the exact
identity/configuration with radio ready and no serial controls. Physical
one-press/one-strike validation remains pending.

Before distribution, this needs a measured low-power/button-wake design for the
200 mAh Atomic Battery Base, target provisioning and labels, useful actuator
acknowledgement, multi-clicker RF/abuse testing, and real command authentication
plus fixture authorization. The current type-17 ESP-NOW packet is not
authenticated; a reduced UI alone is not a security boundary.

## 2026-08-09 -- Ben + Codex -- SW1 40 ms takeover OTA-deployed; 12 V RF path selected

The fabricated rev-2 receiver socket measured 11.76 V from its nominal `5V`
pin to ground with RECVR empty. This confirms that reversed U1 is exposing the
socket to approximately VBOOST, and explains two smoked RX480E modules. Their
factory momentary configuration does not change the diagnosis: overvoltage
failure can look like a latched output, while the series-capacitor one-shot
prevents that output from sustaining the solenoid gate. RECVR remains
quarantined on every fabricated v2.0 board.

No credible 12 V receiver was found in the same RX480E 1x7 footprint. The
leading retrofit is a QIACHIP KR1201MINI2-V05B: 3.7-24 V supply, isolated dry
contact, 31 x 14 x 7 mm, and momentary/toggle/latching modes. A five-wire
harness would power it from true VBOOST/GND at DRIVER and use COM/NO to connect
raw VDCIN to the existing D0/BTNP pad, exactly imitating SW1 upstream of R7 and
the hardware one-shot. The damaged P5V pad is not a qualified 12 V supply.

Fixture firmware now detects one rising edge from the released capboard
one-shot and takes over D7 for the proven 40 ms MCU pulse. It requires a LOW
after boot and after each event, so boot-high or stuck-high inputs neither fire
nor retrigger. Normal MCU strikes still refuse an externally high D7; timer and
loop failsafes remain. All 238 native checks passed. The audited Party/prod/ch11
artifact is
`firmware/fixture/build/nc-fixture-20260809-soltest-sw1-9e5b8c-r2/fixture.ino.bin`
(1,165,392 bytes, SHA-256
`29C587ECF9AABBFB7BC3CFD2462FC2FDF403C62243BC3A899D6427E2D1A2E730`).
Targeted shared-WiFi OTA validated peer `9E5B8C` at `192.168.1.167`; it rejoined
the bridge as `fixture-2026-08-09.2` after a software reset. Physical SW1 strike
strength and exactly-once telemetry are the remaining immediate checks.

## 2026-08-09 -- Ben + Codex -- Rev-2 receiver rail quarantined after two failures

Physical manual-control testing on `9E5B8C` separated three paths. The
PowerFeather USER button produced a strong firmware-bounded 40 ms strike, proving
the boosted bank, driver, solenoid, and MCU D7 path. Capboard SW1 produced only a
tiny strike with RECVR empty and no useful strike with the receiver installed.
Two RX480E receivers then appeared to latch and emitted smoke after several
minutes; both are quarantined and no receiver should be reinserted.

The smoke is not a D7 firmware latch. The capboard one-shot places a series
capacitor between receiver D0/SW1 and D7, and receiver latch mode cannot sustain
the solenoid gate through it. Inspection instead found a fabricated-board power
error: C16106 is the Holtek HT7550-1, whose SOT-89 pinout is 1=GND, 2=VIN,
3=VOUT, while both authoritative placement files assign pin 2 to P5V and pin 3
to the approximately 12 V VBOOST rail. The design comment had incorrectly
carried forward the old AMS1117 ordering. Reverse-powering this LDO can put
approximately boost voltage on the receiver's nominal 5 V supply and explains
the delayed receiver failures. Fabricated v2.0 RECVR is now marked unsafe.

Next: with no receiver fitted, measure the dock supply on a current-limited
setup; choose a receiver-free depopulation or measured VIN/VOUT ECO; and release
the correction only as a new board revision. Separately scope SW1 at D7. Its weak
hardware pulse may need C1B/component tuning or firmware detection followed by a
bounded 40 ms takeover. Continue using the PowerFeather USER button or targeted
ESP-NOW command for safe manual strikes.

## 2026-08-09 -- Ben + Codex -- Rev-2 solarnoid manual-control image flashed

Peer `9E5B8C` was visible through the CoreS3 bridge and remained stable after a
targeted 5 ms strike request, but its locked production image could not execute
the requested indoor test: `sol_en` was disarmed and the production lifecycle
reported no solar surplus. Inspection also confirmed that fixture firmware held
D7 OUTPUT LOW at idle, clamping the rev-2 capboard's SW1 and optional RX480E D0
hardware one-shot sources despite the board's own 10k pulldown.

Fixture firmware now releases D7 to INPUT/high-Z between MCU pulses when armed,
returns to that state from the timer cutoff, timer-start failure, explicit stop,
and loop failsafe, and retains OUTPUT LOW when disarmed. An MCU request is refused
if an external one-shot already holds D7 high. A targeted `--solenoid-test` build
option forces the arm bit and relaxes only the solar-surplus gate; the FULL-tier
battery and night vetoes remain. All 238 native fixture checks passed.

The first named build (`...-r1`) reproduced the stale `WonkyHouse` credential and
was not flashed. A second build used `BubbyNet`, but Ben clarified that neither
network is a production-peer SSID, so it also was not flashed. Binary inspection
proved that the deployed Aug-8 production artifact contains only `WonkyHouse`,
not the `BubbyNet` profile named in the earlier Aug-8 log entry. Production
maintenance correctly refuses that retired SSID, so remote OTA was impossible
and no OTA upload was attempted. All 24 previously commissioned outer-ring units
therefore need the corrected production credential on their final USB pass before
their lids close.

The corrected `Party In The Woods` targeted artifact is
`firmware/fixture/build/nc-fixture-20260809-soltest-9e5b8c-r4/fixture.ino.bin`,
1,165,152 bytes, SHA-256
`1545DB0F4C115BB21F88EC5E2286A2CE6F75D3D874EE9273F587A8B088750B57`.
Its audited flags select PowerFeather V2, channel 11, prod lifecycle,
`RES_SOLENOID_FORCE_ENABLED=1`, and `RES_SOLENOID_TEST_OVERRIDE=1`; binary strings
contain `Party In The Woods` and neither `WonkyHouse` nor `BubbyNet`. With VDC
empty and USB as the only source, COM53 identified the exact target `9E5B8C` and
the artifact was uploaded without recompiling; esptool verified every written
region. Post-flash telemetry reports `fixture-2026-08-09.1`, FULL power tier,
`solenoid_enabled=true`, zero solenoid faults, and healthy TMF8820/MSA311/BMP581.
The CoreS3 bridge sees the rejoined peer at -26 dBm with no packet gaps or send
failures. Physical SW1 validation remains next, before RX480E or a bridge sweep.

## 2026-08-08 -- Ben + Codex -- Outer 24-light BMP downlight ring complete

Commissioned the final four outer-ring downlights: `F2BEA4`, `F40384`, `F2BDB0`,
and `9F0E54`. Three passed the automated gate immediately. `F40384` initially
saw the TMF8820 before the capacity reboot but lost it afterward while its
downstream MSA311/BMP581 stayed healthy. Reseating/replacing the local STEMMA
connection and cold-power-cycling restored the TMF; the closing check recorded
174 reads with zero errors and healthy MSA311/BMP581 samples. All four now pass
the locked `fixture-2026-08-08.1` image, 15 Ah profile, IDLE guard, downlight
class, and complete sensor gate.

ADR 0034's BMP allocation is now fully commissioned: **24/24 outer hanging-ring
downlights**, each registry-tagged `outer hanging ring - position TBD`, plus six
purchased BMP581 spares. One additional PowerFeather (`F2BE74`) remains hardware
quarantined and is not part of the 24. Final-batch evidence:
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-05-production.jsonl` and
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-05-F40384-cable-recheck.jsonl`.

## 2026-08-08 -- Ben + Codex -- Production downlight batch 4 commissioned; one PowerFeather quarantined

Commissioned outer-ring downlights `F40268`, `F2BF54`, `9E668C`, `F2BE8C`, and
replacement `F2BF5C` with the locked production artifact and 15 Ah profile. The
first four passed the complete sensor gate. Original fifth board `F2BE74` flashed
but repeatedly failed PowerFeather initialization or external sensor-bus
operation, accompanied by the BQ STAT fault blink and unstable/dark TMF power.
The bare PowerFeather could initialize, but two TMF modules and two first cables
reproduced the loaded sensor-rail failure. `F2BE74` is registry-quarantined with
an explicit `DO NOT INSTALL BATTERY` note. Fresh PowerFeather `F2BF5C` passed on
the same assembled MSA311/TMF8820/BMP581 chain, confirming the controller/rail
fault. The outer BMP ring is now 20/24 commissioned. Evidence:
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-04-production.jsonl` and
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-04-replacement-production.jsonl`.

## 2026-08-08 -- Ben + Codex -- Production downlight batch 3 commissioned

USB-commissioned outer-ring downlights `F2BEE4`, `9F26AC`, `F2BE0C`, `9F26E4`,
and `F3FC90` with the locked `fixture-2026-08-08.1` artifact and 15 Ah profile.
All five passed on the first attempt: exact upload, IDLE guard, downlight class,
and sustained healthy MSA311/TMF8820/BMP581 samples with zero TMF errors. Registry
rows are tagged `outer hanging ring - position TBD`. The outer BMP ring is now
15/24 commissioned. Evidence:
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-03-production.jsonl`.

## 2026-08-08 -- Ben + Codex -- Production downlight batch 2 commissioned

USB-commissioned outer-ring downlights `9F26BC`, `9E5A84`, `9E5B8C`, `F2BE20`,
and `F2BF8C` with the same locked `fixture-2026-08-08.1` artifact and 15 Ah
profile as batch 1. Four passed the automated gate immediately. `9E5A84` saw
the TMF8820 at first but accumulated four failed reads/recoveries and then lost
the device while its downstream MSA311 and BMP581 stayed healthy. Replacing one
STEMMA cable restored it without a firmware reflash: the sustained recheck had
166 TMF reads, zero errors/recoveries, and healthy MSA311/BMP581 samples. A final
five-wide read verified firmware, 15,000 mAh capacity, IDLE guard, and all three
sensors on every unit with zero TMF errors. All five registry rows are tagged
`outer hanging ring - position TBD`; the outer BMP ring is now 10/24
commissioned. Evidence:
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-02-production.jsonl` and
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-02-9E5A84-cable-recheck.jsonl`.

## 2026-08-08 -- Ben + Codex -- BMP outer-ring allocation; USB reconnects isolated

Allocated the 30 bought BMP581s by ADR 0034: 24 go on the complete outermost
hanging-downlight ring and 6 remain spares. The first five commissioned units are
tagged as outer-ring, exact position TBD. This replaces the tentative trunk-light
allocation and gives the build a simple physical rule.

Investigated repeated Windows USB connection sounds before removing batch 1. All
five devices were present with clean PnP status and stayed continuously present
during a 48 s passive poll. The commissioning reader then reproduced a reset on
every board: pyserial opened native USB CDC at its default DTR/RTS state before
the script lowered both lines. `fleet_usb_bringup.py` now establishes DTR and RTS
low before opening the port. On `F40364`, two corrected reads 12 s apart advanced
uptime by 12.016 s instead of restarting. A subsequent two-minute, 24-sample
monitor of all five produced zero uptime drops; supply readings remained
4.756-4.843 V and every guard stage remained IDLE. `F2B7DC` retained a `brownout`
label from the earlier induced reset and `9E5A94` retained `poweron`, but neither
recurred during the corrected monitor. Going forward, one USB reconnect per
board is expected for the actual firmware upload; telemetry reads should be
silent.

## 2026-08-08 -- Ben + Codex -- First five production downlights USB-commissioned

Commissioned the first five of 72 hanging downlights in Nevada City: `F40364`,
`F2B7DC`, `9E5A94`, `9F275C`, and `F2BE48`. All five now run production peer
firmware `fixture-2026-08-08.1` on channel 11 with the Generic_LFP 15,000 mAh
profile, 2,000 mA charge limit, 4.6 V VINDPM, and BubbyNet OTA profile. The
exact 1,165,104-byte artifact is
`firmware/fixture/build/production-nobattery-bmp-20260808-r1/fixture.ino.bin`,
SHA-256
`B0A2D5181769DCA546BCBDC588F04F56B9DD60A7DD524778269088714C696F5D`.
Every board passed ESP32-S3/8 MB flash/2 MB PSRAM preflight, exact-artifact USB
upload, post-flash configuration, and sustained serial acceptance. Each
auto-classified as a downlight and returned healthy live samples from the
physical TMF8820 -> MSA311 -> BMP581 chain; all TMF counters had zero errors.
No battery or VDC source was attached during commissioning.

The intake exposed two production blockers before the lids were closed. First,
the USB commissioner checked but did not apply the requested battery profile
and sampled sensors before startup completed; it now writes/rechecks the
profile and waits for device uptime before class-specific sensor acceptance.
Second, an empty BAT port could report a small floating voltage and combine
with a persisted PROTECT stage to park the rails and enter 900 s sleeps despite
good USB power. Firmware now recognizes only plausible LFP voltage as battery
presence, keeps a battery-absent externally powered board parked but USB
serviceable, and provides a tightly guarded serial `X` clear for this bare-board
commissioning state. Downlights now initialize and report their fitted BMP581
in addition to MSA311/TMF8820. Native regression coverage passes 238 checks.
Evidence is in
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-01-rest4-production.jsonl` and
`ops/bench/data/ca/2026-08-08-nc-downlight-batch-01-canary-9E5A94-final.jsonl`;
all five registry rows are `commissioned` with role `downlight`.

## 2026-08-08 -- Ben + Codex -- Gotion sample-1 resumed with hardware 5 A -> 1 A knee stage

Resumed the rested, unrecharged sample after the overnight pause. The isolated
cell measured 3.331 V by DMM and 3.332 V at the ET5406A+ before loading. Extended
`ops/bench/et5406_discharge.py` to configure and verify the load's documented
multi-stage battery mode. Segment 3 uses 5.000 A until 2.550 V at the ET input,
then the instrument itself switches to 1.000 A until the final 2.500 V cutoff.
The first threshold compensates the measured approximately 0.093 ohm 22 AWG
lead/contact path: at 5 A, 2.55 V at the load estimates about 3.0 V at the cell.
Because the stages and final cutoff live in the instrument, USB or host loss can
only lose logging; it does not defeat the transition or termination.

The two-stage configuration read back exactly with HIGH current range, 6.000 A
OCP, 25 W OPP, and output OFF before connection. Segment 3 started at 09:26 PDT;
the first minute held 4.992-4.993 A, 2.758-2.760 V, about 13.78 W, no abnormal
state, and remained in the bulk stage. Active trace:
`ops/bench/data/ca/2026-08-08-et5406-discharge-162633Z-gotion-33140-15ah-sample-1-segment-3.jsonl`.
Logger PID 31560 and scoped sleep-guard PID 38248 are active.

## 2026-08-07 -- Ben + Codex -- Gotion sample-1 paused at 4.875 Ah delivered

Ben manually paused the supervised 5 A segment before leaving the Nevada City
bench. Segment 2c ran for 3,250 s / 54.2 min and delivered 4.501 Ah / 12.600 Wh
by host integration (ET: 4.506 Ah / 12.613 Wh). Combined with the initial 1 A
segment, sample 1 has delivered 4.875 Ah / 13.802 Wh so far. The segment logger
again classified the manual front-panel transition as a serial error, but its
final measurement was 0 A and its safety cleanup reported no output-off error.
A fresh independent status read verified `output: OFF`, 0.000 A, no abnormal
state, and 3.301 V rebound. Ben disconnected and insulated the cell for an
overnight pause; do not recharge before resuming the remaining-capacity run.

## 2026-08-07 -- Ben + Codex -- Gotion sample-1 accelerated into supervised 5 A segments

Ben prioritized a rough production-capacity answer that can be split around
laptop travel over the original uninterrupted 1 A qualification trace. A loaded
DMM comparison measured 3.306 V at the cell while the ET read approximately
3.213 V at 1.001 A, locating about 93 mohm in the 22 AWG alligator-lead/contact
path. Segment 1 was manually stopped after 0.374 Ah / 1.202 Wh. Ben accidentally
used main power rather than channel OFF, so the logger recorded a serial error,
but the ET output did turn off and the numeric endpoint was preserved.

Extended `ops/bench/et5406_discharge.py` to select the verified HIGH current
range above 3 A and to scale OCP/OPP with requested current. The first 5 A start
was correctly blocked by the old 10 W OPP; the second preflight saw the latched
OP fault and also delivered zero. After main-power fault clearing and correcting
OPP to 25 W, segment 2c started at 21:58 PDT with 5.000 A requested, 2.500 V
hardware cutoff, 6.000 A OCP, and a four-hour host limit. Initial regulation was
4.993 A / about 13.8 W with the ET reading 2.75-2.78 V through the lossy leads,
no abnormal state, and 0.090 Ah delivered after 67 seconds. Ben is supervising
lead and clip temperature and will manually pause before leaving. Active trace:
`ops/bench/data/ca/2026-08-08-et5406-discharge-045806Z-gotion-33140-15ah-sample-1-segment-2c.jsonl`.
Logger PID 15716 and scoped sleep-guard PID 48356 are active.

## 2026-08-07 -- Ben + Codex -- Gotion 33140 sample-1 discharge started in Nevada City

After the completed LFP charge and an overnight disconnected rest, Ben connected
Gotion 33140 sample 1 to the ET5406A+ in Nevada City. Ambient was approximately
78 F / 25.6 deg C. The rested DMM voltage was 3.362 V and the load read 3.365 V,
an agreement within 3 mV. Preflight on the travel-renumbered COM44 verified the
channel OFF, no abnormal state, 1.000 A constant-current battery mode, 2.500 V
cutoff, 4.000 V OVP, 1.200 A OCP, and 10 W OPP.

Started the guarded run at 21:18 PDT. The initial loaded point was 3.262 V at
1.001 A; it settled to 3.221 V during the first two minutes with no instrument
fault, and Ben confirmed that the cell and connections remained cool to the
touch. The script's initial 102.9 mohm estimate includes the cell, contacts, and
leads and is not yet a cell-only IR result. The active trace is
`ops/bench/data/ca/2026-08-08-et5406-discharge-041818Z-gotion-33140-15ah-sample-1.jsonl`.
Logger PID 33056 owns COM44, and scoped sleep-guard PID 26500 prevents Windows
idle sleep until the logger exits. The ET5406A+ hardware voltage cutoff remains
primary if USB or host logging fails. Run is ongoing.

## 2026-08-06 -- Ben + Codex -- Channel-11 migration and CoreS3 audio-reactive bridge

Corrected the earlier channel-6 diagnosis. The presence bench and LED Studio did
not require or persist an ESP-NOW channel. The first Cambium fixture artifact had
omitted `--channel 11`, so the old source fallback compiled as channel 6; the
later explicit `H6` restore then wrote that value into `resfx` NVS. Production
channel 11 was already the project standard, so no new ADR was needed.

Fixture `.5` now makes 11 the source default and adds channel-policy v1. An absent
channel or the known legacy channel-6 value migrates once to the compiled default,
while any other deliberate lab channel and all later `H1..H13` choices remain
persistent. Native coverage includes default, legacy, explicit, and corrupt-state
cases. USB-flashed the exact channel-11/dev artifact to the three authorized
Nevada City perimeter fixtures only: COM37/F3FD88, COM38/F2BE80, and COM39/F2BFEC.
All three reported `fixture-2026-08-06.5`, perimeter class, channel 11, 2,000 mA
charge policy, successful downlink matching, and zero ESP-NOW TX failures. COM4,
the older CoreS3 on COM40, and unrelated serial devices were not touched.

Added an independent CoreS3 audio-reactive build mode. It samples either the
CoreS3 dual onboard microphones or M5Stack Module Audio's external TRS mic input,
calibrates a DC-removed RMS envelope for two seconds, and sends 10 Hz type-25
direct frames with fast attack/slow release and stable ID-sorted RGB slots. Screen
tap or serial `A` pauses/resumes. It does not persist lifecycle state, and the
fixture's existing three-second direct-frame staleness fallback remains
authoritative. Known non-fixture heartbeat senders are excluded from the audio
roster; this kept the nearby legacy F3FD7C net-bench node from consuming one of
the three test colors.

Flashed the Module Audio build to the fresh CoreS3 at COM43/4D5DB0. Live radio
verification passed: all three fixtures were heard at about -18 to -26 dBm,
heartbeats were initially 100% delivered, direct frames were matched on all three,
and the bridge reported zero send failures or receive drops. The Module Audio
controller and codec initialize and every I2S read returns successfully, but the
samples initially remained exact digital zeros (`rms=0.0`, `readfail=0`). This
isolated M5Stack's physical A/B I2S selector requirement: it must be B for CoreS3.
No `N1` daylight override was issued until that hardware configuration was fixed,
so the fixtures stayed in automatic lifecycle during diagnosis.

Follow-up completed the hardware acceptance. Moving the Module Audio selector to
B changed the Rode path from exact zeros to a live noise floor around RMS 30;
speech/claps then peaked at RMS 3,422.6 and a 0.987 normalized envelope. A first
attempt also proved the touchscreen pause control by accidentally toggling audio
off; serial `A` restarted it without resetting the bridge. With only the three
perimeter fixtures forced to night in RAM, all three reported lifecycle 3,
program 3, and matched every addressed direct frame. Turning audio off stopped
the stream; after 4.5 seconds all three reported autonomous program 1, proving
the stale-frame fallback. COM37/38/39 each acknowledged `N2` afterward and
reported channel 11. The bridge was left audio-off and all fixture overrides were
returned to automatic lifecycle.

Verification passed: 266 fixture-native checks, 11 audio-native checks, scoped
`git diff --check`, and final sequential Arduino builds for fixture, normal
CoreS3, Cambium CoreS3, and Module Audio CoreS3. Named artifacts:

- fixture: 1,166,496 bytes, SHA-256
  `69D4117E61D578E623DA96144D80E764F413A2A4D307855D6C35D603B84A861C`;
- normal CoreS3: 1,101,376 bytes, SHA-256
  `13C1B9FA06561B98D6AA91DC95D38F051B7217131A0FE0A408F9B70B036DEBE3`;
- Cambium CoreS3: 1,095,424 bytes, SHA-256
  `EE00FA87D6C80DD096A13CA85CA1AA4699F991E669B0032B1BCDCD445C7F6059`;
- Module Audio CoreS3: 1,156,448 bytes, SHA-256
  `7EAFB2AD44DF7BFC7C3F96164D5B7584188283431CE38945FD8F5ED4D6CACB5D`.

## 2026-08-06 -- Ben + Codex -- Cambium fork published

Created the `beneckart/cambium` fork from Justin's unchanged `a39f9f8` `main`,
then published `codex/fleet-130-bench3` and fast-forwarded the fork's `main` to
the reviewed `b071542` integration. The fork is zero commits behind and five
commits ahead of `justinlange/cambium` `main`; no upstream history was
rewritten. Local remotes use `origin` for Ben's fork and `upstream` for
Justin's repository.

The published commit is the exact tree that passed 340 pytest checks with one
skip. No license file is present in either source or fork; Justin's explicit
license remains pending, and none was inferred locally.

## 2026-08-06 -- Ben + Codex -- Cambium firmware landed on main

Fast-forwarded `beneckart/resonance-lighting` `origin/main` from `176cf3b` to
`d9333ab`, preserving Justin's authored direct-frame commit and the separate
review/integration history. The exact reviewed branch remains published as
`codex/cambium-direct-frames` at the same commit.

Prepared the companion Cambium integration locally at `b071542`: nominal
130-fixture and Nevada City three-perimeter configuration, corrected lifecycle
reporting and isolated roster tests, firmware-derived type-25/26 golden pins,
and CoreS3-as-primary bridge documentation. Its complete pytest result is 340
passed, 1 skipped. Publication is waiting only for the requested
`beneckart/cambium` GitHub fork to exist; Justin's explicit license remains
pending and was not guessed or added locally.

## 2026-08-06 -- Ben + Codex -- Cambium rebased over 2 A policy and smoke-tested

Rebased `codex/cambium-direct-frames` onto `origin/main` at the ADR 0033 2 A
charge-policy commit. The combined fixture keeps both NVS behaviors: known legacy
charge defaults migrate to 2,000 mA while the persisted ESP-NOW channel and
`H1..H13` setter remain intact. Justin's direct-frame commit remains separately
authored in history, and the CoreS3 PSRAM/M5Canvas flicker fix remains the base of
both normal and binary-modem builds.

Verification passed after the rebase: all 254 native fixture checks and fresh,
sequential Arduino builds for the fixture plus both CoreS3 modes. Named artifacts:

- fixture, channel 11/dev: 1,166,208 bytes, SHA-256
  `C7EF277F583BFDD720AC16F9EDEDC854CF992F2F9DCD17CA8053E28C88EC572B`;
- normal CoreS3: 1,101,392 bytes, SHA-256
  `40B977E661DE769C0F2EA8BFE97DC0B7170A000EE023E99D4A677C8282A9D536`;
- Cambium CoreS3: 1,095,440 bytes, SHA-256
  `3ED4F26AB1931D258743FAE2DA65ED297F0EBE6FD8561668CB83F7C01D177893`.

Ran the scoped post-rebase hardware smoke on authorized perimeter fixture F3FD88
only. Its old 500 mA setting migrated to 2,000 mA, telemetry reported the expected
`.4` firmware and perimeter class, and `H11` rebooted onto channel 11 while retaining
the charge policy. It then acknowledged `H6` and was restored to the exact prior LED
Studio `.3` artifact (`F451B0C6E9015C1340801FDA5F0732C2197479208BEE437EED742EF9FBCABF50`),
with every flash segment hash verified. The other perimeter fixtures, CoreS3, and
active battery-prep device were not touched.

## 2026-08-06 -- Ben + Codex -- Cambium integration and 130-fixture production update

Pulled `origin/main`, fetched Justin Lange's `cambium-direct-frames` branch, and
onboarded the separate `justinlange/cambium` daemon in isolated worktrees so Ben's
dirty main worktree remained untouched. Justin's fixture commit applied cleanly to
current main and retains his authorship. The protocol addition is small and
append-only: direct-frame type 25 and lifecycle-force type 26, with a one-second
hold/fade and autonomous fallback after three seconds of sender silence. Existing
power and lifecycle gates remain authoritative.

Completed the integration locally instead of waiting on a rebased firmware PR.
Added a build-time binary Cambium mode to the existing CoreS3 bridge, persisted
radio-channel reporting/configuration (`H1` through `H13`) to the fixture, and direct
seen/matched telemetry counters. The CoreS3 path was necessary because Justin's
earmarked PowerFeather F2BED4 was not present; the active COM4 battery-prep fixture
was explicitly left untouched. Default human-readable CoreS3 behavior still builds
unchanged when the Cambium flag is absent.

Adapted Cambium to the current 130-fixture plan (8 packets per direct wave, 64
packets/s at 8 Hz), added `trunk` as a host-side alias for the wire-stable UPLIGHT
class, and added a three-perimeter-device roster/config. Review found and fixed a
real host-side lifecycle mismatch: fixture lifecycle values are boot=0,
day-charge=1, day-active=2, night=3, while the fake fleet and doctor had treated
0/1 as day/night. Unknown short-heartbeat lifecycle is now reported as unknown
instead of falsely diagnosed as day.

Verification passed: 254 native fixture checks; 334 Cambium tests with 1 skipped;
sequential clean Arduino builds for fixture, default CoreS3, and binary CoreS3. On
the three Nevada City perimeter devices (F3FD88, F2BE80, F2BFEC), the doctor heard
3/3 heartbeats, blink/identify worked, and distinct red/green/blue direct frames put
all three in program 3 at intensity 255. Stopping Cambium returned all three to
autonomous program 1 at intensity 25 after the lease expired. Acceptance counters:
67/67 radio sends, zero send failures, CRC errors, or receive drops. The test also
exposed persisted channel 6 from the presence bench overriding the build channel;
the new channel command corrected that before the pass.

After acceptance, all three fixtures acknowledged `H6` and were reflashed
sequentially with the exact prior LED Studio artifact
`F451B0C6E9015C1340801FDA5F0732C2197479208BEE437EED742EF9FBCABF50`.
The CoreS3 was restored to its exact prior bridge artifact
`170F2CABE5E242BFFEAF01E8CEA18E824AB6F73E03041340312CD96A43748347`;
flash hashes and the live ASCII banner were verified.

Recorded Nevada City production convergence in ADR 0032 and the living docs: 72
downlights in three rings of 24, 24 all-HEX perimeter lights, 18 mixed HEX/RGBW
chandelier lights, and 16 trunk/uplights moving toward all RGBW while the smaller
3 W RGB + lens option is tested for throw. Total deployment target is 130. The 158
production PowerFeathers now leave 28 boards beyond the deployment target, and the
small-enclosure allocation is 40 against 61 bought.

## 2026-08-06 -- Ben + Codex -- Full capboard USB-service reproduction passes; Cambium OTA hypothesis rejected

Completed the staged VUSB+VDC reproduction on surviving PowerFeather `F3FD7C`.
The clean PowerFeather passed VDC-first and VUSB-first hot-plug tests at 5.0 V,
same-hub VDC+VUSB, and a bench-supply 5.8 V panel simulation. Native USB
enumerated normally, the higher source won through the documented Schottky OR,
and application uptime continued without an unexpected reset or hotspot.

Reintroduced the rev-1 three-pin Y harness, external 12 V boost, charged 59,000 uF
bank, D7, VSNS->A5, and D7S->A4 one variable at a time. The boost plus capboard
alone settled at only 0.17 W / 29 mA from 5.8 V. The earlier approximately 3 W
"capboard" observation was the PowerFeather charger: once cap inrush ended and
VDC recovered, telemetry showed about 489-496 mA into the battery and 410-412 mA
from VDC. The 500 mA bench limit had made the PowerFeather charger and boost
startup compete, stalling the bank near 2.5 V; a 1 A limit charged it to 12.17 V
in about 2-3 s. This was source-current contention, not a capboard fault.

With every connection present, read-only telemetry measured VSNS 3.059 V
(12.329 V calculated bank), D7S 0 V, and gate OFF. Twenty consecutive native-USB
ROM/stub reset and flash-ID cycles then passed 20/20 with JEDEC `20:4017`, 8 MB,
and 3.3 V flash configuration. The application rejoined after the final reset;
Ben observed no solenoid twitch, red driver signal LED, warmth, or other anomaly.
This strongly rejects normal dual-input operation, the shared hub, boost/capboard
power, static telemetry, D7, and the USB bootloader/reset sequence as sufficient
causes of the two quarantined-board failures. It does not replace scope captures,
dead-board rail/JEDEC forensics, actual-panel testing, or the required 50 physical
USB-service insertion cycles, so the conservative field-service rule remains.

Checked the concurrent Cambium integration as a possible hidden fleet OTA.
Cambium contains no fixture-firmware uploader; it sends direct color/lifecycle and
identify packets. The other Codex task's only fixture flash was an explicitly
selected perimeter unit `F3FD88`, which it restored, and no repo OTA record targets
`F402F4` or `F402B4`; today's recorded OTA targets only `F3FD7C`. A power cut during
a genuine application OTA could leave an incomplete inactive app partition, but
the updater commits only at upload end and ordinary content corruption cannot make
the flash chip's raw JEDEC identity unreadable. Run no-stub flash ID and preserve a
full dump on each quarantined board before any erase; valid raw JEDEC would reopen
the interrupted-write theory, while persistently invalid JEDEC would reject it.

## 2026-08-06 -- Ben + Codex -- Rev-1 boost required; USB service risk promoted to P0

Ben reported that every strike in the direct-5-V 8/12/20/35/50 ms A/B was very
weak, matching the measured capacitor contribution. This closes the rev-1
functional decision: any deployed rev-1 capboard using the HS-0730B needs an
external approximately 12 V boost retrofit; direct 5 V is not a production strike.
ADR 0030 now records that amendment while retaining bounded-pulse, low-SOC, hot-panel,
and thermal/endurance qualification as open.

The apparently triangular 50 ms boosted trace is quantitatively expected. A nominal
6 ohm coil with 0.059 F gives RC = 0.354 s and an initial normalized exponential
slope of -0.282 percent/ms, nearly Ben's visual -0.3 percent/ms. Fifty milliseconds
is only 0.14 time constants, so the exponential is almost its linear first-order
approximation: ideal V(50 ms) is 10.67 V from 12.284 V, versus the robust measured
10.511 V. A fit to the 2 ms medians from 15-49 ms is highly linear (R^2 about 0.978),
while an exponential fit is only marginally better (R^2 about 0.981). The measured
recovery fit from 61-191 ms is about +0.0118 V/ms (+0.096 percent/ms, R^2 about
0.971). A +0.1 percent/ms recharge corresponds to about 0.725 A into 0.059 F, about
8.3 W at the bank midpoint or roughly 1.9 A from 5 V at 85 percent efficiency,
consistent with a boost/input-limited near-constant-current recharge.

The capboard button and optional RX480E D0 output both feed the same 470 ohm / 10 uF
one-shot into D7. Current firmware parks D7 OUTPUT LOW, so it clamps both sources,
not just the physical button. A firmware candidate is to leave D7 high-impedance at
idle and drive it only HIGH for a bounded command; the board's 10k pulldown then
defines OFF, including during boot/reset. A series diode in the PowerFeather D7 lead
is the firmware-independent hardware-OR alternative. Neither change was implemented
in this diagnostic turn; each needs boot, watchdog, receiver, physical-button, and
commanded-strike validation.

The dual-input failure was promoted to a separate P0 because deployed enclosures expose
native USB while a panel may be hot on VDC. Official PowerFeather documentation and
the V2 schematic explicitly permit simultaneous VUSB+VDC through separate Schottky
diodes, so the two failed boards are not explained by normal dual-input operation or
a DC ground loop alone. Added
`docs/tests/POWERFEATHER_V2_DUAL_INPUT_USB_SERVICE_PLAN_2026-08.md` with dead-board
forensics, current-limited staged reproduction, large-bank/harness isolation, and a
50-cycle field-sequence acceptance test. Until it passes, the service rule is
data-only/VBUS-blocked native USB while VDC is live; powered USB requires VDC/panel
removal first.

## 2026-08-06 -- Ben + Codex -- Gotion charge-completion sentinel started

Added `ops/bench/charge_taper_watch.py`, a read-only serial sentinel for the
PowerFeather charge telemetry on COM4. It arms only after observing at least
500 mA of charge current, then declares the 33140 LFP charge complete only when
the USB supply remains good at >=4.5 V, battery voltage is >=3.58 V, and
absolute battery current remains <=120 mA continuously for five minutes. This
guards against mistaking a disconnected or failed USB source for charge taper.
Completion writes a `.done.json` marker, sounds three Windows beeps, displays a
desktop message, and exits; the pack should then be disconnected and rested for
at least one hour before the ET5406A+ discharge test.

Started the sentinel as PID 29612, sampling every 15 seconds into
`ops/bench/data/ca/20260806-151515-gotion-33140-sample1-charge-taper-COM4.jsonl`.
Initial healthy readings were about 3.55 V and +1.45 A with a good 4.72 V USB
supply, so the pack was still accepting bulk charge and the completion hold had
not begun. No charger configuration was changed.

## 2026-08-06 -- Ben + Codex -- Flicker-fixed CoreS3 restored after Cambium bench

Ben reported that the CoreS3 LCD had resumed blinking. Live serial identified
`cores3-bridge-2026-08-06.1`; the separate Cambium integration log confirmed
that its acceptance cleanup had restored the pre-framebuffer `r3` artifact
(`170F2C...`) rather than the later flicker-fixed image.

Reflashed COM40 / `44:1B:F6:E3:9F:1C` with the previously compiled and verified
`nc-cores3-bridge-20260806-r4` artifact, SHA-256
`556D951E91A658F1609E63FE5A2A7AD53750F79AE773DD1926DCA4FFCEC778EC`.
All flash-region hashes verified. A cold boot confirmed `.2`, successful PSRAM
framebuffer allocation, ESP-NOW channel 11, and master telemetry. The restored
bridge immediately received peer `F3FD7C`, confirming that the middleware
task's fixture-side radio work still operates. No control packet was sent, and
no fixture, Cambium branch, or daemon state was changed.

## 2026-08-06 -- Ben + Codex -- Rev-1 3P capbank boost A/B exposes actuation reset

Pulled `origin/main` fast-forward from `5c6e22d` to `b10bb1b`, then onboarded the
Nevada City rev-1.0 three-pin capbank bench: 2x 22,000 uF plus 15,000 uF
(59,000 uF total), Gangbei HS-0730B 6 V / 1 A solenoid, external 12 V boost in
the capboard V+ branch, and the as-wired A4->D7S / A5->VSNS telemetry swap. Added
an opt-in, timer-bounded `net_bench` capbank probe with idle `j` and targeted
`J<id>:<ms>` serial controls; there is deliberately no boot strike. The verified
peer artifact is
`firmware/net_bench/build/capbank-ab-e39f1c-20260806-r1/net_bench.ino.bin`,
1,043,440 bytes, SHA-256
`81E5F18D777AA4654596830C264853AEDF2F1F26D8BA86E9BC39B55408F85D70`.

The unboosted 5 ms baseline at 5.14 V produced only an almost imperceptible
twitch. Instrumentation captured pre/min/drop/post20/post100/post250 of
5.139/4.776/0.363/5.038/5.239/5.143 V, a 3.137 V D7S peak, 1,210 samples, and
zero firmware failsafes. With the PowerFeather absent, the boosted capboard
measured 12.17 V and its onboard button produced three strong strikes followed
by one weak residual-energy strike. Static PowerFeather-facing checks were
benign: D7S 0 V, VSNS 3.00 V through the rev-1 100k/33k divider, D7 0 V, and
the shared Y-cable stem 5.18 V pre-boost. No static 12 V path to A4/A5, D7, or
PowerFeather VDC was found.

Two previously commissioned PowerFeathers failed during setup and are now
quarantined. `F402F4` entered an invalid-header loop and returned invalid flash
JEDEC data; `F402B4` first passed its 8 MB `20:4017` preflight, probe flash, and
unboosted pulse, but later developed the same unreadable-flash symptom. Neither
board was exposed to an energized 12 V boost while connected, so the exact
failure mechanism is still open and must not be attributed to static telemetry
injection without transient evidence.

A third board, `F3FD7C`, passed the same 8 MB flash preflight and hash-verified
probe upload. The corrected boosted test removed USB entirely and powered both
branches only from the 5 V Y source. Before actuation it held 4.914 V / about
68-70 mA at the PowerFeather, 12.17 V at the bank, and 100% ESP-NOW PDR. One
targeted 5 ms strike visibly budged the solenoid but immediately power-cycled
the PowerFeather: uptime/sequence dropped from 84 s / 81 to 1.9 s / 0. It
recovered, remained cold, and later passed a USB-only application/probe check.
This proves a dynamic actuation disturbance in the shared supply/ground/control
system, but does not yet distinguish VDC collapse, ground bounce, or D7 coupling.
Longer boosted pulses are blocked until that transient is scoped and isolated.

The initial USB-only disappearance also exposed a separate commissioning edge
case. Fixture policy treats battery readings below 0.5 V as invalid rather than
new low-voltage evidence, but a previously persisted PROTECT stage remains
latched when the battery is absent. It therefore cannot meet the battery/charge
release condition and returns to deep sleep after the 30 s cold-boot grace even
while external power is present. A safe no-battery maintenance behavior and
native regression test remain open.

Two follow-up A/Bs narrowed the actuation reset. First, the capboard's physical
button stopped striking whenever the PowerFeather was plugged into the Y stem,
then immediately worked again when the PowerFeather was removed. This was not
capbank starvation: rev-1 routes its 470 ohm / 10 uF one-shot onto D7, while the
probe firmware deliberately holds D7 as an OUTPUT LOW between commands. The
PowerFeather therefore sinks/clamps the physical-button pulse. Do not repeat
that contention test; future firmware/hardware must tri-state or OR the two D7
sources if the capboard button must remain usable while the MCU is attached.

Second, Ben connected the 3.33 V 32700 LFP while leaving A4/A5 and the rest of
the reset-producing topology unchanged. Battery-only telemetry was stable at
3.323-3.324 V and about -152 mA. After VDC was connected, the source sat at
4.660-4.664 V / 476-482 mA and the battery charged at about 489-505 mA; the
bank remained 12.17 V. The same targeted 5 ms command then completed with no
reset: uptime advanced from 182.3 s to 184.7 s and sequence from 179 to 181.
Ben judged the resulting strike stronger than the no-battery pulse. This is a
strong controlled result for inadequate transient headroom in the USB/cable/Y/
boost source path: the BQ power path and LFP rode through the disturbance and
may also have prevented the D7 pulse from being truncated. Because A4/A5 stayed
connected, static telemetry wiring is no longer a leading reset cause. Scope the
source before assigning the droop to the hub rather than the cable/Y/boost branch,
and still qualify the intended production pulse width with the battery installed.

The battery-installed bench envelope then passed every requested one-shot from
5 through 50 ms (5, 8, 12, 16, 18, 20, 25, 30, 35, 40, and 50 ms) without a
PowerFeather reset, supply-loss report, solenoid failsafe, or warm component.
A 10-strike 20 ms endurance run at 15 s intervals also passed 10/10; Ben judged
the strikes hard and consistent. The earlier apparent double strike did not
recur on a second 16 ms sweep and is most consistent with mechanical bounce;
the six bridge retransmissions are rejected by the peer's 80 ms rest guard,
though production firmware should still deduplicate explicit event IDs.

Added a bench-only high-rate waveform path and
`ops/bench/capbank_waveform.py`. HTTP arms one 5-50 ms pulse, the peer turns
WiFi fully off, ADC DMA records VSNS and D7S, then the peer rejoins shared WiFi
and serves the calibrated CSV. The ESP32-S3 limit is 83,333 total conversions/s;
the deployed profile requests 80,000 and measured 40,318 samples/s on each of
the two interleaved ADC1 channels. The first `.3` capture exposed and preserved
an unsigned pre/post-trigger timestamp bug (416 samples); `.4` fixes it, and the
host now rejects any trace shorter than 80 percent of the requested post-trigger
window. The exact `.4` artifact is
`firmware/net_bench/build/capbank-wave-f3fd7c-20260806-r3-bubbynet/net_bench.ino.bin`,
1,058,496 bytes, SHA-256
`418DDE5E7CFFEBBC048B170E4F42A2BCAD433A89F26BA778F67784E9C4A5DF55`.
USB upload, BubbyNet maintenance, no-touch OTA, software-reset ESP-NOW rejoin,
and targeted return to maintenance all passed on `F3FD7C`.

Radio-quiet 8/12/20/35/50 ms captures each recorded 10,848-12,544 samples with
no overflow or reset. D7S measured 7.986/12.004/19.991/34.996/50.002 ms, a
worst command error of 0.014 ms. Two-millisecond median VSNS bins put the robust
bank drops at about 0.05/0.21/0.57/1.20/1.77 V from a 12.28-12.29 V baseline.
Raw VSNS is visibly noisy during the strike, so it must not be interpreted as
oscilloscope-bandwidth truth: rev-1 intentionally has 100k/33k plus 100 nF,
about a 2.48 ms time constant / 64 Hz cutoff, and the ADC input sits close to
its upper range. Original CSVs are under `ops/bench/data/ca/capbank/`; the clean
comparison uses medians plus the raw 10-90 percent spread.

Ben then discharged the bank, replaced the boost branch with the direct 5 V
cable, and repeated the identical radio-quiet 8/12/20/35/50 ms sweep with the
battery and VDC still attached and USB absent. All five captures again completed
at about 40,318 samples/s/channel with no reset, failsafe, overflow, or supply
loss. The local pre-strike bank was 5.086 V. Robust pre-to-min drops were
0.081/0.113/0.181/0.234/0.286 V, or 1.59/2.22/3.57/4.60/5.63 percent. Using the
nominal 0.059 F bank, those voltage changes represent about
0.024/0.034/0.053/0.069/0.083 J removed from the capacitors. The paired boosted
50 ms run removed about 1.193 J, 14.3x the direct run's bank contribution, and
bottomed at 10.511 V versus 4.800 V direct. This is not total coil or mechanical
energy because the active source also supplies current during the pulse. The
direct-run manifest is
`ops/bench/data/ca/capbank/20260806T225131Z-f3fd7c-nonboosted-5v-summary.json`;
all ten calibrated CSVs are retained beside it. A proper GET `/resume` then
returned `F3FD7C` to ESP-NOW comms; the bridge received firmware `.4` at 100
percent local PDR with 4.934 V supply and no restart.

This session also found why the first remote OTA discovery failed: the peer's
old image tried to join retired `WonkyHouse` while the NC laptop was on
`BubbyNet`. The local net_bench profile now uses BubbyNet, and `build.sh` no
longer copies credentials from another sketch. It rejects WonkyHouse unless
Dad's legacy personal bench explicitly sets `RES_ALLOW_LEGACY_WONKYHOUSE=1`.
Fleet `fixture` migration and a camp-Starlink fallback remain open until the
real credentials and channel behavior are available.

## 2026-08-06 -- Ben + Codex -- CoreS3 display refresh made flicker-free

Ben confirmed that the dedicated bridge image now boots from the CoreS3's own
USB-C connection with the ESP32-H2 Gateway module and DIN base removed. This
closes the standalone-power concern; the earlier no-boot symptom is no longer
reproducible after the bridge flash.

The first bridge display implementation cleared and redrew the physical LCD at
1 Hz, which produced a distracting black flash. Replaced that path with a
320 x 240, 16-bit M5Canvas allocated in PSRAM: each status page is now rendered
off-screen and pushed to the LCD only after the frame is complete. The bridge
continues headless with a serial diagnostic if framebuffer allocation ever
fails.

Built and flashed `cores3-bridge-2026-08-06.2` on COM40. The exact artifact is
`firmware/cores3_bridge/build/nc-cores3-bridge-20260806-r4/cores3_bridge.ino.bin`,
1,101,344 bytes (1,101,191 bytes of reported sketch flash usage), SHA-256
`556D951E91A658F1609E63FE5A2A7AD53750F79AE773DD1926DCA4FFCEC778EC`.
The audited build options select the CoreS3 FQBN and `NB_CHANNEL=11`; every flash
segment hash verified. A cold-reset check confirmed the `.2` banner, successful
framebuffer allocation, ESP-NOW on channel 11, and a broadcast completing with
`sendok=4 sendfail=0`. Ben's visual confirmation of the now-steady LCD is the
remaining human check.

## 2026-08-06 -- Ben + Codex -- Two-amp charge ceiling replaces conservative firmware defaults

Accepted ADR 0033: the known 6 Ah and 15 Ah production LFP cells now use the
PowerFeather V2/BQ25628E maximum 2,000 mA battery-side charge-current setting as
their default ceiling. This is not a promise of 2 A delivered and does not grant
permission to pull 2 A from an arbitrary USB port: source IINDPM/VINDPM, available
input power after system load, CV taper, charger thermal regulation, cell
temperature/specification, and wiring limits still apply. A lower `--charge-ma`
or `G<ma>` remains available for a different or limited cell. Promoted the 103AT
battery-thermistor path to an explicit unattended sealed-hat qualification gate.

Scrubbed the active lower defaults from unified `fixture`, `net_bench`,
`power_bench`, LED Studio, presence/sway/speaker/solenoid/LED-solenoid demos, the
ported PowerFeather demo, field-cycle OTA tooling, fleet USB commissioning, and
the deep-discharge setup instructions. Historical logs, named artifacts, and the
2026-07-27 packing profile retain the currents they actually used. Firmware
versions advanced to dated 2026-08-06 identities where applicable.

A normal application reflash does not erase Preferences/NVS, so charge-policy v1
now upgrades only the known unset/500/1,000/1,500 mA historical defaults in both
`fixture` and `net_bench`. It preserves a nonstandard old value as a possible
intentional small-cell limit, and all later `G<ma>` overrides remain persistent.
Also fixed `firmware/fixture/build.sh` to translate Git-Bash `/c/...` include paths
for the Windows `arduino-cli`; the wrapper had failed to find the shared solar
guard header during verification.

Verification passed: fixture native suite 233 checks / 0 failures, Python
`py_compile` for all edited bench tools, shell syntax checks, scoped
`git diff --check`, and full uncached ESP32-S3 compiles of both affected images.
The verified production-profile fixture artifact is
`firmware/fixture/build/charge-policy-20260806-r4/fixture.ino.bin`, 1,164,624
bytes, SHA-256
`178D12C281F048C48331EE182A8AF289E62B44E2543C087871CAFAA8829C627A`.
The LFP/6 Ah peer build at
`firmware/net_bench/build/charge-policy-20260806-r2/net_bench.ino.bin` is
1,024,208 bytes, SHA-256
`6686F344E7750E5A271BFDD1D4A02610B946F703B8B40A2A04F23300C451BF8C`;
its build flags intentionally omit `RES_PF_MAX_CHARGE_MA`, proving the compiled
2 A default. No hardware was flashed in this change.

## 2026-08-06 -- Ben + Codex -- Nevada City production layout converges at nominally 130 lights

The physical build has converged on a new production allocation: 72 hanging
downlights in three rings of 24, 24 perimeter lights all using HEX, 18 chandelier
lights with a mixed HEX/RGBW population, and about 16 trunk lights moving toward
all RGBW. A smaller lensed 3 W RGB trunk-light variant is also under test for extra
throw. The planning baseline is the full nominal 130; fewer lights are a contingency
for an unforeseen integration or field issue, not the target.

Added ADR 0032, which supersedes only the old 150-152 count/class allocation in
ADR 0024 while retaining the COTS PowerFeather and fungible-electronics decisions.
Updated the canonical SYSTEM fleet table and current onboarding, BOM, enclosure,
roadmap, procurement, glossary, and TODO references. Historical test reports,
procurement transactions, and dated plan snapshots retain the counts that were true
when written. Hardware bought beyond the 130-fixture target is now explicitly build
recovery stock, field spares, or optional off-tree inventory.

## 2026-08-06 -- Ben + Codex -- Gotion 33140 sample-1 charge preflight and ET5406A+ discharge rig

Onboarded against the current two-tier battery decision, ADR 0023 discharge map,
and prior 32700 shootout method. Windows saw the newly attached EastTester/Yertai
ET5406A+ as USB VID/PID `1A86:7523` but initially had no CH340 driver (Code 28).
Installed the signed WCH CH341/CH340 driver from the chip vendor; the load now
enumerates as COM41 and identifies as
`ET5406A+ 09552613034 26011 2446.001`. Live SCPI checks verified battery mode,
low voltage/current ranges, and readback. It is armed for 1.000 A CC with one-stage
2.500 V voltage cutoff, 4.0 V OVP, 1.2 A OCP, and 10 W OPP; the channel is OFF and
there is no cell connected to the load.

The non-production PowerFeather sample on COM4 is MAC `D8:85:AC:9E:5A:B8`
(`9E5AB8`). Its old `net-bench-2026-07-13.3` image was configured as
`Generic_3V7`; with the 33140 connected it showed 3.528 V and about +1.49 A into
the cell. That Li-ion profile was unsafe for an LFP cell. Replaced it with the
previously verified LFP artifact
`firmware/net_bench/build/serial-bridge-20260708-adr23-latch-tail`
(app SHA-256
`71121ECA411F034892503B50E349CA6CA94ED548A819B82267114160E193A2A8`). The
artifact's audited build options select `Generic_LFP`, 1,500 mA maximum charge,
and 4.6 V input maintenance. Upload and flash hashes verified. Follow-up telemetry
showed `battery_type=Generic_LFP`, 3.528 V, and about +1.61 A, so the cell is still
accepting essentially full charge current and is not full. Its 99 percent gauge SOC
is invalid because this old artifact retains a 6,000 mAh DesignCap.

Added `ops/bench/et5406_discharge.py` plus
`docs/tests/BATTERY_33140_GOTION_ET5406_PLAN_2026-08.md`. The logger configures and
read-verifies the ET, exclusive-creates JSONL, records ET and independent host Ah/Wh
plus capacity above 3.0 V and initial loaded sag, rejects an absent or implausible
cell, requires explicit `--run --yes`, uses the load's hardware cutoff as primary,
and forces output OFF on exit. Live verification passed: arm/readback, a deliberate
no-cell start refusal, and confirmed OFF afterward. Next: finish LFP CV/taper near
3.60 V / <=120 mA, disconnect and rest at least one hour, DMM-check polarity and
rested voltage, then run sample 1. Repeat on at least one other 33140 before batch
qualification or changing ADR 0023 thresholds.

## 2026-08-06 -- Ben + Codex -- CoreS3 dedicated fleet bridge flashed and verified

Origin was fetched and already matched local `main`; existing uncommitted bench,
firmware, registry, LOG, and TODO work was preserved. USB enumeration found four
PowerFeathers plus the M5Stack CoreS3 Thread BR. The CoreS3 is COM40 with stable
USB/WiFi MAC `44:1B:F6:E3:9F:1C` and bridge ID `E39F1C`; the fourth PowerFeather
now enumerates as COM25 / `68:EE:8F:F4:02:F4`.

Added `firmware/cores3_bridge/`: a dedicated CoreS3 target using M5Unified to
initialize the AXP2101 and LCD, the canonical fixture packet header, pure
unassociated ESP-NOW on channel 11, a 192-peer table, the existing dashboard
`nb-*` serial schema and control commands, a nonblocking maintenance burst, and
an on-device bridge/peer status screen. The purchased Thread BR kit's ESP32-H2
Gateway module is not used by Resonance; it can remain mechanically installed.

The first live image exposed and fixed an Arduino-ESP32 3.x integration error:
`WiFi.disconnect(true, false)` powered the STA interface off before channel
pinning. The final exact artifact is
`firmware/cores3_bridge/build/nc-cores3-bridge-20260806-r3/cores3_bridge.ino.bin`,
1,095,760 bytes (1,095,611 bytes of reported sketch flash usage) with SHA-256
`170F2CABE5E242BFFEAF01E8CEA18E824AB6F73E03041340312CD96A43748347`.
It was USB-flashed to COM40 with all segment hashes verified. Live checks passed:
`esp-now up, ch=11`, 4.10 V CoreS3 battery, stable master telemetry, `r`/`t`
commands, existing dashboard parser recognition, and a four-copy harmless
RESUME transmit with `sendok=4`, `sendfail=0`. No peer was expected or observed:
the three perimeter boards are on LED Studio (WiFi, not ESP-NOW), and the fourth
PowerFeather was not changed in this session. Open: physically retry the CoreS3
on its own USB-C with the Gateway/DIN stack removed, and exercise bridge RX plus
one targeted command after a PowerFeather is returned to fixture/net_bench peer
firmware.

## 2026-08-01 -- Ben + Claude -- Nevada City: first three perimeter lights flashed; trio bench dashboard

Site moved TN -> Nevada City (crew unloading; Starlink left at the VRBO, bench
net is Ben's phone hotspot "BenPhone"). First three BUILT perimeter lights
(HEX + gobo + VL53L5CX, 6 Ah LFP) USB-flashed on the hub with
`led-studio-2026-08-01.3` (l5cx build, same recipe as 9E5AE8: --l5cx --cap 6000
--charge-ma 500 --maintain 4.6). All three verified: L5CX 4x4 @ 10 Hz up, WiFi
joined, /state + /set + OTA /update answering 200 from the laptop.

New boards, fleet OUI, intaken to registry as enumerated/perimeter_demo:
F3FD88 (68:EE:8F:F3:FD:88, COM37), F2BE80 (..F2:BE:80, COM38),
F2BFEC (..F2:BF:EC, COM39); mdns ledstudio-<mac6>.local each.

led_studio .1 -> .3 changes (all in the deployed image):
- /set + /state now send Access-Control-Allow-Origin: * so laptop-hosted
  dashboards can drive several boards (file:// works).
- Dual-SSID boot preference: wifi_secrets.h gained RES_WIFI_SSID2; setupWifi
  scans at boot and prefers the bench AP (ResonanceBench, laptop-hosted
  Windows mobile hotspot, pw same as BenPhone) when visible, else falls back
  to the primary (BenPhone). Rationale: right after the first flash the phone
  hotspot dropped laptop->board ARP/HTTP entirely for ~20 min (looked exactly
  like AP client isolation) then recovered; the bench AP is the
  phone-independent escape hatch. Boards only migrate at boot -- bring the
  bench AP up first, then power-cycle.
- /state now carries "host" (the per-device mdns name) so multi-board tools
  can identify who answered -- subnet-scan discovery keys off it.

New: `ops/bench/perimeter_trio.html` -- 3-light bench dashboard (open the file
directly, no server). Live /state poll per light: presence badge
(idle/visitor/gobo-near/gobo-palm), visitor + closest mm, wheel %, 4x4 zone
grid vs baseline, SOC/V/mA/RSSI. Per-light + all-three controls: anim
(Static/Spiral/Orbit/Breathe/Twinkle/Presence), bri/speed/color, gobo thresh,
re-zero scene, off; mood presets (Presence demo / Ember breathe / Aurora
spiral / Twinkle). "Scan subnet" probes base.1-254 /state and slots boards by
reported host.

Open: the 4th board on the hub (bare PowerFeather, candidate desk bridge)
never enumerated as a USB serial device -- only COM37/38/39 appeared; needs
cable/power check (or it is in ship mode). OTA-verify was endpoint-GET only;
a full artifact POST cycle is still unexercised on these three.

## 2026-07-30 -- Ben + Claude -- production fixture firmware: milestone 1 code complete

New sketch `firmware/fixture/` -- one image for all four fixture classes,
extracted from the proven bench donors into the ARCHITECTURE.md layered shape:
`src/core/` (platform-independent, ~220 native g++ checks in `tests/`) +
`src/esp32/` glue. Compiles at 1.16 MB / 34% flash. net_bench stays the desk
bridge build; protocol v1 is kept and extended append-only (new types 18-24:
choreo state, program-set lease, profile flip, pinned neighbor adjacency;
20/22/23 reserved for time anchors, locate reports, event fabric). Heartbeat
discipline: 29 B hb-short at 0.2 Hz (send-side truncation at a tail boundary --
receivers already length-check) + full heartbeat every 60 s; fast show state
rides the new 22 B NB_CHOREO_STATE instead (150 nodes of 1 Hz full heartbeats
would eat ~25% airtime).

Carried forward verbatim: solar guard, maintenance/OTA lifecycle + the exact
fleet_usb_bringup serial/HTTP contract, solenoid safety pattern, NVS POR
boot-loop guard (now unconditional, with the Phase-4 reset matrix as a native
test), cooperative TMF8820 one-shot machine, hex geometry. New: ADR 0023
LEDS_OFF tier + compound PROTECT release (coulomb-primary inputs, SOC
structurally excluded); deferred OTA rollback verify (extern "C" hooks + t+20s
self-test; --ota-fail-selftest drill build); class-by-probe (TMF ID-verified vs
the bench INA219 at 0x41; sensor-death keeps class_last); Greenberg-Hastings CA
+ bridge-show programs behind a lease-aware runtime (2 s crossfade fallback);
supply-based day/night lifecycle with bounded night (night_max default 10.5 h)
and energy-gated wakefulness (surplus = always reachable; dev profile never
day-sleeps -- the bringup posture, default in this image); minimal raw BMP581
driver (no new lib dependency); single canonical VL53L5CX ULD copy (from
sway_demo, all 5 edits).

Ops diffs: fleet_usb_bringup.py grew --sketch-dir (default net_bench,
back-compatible); dashboard RX_BOOT regex accepts both banners. ADR 0005
annotated (cooperative loop, constrained by ADR 0028). Everything up to the
hardware gates is done; the flash-and-verify checklist lives in
firmware/fixture/README.md. NOT yet run on hardware -- next session flashes a
board, runs the bringup tool against it, and starts the P0..P5 gates.

## 2026-07-29 -- Ben + Codex -- LED Studio TMF stalls removed

The sensor-reactive Studio UI was effectively unusable because each TMF8820
`startMeasuring(results)` call blocked the Arduino loop until a complete report,
delaying HTTP responses by 0.7-1.8 seconds. The old presence bench did not make
this call nonblocking: it quarantined blocking sensors in a core-0 FreeRTOS task
and served cached HTTP frames from core 1. That bench pattern was not reused
because this production-like chain shares `Wire1` with the charger/gauge and must
follow ADR 0028's single-threaded 100 kHz rule.

LED Studio now cooperatively executes the TMF driver's start -> process IRQ ->
stop one-shot sequence across main-loop iterations, caches each result, and
self-recovers a failed or overdue shot without blocking HTTP. Browser polling
also refuses overlapping requests. The UI now reports WiFi RSSI, measured request
latency, TMF frame age, and recovery count.

An intermediate `.2` continuous-mode experiment proved the low-level path but was
rejected after it restarted once per frame; it is not the fleet registry image.
The final `led-studio-2026-07-29.3` artifact is 1,067,200 bytes with SHA-256
`ED2CB3DA07A6F18363D1273B6D2124D0858B4F8FDA7651690215F9A2058424E9`.
On enclosed `F2BFA0`, 24 state requests measured 41.5 ms minimum, 111.7 ms mean,
204.1 ms p95, and 207.6 ms maximum; 20 button commands averaged 35.4 ms. RSSI
stayed -43 to -46 dBm. After 776 TMF frames there were zero errors or recoveries,
all three sensors remained healthy, and OTA recovery returned HTTP 200. The
fixture was left in warm-amber ToF-depth mode.

## 2026-07-29 -- Ben + Codex -- Sensor-reactive RGBW LED Studio OTA

Extended the unified RGBW LED Studio with an opt-in sensor-triad profile for the
enclosed `F2BFA0`. MSA311 tilt, BMP581 pressure-derived relative elevation, and
TMF8820 depth are sampled from the shared 100 kHz `Wire1` bus in the Arduino main
loop. The browser UI now has three reactive modes, live sensor readback, and
manual tilt/elevation zero controls. The ToF mode rejects the repeatable 20-21 mm
enclosure/window return and uses the nearest confident 80-2500 mm target.

Built once with the 6 Ah LFP / 500 mA / 4.6 V profile and OTA-uploaded that exact
artifact. `led-studio-2026-07-29.1` is 1,066,256 bytes with SHA-256
`B6540233C0FC2EDCB1E15B00832F660F444FDB5C18C93B5F7AEA17B68606413A`.
The live app at `http://10.0.0.200/` reported all three sensors healthy; browser
tests passed for manual RGBW, warm-amber ToF, tilt/re-zero, and elevation/re-zero.
The OTA recovery endpoint also returned HTTP 200. The fixture was left in warm
amber ToF-depth mode. This temporarily replaces its ESP-NOW net-bench image; the
qualified `.4` binary remains available at
`firmware/net_bench/build/tn-sensor-triad-f2bfa0-20260729-r1/net_bench.ino.bin`.

## 2026-07-29 -- Ben + Codex -- Dashboard controls and VINDPM floor corrected

The live dashboard made targeted controls look broken while the `All` view was
selected: the buttons silently refused to act and the error appeared only in a
small status line below the controls. It now automatically targets the sole fresh
peer, disables targeted actions when there is not exactly one target, and reports
command progress/errors prominently. Browser validation against the live COM13
bridge passed: a dashboard `KF2BFA0:40` reached the peer with no reset, a 4.0 V
entry was visibly rejected, and 4.6 V / 5.2 V commands were verified from returned
charger telemetry.

The PowerFeather SDK's actual VINDPM range is 4.6-16.8 V, not the 4.0 V that the
dashboard and current README had claimed. The SDK explicitly rejects values below
its 4.6 V reset floor. `net_bench` now uses 4.6 V as its command floor and only
updates its reported setpoint after `setSupplyMaintainVoltage()` succeeds. The
validation-only `net-bench-2026-07-29.5` peer build is 1,065,072 bytes with SHA-256
`FF4FDB54730C611BB60BF81504D32FC4F5B75CB50B83DB047BFE17AB2A5940A5`;
it compiled cleanly with the F2BFA0 sensor-triad/solenoid/6 Ah profile but was not
deployed, so the enclosure remains on qualified `.4`.

To test the apparent lamp-lit panel stall without rebooting, the live peer was
stepped from 4.6 V to 5.2 V and back. The BQ VINDPM register and input voltage
immediately followed both commands (5,200 mV / 5.216 V, then 4,600 mV /
4.644-4.648 V), proving the command and charger-setting paths were responsive.
Input remained 0 mA, the battery continued discharging approximately 0.11-0.16 A,
`supply_good` remained true, and BQ fault status remained zero. This rules out a
stale VINDPM setting; `supply_good` only proves acceptable input voltage, not
usable panel power. Next isolate the 60 W lamp/panel/cable path with a direct
loaded voltage/current measurement or a cover/uncover/reconnect test.

## 2026-07-29 -- Ben + Codex -- Enclosure sensor triad read over OTA

Added an opt-in `NB_SENSOR_TRIAD` diagnostic to `net_bench` for the production
MSA311 accelerometer, TMF8820 ToF, and BMP581 temperature/pressure chain. All three
run on the PowerFeather `Wire1` bus at the ADR 0028 limit of 100 kHz and cache a
sample every two seconds into maintenance `/telemetry`. The build wrapper exposes
the profile as `--sensor-triad`; the sensors are not yet carried in the ESP-NOW
heartbeat or shown on the live dashboard.

Built the peer once in
`firmware/net_bench/build/tn-sensor-triad-f2bfa0-20260729-r1/` with channel 11,
WonkyHouse, Generic_LFP / 6000 mAh / 500 mA / 4.6 V, guarded D7, and the sensor
triad. The 1,065,024-byte `net-bench-2026-07-29.4` binary has SHA-256
`C78342161A6B9E1E3ABF049AD0DB3187C3077D34878934947816722DBE95F9CD`.
Targeted shared-WiFi OTA to enclosed fixture `F2BFA0` at its observed
`10.0.0.200` address passed in 3.47 seconds without opening the enclosure or
pressing a button.

Live readings passed for all three sensors: MSA311 approximately
(-0.029, -0.002, 1.025) g; BMP581 28.17 deg C and 978.96 hPa; and TMF8820
16 results per frame with zero read errors, approximately 2,000 ambient units,
28 deg C internal temperature, and a closest return of 20 mm at confidence 255.
The 20 mm zone is likely seeing the enclosure/window or another very near object
and needs geometric interpretation before using it as a presence result. A final
`/resume` after the bridge command tail expired returned `.4` to sustained
ESP-NOW heartbeats at approximately -50 dBm. This proves electrical/software
bring-up only; production sampling cadence, energy cost, and enclosure ToF
calibration remain open.

The same live peer then accepted one targeted `KF2BFA0:40` bridge command. Maintenance
telemetry confirmed one 40 ms strike, zero failsafes, and no reset. The strike briefly
pulled the weak lamp-lit VDC source from approximately 4.65 V to 4.04 V and toggled
`supply_good` false before recovery. At steady state the 60 W incandescent-lit panel
reported 4.64 V but 0 mA input while the 97 percent-reported battery discharged at
approximately 0.13 A, so charge termination/full battery does not explain the zero
panel power; the artificial-light source was not sustaining net charge.

Local-control telemetry distinguishes the three paths. USER/GPIO0 was armed, while
neither a supported DFR0991 nor navigation switch was detected. The probe's
`0x2A` ACK / PID `0x0000` is also present in bare-board fleet records and therefore
does not prove that an external button is attached. The dashboard command path itself
works when `F2BFA0`, rather than `All`, is selected. After the diagnostic maintenance
read and command-tail expiry, `/resume` returned the fixture to fresh `.4` ESP-NOW
heartbeats.

## 2026-07-29 -- Ben + Codex -- First Tennessee enclosure peer joined WonkyHouse

At the Tennessee bench, the laptop was associated with WonkyHouse on 5 GHz and the
network scan confirmed a strong 2.4 GHz BSSID on channel 11, matching the fleet's
fixed ESP-NOW channel. The one USB-attached official fixture was `F2BED4` on COM13.
It was temporarily flashed with the previously qualified channel-11 serial-bridge
artifact (`net-bench-2026-07-13.3`, SHA-256
`9FEA2C68B72CD5F68AB6D975D83A5FD12CCDEECE6E424924F8D2A0A31BCCB7F4`)
and left running the dashboard at `http://127.0.0.1:8765/`.

The bridge immediately received the enclosed production peer `F2BFA0` running
`net-bench-2026-07-27.3`: approximately -61 to -62 dBm ESP-NOW RSSI, 3.33 V battery,
and good external-supply telemetry. A targeted `UF2BFA0` then moved only that peer
into shared-WiFi maintenance. It joined WonkyHouse at observed DHCP address
`10.0.0.200`, served matching `/telemetry`, confirmed Generic_LFP / 6000 mAh /
500 mA / 4.6 V, battery present, charging enabled, and solenoid support, then accepted
`/resume` and rejoined ESP-NOW. The first resume occurred before the bridge's
35-second targeted-command tail expired and the peer was pulled back into maintenance;
after the tail expired, a final `/resume` produced sustained fresh heartbeats at about
-55 dBm. No OTA image was uploaded.

This is the first live Tennessee proof of the complete enclosure path: power,
ESP-NOW discovery, targeted maintenance, the new case-sensitive WiFi credentials,
host reachability, telemetry, resume, and comms rejoin. The DHCP address is an
observation, not identity. Registry now marks `F2BFA0` WonkyHouse OTA-capable and
records `F2BED4` as the active temporary TN serial bridge.

## 2026-07-27 -- Ben + Codex -- Twenty-six peers migrated to WonkyHouse profile

Ben confirmed the two guarded `F4044C` pulses produced physical solenoid kicks, closing
the one-board VDC/actuator test. He then returned that fixture to USB for the Tennessee
packing pass. The temporary bridge `F4031C` had already been restored and verified as
a peer; this run put it and every other connected board on one uniform peer image.

Updated the gitignored `net_bench/wifi_secrets.h` to the case-sensitive WonkyHouse
credentials and advanced the firmware identity to `net-bench-2026-07-27.3`. The new
common artifact keeps ESP-NOW channel 11, 1 Hz heartbeats, Generic_LFP / 6000 mAh,
500 mA charge cap, 4.6 V VINDPM, and guarded D7/GPIO37 solenoid support. The isolated
build is `firmware/net_bench/build/fleet-tn-wonkyhouse-20260727-r1/`; its
1,031,328-byte binary SHA-256 is
`F24A1A930E26490C87A72F5760ED3D1355201A49680A3EAF1EEEA67DEBA316CC`.
Artifact inspection found the new SSID/password and no BubbyNet string.

All 26 USB-enumerated fixtures passed ESP32-S3/8 MB flash/2 MB PSRAM preflight,
exact-artifact upload, MAC-derived identity verification, peer role, `.3` version,
PowerFeather controller initialization, 6 Ah/500 mA/4.6 V configuration, solenoid
support enabled, and gate-low-at-rest telemetry. The run used the qualified 12-worker
limit and ended with 26 native-USB devices and zero Windows device errors. Evidence is
`ops/fleet/bringup/2026-07-27-ca-usb-wonkyhouse-fleet.jsonl`.

WonkyHouse was not visible from the California bench, so this pass could not prove
association, `/telemetry`, or `/resume`. The registry deliberately marks all 26
WonkyHouse profiles `ota_verified=false` until the first Tennessee network check.
`fleet_usb_bringup.py` now retains historical OTA success only when the credential
profile is unchanged, and has an explicit `--allow-battery-present` opt-in for a
deliberate mixed installed-battery batch while retaining bare-board safety by default.

## 2026-07-27 -- Ben + Codex -- VDC-only targeted OTA and solenoid strike passed

Used commissioned fixture `F4044C` as the first one-board indoor VDC qualification:
PowerFeather USB remained disconnected while a switched Sabrent port fed the fixture
harness into VDC/GND. A temporary channel-11 serial bridge heard the board over
ESP-NOW, sent only targeted `UF4044C`, and discovered it on BubbyNet at
`192.168.4.153`. The board accepted the exact 1,031,344-byte solenoid-enabled image
over shared-WiFi OTA in 4.03 seconds and rejoined ESP-NOW without a button press.

Post-OTA telemetry confirmed Generic_LFP / 6000 mAh / 500 mA / 4.6 V, a present
3.39 V battery, charging enabled, about 4.80 V and 472-474 mA at VDC, good supply
status, D7/GPIO37 solenoid support enabled, and the gate low at rest. Two separately
issued targeted 40 ms commands were ultimately recorded as two guarded strikes; the
first counter read was too early and still showed zero, so no further strike was
sent after the later count reached two. The supply remained good and the gate
returned low. Ben subsequently confirmed that the physical solenoid kicked.

The temporary bridge was official fixture `F4031C` on COM33. It was restored to the
exact standard fleet artifact and passed ESP32-S3 flash-ID, upload, and serial
verification afterward. The target registry row now records the distinct
solenoid-enabled artifact. This validates one fixture on hub-fed VDC for
power/charging, targeted OTA, and guarded solenoid actuation; multi-fixture VDC
loading remains open.

For planning a roughly 100-node tree OTA, current approximately 1.02 MB images take
about 5-6 seconds per node historically (4.03 seconds in this run). At the existing
five-job default, pure upload time is about two minutes. Budget 10-15 minutes when
all nodes are awake, and reserve 20-30 minutes for the first full-tree pass because
maintenance discovery, a possible 300-second sleep cadence, DHCP/client capacity,
rejoin verification, and retries dominate. A router-backed 100-node rehearsal is
still required before treating that as a guaranteed field time.

## 2026-07-27 -- Ben + Codex -- Twenty-six-device census and 12-way flash stress passed

Activated the final two Sabrent ports and reached 26 simultaneous native-USB
PowerFeathers (COM9-COM34), split as expected across 16-port and 10-port downstream
trees. The final two boards passed the full hardware, exact-artifact, serial, and
BubbyNet OTA commissioning path. A final census retained all 26 registered devices
with zero present USB error devices.

Stress-tested flash concurrency above the conservative four-worker starting point.
An eight-way full run passed 8/8 hardware preflight, upload, and serial verification;
one board hit a transient Windows `ClearCommError` only during the later simultaneous
serial-to-WiFi transition, then passed a single-board retry. Two disjoint, hub-balanced
12-way runs each passed 12/12 preflights, uploads, and post-flash serial checks in
29 seconds. The production-shaped confirmation then passed 12/12 uploads with WiFi
verification separately capped at four, completing in 42 seconds. For comparison,
the earlier 12-board four-wide full commissioning took 84 seconds.

`fleet_usb_bringup.py` now defaults to 12 flash workers and independently limits WiFi
verification to four. Twelve is the adopted operating point: it makes a 24-fixture
ring exactly two waves, has two independent clean upload runs plus a clean full-path
run, and increasing to 16 would still require two waves while reducing margin.
Historical OTA success is no longer erased by a later transient stress-test failure.

Ben proposed repurposing the powered hubs as an indoor 5 V source into PowerFeather
`VDC/GND`, using the same female-USB-C-to-XH fixture harness used by the Voltaic
panels while leaving the PowerFeather USB-C unused. The architecture supports this:
VDC accepts 5-18 V and is the actual solar/charger power path. Qualification remains
open and must start at one board with polarity/voltage measurement, the 4.6 V
maintain setting, and the current 500 mA charge cap before scaling. It is a stiff
indoor supply simulation, not a solar/MPP/shade/harvest test.

## 2026-07-27 -- Ben + Codex -- Twenty-four-board USB batch qualified

Completed the Sabrent/Anker scale-up after the earlier 12-board pause. Windows
enumerated 24 unique PowerFeather native-USB serial devices simultaneously (COM9
through COM32). The 12 newly visible MACs were added to the fleet registry and all
12 passed the same bounded four-wide commissioning sequence: ESP32-S3 revision 0.2,
8 MB flash, 2 MB physical PSRAM, exact `.2` artifact upload, PowerFeather controller
telemetry, live Generic_LFP / 6000 mAh / 500 mA / 4.6 V configuration, bare-board
charging disabled, BubbyNet `/telemetry`, and `/resume`.

After commissioning, held all 24 powered and repeated the census. All 24 remained
present and commissioned, with zero present USB devices in a Windows error state.
This qualifies the current two-hub topology for one 24-fixture bare-board intake
batch. Keep upload concurrency capped at four even when 24 are connected. This test
does not yet qualify 26 devices or simultaneous battery charging/LED loads; those are
not required for the initial bare-board flash workflow.

## 2026-07-27 -- Ben + Codex -- Production USB commissioning began; 12-board stage passed

Identified both powered Sabrent bench hubs behind the laptop's Anker USB-C adapter:
the HB-PU16 exposes four downstream four-port branches and the HB-BU10 exposes a
separate cascaded branch, though both ultimately share the laptop's USB 2 host path.
Staged native-USB enumeration from 2 -> 6 -> 12 bare PowerFeather V2 boards with no
device loss. The first twelve official boards now have MAC-derived fixture IDs in
`ops/fleet/registry.csv`; append-only discovery, flash, serial, and WiFi evidence is
under `ops/fleet/bringup/`. COM ports, USB paths, and maintenance IPs are explicitly
observations rather than identity or installation assignment.

Added `ops/bench/fleet_usb_bringup.py` for production intake. It recognizes the
ESP32-S3 native USB VID/PID, refuses an unexpected selected-device count, performs a
ROM/eFuse flash-ID preflight, uploads only an existing named Arduino build (never
compiles), limits concurrent uploads to four by default, checks live serial telemetry,
optionally verifies shared-WiFi `/telemetry` plus `/resume`, exclusive-creates its
JSONL evidence by default, and atomically updates the CSV registry. All twelve boards
passed ESP32-S3 revision 0.2, 8 MB flash, 2 MB physical PSRAM, PowerFeather
controller initialization, bare-board charging disabled, live Generic_LFP / 6000 mAh
/ 500 mA / 4.6 V configuration, exact-artifact USB upload, BubbyNet maintenance, and
return to ESP-NOW comms. Four simultaneous USB uploads and WiFi joins passed in each
full wave.

`net-bench-2026-07-27.2` adds a lowercase local-peer `u` commissioning path for
shared-WiFi maintenance without a separately flashed bridge, exposes live
flash/PSRAM/configuration and battery-presence/charging state in `/telemetry`, and
ports the proven deferred charge-enable guard into `net_bench`. A bare board now
starts with charging off and enables it only after a warmed, plausible battery
reading, avoiding the known missing-cell brownout loop. Battery capacity remains an
NVS runtime setting (`C6000` fleet-wide or `C<id>:6000` targeted); chemistry remains
build-time.

The final isolated build is
`firmware/net_bench/build/fleet-commission-20260727-r3/`: peer, channel 11, 1 Hz
heartbeat, Generic_LFP, 6000 mAh, 500 mA charge cap, and 4.6 V VINDPM. Its
1,023,488-byte binary SHA-256 is
`D4390B53400FBD6C187B6A5766ED24B0FF1343620182ABD1C80ED15F0AD5AD81`; compile usage
was 30 percent flash / 15 percent RAM. A prior `r2` compile stopped on a telemetry
variable-name typo and was abandoned rather than reused. An initial verification
mistook zero *initialized* runtime PSRAM for absent hardware; the corrected test
requires the ROM/eFuse probe to report the physical 2 MB and accepts the current
FQBN's intentionally uninitialized runtime PSRAM. The corrected six-board rerun and
the next six-board wave both passed completely.

Qualification is paused at 12 boards pending Ben switching on the next ports; continue
12 -> 18 -> 24 and hold a final 24-device census before declaring one-ring batches
qualified. The firmware currently embeds BubbyNet. Tennessee needs an AP cloned to
those credentials (preferably channel 11), or a final USB/BubbyNet OTA credential
migration before the known network is unavailable.

## 2026-07-27 -- Codex -- Synced origin/main and completed onboarding pass

Fast-forwarded local `main` from `cb61ec8` to `18e40f0` and reviewed the repository
orientation, current LOG/TODO state, canonical system architecture, and ADRs 0001-0031.
The incoming commits add corrected cap-bank CAD exports using the true 35.5 mm capacitor
height, a silkscreen/soldermask STEP variant for Fusion rendering, and the current
24-fixture perimeter-ring direction at about 6.5 m radius. No architecture decision or
new open item was introduced during this onboarding pass.

## 2026-07-27 -- Ben + Claude -- Gobo roles corrected; enclosure SKUs pinned; solarsim footprint fix validated against Elliot's rerun

**Gobo role correction (Ben):** perimeter fixtures DO carry gobos -- HEX + gobo,
the "dancing gobo": stepping the single lit pixel around the HEX board shifts the
apparent pattern on the ground. No gobo on trunk lights or chandelier (uplights
already correct). bom.md, SYSTEM.md, and the glossary said bare HEX on perimeter;
fixed.

**Enclosure figures pinned (Ben):** large = Polycase ML-70F*15 (10x7x4 in, NEMA;
72 downlights), small = Polycase HN-57-03 (6.7x5x3 in, NEMA 4x; perimeter +
trunk/uplight boots). Panel sits flush with the lid (raised a few mm for the
DC-cable bump on the panel back); light + ToF sit flush with the enclosure
bottom -- so the source hangs 4 in under the panel on downlights, 3 in on
perimeter, not the 6 in previously assumed from the 07-24 "deep LED drop" note.

**Solarsim panel-footprint fix:** per Elliot's RERUN_2026-07-27 finding, raytrace.py
sampled the 0.50x0.35 m stand-in rectangle instead of the true 5x3.5 in SOLAR_LIGHT
panel. Fixed (PANEL_W/H = 0.127/0.089); validated against the regenerated 88-panel
corrected-canopy reference: median wh ratio 0.69 -> 0.85. Residual localized: power
chain is near-exact given reference lit+svf (ratio 0.95, r 0.98); remaining ~10 pts
is the web-viewer Draco occluder mesh over-occluding vs the pinned .skp layer state
(SOLAR_REF hidden, all else visible incl SITE_CONTEXT). Details in ops/solarsim/README.

**Rotator viewer rebaked + extended** (ops/solarsim/bake_rotator.py, new): 112-panel
candidate + hinge variants + 36-angle rotation sweep all rebaked at the true
footprint (fleet median 9.9 Wh/day batt; rotation stays energy-flat). New viz:
gobo light cones (18.6 deg from 50 mm gobo at 6 in under the source) with overlap
readout -- 96 overlapping pairs at modeled hangs vs 1 at -1 m; hang-height selector
with re-raytraced Wh at -0.25/-0.5/-1.0 m (fleet median nearly flat, individual
lanterns swing 2x; -1 m breaks the 7 ft rule on 24/72); 6 ft scale figure; ToF
"baby monitor" FoV pyramids (TMF8820 down 60 deg 3x3, VL53L5CX outward 65 deg 8x8).
Also fixed a header-row hover crash in the panel list (from the Wh/day column
commit). All uncommitted pending the solar-visualizer-lights branch merge.

## 2026-07-26 -- Ben + Codex -- Seven-day solar result normalized; scheduled GPS/RTC timing adopted

Reviewed the completed 604,800-second P105/P126 logger
`ops/bench/data/ca/2026-07-17-ca-field-cycle-9F26F8-9F2690-weather-range-r2.jsonl`.
It ended cleanly on schedule at 2026-07-24 10:18 PDT rather than crashing. Fresh-
telemetry integration over seven consecutive 24-hour windows put the P105/RGBW at
about 113 Wh charger input, 94 Wh positive battery charge, and 102 Wh battery load
(about -8 Wh as run). P126 measured about 52 Wh charger input, 35 Wh positive battery
charge, and 49 Wh load (about -14 Wh as run). No active BQ fault was observed.

The P126 result requires an explicit sizing correction. Its no-lux field-cycle policy
ran roughly 13-15-hour shows; the earlier clean session measured 14 h 46 min at
157.7 mA average. That is a bench-policy artifact, not the intended 9-10-hour Black
Rock Desert show. At the same load, 9-10 hours is about 4.6-5.1 Wh/night, or roughly
32-36 Wh across this seven-day weather sample versus about 35 Wh charged. The P126
role was therefore approximately break-even after schedule normalization, with little
poor-weather margin; the literal "every day negative" chart must not be presented as
evidence that the 2 W panel is inherently undersized.

Ben chose deterministic scheduled dusk/dawn lightshows as the production direction.
ADR 0031 records a redundant sparse time architecture: four purchased SAM-M8Q modules
are the initial onboard-antenna GPS/GNSS anchors for absolute UTC, four purchased
Adafruit DS3231 modules are the initial battery-backed RTC holdover anchors, and
ESP-NOW distributes source/age/uncertainty to ordinary fixtures. This does not require
RTCs on all 150 nodes and does not make one anchor a permanent leader. Starlink may
commission/verify time during build week but is not an event dependency. Panel/lux
inference is demoted to bench telemetry, sanity checking, and a still-open bounded
degraded mode. Updated the canonical system architecture, solar test report,
choreography research note, TODO, and agent onboarding notes. GPS reception/energy,
RTC drift/backup behavior, final counts, schedule offsets, and invalid-time behavior
remain open; no firmware or hardware was changed in this documentation pass.

During the origin-integration review, also documented the previously unlogged
SparkFun PRT-27576 Qwiic Navigation Switch path already present in `net_bench`. With
`--solenoid-d7`, firmware read-only probes PCA9554 addresses `0x20`-`0x27`, recognizes
the expected input/non-inverted switch configuration, and maps debounced DOWN/GPIO1
to the existing guarded 40 ms strike. It leaves RGB and INT untouched and exposes
probe/press/error telemetry. This path remains an awake-only, physically unqualified
bench trigger; USER remains the deep-sleep wake source. No new firmware behavior was
added during this documentation/integration pass.

Pre-integration validation passed five focused `net_bench_log.py` output-safety tests
plus `py_compile` for the logger and OTA helper. An isolated exact-profile P126 build
also passed at 31 percent flash / 15 percent RAM:
`firmware/net_bench/build/field-cycle-peer-20260726-p126-solenoid-nav-validation-r1/`
used fixed 5.8 V VINDPM, 6 Ah LFP, 1.5 A charge cap, three full-bright spiral RGB
pixels, and `NB_SOLENOID_D7`. The 1,042,832-byte binary SHA-256 is
`967F420FFCABCFCBB429F8F682502C324FE3870449851E3C8FF9A1E85C664449`. This was a
build-only validation; it was not OTA-deployed.

The merge inventory also found several raw field-cycle captures from about 45 MB to
2.89 GB. They remain local and are now ignored by capture type; Git receives summaries
or explicit downsamples instead. The locally appended 181.6 MB June 30 trace was
hash-preserved as
`ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8-v7-bq-local-full.jsonl`
(SHA-256 `2536B3A30E231B25115A5AFFA4D62386C3CBBA31135EDA63697388542E03C5DD`)
before restoring the repository's original 8.6 MB tracked capture.

## 2026-07-26 -- Steve + Codex -- Local work products recovered for handoff

Recovered Steve's previously local-only Resonance artifacts onto a dedicated handoff
branch based on current `origin/main`: the 50 mm bamboo-leaf gobo SVG sources, a local
snapshot and previews of the Tri Star Print Farm Tracker, and the 2026-07-25 editable
solenoid comparison workbook with its preview and generator. Added README notes where a
local workbook is only a snapshot of a later live Google Sheet or where blank bench results
remain intentional. Dependency trees, inspection dumps, temporary files, and the open
Excel lock file were excluded. Superseded June battery-document edits remain preserved on
Steve's local safety branch and were not replayed over the current two-tier battery design.

## 2026-07-24 (cont. 2) -- Ben + Claude -- Gilisymo ToF "goggles" identified; install + integration notes

Research session ahead of attaching the Gilisymo covers to the VL53L5CX units; no
hardware touched yet.

- **SKU identified (high confidence, unconfirmed against invoice -- Gmail token
  expired):** the 60x "protective optical covers" are almost certainly Gilisymo
  **CG-VL53L5-D "dust free"** (supplier ref Hornix IR109C0-IC09-A066) -- the price
  math fits (EUR 7.50 base / ~6.00 at qty vs ~$384/60) and ADR 0027's stated dust
  purpose matches. ST community confirms IR109C0 as THE cover for L5CX. Visual
  check: dust-free = molded black oval, TWO windows with a raised center rib
  (septum); standard CG-VL53L5 = single continuous window, no rib. Confirm which
  is in the bag on receipt.
- **Mechanicals (from Hornix drawings on the product page):** 15.5 x 9.5 mm oval,
  3.08 mm tall; underside pocket 6.50 x 3.22 x ~1.5 mm deep drops directly over
  the module; 0.5-0.6 mm septum descends between TX and RX (this is why crosstalk
  is spec'd 0 kcps vs 0.1-0.3 for the standard). PEI+PC, 0-100 degC, >90 %
  transmission. Bottom tape, pre-applied.
- **Orientation:** the two windows are identical plain optical-grade windows (no
  lensing) -- in-plane 180-degree rotation is functionally equivalent; nothing
  distinguishes a TX vs RX end. Can't be installed upside-down (pocket + tape
  down, domed rib up). Minor mold features are not perfectly rotation-symmetric,
  so dry-fit first; if it rocks, rotate 180.
- **Install is ONE-WAY:** Gilisymo warns removal can damage the ToF. Therefore:
  functional-test each sensor BEFORE goggling; apply on a clean bench (dust
  sealed under the cover is permanent); press on the rim, never the windows.
- **Enclosure constraint for Steve:** the goggle IS the sealing/window element.
  The perimeter-hat outward aperture should be an open cutout clearing
  15.5 x 9.5 x ~3.1 mm -- do NOT put enclosure plastic/glass in front of it
  (ST AN5856: any extra window with an air gap >0.5-0.7 mm needs a gasket and
  reintroduces crosstalk).
- **Firmware/cal (to verify, n>=2 per shootout culture):** with 0 kcps claimed,
  default ULD xtalk data should be fine (the "calibration-free" selling point).
  Before locking a no-per-unit-cal fleet policy, bench-compare ranging with vs
  without the cover on a couple of units, incl. min-range/target-status, and try
  `vl53l5cx_calibrate_xtalk` (UM2884; target >=600 mm covering full FoV) once to
  see if it moves anything.
- 60 covers / 48 sensors = 12 spares; treat covers as consumables given the
  one-way install.

## 2026-07-24 (cont.) -- Ben + Claude -- "Solarnoid" design FINALIZED (downlights only); GPS/RTC timing experiments ordered

- **The solarnoid has its final form** -- VDC-tap solar supply + 22,000 uF cap +
  solenoid + craft-store bulk mallet (mallet order details TBC, "very cheap").
  Scope SETTLED narrower than the 07-16 promotion trend: **paired with the LARGE
  enclosures only** (needs the extra space) -> downlights, <=110. Perimeter's
  small hats sit it out; the 160 MOSFET drivers now carry a healthy surplus
  (+50 at the cap, more at the 72-downlight plan). ADR 0030 annotated; SYSTEM
  diagram + gates, BOM, glossary ("Solarnoid" entry) updated.
- **Timing experiments (ordered 2026-07-20, recovered)**: 4x SparkFun SAM-M8Q
  Qwiic GPS ($132.68) + 4x Adafruit DS3231 STEMMA RTC w/ batteries ($97.09).
  Purpose: accurate clock/time makes dusk/dawn bring-up and sleep scheduling
  trivial -- need not yet certain, bench quantities. Side note: the GPS units
  double as candidate position anchors for the ops/locate auto-localization
  work. Committed spend ~$25.2k; brief rev 16.

## 2026-07-24 -- Ben + Claude -- BATTERY FLEET GOES TWO-TIER: 130x 33140 15 Ah at an absurd price; 32700 6 Ah stays for small hats

**batteryhookup.com turned up 33140 LiFePO4 15 Ah cells at ~$4.50/cell** -- the
Alibaba price point that killed the 20 Ah idea, but domestic, no ocean freight.
Ben bought 130 over two orders on 2026-07-24 ($52.76 for 10 to Steve/TN +
$532.84 for 120 to CA = $585.60).

- **33140 15 Ah = the new fleet standard paired with the LARGE enclosures**
  (downlights, <=110 deployed): 2.5x the night budget for the class that spends
  the most light.
- **32700 6 Ah stays for the small-enclosure classes** (perimeter + uplight
  boots -- the small Polycase physically fits nothing bigger) and the
  chandelier. The 175 x 6 Ah now cover ~78-80 positions with ~+95 margin.
- **Qualification PENDING, per our own shootout culture**: capacity/IR run on
  the 33140 (n>=2, shootout rig) + re-derive the ADR 0023 dim/off/sleep map on
  the new cell (current tiers are 6 Ah-derived); DesignCap 15,000 fits under
  the MAX17260 16,383 driver cap (unlike the 20 Ah); verify physical fit in the
  large Polycase with panel + board + LED. TODO added.
- ADR 0025 annotated; SYSTEM fleet table + block diagram, BOM (two battery
  rows + spares math), AGENTS, README, glossary (33140 entry), ledger
  (~$24.9k committed) all updated; brief rev 15.

## 2026-07-17 -- Ben + Codex -- Outage-safe field logger deployed; P105/P126 capture resumed

Closed the host-side data-loss failure exposed by the July 15 Windows Update reboot.
`ops/bench/net_bench_log.py` no longer opens outputs with unconditional `w`: normal
launches exclusive-create and refuse any existing path, `--append` validates the first
and last JSONL rows and preserves the original run identity, and only explicit
`--overwrite` can truncate. New and resumed runs write machine-readable `src=segment`
boundaries; all following rows carry `segment_index` and `segment_started_utc`, making
the per-process `elapsed_s` reset explicit. `--segment-notes` records outage context.
Append refuses an empty/malformed file or repeated run-identity flags instead of
guessing. Added five stdlib regression tests covering exclusive creation, unchanged
collision refusal, identity-preserving resume, malformed-tail refusal, and deliberate
overwrite; `py_compile`, all tests, and `git diff --check` pass.

Started a fresh 604800 s run as PID 3132:
`ops/bench/data/ca/2026-07-17-ca-field-cycle-9F26F8-9F2690-weather-range-r2.jsonl`.
It began at about 10:18 PDT with an explicit segment-1 start row. A six-second live
verification saw the file grow from 100,235 to 130,175 bytes with empty stderr and 26
fresh peer rows apiece from P105 `9F26F8` and P126 `9F2690`. At the check both were in
charge: P105 about 3.304 V / +178 mA with 0.943 W charger input; P126 about 3.246 V /
0 mA with 0.303 W input. The COM4 dashboard remains live alongside the logger. Updated
the net-bench README and agent gotchas with restart/resume commands and the requirement
to verify actual file growth plus expected peer IDs. The separate firmware task to
retain the previous completed cycle across sunrise remains open.

## 2026-07-16 (cont.) -- Ben + Codex -- DFR0991 local D7 trigger OTA; module not visible

Added optional DFRobot DFR0991 I2C RGB-button input to the P126 solenoid build. The
implementation uses direct Wire1 register access rather than adding a new Arduino
library, reasserts the ADR 0028 100 kHz ceiling for every transaction, scans only the
module's eight DIP-selectable addresses (`0x23` through `0x2A`), and requires the
official `0x43DF` PID. A 50 ms debounced press edge calls the existing bounded 40 ms
D7 strike path once; release re-arms it, a hold does not repeat, and maintenance-mode
presses are ignored. It is deliberately awake-only: the four-wire Gravity I2C lead
does not carry the separate INT signal, and field deep sleep cuts the external rail.
USER/RESET remains the wake mechanism. `/telemetry` now exposes DFR0991 presence,
address, pressed state, press count, and read-error count.

Built the exact P126 field profile in
`firmware/net_bench/build/field-cycle-peer-20260716-p126-dfr0991-trigger-r3` (6 Ah LFP,
1.5 A charge cap, fixed 5.8 V VINDPM, three full-bright spiral RGB pixels, D7 enabled).
The 1,039,616-byte binary has SHA-256
`3EB4DE93FE4F5C2A9B71AA31403072CC3E71AE10A537070391283DF3A7229E5B`; its compile
flags exactly match the prior P126 image. A short outer-wrapper timeout lost the live
compile output, but the single compiler process remained active and was allowed to
finish; no competing build was started. Targeted OTA to `9F2690` succeeded and the
peer rejoined as `net-bench-2026-07-16.3`, software reset, charge phase, about 3.34 V.

Post-OTA maintenance telemetry reported `solenoid_rgb_button_present=false`, address
0, and zero runtime read errors. The direct transaction matches DFRobot's official
library (PID registers `0x09`/`0x0A`, status register `0x04`), and Wire1/VSQT was
enabled before probing, so the remaining first check is the physical Gravity-to-
STEMMA power/SDA/SCL adapter and cable seating. The peer returned to comms/field-cycle
operation after the diagnostic. Firmware support is deployed, but no physical
DFR0991 press has yet been observed and no D7 strike from that button is claimed.

Follow-up after reseating: Ben confirmed all DIP switches at `1` (`0x2A`) and correct
signal mapping from the STEMMA-to-female-Dupont cable (yellow -> SCL/C, blue -> SDA/D).
A probe from the subsequently confirmed `poweron` boot still reported `pf_ready=true`
but DFR0991 absent/address 0. During the next sustained-maintenance catch the peer also
logged one `brownout` reset at about 3.33 V battery, then recovered at about 3.34 V.
That single reset does not by itself prove the button wiring caused the brownout, but
combined with no I2C ACK it raises the priority of checking red -> `+`, black -> `-`,
measuring the module supply while VSQT is awake, and inspecting Dupont contact/shorts
before more powered reseating.

Ben then measured 3.3 V directly across the DFR0991 `+`/`-` header, confirming the
VSQT rail and power contacts reach the module. The one brownout most likely coincided
with accidentally bridging 3V3 and GND using the meter probes; treat that as the
leading explanation rather than evidence of a firmware regression. The unresolved
fault is now narrowed to SCL/SDA continuity/contact, a faulty module, or (less likely)
an unexpected I2C response/PID. Check the unpowered data wiring before adding more
firmware diagnostics.

`net-bench-2026-07-16.4` added eight non-blocking probes at 250 ms spacing plus raw
DFR0991 ACK/PID telemetry, without changing the P126 field profile or D7 safeguards.
The isolated artifact is
`firmware/net_bench/build/field-cycle-peer-20260716-p126-dfr0991-diag-r4/net_bench.ino.bin`
(1,040,496 bytes, SHA-256
`8807EEE6B0FBBF5DFC4E0947B8A2BE376CE44DE4B474D94E599964CE2AD47385`). It compiled
at 31 percent flash / 15 percent RAM with a flag string exactly matching `.3` and was
successfully OTA-deployed to `9F2690`; the peer rejoined as `.4`, software reset, about
3.34 V.

The new data narrowed the fault: after seven delayed attempts the module ACKed at
`0x2A` (`ack_mask=0x80`), proving the powered SCL/SDA path and DIP address, but PID
registers `0x09`/`0x0A` returned `0x0000` rather than DFRobot's `0x43DF`. DFRobot
specifies 3.3-5 V operation, so do not put 5 V pull-ups onto the ESP32 bus merely to
work around the invalid PID. Next independent test: measure the module's active-high
INT pin at 3.3 V. If it toggles on press, use a free 3.3 V GPIO as the simple trigger
and bypass the nonconforming I2C register interface; if not, treat the module as faulty
or separately bench-test it while electrically isolated from the PowerFeather.

## 2026-07-16 (cont.) -- Ben + Codex -- USER-button repeat-wake bug fixed in `.2`

Ben's first physical test found that USER/GPIO0 could wake and strike once, but after
the peer returned to daylight deep sleep later presses did nothing until a manual reset.
Live telemetry showed that the peer itself remained healthy and continued normal timer
wakes, isolating the fault to the new button re-arm path rather than the solar state
machine, ESP-NOW, or board power. The `.1` design retained an extra armed/disarmed flag
in RTC memory. That state was unnecessary and could remain disarmed across the
wake/sleep handoff.

`net-bench-2026-07-16.2` removes the retained button state. An actual EXT0 wake is now
the complete proof of one deliberate sleeping-button event and produces exactly one
40 ms strike. Active-mode presses still use 30 ms edge debounce. Before every deep
sleep, firmware explicitly clears the previous EXT0 configuration, samples GPIO0, and
re-enables active-LOW wake only when the button is physically released. If it is held,
the normal timer remains the only wake source for that sleep, preventing an immediate
reboot/strike loop. Maintenance suppression and every D7 pulse cutoff remain unchanged.

The corrected P126 artifact is
`firmware/net_bench/build/field-cycle-peer-20260716-p126-solenoid-button-rearm-r2/net_bench.ino.bin`
(1,038,224 bytes, SHA-256
`935A608CBA6A94E59CF31C29F9FBAF25E9BA22A48D29976E40CA7BE03388D28D`). Its compile
flags exactly match the prior P126 field profile. Targeted OTA waited for the broken
image's timer wake, uploaded successfully, and verified `9F2690` rejoined ESP-NOW as
`.2`, software reset, charge phase, about 3.34 V. Repeat physical wake testing remains.

Ben also proposed the DFRobot DFR0991 illuminated I2C button. It accepts 3.3-5 V and
has a separate INT output that goes HIGH on press, so it is not limited to I2C polling.
It can be an attractive awake-mode trigger. It cannot wake the current sleeping fixture
when powered from the switchable external rails because field sleep cuts both rails;
INT-based wake would require keeping the module powered, with its idle-energy cost, or
providing a separate always-on regulated source. No DFR0991 firmware was added here.

## 2026-07-16 -- Ben + Codex -- P126 local USER-button solenoid wake/strike OTA

Added a laptop-free local trigger to the existing opt-in P126 D7/VDC solenoid path.
With `--solenoid-d7`, the PowerFeather USER/BOOT button (`BTN`/GPIO0, active LOW) now
requests the same bounded 40 ms strike as the dashboard. The input has 30 ms debounce,
one press per release, and no hold-to-repeat. Because the daylight field cycle normally
sleeps for five minutes at a time, GPIO0 is also an EXT0 deep-sleep wake source. An
RTC-retained armed latch and a held-low check prevent immediate wake/strike loops.
Button presses are suppressed during OTA maintenance, and only a real EXT0 wake can
request a strike during boot; an arbitrary reset with GPIO0 low cannot do so. Existing
5-300 ms clamping, 80 ms rest guard, `esp_timer` cutoff, loop failsafe, and forced D7
LOW handling are unchanged.

Built the exact prior P126 field profile in the isolated directory
`firmware/net_bench/build/field-cycle-peer-20260716-p126-spiral-solenoid-button-r1`:
fixed 5.8 V VINDPM, 6 Ah LFP, 1.5 A charge cap, three full-bright spiral R/G/B pixels,
and all previous dusk/dawn/protection settings. The 1,038,368-byte binary has SHA-256
`61868DF9EB0295207E5E81E306E60361D5E6F9BE0468C5D844727A6331E80B45`; its compiled
flag string exactly matches the July 13 P126 image. Targeted shared-WiFi OTA to
`9F2690` succeeded without a button or physical reset. It rejoined ESP-NOW as
`net-bench-2026-07-16.1`, software reset, charge phase, about 3.34 V, with no DIM or
PROTECT latch. Physical awake-press and deep-sleep-wake validation remain pending.

Ben also noted that both solar devices were moved indoors in late afternoon July 15,
which likely satisfied the dusk qualification and turned the LEDs on early. Moving them
back outside late in the day did not visibly reverse the transition. Treat July 15 as
an intervention day rather than a clean weather-cycle point; retain the event as a
useful dusk/dawn hysteresis and weak-late-sun test case.

## 2026-07-16 (cont. 2) -- Ben + Claude -- 50 more MOSFET drivers (scope promotion in the air) + 30x BMP581 env sensors for the uplights

Missed order recovered: Adafruit 2026-07-16, $488.13 total.

- **50 more MOSFET drivers ($178) -> 160 total.** "The solenoids are cool enough
  we may promote them to a feature on all the downlights and perimeter lights"
  -- ADR 0030's scope sub-decision is trending fleet-wide on those two classes
  (annotated).
- **30x BMP581 temp + barometric-pressure sensors ($268.80)** -- a NEW sensor
  class: they ride the uplight STEMMA chain as generic environmental loggers
  (playa weather telemetry feeding the 2027 design). Uplights are no longer
  sensor-less; ADR 0027 annotated, SYSTEM fleet table + BOM + glossary updated,
  firmware TODO added (100 kHz bus rules apply; add temp/pressure to the
  telemetry tail).

Committed spend ~$24.4k. Brief rev 14 (sensors tile 330, donut re-flowed --
sensors now the #3 category, above solar).

## 2026-07-16 (cont.) -- Ben + Claude -- Solenoid bake-off status (Ben-reported; bench data pending commit): stronger solenoids, 0730B 6 V/1 A leads; transients benign

Status capture from Ben's bench work -- **the post-07-11 experiment data is NOT
yet in the repo** (likely on the bench laptop; only `solenoid_demo`,
`led_sol_bench`, and the 07-11 VDC sweep are committed -- commit-from-laptop
TODO added):

- **22,000 uF buys headroom for STRONGER solenoids** -- that is what the 07-16
  cap buy is really for, not just a better kick from the current parts.
- **A solenoid part bake-off is mid-flight; primary candidate: 0730B 6 V / 1 A.**
  The in-transit 3 V/5 V AliExpress units (150x, $319.12) may be RETURNED --
  return-window decision flagged in TODO + ledger.
- **Transient correction:** strikes do NOT confuse the BQ charger. They appear
  as droops on VDC, indistinguishable from a passing cloud / shadow on the
  panel. The "verify BQ transients" item is closed-benign; wording fixed in
  ADR 0030 annotation, ledger, BOM, TODO.

Treat the bake-off findings as directional until the data lands in the repo
(mid-experiment, Ben-reported).

## 2026-07-16 -- Ben + Claude -- Strike caps ordered: 210x 22,000 uF 16 V ($161.39)

Fleet-scale storage for the solenoid VDC-tap strike supply: 210x 22,000 uF 16 V
capacitors, $140.89 + $20.50 across two AliExpress sellers (lead-time hedge
habit). 2.2x the 10,000 uF that produced the excellent 07-14 kick -- hardware is
now committed to the VDC-tap direction; verification remains (confirm the
22,000 uF kick and that charge/strike transients don't confuse the BQ charger
input). Committed spend ~$23.9k. Ledger/BOM/TODO/brief updated; the noisemaker
to-buy residual shrinks to driver control cables + mallet mounting.

## 2026-07-15 -- Ben + Codex -- Planned Windows restart interrupted host trace; retained counters bound RGBW night

Windows Event Log showed that Windows Update (`MoUsoCoreWorker.exe`, followed by
`TrustedInstaller.exe`) initiated a planned service-pack / operating-system-upgrade
restart at 03:34 PDT. This was not a crash or power failure. The active JSONL ends
cleanly at 03:34:09, exactly when the restart began; Windows booted at 03:37. Neither
the dashboard nor logger restarted automatically. Reconnected the COM4 dashboard at
07:35 without sending fixture commands. Did not restart `net_bench_log.py` against the
old path because the current script opens output with `w` and would erase the
pre-reboot trace.

P105 `9F26F8` entered full-RGB DRAW at 20:29:37 and the trace remained continuous for
7 h 04 min 32 s through the restart. Subtracting the DRAW-boundary counters gives
2.973 Ah / 9.7 Wh over that interval, or 420.2 mA / 1.371 W average. At the last host
sample it was 3.241 V / -422 mA, with no DIM or PROTECT. Fresh retained telemetry at
07:36 showed cycle 2 CHARGE had begun at about 06:41, placing the complete RGBW window
at about 10 h 11 min. The new cycle minimum was 3.195 V, still above the 3.10 V DIM
threshold. Because sunrise reset the previous cycle counters before host reconnection,
the final night is a tight extrapolation rather than an exact retained endpoint:
about 4.25-4.30 Ah / 13.8-14.0 Wh (central estimate 4.28 Ah / 13.9 Wh). This is exactly
in line with the prior 417.6 mA RGB-full bench result and below the 14.7 Wh HEX stress
night.

As of 07:36, P105 had been in the new charge cycle for about 56 min but had recorded
<1 mAh / <0.1 Wh positive charge. The wake sample was still weak early light: panel
4.64 V / 2 mA, battery 3.246 V / -70 mA. Its apparent 113 mAh cycle discharge is the
known charge-sleep sample-and-hold overestimate, not a real one-hour drain.

P126 `9F2690` was still in DRAW at 07:37 because its no-lux fallback had not yet seen
>=20 mA useful panel input. It was 3.179 V / about -145 mA, panel 5.33 V / 0 mA, no
DIM/PROTECT. From the first observed DRAW row to the fresh retained row, its old `.3`
counters added 1.820 Ah / 5.9 Wh at about 156 mA; those totals retain the known
millisecond-truncation undercount. The wall-clock show was already roughly 13.4 h,
again demonstrating why the no-lux useful-current fallback is not the production show
schedule.

Queued two outage-recovery fixes: make the host logger refuse overwrite and support
explicit append/resume segments, and retain a previous-completed-cycle summary across
sunrise so a host outage spanning dawn cannot erase the exact night endpoint.

## 2026-07-15 (cont.) -- Ben + Claude -- NOISEMAKER DECIDED: solenoid bamboo-strike; #3885 speaker path abandoned (ADR 0030)

**The solenoids strike the bamboo so well that the speaker path is abandoned.**
ADR 0030 records the verdict: the MOSFET-driven solenoid mallet physically
knocking the bamboo IS the fleet noisemaker -- real percussion, the lantern as
the instrument, daytime solar-surplus by design intent (07-12). The #3885
percussion synth survives only as a bench/preview tool (`speaker_demo`); the
spare-speaker buy is cancelled; relay clicks and beeps are not pursued.

Remaining engineering (not candidate questions): 3 V vs 5 V variant A/B, strike
power source (VDC-tap + 10,000 uF cap leads after the 07-14 result), mallet
mounting vs O(1)-ops, per-class scope, daytime gating policy, and the strike
current/loudness numbers. Swept: AGENTS Decided/Open lists, SYSTEM diagram +
gates, README, BOM, ledger (+#3885 spares cancelled), TODO (candidate A closed,
opinions item overtaken, candidate B promoted), glossary (Solenoid mallet
entry), team brief rev 11.

## 2026-07-15 -- Ben + Claude -- Uplight power RESOLVED (hinged solar wing); 20 Ah cancelled; Polycase pinned; enclosure mapping corrected

Ledger + docs reconciliation from Ben's updates:

- **20 Ah is OUT -- and not on the merits.** The cell verified honest (07-12),
  but batteryspace cannot supply ~40 in time, and the Alibaba counterpart (a
  bargain at ~$4.50/cell bulk) needs ocean freight that misses 2026. Recorded as
  a 2027 lead. **Uplights instead get a hinged solar "wing" on the small Polycase
  boot** -- partial/shaded sun, likely carrying the P105 5 W (fits the panel buy:
  ~96 of 110 P105s allocated), 6 Ah cell, run mostly at low brightness with the
  budget tuned by Nevada City prebuild experiments. ADR 0025/0026 annotated;
  SYSTEM/README/AGENTS/glossary/BOM/ROADMAP/TODO swept. Chandelier stays likely
  6 Ah + USB-C in its carpenter box.
- **Enclosure vendor = Polycase; both orders placed 2026-07-13.** Mapping
  CORRECTED from the 07-13 entry: **large (111) -> downlights ONLY (<=110
  deployed); small (61) -> perimeter + uplight boots (<=60 combined)**. That
  retires the "zero large spares" flag (72 downlights planned vs 110 available)
  and replaces it with a softer one: the small pool caps perimeter + uplights at
  ~60 vs the loose 62-64 sketch -- allocation flexes at installation, and Elliot
  is flexible on the split. Two enclosures (1 large + 1 small) have TRANSPARENT
  LIDS -- show-and-tell demo units for explaining the fixture to visitors.
- Also propagated the 07-11 rail-fed amendment into the spots that still said
  "RGBW feed OPEN" (AGENTS Decided list, SYSTEM block diagram + validated list +
  LED section, firmware/ARCHITECTURE, README, glossary).
- To-buy queue now: uplight wing hardware, solenoid strike-power residuals,
  spare #3885s. Team brief updated to rev 10.

## 2026-07-14 (cont.) -- Ben + Codex -- RGBW dusk turn-on matches the 418 mA ceiling

Checked P105 `9F26F8` after Ben installed the production 4 W RGBW and the autonomous
dusk transition occurred. The peer entered DRAW at 20:29:37 PDT after the five-minute
low-lux qualification; the first DRAW heartbeat reported about 102 lux. The four-step
ramp settled near -385 to -420 mA. At the 20:54 check it was 3.295 V, -414 mA, or
1.364 W battery-side, with lux 7.3, no DIM/PROTECT latch, and no fault reset. That is
essentially the same current as the earlier independent full-RGB measurement of
417.6 mA, so the new `NEO_RGBW`, `R=G=B=255`, `W=0` profile is electrically correct.

The retained integrator correction also passed its first live cross-check. The
cycle-total discharge counter was already 238 mAh at the DRAW boundary because the
daytime charge/wait sleep estimator had accumulated wake-current extrapolations. After
1,535 DRAW seconds it read 416 mAh. The phase delta is therefore 178 mAh / 1,535 s =
417.5 mA average, matching the direct telemetry. `field_elapsed_s` is phase-local but
`field_discharge_mah/wh` is cycle-total; nightly analysis must subtract the counter at
the DRAW boundary rather than divide the absolute cycle counter by DRAW elapsed time.

## 2026-07-14 (cont.) -- Ben + Codex -- P105 production-RGBW ceiling OTA and build-pipeline hardening

Added an explicit single-pixel production-RGBW field-load profile to `net_bench` and
OTA-deployed it to P105 peer `9F26F8` for tonight's literal production-load ceiling
run. `--field-led-rgbw` selects one `NEO_RGBW` pixel in the module's slot-tested RGBW
wire order and drives `R=G=B=255`, `W=0`; the deployed image uses brightness 255 on
A0/GPIO10 and the switchable 3V3 rail. Fixed 4.6 V P105 VINDPM, five-minute
charge/wait sleeps, dusk/dawn thresholds, four-step load ramp, 3.10/2.95/2.90 V
DIM/LOW/CRITICAL thresholds, durable FULL/DIM/PROTECT guard, and recovery policy are
unchanged. This is deliberately a load substitution, not a state-machine experiment.

Also fixed the retained active-time integrator's known low bias. It now advances its
millisecond cursor only by the whole seconds actually integrated, preserving the
sub-second remainder for the next pass. This affects retained Ah/Wh/time accounting,
not load or transition policy. Tonight's logger-vs-retained comparison will validate
the correction.

The verified artifact is
`firmware/net_bench/build/field-cycle-peer-20260714-p105-rgbfull-r2/net_bench.ino.bin`
(1,029,216 bytes, SHA-256
`B7C75B53F278CE3A87E3301A376578569C994A51030E1F48EC3B42D233367FA6`). Compile used
30 percent flash and 15 percent RAM. The first uncached build was killed by an outer
120 s timeout; reusing that partial directory produced the expected `core.a` `bad
reloc symbol index` linker flood. It was abandoned and the clean `r2` build completed
in 133.5 s. The first OTA discovery safely expired at 75 s without uploading because
it missed the peer's 300 s sleep / 8 s listen window. A 360 s retry found
`192.168.4.105`, uploaded successfully, and verified ESP-NOW rejoin as
`net-bench-2026-07-14.1`, software reset, daylight-wait phase, no DIM/PROTECT latch.
P126 `9F2690` remained on `.3` and was untouched. Physical HEX-to-RGBW swap remains
Ben's before-dusk step.

Hardened the workflow around both failures. `field_cycle_ota.py` now supports
`--rgbw`, labels the profile in artifact/OTA notes, and defaults sleeping-peer
discovery to 360 s. `AGENTS.md` now tells agents to budget 2-3 minutes / >=300 s for an
uncached PowerFeather build, wait on yielded builds, never reuse an interrupted build
directory, recognize partial-archive linker corruption, verify `.bin` plus
`build.options.json`, build once then OTA by `--bin`, and distinguish discovery timeout
from failed flash. `firmware/net_bench/README.md`, `TODO.md`, and the consolidated
P105/P126 field record were updated with the new profile, artifact, and remaining
validation.

## 2026-07-14 (cont.) -- Ben + Codex -- P105 ideal-day closure and production RGBW full-night bound

Interpreted the first clean P105 night/day pair against the measured production-cell
capacity and the prior 4 W RGBW power sweep. The P105 cycle very likely began full:
its retained maximum was 3.598 V. It then delivered 4.505 Ah / 14.7 Wh over about
9.76 h before dawn, with a 3.179 V logged minimum. Against ADR 0023's 5.139 Ah usable
above the 3.0 V product floor, that is an 87.7 percent draw, not a literal full drain;
about 0.634 Ah remained above the conservative product floor (and about 1.25 Ah to the
2.5 V lab endpoint of the 5.75 Ah measured cell).

By 15:15 the same cell was back at 3.593 V with corrected charge current tapered to
+43 mA, then remained around 3.55 V with near-zero acceptance. That is strong direct
evidence that this outdoor P105 refilled the deep stress cycle by mid-afternoon under
the July 14 weather. The sparse charge integrals are less reliable than the CV/taper
observation. This supports an ideal-weather energy-closure claim, not a production
policy of deliberately emptying the cell nightly: tree shading, panel angle, dust,
cloud/smoke days, cold, aging, and recovery reserve still require margin.

The battery-only 4 W RGBW sweep (`2026-06-10-afk-sweep-0031.jsonl`) measured total
system draw, including the active controller/WiFi, at about 417.6 mA for full RGB
(`R=G=B=255, W=0`), 463.9 mA for all-four-channel RGBW full, and 193.8 mA for W-only.
For the roughly 9 h 53 min to 10 h 15 min playa civil-dark window, using 3.2 V as a
representative LFP energy voltage:

| Continuous look | Ah/night | Wh/night | Fraction of 5.139 Ah product-usable capacity |
|---|---:|---:|---:|
| RGB full | 4.13-4.28 | 13.2-13.7 | 80-83 percent |
| RGBW all four full | 4.59-4.76 | 14.7-15.2 | 89-93 percent |
| W-only full | 1.92-1.99 | 6.1-6.4 | 37-39 percent |

Therefore full RGB from the production point source should consume less than the
P105 HEX stress night's 14.7 Wh, but only by roughly 1-1.5 Wh. The as-measured full-RGB
case is conservative for a future duty-cycled radio policy; the exact production
sensor/radio load remains to be measured. Under weather like July 14, the P105 evidence
says an all-night full-RGB ceiling should refill. The next decisive qualification is
the literal production pairing: P105 + 32700 + rail-fed 4 W RGBW, `RGB=255/W=0`, a
fixed 10 h show, repeated across several weather/shading days. The right sizing output
is recharge margin by weather class, not permission to hit empty every dawn.

## 2026-07-14 (cont.) -- Ben + Codex -- Solar-cycle evening check; P126 resets identified as solenoid-test interventions

Checked the seven-day P105/P126 logger at about 16:55 PDT. The original PID 25296
was still running on UDP/54321, the JSONL was writing continuously at about 335.5 MB,
stderr remained empty, and a full streaming parse found zero malformed rows. There
were no host gaps over 5 s. The run remains healthy.

P105 `9F26F8` completed a roughly 9.27 h logged draw window and changed to charge at
06:35 PDT with dawn lux about 502. Its complete retained night counter reached about
4.505 Ah / 14.7 Wh discharged; minimum logged VBAT was 3.179 V. It had no
non-deep-sleep reset, dim event, protect latch, or BQ fault. The P105 BQ-input total
through about 17:00 was 16.58 Wh with a 3.865 W peak. It reached the 3.6 V CV/taper
region at about 15:15 (3.593 V, corrected battery current +43 mA), and was about
3.55 V at the check. The host sample-and-hold battery integral over the logger's
partial-cycle coverage was 13.21 Wh charged versus 13.91 Wh discharged (-0.70 Wh);
that small apparent deficit is within the known sparse-wake/current-hold uncertainty,
while the live taper is direct evidence that the cell refilled.

P126 replacement `9F2690` stayed in draw for roughly 11.64 h from logger start until
08:57 PDT -- intentionally longer than the planned playa show. Its retained counter
just before the first intervention showed about 2.275 Ah / 7.3 Wh discharged. Minimum
logged VBAT was 2.989 V at 08:56. The first manual reset at 08:48 was useful fault
injection: the persisted-session guard recognized the interrupted full-load session,
selected the one DIM retry (`field_reason=8`, `field_load_dimmed=true`), and remained
stable near 3.0 V until dawn cleared the session. It did not enter PROTECT.

Ben identified all 15 P126 `poweron` uptime drops as manual/solenoid-test interventions:
one at 08:48 and fourteen from 11:28-11:59. The later resets occurred during charge
and repeatedly restarted the retained field-cycle counters, so July 14 P126 endpoints
must not be subtracted directly or treated as spontaneous reliability failures. The
continuous host integration, which is the better summary for this intervention day,
showed 10.48 Wh at the BQ input, 6.66 Wh battery charge, 5.84 Wh battery discharge,
and net +0.82 Wh over the logged interval. Peak BQ input was 1.599 W. At the check it
remained in charge phase around 3.31 V with no BQ fault, dim state, or protect latch.

Shared P105 weather telemetry recorded 63.5 klux maximum, panel-back temperature
13.4-59.0 deg C, and 8-89 percent RH. Because the P126 day includes manual resets,
long cold-listen windows, and solenoid-driver testing, retain it as an intervention
day rather than a clean weather-only daily point. Even with that overhead and the
overlong night, the 2 W system was net-positive by late afternoon -- an encouraging
margin result for the planned 9-10 h HEX show.

## 2026-07-14 (cont.) -- Ben + Codex -- 10,000 uF turns P126 VDC solenoid into leading daytime design

The first P126-panel solenoid strike without local storage was qualitatively weak. Ben
then soldered a 10,000 uF, 16 V electrolytic directly across V+/GND at the panel-input
adapter and found that the strike "works so well." This materially reverses yesterday's
roughly 90%-likely 3V3 harness preference: VDC + local storage is now the leading
candidate, while the 815-strike-proven 3V3 path remains the fallback. This is a strong
bench/design result, not yet a production lock.

The assembly is unexpectedly elegant. Voltaic P105/P126 panels end in a 3.5 x 1.1 mm
male DC plug; the path uses Voltaic's female-DC-to-male-USB-C adapter, then a small female
USB-C-to-four-pin JST-XH breakout. Only V+ and GND are moved into the two-pin XH housing
that lands on PowerFeather VDC/GND; the other two breakout conductors remain unused.
The capacitor's diameter/lead geometry happens to align its leads directly with the
breakout's V+ and GND holes, making the addition about one minute of soldering with no
bare-wire PowerFeather work. The Amazon prototype capacitors cost roughly $1 each;
volume sourcing has not been investigated.

The daytime-only behavior is now attractive in its own right: solar percussion by day
and a lightshow by night gives visitors a reason to experience the tree twice. At 5.8 V,
10,000 uF stores an ideal ~0.168 J -- enough instantaneous energy for a convincing short
kick even though the 2 W panel replenishes it slowly.

Remaining qualification is deliberately retained: exact coil/pulse and VDC droop/
recharge capture; sun/cloud/shade and P105/P126 behavior; hot-plug, repeated-strike,
BQ/reset/fault, and dusk-residual-energy tests; possible bleeder; ESR/tolerance/
temperature/lifetime; polarity/keying and mechanical retention. The successful bench
part is 16 V while P126 published nominal Voc is about 8.59 V; measure worst-case cold
Voc and production tolerance to document the actual derating margin.
Updated the distributed choreography concept, solar field-cycle record, net_bench README,
and TODO. No ADR was made.

## 2026-07-14 -- Codex -- Seven-day solar-cycle JSONL logger health check

Checked the paired P105/P126 weather-range run without changing the process or
firmware. The original logger process (PID 25296, started 2026-07-13 21:19:22 PDT)
was still listening on UDP/54321 and writing continuously to
`ops/bench/data/ca/2026-07-13-ca-field-cycle-9F26F8-9F2690-weather-range-r1.jsonl`.
Its configured 604800 s duration should end about 2026-07-20 21:19 PDT. The stderr
file remained empty and stdout remained live.

At about 07:39 PDT, the file contained 111,471 rows / 177 MB covering 37,202 s.
A complete streaming parse found zero malformed or blank rows, zero timestamp
regressions, only master `192.168.4.72`, and only the two expected peers. The largest
per-source host gap was 3.0 s and no gap exceeded 5 s. P105's maximum reported
`age_ms` was 297,888 ms, matching its intentional 300 s charge sleep; P126 remained
fresh within 3.5 s. All observed reset reasons for both peers were `deepsleep`.
The logger's stdout `REBOOT` counter therefore reflects P105 timer wakes, not crashes.

Current write rate was about 4.6 KiB/s, projecting roughly 2.7 GB for the full run;
C: had about 564 GB free. Latest telemetry showed P105 in charge phase on cycle 2
with dawn lux available, and P126 still in draw phase at about 3.15 V. Neither peer
reported a BQ fault, dim state, or protect latch. No intervention or new TODO was
needed.

## 2026-07-13 (cont.) -- Ben + Claude -- Harness abundance + 172 COTS enclosures bought; chandelier goes carpenter-built

Procurement wave folded into the ledger/BOM/ROADMAP/TODO/enclosure docs
(~$5.9k this wave; committed total now ~$23.7k):

- **XH cabling in deliberate abundance** (~$575, multiple vendors as a lead-time
  hedge -- final harness lengths are unknowable until hats + fixtures mate, and
  the cables are cheap): 150x 10 cm red + 150x 10 cm black + 60x PH pigtails
  (Keszoox $220.26); 1,800x double-ended pre-crimped XH -- 150 each of
  yellow/blue/black/red in 30/20/10 cm (AliExpress $139.22); 70x + 90x JST XH
  5-pin Y-splitters ($94.96 + $120.81, split TN/CA); plus small receptacle/header
  orders. Sidebar settled first: EH pre-crimped cables are NOT XH-compatible
  (different housings AND terminals despite the 2.5 mm pitch) -- the buy stayed
  XH per ADR 0029/BOM.
- **Enclosure strategy update: the hat bodies are BOUGHT, not printed.** 172x
  COTS sealed enclosures + screws (~07-12/13, $822.67 to TN [11 large + 11
  small] + $4,483.83 to CA [100 large + 50 small]; vendor/part details TBC).
  Large (111) fits the larger panel -> downlight + perimeter hats; small (61)
  fits the smaller panel + doubles as the uplight boot. **Large-line margin is
  ~zero** (111 vs 110-112 needed) -- flagged in BOM + risk register. Steve's
  workstream shifts to integration (panel mount, bamboo clamp, USB-C gasket,
  ToF windows, thermal/RF on the real boxes); printing continues for
  gobos/fittings.
- **Chandelier housing: a team carpenter builds a box** for the 16-light
  cluster (not a hat variant) -- coordinate venting/access/USB reach.

## 2026-07-12 (cont. 2) -- Ben + Claude -- Noisemaker design intent: solenoids are DAYTIME-ONLY (solar-surplus percussion); night is the light show

Ben clarified the solenoid concept: strikes are a daytime feature ("something
cool the lights can do during the day occasionally"); nighttime is the light
show, untouched. This is why the VDC-tap strike supply (bench-supply results
good on the 2 on-hand units; panel-in-sun still untested) is the intended
topology, and it resolves the night-supply objection by design:

- Self-gating: no sun -> no VDC -> strikes physically impossible at night; a
  stuck gate at night has nothing to drain (pack-killer failure mode gone).
  Firmware interlock is cheap: strike only when sgood (solar guard reads it).
- Night energy budget untouched: ~0.2 J/strike from solar surplus; even 1000
  strikes/day ~ 0.06 Wh vs ~10-25 Wh/day harvest. ADR 0023 coulombs unaffected.
- Strike force tracks panel voltage (harder in bright sun, softer in clouds,
  harder again near Voc when a full battery tapers the charger) -- accepted as
  organic behavior, but LISTEN to the SOC/taper extremes on the bench before
  calling it charming.
- Engineering residuals for the VDC sweep: storage-cap bank sizing (~40-80 mC
  per strike; droop partly self-correcting via force ~ V^2), cap voltage rated
  for COLD-MORNING Voc (~7 V on P105 -> use 16 V caps), VINDPM sharing during
  cap refill (graceful by design; verify no input-requal latch), winding choice
  at panel voltage (5 V winding = matched ~3.5 W; 3 V winding = 2.2 A overdrive).

Battery/VS-fed strikes are now the fallback branch only; they would violate the
solar-surplus premise. Qualification of shipment samples (75x 3 V + 75x 5 V in
transit, different listing than the proven DS-0420S) still gates everything.

---

## 2026-07-15 (cont.) -- Ben + Claude -- Elliot confirms structure geometry; light placement is OURS to design

From the build dashboard (resonancenetwork.org/camp/build) + Elliot's
clarification via Ben: STRUCTURAL numbers are correct and supersede
BACKGROUND's early spec -- 6.5 m tree (was ~7.5), 10 m canopy, 24 limbs
(was 30), 2.7 m waist, 48x14 m grid shell, 9-day build in 3 shifts, Windelier
(55 chimes) Day 7. The dashboard's ~90-light sketch is NOT the lighting plan:
fixture count and placement are Ben + Steve's (+ Claude's) creative call;
all ~150 fixtures deploy, extras become off-tree "camp lights" (which also
double as the hot-spares pool for the 30-second swap flow). Consequences:
(1) the 10 m canopy stretches downlight ground spacing ~1.3x vs the 0.3.1
CAD, which combined with a deep (6") LED drop makes 7 ft hangs workable for
gobo non-overlap on the outer rings -- the inner ring still needs to move
outward, which Ben + Steve already planned for solar-shading reasons (bamboo
criss-cross overhead); (2) the next fixtures.json is authored by this
workstream (layout design TODO added: gobo spacing >= ~0.85 m, shading,
mild perimeter asymmetry for the registration gauge, real hang points from
the structural export); (3) localization conclusions are scale-invariant to
the wider canopy (spacing and position error scale together).

## 2026-07-15 -- Ben + Claude -- Auto-localization: sensor complement = class ID; uplight/chandelier ambiguity measured free

Ben's observation that the ToF payloads identify class on I2C (TMF8820 =
downlight, VL53L5CX = perimeter, neither = uplight/chandelier) is exactly the
assumption the ops/locate solver builds on (per-class assignment, class-typed
anchors, per-class registration cost). The one distinction hardware cannot
give -- uplight vs chandelier, both sensorless -- was tested by merging them
into a single assignment class: ZERO accuracy cost at the realistic operating
point across 3 seeds, with 100% class recovery from geometry alone (crown
clump vs uplight rings are far apart vs ~0.3 m position error). No
provisioning step needed to distinguish those boards. Report addendum added.
Also quantified the flag-rule ROC on a representative run (margin-score AUC
0.83): at the default threshold all silent-wrongs were chandelier -- the
non-chandelier fleet had 2 wrongs, both flagged.

## 2026-07-13 -- Ben + Claude -- CAD downlight artifacts patched; uplight elevation is (possibly) intentional

Ben inspected the CAD top-down and decomposed the 78 downlights: outer ring
24/24, middle 22/24, inner 20/24 DISTINCT positions (6 fixtures stacked at
duplicate coordinates mask the counts), plus 6 strays at the trunk base --
procedural-export glitches. New `ops/locate/patch_cad_0.3.1.py`
deterministically moves the 6 strays into the 6 ring holes (slots inferred
from angular gaps); `fixtures-0.3.1-patched.json` is now the tooling default
until the refined Blender export (already a TODO). Effect at the 4 dB /
3-beacon point: downlight class 0.94 -> 0.97 median (the strays were exactly
the trunk-occluded devices the rescue machinery kept fishing back); overall
~0.885 -> ~0.91. Also per Ben: the export's elevated uplights (two rings of
12 at mid-height) may be INTENTIONAL -- uplighting the upper trunk -- so they
are not treated as artifacts. Sidebar answered along the way: chandelier
devices can self-identify their CLASS from solved positions (~100% zone-ID;
only the within-crown shaft order is below the RSSI floor), and hardware
already distinguishes downlight/perimeter (ToF complement), leaving only
uplight-vs-chandelier needing zone-ID at all.

## 2026-07-12 (cont. 2) -- Ben + Claude -- Fixture auto-localization feasibility: ops/locate sim study, verdict = feasible-with-recipe

Ben's ask: simulate ~150 devices with playa-realistic noisy RSSI, solve a 3D
point cloud from pairwise RSSI + the per-class ToF z-anchors, register it onto
the CAD fixture layout, and find where it breaks -- the go/no-go gut check for
the "autoconfiguring tree" vs photogrammetry vs manual entry. Full study:
`docs/tests/AUTOLOCATE_RSSI_SIM_FEASIBILITY_2026-07-12.md`; tooling:
`ops/locate/` (NEW -- also the repo's first sanctioned numpy/scipy area, per
Ben; ops/bench stays stdlib-only).

- **Architecture**: solver library (`locate/`) strictly separated from the
  simulator (`sim/`) behind a JSONL contract real hardware will emit
  (pairwise censoring-corrected median RSSI + roster with ToF heights), so
  the identical math runs on real captures via `locate_run.py --pairwise`.
  Pipeline: anchored 2D-MDS init -> robust NLS in dB space (positions +
  per-device offsets + P0, huber, one-sided residuals for floor-censored
  links) -> CAD-footprint scale fix -> stranded-device rescue -> beacon-pinned
  gauge search -> per-class rectangular assignment -> confidence (exact LAP
  margins, registration-ambiguity ratio, Hessian proxy) -> flags. 30 tests
  (`locate_selftest.py`), 4 CLIs, PNG figures + a self-contained interactive
  HTML 3D viewer per run.
- **Verdict (sim)**: at the optimistic-to-middle playa noise estimate
  (sigma_link 2-4 dB) with 3 surveyed beacons: ~90% of 152 devices correct
  (~95% excluding chandelier), ~0.3 m median position error, silent-wrong
  4-6%, ~17% flagged for manual check. Breakage knee at ~5-6 dB -- INSIDE the
  upper half of the 2-6 dB playa estimate band, so the queued small-N real
  capture is the calibration gate before trusting it. Indoor-band noise
  (8-17 dB) is past breakage, consistent with ADR 0004's caveat.
- **Findings with operational teeth**: (1) receiver-floor censoring bends the
  map -- the firmware neighbor dump must record expected packet counts so the
  survivor-median bias is correctable (contract field `n_expected`); (2) a
  FROZEN solar-panel shadow collapses the method (0.18 acc) -- capture needs
  wind/orientation churn; (3) without beacons the rotational gauge rests on a
  measured 1-2% cost margin (dense layouts re-match under wrong rotations) --
  THREE hand-surveyed devices close it, and 2 beacons alone CANNOT pin the
  mirror (regression-tested); (4) downward ToF anchors are worth ~10 accuracy
  points and halve silent-wrong; (5) chandelier (0.24 m spacing) is below the
  RSSI resolution floor -- manual mapping for those 16.
- CAD ground truth: fixtures.json from `Lighting-Controller` (vendored,
  commit 0558a5d) with Ben's scale ruling (downlights 7-10 ft is truth, the
  export's unit claim is not); perimeter ring synthesized (absent from
  export); duplicate-slot groups + 78-vs-72 handled in scoring.
- New TODO section "Fixture auto-localization" (small-N capture gate,
  firmware pairwise dump, CAD re-export, beacon planning, perimeter ToF
  downtilt, ADR 0030 after Ben's review). Machine notes: user-site scipy
  1.15.3 installed (system 1.8 broken vs numpy 2.x); system mpl_toolkits
  shadows user matplotlib (no mplot3d -- 2D panels + HTML viewer instead).
## 2026-07-13 (cont.) -- Ben + Codex -- Rootless autonomous choreography concept documented

Added the explicitly non-binding research/design note
`docs/research/AUTONOMOUS_DISTRIBUTED_CHOREOGRAPHY_CONCEPT_2026-07-13.md` as a
strong candidate production-firmware direction. The top-level abstraction is a generic
distributed choreography runtime, not a CA-only engine: CA, deterministic timelines,
spatial light/solenoid ripples, presence-triggered easter eggs, and temporary
bridge-directed performances share one observation/event/scheduling layer. The normal
artwork remains autonomous with no permanent coordinator. A bridge is an optional
leased participant for DJ modulation, special shows, health checks, identification,
photogrammetry/registration, and maintenance; expiry returns nodes to autonomy.

The note records the field-inferred default sleep-clock error (roughly 9-15 minutes/day
fast across the observed 300/900 s sleeps), distinguishes wall time from local monotonic
time and peer-corrected fleet phase, and proposes an A/B of the ESP32-S3 internal
8.5-17.5 MHz/256 RTC source. Its documented roughly +5 uA cost is only 0.12 mAh/day,
negligible beside the current eight-second/five-minute bench radio window. The better
clock would improve holdover/rendezvous efficiency but is not a correctness dependency;
POR reacquisition and peer resynchronization remain mandatory.

Daytime telemetry windows may double as rootlessly synchronized solenoid chorus/ripple
events, with compact future-scheduled programs for longer audio scenes. Autonomous night
behavior is explicitly broader than CA; a distributed presence condition can schedule
a preloaded synchronized sequence without a master. Raw distinct-origin observations,
idempotent future events, local energy vetoes, adaptive day/twilight radio duty, bridge
leases, fungible runtime capability/site data, failure behavior, implementation phases,
and measurement gates are all captured.

Solenoid power remains OPEN pending the capacitor arrival/test on July 14. Current
roughly 90 percent likely MVP is the previously battle-tested switchable 3V3/GND path,
using the purchased five-pin JST-XH Y-splitter to keep the harness in XH land, tentatively
sharing power with LED signal A0 and solenoid signal A1. The VDC-plus-capacitor branch
must produce a clearly better strike or power result to earn its extra parts, inrush,
packaging, and assembly. This records a strong design hypothesis, not a hardware lock.

## 2026-07-13 (cont.) -- Ben + Codex -- Seven-day paired P105/P126 weather-range logger started

The prior P105/P126 JSONL run did not crash: it reached its configured 259200 s
(72-hour) duration and closed cleanly at about 15:25 PDT. At about 21:19 PDT a new
seven-day UDP logger was started for the adjacent outdoor peers `9F26F8` (P105 5 W)
and replacement `9F2690` (P126 2 W):
`ops/bench/data/ca/2026-07-13-ca-field-cycle-9F26F8-9F2690-weather-range-r1.jsonl`.
It is configured for 604800 s, is not pinned to a DHCP address, and therefore can
coexist with the COM4 dashboard and survive a bridge IP change. Initial verification
captured both peers through master `192.168.4.72`; the logger error file was empty.

The analysis target is one row per complete America/Los_Angeles day and peer: positive
corrected battery Ah/Wh, discharge Ah/Wh, net delta, charge/draw phase duration, peak
charger-input power, minimum loaded VBAT, and reset/dim/protect events. Do not use the
MAX17260 SOC field for this comparison. Sum monotonic device-counter increments within
each day and segment counter decreases, cycle changes, and real uptime resets rather
than subtracting endpoints blindly. P105 lux, panel temperature, and humidity are the
shared weather proxy for the two adjacent panels; BQ supply power remains charger-input
telemetry, not panel-lead ground truth.

## 2026-07-13 (cont.) -- Ben + Codex -- Failed P126 board retired; replacement 9F2690 USB-flashed and safety-verified

The former P126 peer `9E5B0C` did not recover after the D7/VDC/GND header rework. With
the battery and USB paths checked separately it still had the expected battery voltage
at JST/VBAT, about 3.3 V at RST/BTN, and about 5.0 V at VS on USB, but it would not
enumerate or enter the ESP32-S3 ROM loader on a known-good cable. The likely fault is
physical damage or a solder bridge near the ESP module pads during header rework; the
board is retired rather than treated as an OTA/firmware failure. No `.3` image or strike
command ever reached it.

Replacement PowerFeather `9F2690` (`D8:85:AC:9F:26:90`) passed the ROM-loader probe and
was USB-flashed with the already-built P126 image
`field-cycle-peer-20260713-p126-spiral-solenoid-d7-r1` (`net-bench-2026-07-13.3`, binary
SHA-256 `EE26C8CCE33A2EE4939B3334F76DFEB49EA1B0FEF4A65E30D8F85F995D4FB992`). Flash
verification and reset succeeded. The peer then joined the channel-11 bridge with
65/65 frames, RSSI about -4 dBm, and the expected fixed 5.8 V VINDPM, 6000 mAh LFP,
1500 mA charge limit, and three-pixel full-bright R/G/B spiral profile.

Direct telemetry before any driver connection verified `solenoid_enabled=true`, GPIO37,
gate LOW, zero strikes, zero blocked commands, and zero failsafe trips. VBAT was about
3.28 V. The board was explicitly returned from targeted maintenance to normal ESP-NOW
mode. Solar and the solenoid driver remained disconnected throughout; the next action is
the first bright-sun, no-capacitor 40 ms strike while watching reset and supply telemetry.

## 2026-07-13 (cont.) -- Ben + Codex -- P126 D7/VDC solenoid control built; bridge deployed, peer OTA waiting for power

Added an opt-in `--solenoid-d7` path to `net_bench` for the P126 bright-sun
strike-power experiment. PowerFeather D7 is GPIO37, not GPIO7. The feature has no
boot strike or repeat mode: a targeted `K<id>:<ms>` command produces one 5-300 ms
pulse (40 ms dashboard default), with an 80 ms rest guard, `esp_timer` one-shot,
loop deadline, and forced gate LOW before board initialization plus OTA, maintenance,
and sleep transitions. `/telemetry` exposes the enable/pin/gate state and
strike/block/failsafe counters.

The local dashboard now has a selected-peer `Strike D7` button and validates the
same pulse bounds. The USB serial-bridge master `9E5AB8` was flashed successfully to
`net-bench-2026-07-13.3`; the dashboard restarted on COM4, reconnected, and served the
new control. Both the bridge and P126 variants compiled in separate build paths, and
the P126 image preserves its fixed 5.8 V VINDPM, three-pixel full-bright R/G/B spiral,
6 Ah capacity, and 1.5 A charger profile.

The intended test wiring is the Adafruit #5648 MOSFET driver on D7/VDC/GND, initially
without the VDC storage capacitor. Its official schematic confirms a 10K SIGNAL-to-GND
pulldown plus onboard flyback diode. Therefore the previous P126 firmware, which left
D7 as a high-impedance input, did not actively command the solenoid on; the hardware
pulldown held the MOSFET off.

Peer OTA is NOT yet complete. `9E5B0C` was already stale before deployment work and
did not appear during 20 minutes of targeted `U9E5B0C` retries, 324 fixture-ID-checked
maintenance scans, or its expected 900 s protect wake boundary. The last cached sample
was old firmware `.1`, 3.285 V, no supply, phase protect. Next step: leave the driver
unplugged, restore panel/battery power, tap RESET once, then rerun the already-built
targeted OTA and verify `.3`, `bq_vindpm_mv=5800`, `solenoid_enabled=true`, pin 37,
gate LOW, and zero strike/failsafe counters before Ben connects the driver and initiates
the first 40 ms strike.

## 2026-07-13 (cont.) -- Ben + Codex -- Production power-policy hardening plan codified

Recorded the production disposition of the P105 POR work in
`docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md` and ADR 0023. Working assumption is
that the external INA harness caused the unusually early P105 collapse, but that event
is treated as useful fault injection rather than a reason to remove the recovery logic.

The plan keeps cause-independent default-off rail ownership, durable load-tier state,
one pre-consumed DIM retry, staged startup, night latching, and PROTECT persistence
across POR/watchdog/OTA resets. It explicitly leaves the P105 3.10 V dim point and exact
ramp timing as bench calibration to be re-derived on production wiring. The canonical
3.00 / 2.95 / 2.90 V tier remains an ADR 0023 starting point subject to known-load,
class-specific, and cold qualification.

The first post-INA run is a controlled A/B: remove the instrumentation but do not change
`net-bench-2026-07-13.2`, the load, or the thresholds for one complete cycle. The plan
defines telemetry to retain, comparison points, a deterministic reset/fault matrix, and
production exit criteria. It also identifies the present single-sample +20 mA PROTECT
release as not production-ready; replace it with valid charger/no-fault state, sustained
positive corrected current, recovered VBAT with hysteresis, and preferably positive
coulombs. RepSOC remains advisory only.

## 2026-07-13 -- Ben + Codex -- P105 dark-show repair OTA: GPIO4 rail truth, stable night latch, durable protect, P105=4.6 V

Investigated why outdoor P105 peer `9F26F8` reported drawdown near 22:00 while its
HEX was dark. The live logger separated two firmware regressions in
`net-bench-2026-07-12.1`:

- Phase 4 drew only about 116-134 mA INA instead of the historical roughly 470 mA.
  `drawdownPixelsRailOnCleared()` deinitialized EN_3V3/GPIO4 immediately before the
  PowerFeather SDK's RTC-pin setter. Worse, the SDK ignores the result of its actual
  `rtc_gpio_set_level()` call and can return `Result::Ok` while the physical rail stays
  low. Final firmware explicitly initializes GPIO4 as RTC input/output, drives it high,
  reads the level back, and re-enables hold before applying pixels.
- Intermittent TSL2591 samples switched dusk qualification dynamically between the
  5-minute sensored threshold and 30-minute bare fallback. That turned missing lux into
  synthetic daylight and produced repeated 1-2 s drawdown / false-sunrise cycles.
  Sensor capability is now retained across deep-sleep wakes, and entry to drawdown is a
  durable darkness latch: only positive supply/lux dawn evidence can end the show.

Live `.3` rail validation proved the electrical drive and exposed a real startup event
that the falsely-off rail had hidden. Full HEX load reached about 493-506 mA INA and
2.93 V, then POR. The NVS guard consumed its one dim retry; that ran about 299-308 mA at
3.07-3.09 V for roughly 9 s, then POR again. The next boot hard-parked with phase/reason
`5/8` instead of looping. The final `net-bench-2026-07-13.2` image therefore also:

- stretches the four-step ramp from 0.4 s to 3.2 s so delayed harness/cell sag is seen
  before full brightness;
- waits 10 s after a full-load POR before the single dim retry;
- preserves the P105 3.10 V dim profile; and
- treats persisted NVS `PROTECT` as authoritative across every reset type, including
  OTA software reset when RTC phase state is reinitialized. Persisted DIM also survives
  deliberate software reset.

Corrected a deployment-profile mix-up during the repair: **P105 5 W uses 4.6 V VINDPM;
P126 2 W uses its separately swept 5.8 V point.** The generic/P105 OTA-helper default is
back to 4.6 V; P126 must request `--maintain 5.8` explicitly.

Final targeted shared-WiFi OTA to `9F26F8` succeeded with no button/USB. Direct
maintenance telemetry and the independent ESP-NOW logger both verified
`net-bench-2026-07-13.2`, `bq_vindpm_mv=4600`, phase 5/reason 8, and the protect latch
still set after `/resume`. This is intentionally dark until verified charge releases
protect. Next daylight/USB recovery must validate the slow start and distinguish a
remaining dim-load POR between instrumented-harness resistance/protection behavior and
the hypothesized power-I2C/BATFET disconnect path.

## 2026-07-12 (cont.) -- Ben + Claude -- Procurement update: 90-board batch ordered, noisemaker fleet buys, ledger reconciled

Ben's order updates folded into `ops/PROCUREMENT.md` / `ops/bom.md` / ROADMAP /
ADR 0024 (annotation) / TODO:

- **pf-batch-2 is real and bigger: 90x PowerFeather V2 ordered 2026-07-09 for
  $3,494.24** ($30/board + s&h + bank fee + tariff) -- grew from the planned 82.
  Production boards now total 158 (+~8 bench); the spares-thin risk is RESOLVED.
  Residual risk is CN transit only (chase tracking by ~07-16).
- **Noisemaker candidate-B fleet buys (2026-07-10):** 100x Adafruit MOSFET drivers
  ($345; 110 total with 10 from a prior ~$46 order) + 150x AliExpress push-pull
  solenoids, 75x 3 V + 75x 5 V for the voltage A/B ($319.12).
- **USB-C rescue port goes UNIVERSAL (2026-07-10, $860.34 Adafruit order):** 150x
  waterproof panel-mount USB-C extension cables ($540) -- one per fixture, wired to
  the PowerFeather USB-C, so any lantern can be rescued/charged without opening the
  hat (solar-free classes charge through it). Same order: **50x RGBW top-up**
  ($247.50) -- 150 RGBW total, spares healthy at any chandelier mix. Committed
  electronics spend now ~$17.7k.
- **Grove breakouts DONE, twice over:** 55x Electromaker 07-10 ($85.26) -- and a
  FORGOTTEN ORDER recovered while reconciling: 70x RobotShop 2026-06-18 ($64.86),
  shipped straight to Steve in TN. 125 total vs ~46-48 needed. Committed spend
  ~$17.8k.
- Ledger audit of what remains un-placed: the JST-XH harness set (UNBLOCKED by the
  07-11 rail decision -- now the biggest outstanding order), ~40x 20 Ah cells +
  end-cap connection hardware (sample-2 gate), solenoid strike-power residuals
  (VDC-tap Y-cables + storage caps vs battery/VS, driver control cables, mallet
  mounting), spare #3885 speakers. Camp-side USB-C charging gear (chargers/hubs
  off camp power for the solar-free classes) belongs to no workstream yet -- raise
  with Elliot.
- Team brief refreshed to match (board counts, ~$16.9k spend donut with a
  noisemaker slice, timeline, risks).

## 2026-07-12 -- Ben + Claude -- 20 Ah UPLIGHT CELL VERIFIED: 19,412 mAh (97.1% of label), knee so tight 95% clears the product floor

**The batteryspace 20 Ah cylindrical ("34184", sample 1 of 2) is honest — the anti-Palowextra.**
Full charge->discharge on the shootout rig (board 9E5AF0, INA 0x45 truth, HEX37 val224
~0.78 A): **19,412 mAh to cell-side 2.5 V (97.1%)**, **19,055 above the 3.0 V product floor
(95.3% of label)**, knee width just **360 mAh** (vs F's 613, P's 1,301 — the low-IR big-cell
signature). 27.2 h run, zero resets for 23+ h, 181 only in the terminal rattle; ended
"unreachable" when the board could no longer restart at cell ~2.5 V. Gauge bias **x1.071 —
9th consecutive** +8+-1% replication. Report + charts:
`docs/tests/BATTERY_20AH_UPLIGHT_REPORT_2026-07-12.html`.

Method firsts: cell too big for any charger on hand -> **BQ25628E on-board charge through the
same INA** (6.03 Ah in; free polarity check; the charge chart is a three-throttle story: USB
hub 500 mA BC1.2 cap -> wall brick ~820 mA -> CV taper). Leads were alligator clips at a
DMM-verified **0.263 ohm** -> all cell-side numbers use cell_v = board_v + I(t)*0.263; cutoff
2.40 V board-side ~= 2.5 cell-side. SDK gotcha: **MAX17260 driver caps DesignCap at 16,383 mAh**
(20 mohm sense assumption) -- Board.init(20000) fails into silent retry purgatory; ran at 16000
(SOC advisory anyway). Gauge cold-POR mute reconfirmed twice post-rattle (POWERFEATHER_NOTES).

**Uplight verdict:** one cell = **6 h/night x 7 nights solar-free at ~450 mA avg RGBW**
(showable brightness) with margin; W-die palettes trivial. Conditions: watchdogged daytime
sleep (ADR 0023) + night radio duty budget (~1.7 Ah/wk at 40 mA avg). **n=1 — qualify sample 2
before the ~40-cell buy** (rig stays assembled). Production needs a real end-cap connection;
clips survived 27 h on tape and prayer.

## 2026-07-11 (cont.) -- Ben + Claude -- RGBW feed A/B (rail vs VBAT), instrumented: RAIL wins +2.5% mean, ADR 0029 fork closed

Automated the rail-vs-VBAT question with two 4 W RGBW units mounted at once (unit1
on the hand-soldered {VBAT|EN|VS|D13} header, unit2 on the standard 3V3/A0 header),
VEML7700 on the STEMMA-QT port (ina_monitor register pattern: gain 1/8, IT 100 ms),
`led_sol_bench` feed toggle switching pin+rail together, and `ops/bench/ab_lux.py`
(ABBA ordering per look, dark baseline per look, per-row gauge/supply telemetry,
CSV: `ops/bench/data/ab-lux-2026-07-11.csv`, 7 runs). Battery-only the whole
campaign (the production night condition), SOC 88->78%, resting bv ~3.31 V flat.

Design that made it trustworthy: cable-crossover 2x2s at FROZEN geometry (swap
feeds at the converters, touch neither modules nor sensor), three geometries.
Confounds caught en route -- each would have flipped the naive conclusion:

- Run 1's "VBAT +16..45%" was unit binning + geometry, not feed (unit-to-unit
  spread up to ~20% on blue; position worth ±5-20% per look, look-dependent).
- A stray piece of plastic sat on one module for runs ~2-3 (found run 4: its
  removal jumped that unit +4..+19%); the contaminated contrast was discarded.
- mDNS blipped mid-run once (script now resolves-once + retries).

Pooled clean contrasts (5 sets: same unit, same position, feed swapped; rail
advantage vs VBAT):

| look | contrasts | mean |
|---|---|---|
| W only | +3.1 +5.0 -0.8 +1.6 +1.9 | +2.2% |
| red | +2.9 +2.4 -3.0 +2.0 +1.9 | +1.2% |
| green | +4.2 +1.6 +1.6 +1.5 +1.4 | +2.1% |
| blue | +4.5 -0.2 +4.0 +2.1 +2.1 | +2.5% |
| RGB white | +6.6 +4.1 +3.9 +4.4 +4.4 | **+4.7%** |

Rail wins 22/25 comparisons (mean +2.5%, range -3.0..+6.6%); the third 2x2 was
the cleanest (both units agree per look to a few tenths of a %). Physically
coherent: the highest-current look (fringed RGB white) shows the rail's LARGEST
margin -- IR drop in the realistic VBAT path (WAGO + XH + converter) scales with
current. ADR 0029's fat-wire "+33% VBAT" does not survive production cabling; it
inverts. And this ran at SOC 78-88% -- the condition MOST favorable to VBAT;
overnight (3.2-3.3 V terminal) the rail's edge only grows.

**Decision recorded (ADR 0029 amendment): 4 W RGBW stays on the 3V3 rail.**
Rail also keeps the hard LED kill (ADR 0013 by construction), one harness/pinout
for both LED roles, no Y-cables (~100 dropped from procurement), no per-board
soldering, and coulomb accounting intact (VBAT header tap is upstream of the
gauge shunt = gauge-blind to the dominant load). Harness buy unblocked.

Also re-read the shootout data on Ben's challenge -- he was right, prior session
summary corrected: on the production 6 Ah cells (cycle 0), gauge RepSOC is ~2x
pessimistic through the midrange and parks at 1% from ~59-61% delivered
(F: 1% at 3,515 of 5,751 mAh; P: 3,308 of 5,643); the 1->0% step lands at 98-99%
delivered (bv 2.5-2.8) -- a genuine "dying now" edge. Coulomb integral remains
the good signal (+8-9% bias, /1.08, ADR 0023 unchanged). Fleet-ops note: a
fixture reporting 1% SOC all night is Tuesday, not an emergency. Un-answered:
whether learn cycles improve RepSOC -- Ben's long-running outdoor solar-cycle
JSONL (on his laptop) is the dataset for that.

Caveats for the record: n=2 units, one board, one harness build, SOC 78-88%
window, indoor. The "feed" contrast bundles each feed with its own upstream
cabling -- which is the production-realistic comparison, but an idealized
soldered VBAT feed would do better than measured (and is ruled out by ADR 0009
anyway). `led_studio` RGBW order fix landed (GRBW->RGBW, see prior entry).

---

## 2026-07-11 -- Ben + Claude -- led_sol_bench (RGBW+solenoid on VBAT): WAGO ground-fault saga, D13 != GPIO13 (it's GPIO11; 13 is EN0), wire order is RGBW

New `firmware/led_sol_bench/` (solenoid_demo safety machinery + led_studio RGBW
render): RGBW data D13, solenoid driver D12, both loads VBAT-direct (the ADR
0029 fork test). Solenoid worked immediately; the RGBW showed ghost colors
(green bright / no red / faint blue at all-255, picker R/G "swapped") at
near-zero measured draw. The debug chain, for posterity:

- **Channel sweep via telemetry:** every slot at 255 added ~0 mA supply draw ->
  the module wasn't being fed; battery was healthy (gauge blind to VBAT loads
  anyway -- ADR 0029 §4's exact warning; with USB attached, supply current is
  the working ammeter).
- **Firmware-only ground-fault probe (new technique, kept as `/gndprobe`):**
  tri-state the data pin INPUT_PULLDOWN and sample. Healthy module DIN = high-Z
  -> reads ~0% high. Open/floating module GND -> its return current exits via
  the data line and holds the pin ~100% high. Read 100% -- fault confirmed
  electrically before touching hardware. Control probe on empty GPIO10 read 0%
  (validates the method). Corroborating strike-pulse VBAT-sag modulation was
  below digital-read resolution (null, as expected).
- **Red herring with a lesson:** GPIO13 ALSO probed 100% high -> "is 13
  special?" Yes: **GPIO13 = EN0, the SDK-owned FeatherWings enable
  (Mainboard.h:123), and the D13 header position routes to GPIO11**
  (Mainboard.h:97, variant pins_arduino.h). Cost an hour; now in
  POWERFEATHER_NOTES pin table.
- **Root cause:** fine-stranded wire not seated under a WAGO 221 lever on the
  GND path (the JST Y-splitter was exonerated). Reseated -> module fed
  properly. Harness-buy lesson: WAGO levers want full-depth seating on
  fine-strand silicone wire; ferrules are the robust fix at fleet scale.
- **Wire order:** slot-tested (raw `/raw` bytes, mapping bypassed): the
  production 4 W RGBW is **RGBW order, not GRBW**. `led_studio` MODE_RGBW had
  GRBW -> every color it ever showed was R/G-swapped (nobody had cross-checked
  the picker against die color). Fixed in led_studio; led_sol_bench defaults
  RGBW. MODE_RGB (different module) unverified -- check before trusting.

Bench app kept: web UI (strike machinery + RGBW anims + flash-sync strike
percussion+light), runtime feed A/B toggle (3V3+A0 vs VBAT+D13, blanks outgoing
module -- pixels latch), runtime wire-order switch, `/gndprobe`, `/raw`, `/lux`
(VEML7700 on SQT), OTA. GPIO13/EN0 excluded from the pin allowlist.

---

## 2026-07-10 (cont.) -- Ben + Claude -- solenoid strike bench first session: 815 strikes, no resets, no failsafes

First real session on the 3D-printed plastic lantern replica (wooden lanterns
still in the shipping container): Ben reports it "works amazingly well and
sounds quite nice." End-of-session `/state`: strikes=815, blocked=27,
failsafes=0, last=test 120 ms, pulse slider at 85 ms.

Reading the counters:

- No MCU reset the whole session: the counters live in RAM, so strikes=815
  still standing is itself the no-reboot proof.
- failsafes=0 -- the esp_timer pulse-end path never missed; the loop() backstop
  was never needed.
- blocked=27 (~3 %) is the coil-rest/mid-pulse guard refusing rapid-fire
  requests, not lost strikes -- expected under UI mashing.
- The board was on battery at session end (supply 0.00 V / not good,
  discharging ~120 mA, LFP 3.321 V under load, SOC 98 %) -- so it survived a
  USB unplug and at least part of the session ran battery-only. The USB/battery
  split of the 815 strikes was not tracked, so battery-only strike stability is
  suggestive, not measured.

Caveats before calling candidate B validated: n=1 board, n=1 solenoid, plastic
replica not bamboo (acoustics will differ), no deliberate battery-only strike
session, minimum-reliable-width number not yet written down. Noisemaker verdict
(vs speaker synth candidate A) stays OPEN -- more crowd input queued after the
2026-07-09 camp meeting reopened the field.

---

## 2026-07-10 -- Ben + Claude -- solenoid_demo: 3 V solenoid strike bench (noisemaker candidate B)

New `firmware/solenoid_demo/` app, modeled on speaker_demo: drives a 3 V mini
solenoid through an Adafruit MOSFET driver off the standard LED header (3V3
switchable rail / GND / A0 = GPIO10). Web dashboard (STRIKE, fixed-width test
strikes 10-120 ms for the min-reliable-width sweep, double/burst, auto-repeat,
coil-power rail toggle) + `/state` JSON + OTA `/update`. Coil safety: esp_timer
one-shot pulse end + loop() failsafe deadline, 5-300 ms hard clamp, 80 ms
coil-rest gap, gate LOW before rail-up and on OTA start. Standard SDK pattern:
V2 flag, guarded charge-enable, solar guard, EN_HIZ clear, Wire1 at 100 kHz.

Flashed over USB to the bench PowerFeather (D8:85:AC:9F:26:90, /dev/ttyACM1).
Verified: SDK Ok, 3V3 pad reads 1, boot strike pulse fired, STA at
192.168.4.26 / http://solenoiddemo.local/, telemetry live (LFP 3.597 V, SOC
100 %, charging on, supply 4.72 V good). Physical strike + rail-sag behavior
not yet assessed -- that is the point of the bench. Open questions it exists
to answer (see the app README): minimum reliable pulse width, MCU/rail
stability during pulses on USB vs battery, burst/auto endurance (watch
`failsafes` and `reset_reason`).

---
## 2026-07-12 - Codex - Consolidated solar-cycle and POR-loop field record

Added `docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md` as the durable record for
the July two-peer outdoor bench. It consolidates P126's fixed-5.8-V result, preliminary
daily BQ-input and positive-battery Ah/Wh range, the representative 157.7 mA HEX draw,
the 9-10 h playa civil-dark sizing window, and the active-time counter's roughly
12 percent truncation error. It also records the P105 POR regression chain, distinguishes
possible initiating causes from the firmware loop amplifier, preserves the exact
`net-bench-2026-07-12.1` recovery policy/artifact, and lists the reusable rail/NVS/dusk/
telemetry gotchas.

Important deployed-version distinction captured there: P126 remains on the July 10
image, where loss of useful charger input declares dark immediately. The 30-minute bare-
peer dusk confirmation belongs to the July 12 source/P105 image and is not running on
P126. Its observed 14.8 h window is therefore an instantaneous solar-threshold artifact,
not a qualified no-lux timeout.

Mirrored the general POR lesson into `firmware/POWERFEATHER_NOTES.md` and linked the
field record from `firmware/net_bench/README.md` and TODO. Operating decision: do not
OTA P126 solely to shorten its current 14.8 h fallback show. Ben expects to disassemble
it for another experiment; if it remains outside for a few days, preserve the current
image and use those days to expand the weather-conditioned P126 harvest envelope.
The logger was still alive and writing at 18:05 PDT, but its 259200-second duration
expires around July 13 15:25; TODO and the field record now call out the required
continuation if the peer remains deployed.

## 2026-07-12 - Codex - Playa civil-dark interval supports a 9-10 h HEX show

Checked Black Rock Desert sun/twilight tables for the 2026 event rather than using
the page's current July summary. Burning Man runs Aug 30-Sep 7; over that interval,
sunset moves from about 19:31 to 19:18 and sunrise from about 06:21 to 06:29. More
usefully for lighting, evening civil twilight ends about 19:59-19:46 and morning
civil twilight begins about 05:52-06:01, giving roughly 9 h 53 min to 10 h 15 min.
Use 9-10 h as the provisional production HEX sizing/emulation window. At the P126
peer's measured 157.7 mA draw, 10 h costs about 1.58 Ah, versus the good July 11
charge estimate of about 1.74 Ah; that is a small positive-day margin, not yet a
weather/dust/shading-qualified production margin.

## 2026-07-12 - Codex - P126 draw is representative; bench show window is too long

Ben clarified that the P126 peer's three-pixel, roughly 158 mA corrected load is meant
to emulate the intended deployed HEX light show. Retract the earlier recommendation to
raise it to 400-450 mA merely to empty the battery nightly: that would test a different
fixture load.

The clean July 11-12 draw ran from 18:07:33 to 08:53:54 PDT, or 14 h 46 min wall time,
with 14.773 h of continuous logger coverage and 2.33 Ah integrated from corrected
`battery_ma`. The peer's retained counters reported 13.02 h / 2.08 Ah because
`fieldCycleIntegrateActive()` advances by integer `dt / 1000` seconds and resets its
millisecond origin each iteration, losing the fractional remainder every roughly-1 Hz
loop. Those counters under-report this session by about 12 percent.

The P126 peer has no lux sensor, so its fallback show window is 30 minutes after useful
solar input disappears until useful input returns. That made this a 14.8 h solar-loss
window, not a realistic timed show. At the measured 157.7 mA average, an 8 h show costs
about 1.26 Ah, 10 h costs 1.58 Ah, and 11 h costs 1.73 Ah. The July 11 observed charge
estimate of about 1.74 Ah would therefore be roughly break-even at 11 h and positive at
shorter show durations, subject to the known sparse charge-sampling uncertainty. The
previous daily-negative conclusion applies to the artificial 14.8 h window, not yet to
the intended production HEX schedule.

## 2026-07-12 - Codex - P126 avoids POR under its lighter load but is not energy-positive

Reviewed the P126 production-cabling peer (`9E5B0C`) from its July 10 deployment
through July 12 around 17:43 PDT. Its corrected nighttime battery draw is about
160 mA, versus roughly 460-480 mA on the P105/HEX peer. The smaller load, combined
with the lower-resistance production harness, leaves substantially more loaded-VBAT
margin; P126 has remained roughly 3.24-3.33 V and has not exercised the low-voltage
or POR-recovery paths. This is absence of the stress condition, not evidence that its
older dusk/POR policy is safe.

Integrating corrected `battery_ma` at the logger's 1 Hz sample-and-hold cadence,
discarding gaps over 5 seconds and cached protect-mode current, gives the following
provisional calendar-day ledger:

| Local date | Current coverage | Charge | Discharge | Net |
|---|---:|---:|---:|---:|
| July 10 (partial deployment) | 0.72 h | 0.190 Ah | 0.001 Ah | +0.190 Ah |
| July 11 | 16.56 h | 1.738 Ah | 1.315 Ah | +0.423 Ah |
| July 12 through 17:43 | 17.69 h | 1.114 Ah | 1.640 Ah | -0.526 Ah |

The observed deployment-to-date subtotal is therefore only +0.087 Ah before the
July 12 night show, effectively flat within the incomplete-telemetry uncertainty.
Cycle alignment is more informative: the well-observed July 11 solar interval put
back about 1.74 Ah, while the following full show removed about 2.08 Ah, for roughly
-0.34 Ah. July 12's overcast interval put back about 1.11 Ah; against another
2.08 Ah show it would leave roughly -0.97 Ah. The P126 setup therefore does not
currently demonstrate a positive daily Ah balance, although a sunny day is close
to break-even. Sleep-period charge is sparsely sampled, so the battery-side charge
totals are conservative by an uncertain few tenths of an Ah.

## 2026-07-12 - Codex - Fixed P105 dusk/POR regression and OTA deployed 2026-07-12.1

Reworked the compiled-but-undeployed July 11 guard after the firmware-history review
showed that parking on the first POR would preserve the early-sleep/Ah-loss failure.
`net-bench-2026-07-12.1` now:

- persists an NVS LED-session stage (`idle`, `full`, `dim`, `protect`) before rail-on;
- on an unexpected reset from `full`, atomically consumes one retry before any rail can
  turn on and resumes through the staged ramp at dim brightness;
- on a reset from `dim` or `protect`, holds 3V3 off and hard-parks until verified
  positive battery charge; a second POR can no longer recreate the full-power loop;
- drives data/EN_3V3 low before PowerFeather init, explicitly releases the RTC GPIO4
  hold only at deliberate rail-on, clears the pixels, and ramps in four steps;
- separates dim confirmation from low confirmation. The P105 build dims at 3.10 V
  after 10 s, protects at 2.95 V after 60 s, and keeps 2.90 V immediate critical;
- qualifies dusk from the existing TSL2591: <=200 lux for five minutes turns the light
  on, >=500 lux is dawn. A peer without TSL falls back to 30 minutes without useful
  charger input. This replaces the one-sample `input disappeared == dark` decision that
  caused full-battery afternoon cycle chatter.

Added matching build/OTA switches for dim confirm and dusk/dawn thresholds. `/telemetry`
now exposes `field_session_stage`, retry/park booleans, and `field_dusk_s`; the ESP-NOW
heartbeat remains backward-compatible with the existing bridge.

Compiled the exact P105 image with 18 pixels at brightness 128, LFP 6 Ah, 1.5 A charger,
and fixed 4.6 V VINDPM:

`firmware/net_bench/build/field-cycle-peer-20260712-p105-dusk-dim-retry-r3/net_bench.ino.bin`

Compile result: 1,028,373 bytes flash (30%), 51,932 bytes globals (15%). The first two
wrapper builds were deliberately discarded: the short host command orphaned their
compiler process, and the final source also added the required RTC-hold release before
the clean isolated `r3` compile.

OTA deployment to P105 `9F26F8`:

- preflight: old `net-bench-2026-07-08.1`, VBAT 3.577 V, phase charge, 8,770 lux;
- first 75 s maintenance discovery expired without catching the five-minute sleep wake;
  no upload occurred;
- extended discovery caught the peer at `192.168.4.87`; 1,028,704-byte OTA upload was
  acknowledged in 5.82 s with no button (`2026-07-13-ota-results.jsonl` is UTC-dated);
- the image remained in WiFi maintenance, so the helper's ESP-NOW verification timed
  out on a stale old heartbeat. Direct `/telemetry` nevertheless confirmed the new
  revision, software reset, `pf_ready=true`, session stage idle, phase charge, and no
  interrupted state. An explicit `/resume` returned it to ESP-NOW;
- dashboard then verified `net-bench-2026-07-12.1`. Across the next natural five-minute
  charge sleep, it remained cycle 1 / charge / no dim / no protect, and woke at 5,812
  lux with low charger current instead of creating a false dark/sunrise cycle.

The continuing two-peer logger remained alive and captured the deployment. Tonight's
real <=200 lux transition, 3.10 V dim point, and any naturally occurring persisted POR
retry remain the decisive autonomous validation.

## 2026-07-12 - Codex - P105 firmware/threshold flash timeline and regression review

Reviewed git history, preserved `build.options.json`, OTA result JSONL, LOG deployment
notes, and the first observed firmware revision in every P105 field log. Times below are
America/Los_Angeles; the ADR23 OTA file is dated July 8 UTC but the actual local flash
was July 7 at 21:32 PDT.

| P105 flash / first seen | Image | Load | Effective low-voltage behavior | Change |
|---|---|---|---|---|
| Jul 3 <=20:15 (first seen; exact P105 flash record absent) | `net-bench-2026-07-01.1` | likely 18 px @ 128 from the v2 artifact | no dim; soft 3.15 V / 30 s, critical 3.05 V immediate, then 900 s protect sleep; supply clears protect | inherited field-cycle-v2 baseline during role swap |
| Jul 3 21:21 (first seen) | `net-bench-2026-07-03.1` | 18 px @ 96 | no dim; soft about 3.10 V / 60 s, critical 3.00 V immediate; new dark retry when rebound reached soft +80 mV | added HEX field load, debounce, and rebound retry |
| Jul 3 21:33 (OTA ack) | `net-bench-2026-07-03.2` | 9 px @ 64 | same 3.10 / 3.00 V, 60 s, unlatched rebound-retry logic | only blanked loads before maintenance and reduced load |
| Jul 5 10:17 (OTA ack) | `net-bench-2026-07-05.1` | 18 px @ 128 | thresholds and sleep decisions unchanged: 3.10 V / 60 s, 3.00 V immediate, 3.18 V dark retry | restored the heavy load; no policy change |
| Jul 7 21:32 (OTA ack; LOG/file label July 8) | `net-bench-2026-07-08.1` | 18 px @ 128; dim brightness 64 | dim at <=3.00 V for 60 s; protect/sleep at <=2.95 V for 60 s; critical <=2.90 V immediate; dark retry disabled; protect clears only on real >=20 mA battery charge | major ADR23 threshold/latch rewrite; still deployed on P105 |

No later P105 image was deployed. The July 10 image/commit went only to the new P126
peer. `net-bench-2026-07-11.1` with the persistent interrupted-session guard was compiled
for P105 but explicitly not OTA'd.

The leading firmware regression chain is the July 7/8 deployment:

1. The previous heavy-load image would begin its soft exit at 3.10 V. ADR23 moved the
   loaded protect decision down 150 mV to 2.95 V.
2. The new dim decision does not occur until voltage remains <=3.00 V for 60 s. P105's
   July 11 PORs occurred mostly around 3.00-3.05 V, so the source can collapse before
   the dim threshold is reached or confirmed.
3. `rtcFieldLoadDimmed` and `rtcFieldProtectLatched` are RTC-memory state. They survive
   timer deep sleep but not a hard power-on reset. A POR therefore returns to `FC_BOOT`;
   rebound voltage above the low threshold selects draw and reapplies all 18 pixels at
   brightness 128.
4. Every POR also erases the in-RAM 60 s dim/low timers. Repeated resets can therefore
   prevent both intended decisions forever until one instantaneous sample reaches the
   2.90 V critical check.

This firmware explanation fits the timing better than a new constant harness resistance:
the harness did not change, the old 3.10 V policy generally exited before the newly
exposed 3.00 V marginal region, and dense POR behavior appears after the ADR23 image.
It is not the entire story: the same July 7/8 image later delivered roughly 2.7-4.1 Ah
on better nights, so temperature, starting state, connection variation, or a transient
power-path event still determines when the marginal condition is hit.

The already-built July 11 guard would stop the reset storm but would park immediately
after the first POR, preserving the July 11 early-sleep failure. Before deploying it,
decide whether an interrupted full-load session gets one persisted, staged retry at the
dim load; only a second POR (or an already-low preflight) should hard-latch protect. That
retains the POR safety property without automatically abandoning the remaining Ah.

## 2026-07-12 - Codex - Reconstructed P105 daily Ah ledger and POR history

Combined all available outdoor P105 `9F26F8` field-cycle logs from July 3 onward,
cutting the July 10 overlap at the newer two-peer logger. Used battery-lead INA current
as truth. The state-aware estimate interpolates normal 5-minute charge wakes, integrates
the continuously awake draw phase, treats the long cached current during protect sleep
as zero, and does not bridge large host gaps. Rounded calendar-day ledger:

| Local date | Positive charge | Discharge | Net battery delta | Coverage note |
|---|---:|---:|---:|---|
| Jul 3 | 0.00 Ah | 0.56 Ah | -0.56 Ah | from 20:15 only |
| Jul 4 | 2.04 Ah | 0.65 Ah | +1.38 Ah | split partial windows |
| Jul 5 | 1.26 Ah | 1.84 Ah | -0.58 Ah | from 10:19 |
| Jul 6 | 3.16 Ah | 4.48 Ah | -1.32 Ah | near-complete; strongest early cycle |
| Jul 7 | 0.00 Ah observed | 2.85 Ah | -2.85 Ah observed | daylight host gap; actual net unknown |
| Jul 8 | 1.62 Ah | 3.72 Ah | -2.10 Ah | mostly covered; many recovery/protect events |
| Jul 9 | 4.01 Ah | 3.07 Ah | +0.94 Ah | mostly covered |
| Jul 10 | 3.82 Ah | 1.74 Ah | +2.08 Ah | protect/sleep gaps mostly near-zero |
| Jul 11 | 1.80 Ah | 0.86 Ah | +0.94 Ah | complete host log; 0.68 Ah after full before POR |
| Jul 12 | 0.63 Ah | 0.07 Ah | +0.56 Ah | through about 16:25 PDT only |

These are battery-terminal ledger estimates, not panel harvest. A cached-row-only
integration changes most daily net values by less than about 0.2 Ah; July 7 remains
unrecoverable because the logger missed the daylight charge window. Charge during deep
sleep is sampled only at wakes, so the positive side still has systematic uncertainty.

Historical draw counters corroborate that July 11 was abnormal. Better observed P105
nights delivered roughly 2.7-4.1 Ah corrected before low/protect behavior (about 2.9 Ah
raw by July 9 morning and 4.47 Ah raw by July 10 morning, with the MAX17260 divided by
1.08). The July 11 post-full battery-INA integral was only 0.679 Ah before the POR loop.

POR was not completely new: one power-on event appears July 6 around 23:09, about 18
power-on boots occurred July 8 around 10:38-13:04 during low-battery/daylight recovery,
and one appears July 10 around 14:16 near 2.89 V. July 11 was the first clearly captured
dense nighttime storm: roughly 30-plus power-on boots from about 20:38-20:58. July 12
then had repeated dawn recovery retries plus one brownout. Exact event counts vary by a
few depending on cached-heartbeat deduplication, but the qualitative distinction is
strong: prior PORs existed, while July 11's early full-to-POR window was an outlier.

## 2026-07-12 - Codex - Field-cycle audit found invalid full-empty cycling

Reconstructed unique peer heartbeats and phase transitions after the apparent P105
charge termination conflicted with the known 5.7 Ah production-cell capacity. The
current outdoor run is not a valid daily full-to-low cycling validation.

- P126 `9E5B0C` did not sleep or reach a low-voltage threshold overnight. Its three
  single-channel full-bright pixels plus system load averaged about 160 mA corrected.
  From the stable July 11 dusk cycle to July 12 sunrise it ran 13.0 integrated hours,
  removed 2.08 Ah / 6.8 Wh, and remained at about 3.27 V. At that load, using the
  5.37 Ah available above the 2.95 V LED-off threshold takes roughly 34 hours, not one
  night. The small load therefore tests multi-day energy balance, not daily emptying.
- P105 `9F26F8` did stop far too early. From its July 11 charge-full transition through
  the start of its 20:38 POR loop, continuous battery-INA data accounts for only
  0.679 Ah / 2.10 Wh of discharge. The load was about 0.46-0.48 A and the board-side
  voltage had fallen to about 3.04-3.06 V. This is nowhere near the production-cell
  drawdown's 5.1+ Ah-to-3.0-V result. Hard resets, not the intentional 2.95 V / 60 s
  rule, ended the useful draw; the final sub-2.90 V sample only latched protect after
  repeated restarts.
- The firmware's daylight predicate also fails at charge termination. It requires
  `supply_v >= 4.0 V` plus at least 20 mA input or battery charge current. When a full
  battery stops accepting current, firmware declares false darkness, briefly applies
  the LEDs, sees input current recover, declares sunrise, increments the cycle, and
  resets counters. P105 churned through dozens of false cycles on July 11. A latched,
  hysteretic day state must remain day from panel voltage after useful-sun acquisition;
  dusk requires a sustained low-input interval.
- Field-cycle also jumps directly from the 2.95 V LED-off threshold to protect sleep.
  It does not implement ADR 0023's separate LED-off/duty-cycled-OTA state down to the
  2.90 V sleep threshold. This was not the cause of the P105 early stop, but it would
  leave the 2.95-to-2.90 V reserve unused in an otherwise clean cycle.

The P105 battery instrumentation path remains a plausible but unproven common cause for
both early apparent CV and early loaded undervoltage: the charger and onboard telemetry
are downstream of the series lead, so neither reports actual cell-terminal voltage.
The observed load-release slope is about 0.5 ohm effective versus ADR 0023's roughly
0.15-0.17 ohm qualified source path, but that estimate includes cell polarization and
cannot assign the excess to cabling alone. The clean A/B is to replace only the P105
battery power path with the production cable while leaving the panel INA and I2C sensor
topology in place. If the usable window remains compressed, remove the shared-bus
instrumentation next to isolate an I2C/BQ power-path disturbance.

## 2026-07-12 - Codex - Solar harvest through 15:50 PDT: P126 6.09 Wh BQ, P105 6.22 Wh panel

Integrated the continuing two-peer field logger from local midnight through 15:50 PDT.
The host log was continuous over this window. Integration uses the 1 Hz cached
sample-and-hold rows, does not bridge gaps over 5 s, and divides the older P105 peer's
raw MAX17260 battery power by 1.08.

| Fixture | Panel-side harvest | BQ charger input | Positive battery charge |
|---|---:|---:|---:|
| P126 `9E5B0C` | no INA | **6.09 Wh** | **3.57 Wh** |
| P105 `9F26F8` | **6.22 Wh** | **4.85 Wh** | **2.08 Wh corrected** |

P105 panel-side input began around 06:42, peaked at 1.736 W at 11:16, and fell to a
roughly 0.49 W last-15-minute median under the afternoon overcast. Its battery/charger
behavior was acceptance-limited from about noon onward: last-15-minute medians were
about 0.328 W BQ input and effectively zero net battery charge, with VBAT around 3.56 V.
Do not use gauge SOC as evidence for that conclusion.

P126 useful BQ input began around 08:26, peaked at 1.668 W at 14:04, and had a 0.257 W
last-15-minute median at cutoff. Its higher awake/system load meant that this weak
overcast input was no longer net-positive at the battery (about 0.10 W median net draw).
The P126 total is charger-input energy, not panel-side ground truth.

A time split separates panel capability from battery acceptance. From 09:00-noon,
while both peers accepted charge, P105 delivered 3.14 Wh at the BQ input versus P126's
1.65 Wh; corrected positive battery charge was 2.04 Wh versus 0.68 Wh. From noon-16:00,
P105 reached charge taper/termination and delivered only 1.42 Wh at the BQ input and
0.04 Wh to the battery, while P126 remained below CV and delivered 4.42 Wh at the BQ
input and 2.90 Wh to the battery. The daily reversal is therefore primarily battery
acceptance/headroom, not evidence that the 2 W panel had more available solar power.
Exact dawn depth cannot be reconstructed from this run because gauge SOC is unreliable,
the logger holds cached awake current across protect sleeps, and field-cycle coulomb
counters reset during dawn phase chatter. The P105 instrumented battery path can also
raise its charger-side voltage during charge and cause earlier apparent CV/termination.

## 2026-07-11 (cont.) - Codex - Built first-reset persistent LED-session guard for next OTA

Implemented the P105 reset-loop follow-up in `firmware/net_bench` as
`net-bench-2026-07-11.1`; no live device was OTA'd in this session. The protection is
cause-agnostic: a higher-ohm instrumented harness, 3V3 regulator collapse, watchdog, or
an I2C/BQ disturbance that momentarily disconnects the battery all produce the same safe
next-boot behavior.

For `--field-cycle --field-led-load` images, firmware now:

- drives pixel data and EN_3V3 low before PowerFeather initialization, then immediately
  parks the SDK's cold-init rail enable;
- writes NVS `fc_led_active=true` before the LED rail is energized and retains it through
  the whole dark session and low-voltage protect;
- interprets power-on, brownout, panic, or watchdog plus the uncleared marker as an
  interrupted LED session, enters protect with reason 8, keeps the LED rail off, and
  retains the normal cold OTA window;
- clears the marker only after the existing recovery gate sees useful input plus at
  least 20 mA positive battery charge current -- never from gauge SOC or rebound voltage;
- starts LEDs only after ESP-NOW initialization plus 1 s settle, clears the rail, and
  ramps in four steps over 400 ms while checking loaded VBAT; <=2.95 V parks immediately
  and <=3.00 V selects the dim target;
- exposes `field_session_marker` and `field_interrupted_boot` on `/telemetry`; the bridge
  already carries phase 5 / reason 8 / protect-latched without a protocol extension.

Migration is safe for the currently protected P105: after OTA's software reset, its RTC
protect state remains; the new image backfills the NVS marker while in protect. A later
full POR therefore cannot lose the latch. Verified by compiling the exact next-P126
configuration (6 Ah LFP, fixed 5.8 V, three-pixel full-bright spiral RGB) into:

`firmware/net_bench/build/field-cycle-peer-20260711-por-guard-compile-r4/net_bench.ino.bin`

Compile result: 1,032,209 bytes flash (30%), 52,260 bytes globals (15%). The final
revision limits ramp-time I2C reads to MAX17260 voltage/current and avoids four extra
charger-status/solar-guard passes while applying the load. Hardware validation remains
for the next OTA: exercise a deliberate power interruption during
draw, confirm the next boot reports reason 8 with no LED pulse, then confirm real solar
charge clears the marker and starts a fresh cycle.

Also compiled the P105/static-HEX variant (18 pixels, brightness 128, 4.6 V) to exercise
the non-spiral rendering branch:

`firmware/net_bench/build/field-cycle-peer-20260711-por-guard-p105-compile-r2/net_bench.ino.bin`

Result: 1,026,549 bytes flash (30%), 51,924 bytes globals (15%).

## 2026-07-11 (cont.) - Codex - P105 protect at 3.255 V traced to loaded sag plus reset loop

Investigated why P105 peer `9F26F8` showed `FC_PROTECT` while its last reported VBAT
was 3.255 V. The displayed value is post-load rebound, not the cutoff sample. The exact
deployed artifact's `build.options.json` confirms ADR 0023 thresholds: dim 3.00 V,
soft-low 2.95 V for 60 s, and immediate critical 2.90 V.

At 20:38 PDT, the peer began a 31-event `reset_reason=poweron` loop lasting about
19 minutes. Immediately before/among resets, the 18-pixel HEX load drew about 0.47-0.55 A
raw gauge current and VBAT was generally about 3.00-3.05 V, with retained loaded minima
as low as 2.935-2.940 V. Each hard power loss reset RTC field-cycle counters and the
in-RAM 60 s low-voltage confirmation, so the new boot re-enabled the draw load. This is
strong evidence of a rail-collapse/POR loop occurring before the soft-low state machine
could park the load.

At 20:57:53 PDT, firmware finally observed an unlogged/sub-second sample at or below the
2.90 V critical threshold, entered protect with reason 6, blanked the HEX, and latched.
The first reported post-transition VBAT was 3.229 V at about -142 mA raw; later 15-minute
protect wakes reported about 3.255 V at -133 mA raw. `field_min_mv=3004` for that final
short boot does not capture the trigger because the retained min is updated at the
one-second integration cadence, while the immediate critical predicate runs every tick
and removes the load before the next heartbeat.

The current protect state is therefore internally consistent and safe; it will remain
latched until a wake sees real recovery charge current. The reset loop is a separate
production-policy defect: the low-VBAT latch/debounce must survive or infer repeated
power-on resets so a marginal rail cannot repeatedly re-enable the LED load.

Why this does not directly contradict the ADR 0023 drawdown: that test used a qualified
6 Ah production cell at 26.6 deg C under a continuous, already-running load and found
first instability near 2.69 V. This event used a different PowerFeather plus the legacy
instrumented P105 harness at about 15.8 deg C. Just before the loop, its battery-lead INA
was about 3.064 V / -0.475 A; after the HEX was removed, it was about 3.232 V / -0.131 A,
showing materially more effective sag than ADR 0023's 150-170 mohm path assumption. The
current boot order also lets `Board.init()` power the switchable 3V3 rail, waits 150 ms
for sensors, and restores the field LED load before any reboot-loop guard. Repeated POR
therefore exercises rail/LED/radio/sensor startup behavior that the steady shootout did
not. `poweron` rather than `brownout` also leaves a transient BQ power-path interruption
as a secondary possibility on this heavily populated shared Wire1 harness; logged BQ
state showed no persistent BATFET control/fault, so a rail capture or harness A/B is
needed to separate regulator/source sag from a momentary power-path opening.

Ben confirmed that all cells in these outdoor rigs are from the same fullbattery.com
batch; the two qualification samples were extremely close in both capacity and
resistance. Cell-to-cell variation is therefore deprioritized. The leading explanation
is the higher-ohm legacy INA/instrumented path plus cooler, repeated startup transients;
the I2C/BQ disconnect mechanism remains a secondary failure mode worth making safe.

Recommended production pattern: persist an NVS `led_session_active` marker BEFORE
energizing the LED rail and clear it only after a normal rail-off transition. On the
next `poweron`/`brownout`, an uncleared marker plus no verified recovery charge means
"the LED session collapsed power": keep the rail off, persist protect, and timer-sleep.
This costs about two NVS writes per night and requires no flash write during the actual
collapse. Clear protect only after sustained positive battery charge, not gauge SOC.
Then add boot staging as defense in depth: data low and LED rail off, initialize/read
power at low load, let radio/power settle, enable a cleared rail, and ramp brightness
while watching loaded VBAT. Delay/ramp alone is insufficient because unloaded rebound
would otherwise pass preflight and recreate the same loop.

## 2026-07-11 (cont.) - Codex - Post-gap solar harvest: P126 9.02 Wh input, P105 12.39 Wh panel-side

Integrated the surviving July 10 logger from host reconnection at 07:25 PDT through
the end of useful solar on July 11. Integration uses the dashboard's 1 Hz cached
sample-and-hold rows (appropriate to the peers' duty-cycled five-minute charge wakes),
does not bridge the 13 h outage, and applies `/1.08` to the older P105 peer's raw
MAX17260 battery current.

| Fixture | Useful-input window | Peak | Harvest / positive charge |
|---|---|---:|---|
| P126 `9E5B0C` | 08:09-18:19 PDT | 1.679 W BQ input | **9.02 Wh BQ input; 5.84 Wh battery charge** |
| P105 `9F26F8` | <=07:25-19:40 PDT | 2.363 W panel INA / 2.101 W BQ | **12.39 Wh panel-side INA; 10.22 Wh BQ input; 6.43 Wh corrected battery charge** |

These are lower bounds for the complete calendar day because the host was absent until
07:25. The P105 was already harvesting weakly at reconnect, so it misses some dawn
energy. P126 still showed zero useful input at reconnect and did not cross the logger's
20 mW threshold until 08:09, so its missing pre-host contribution was likely negligible.

Do not read the similar BQ/battery totals as equal panel capability: the P105 has
panel-side INA truth and its battery acceptance/load state can cap what the charger
draws; P126 has onboard telemetry only. For this deployment/day, however, the compact
P126 delivered 88% of the P105's BQ-input Wh (9.02/10.22) and 91% of its corrected
positive battery-charge Wh (5.84/6.43).

Battery acceptance materially biased that ratio in the P126's favor, but do NOT use
the logged MAX17260 `soc_pct` values to quantify the morning state: the 32700 shootout
showed that percentage SOC can be grossly wrong on the LFP plateau. Trustworthy evidence
is that P126's corrected onboard coulomb counter retained about 1,958 mAh of overnight
discharge at reconnect, while P105 later reached charger termination around 14:00.
From 14:00-16:00, P105 drew only about 0.3-0.36 W at the BQ input while the still-accepting
P126 drew about 1.3-1.6 W. Thus these daily Wh totals measure energy the complete fixture
could accept, not an unloaded comparison of the two panels' available energy. Exact
morning remaining capacity cannot be reconstructed across the host logging gap without
a known full anchor and continuous corrected coulomb integration.

## 2026-07-11 - Codex - Laptop power loss created 13 h host gap; current logger survived

The laptop charger stopped charging and Windows suspended/hibernated when the battery
ran out. The COM4 bridge board lost USB power and rebooted, but the dashboard and the
current P126 production-cabling logger processes survived suspension with the same
PIDs and resumed automatically when the laptop returned.

Gap analysis of
`ops/bench/data/ca/2026-07-10-ca-field-cycle-9E5B0C-p126-production-cabling.jsonl`:

- last pre-outage row: 2026-07-10 18:20:40 PDT;
- first resumed row: 2026-07-11 07:25:09 PDT;
- missing host interval: 13 h 04 min 28 s, covering almost the entire overnight draw;
- current logger remained alive and continued in the same file; no overwrite/splice;
- both peers were reachable again through the rebooted COM4 bridge.

The fixtures themselves continued autonomously. P126 `9E5B0C` retained field-cycle
counters showing about 1,969 mAh / 6.4 Wh discharged, minimum battery 3.247 V, and
about 43,785 s in draw when host logging resumed. Those counters preserve the overnight
total but not the missing time series, so this file is explicitly NOT the clean
sunrise-to-sunrise sizing capture requested in TODO.

The older dedicated P105 logger
`2026-07-08-ca-field-cycle-9F26F8-adr23-deploy.jsonl` had its configured wall-clock
duration expire during suspension and exited cleanly on resume. It was not restarted:
the surviving July 10 logger already records both P105 `9F26F8` and P126 `9E5B0C`.

## 2026-07-10 (cont. 2) - Codex - Exact P126 4.6 V penalty and apparent peer delta explained

Closed the P126 USB-safe-setpoint question on the production-cabling fixture with a
same-wake 5.8 -> 4.6 V step. Fresh onboard samples (no INA) gave:

| Setpoint | BQ input W median | Corrected battery mA median |
|---|---:|---:|
| 5.8 V, immediately before step | 1.306 W | 261 mA |
| 4.6 V | 1.094 W | 212 mA |

That is a 16.2% input-power penalty and about a 19% battery-charge-current penalty at
4.6 V in the current sun/setup, strengthening the earlier estimate from the June 29
4.8 V P126 point. The live restore command missed the end of the 8 s wake window, so
the charger spent one normal 300 s sleep interval at 4.6 V; the compiled configuration
then reapplied 5.8 V on the next wake. Verified live: `bq_vindpm_mv=5800`,
`supply_good=true`; the three-day logger stayed running.

Also explained the dashboard's apparent `supply_w - battery_w` difference between the
two outdoor peers. They were not on the same battery-current calibration:

- P126 peer `9E5B0C`, fw `net-bench-2026-07-10.1`: MAX17260 current already `/1.08` corrected.
- P105 peer `9F26F8`, fw `net-bench-2026-07-08.1`: raw MAX17260 current, known +8% high.

One representative P105 display sample read 2.088 W supply, 517 mA at the battery,
and a 0.239 W apparent delta. Correcting 517/1.08 = 479 mA changes battery power from
1.849 W to 1.712 W and the same-basis delta to 0.376 W. The P126's typical corrected
delta is about 0.40-0.44 W, so almost all of the apparent 0.4-vs-0.2 W mismatch was
mixed firmware accounting. The remaining few hundredths includes real system load,
conversion loss, different wake/MPPT timing, and the error of subtracting telemetry
from two different IC boundaries; dashboard `load_w` is therefore a residual, not a
pure ESP/fixture-load measurement.

## 2026-07-10 (cont.) - Codex - P126 onboard MPP quick check keeps 5.8 V

Ran a quick live VINDPM sweep on production-cabling P126/HEX fixture `9E5B0C`
using only onboard BQ supply telemetry plus the `/1.08`-corrected MAX17260 battery
current. The field-cycle image normally sleeps 300 s at a time, so the check caught
one natural charge wake and then used target-only 1 s sleeps to create fresh 8 s
measurement windows. Each point had 3-6 live samples; the normal three-day logger
captured the raw rows. No INA or light-normalization channel was present.

| VINDPM | BQ input W median | Corrected battery mA median |
|---|---:|---:|
| 6.2 V | 1.414 W | 292 mA |
| 6.0 V | 1.476 W | 306 mA |
| 5.8 V (anchor 1) | 1.419 W | 306 mA |
| 5.6 V | 1.412 W | 305 mA |
| 5.4 V | 1.398 W | 299 mA |
| 5.8 V (anchor 2) | 1.425 W | 303 mA |

The two 5.8 V anchors differed by only 0.4%, so conditions were reasonably stable.
6.0 V read 3.8% higher at the charger-input boundary, but delivered no measurable
battery-current gain over the first 5.8 V anchor; 6.2 V had already rolled over and
the lower points declined. Verdict: the optimum is broad around 5.8-6.0 V and the
external-INA-qualified 5.8 V setpoint still holds. Kept production fixed at 5.8 V
rather than chasing an onboard-only 3.8% input-boundary difference. Verified final
`bq_vindpm_mv=5800`, `supply_good=true`, logger still running, and the other field
peer retained its own MPPT/default behavior.

## 2026-07-10 - Codex - Deployed P126 production-cabling HEX field cycle on former speaker board

Repurposed the PowerFeather previously running `speaker_demo` as a production-like
perimeter/HEX solar fixture: PowerFeather V2 + fullbattery 32700 6 Ah LFP + Voltaic
P126 2 W panel + production cabling/crimps + 37-pixel HEX, with no external INAs or
Dupont wiring. The old speaker image was reachable at `speakerdemo.local`, so it was
OTA-updated directly without bringing the board inside or attaching USB. New fixture
ID is `9E5B0C`; it rejoined the channel-11 serial bridge with software reset, 100% PDR,
and `net-bench-2026-07-10.1`.

Field draw uses LED Studio's Spiral + Rotate geometry: an anchor ping-pongs from the
center to the outer ring and back while pure full-bright red, green, and blue pixels
stay at 120-degree rotational offsets. Frame interval is 290 ms; ADR 0023 still dims
at 3.00 V, cuts the LED load at 2.95 V after 60 s, and treats 2.90 V as critical.

Applied the replicated MAX17260 +8% current bias at acquisition (`battery_ma = raw /
1.08`) so heartbeat current plus field-cycle mAh/Wh totals are corrected before they
reach the logger. `/telemetry` also exposes `battery_ma_raw` and the divisor. The
supply-side BQ reading remains uncorrected charger-input telemetry; without a
panel-side INA, harvest is useful for this installation's end-to-end comparison but
is not panel-capability ground truth.

The fixed P126 VINDPM is 5.8 V (its measured outdoor optimum). First bridge sample:
`sv=5.839 V`, `sma=252 mA`, `sgood=1`, corrected battery charge `310 mA`,
`bqv=5800`, battery `3.369 V`; no INAs were detected, as intended. A dedicated
three-day logger is running at:

- `ops/bench/data/ca/2026-07-10-ca-field-cycle-9E5B0C-p126-production-cabling.jsonl`

Implementation: added `--field-led-spiral-rgb` / `--field-led-frame-ms` to
`net_bench`, added matching `field_cycle_ota.py` switches, corrected that helper's
stale pre-ADR-0023 3.10/3.00 V defaults to 2.95/2.90 V, and documented the corrected
telemetry boundary. Build artifact:

- `firmware/net_bench/build/field-cycle-peer-20260710-p126-spiral-rgb-r2/net_bench.ino.bin`

## 2026-07-08 (cont.) - Ben + Claude - Review corrections: RGBW feed stays rail-wired (VBAT option open), site timeline corroborated, battery dates pinned

Ben's review of the housekeeping pass produced corrections, all folded in:

- **RGBW feed (the important one):** ADR 0029 was overstated -- the MEASURED
  verdict (VBAT-direct +33 % fringed white) is not the PRODUCTION decision. The
  fleet still runs both LED roles from the switchable 3V3 rail (V+/GND/A0 via
  right-angle JST-XH 4-pin, QON unconnected), and Ben is hesitant to convert:
  clean W-only is unchanged, the rail cut is a robust hard LED kill, and there is
  no clean VBAT tap on the COTS board. His sketched conversion (solder 4-pin
  header on {VBAT|EN|VS|D13}, GND via ~$0.50 JST 2-pin Y-cable off the pin next
  to VDC, ~100 needed, firmware A0->D13, fail-safe redesign so a stuck frame
  cannot kill the pack; side benefit: frees 3V3/GND/A0 for a clacker/relay) is
  recorded in ADR 0029 as an OPEN decision to revisit before the harness buy.
  Cascaded through SYSTEM/README/AGENTS/glossary/hardware/ARCHITECTURE/TODO/bom.
- **Battery dates pinned:** 75 cells ordered 2026-06-11 (same day the first
  sample qualified), 100 on 2026-07-07. ADR 0025 + ledger updated.
- **Sensor count:** 150 depth sensors total incl. bench/sample units (48 L5CX +
  100 TMF8820 orders + bench) -- parity with the 150 accels.
- **Customs delay recorded:** the 05-10 R&D PowerFeather order sat in customs
  ~3 weeks; boards landed ~Jun 2-5. Also recorded: Jun 15-20 was travel-bench
  solar testing in TN, codified by the 06-29 home runs.
- **Noisemakers re-opened wider:** relay clicks and even simple beeps stay on the
  table; the early crowd sample was small. First big camp-wide meeting 2026-07-09
  will collect more opinions.
- **Project timeline corroborated against https://resonancenetwork.org/camp
  (gold standard):** container lands Port of Oakland Jul 12; NC prebuild is
  **Bodhi Hive, Nevada City** Jul 31-Aug 19 (repo docs had said "Grass Valley");
  all-hands container unload Aug 1-2; **lights + camp systems team build
  Aug 8-9**; container load Aug 21; gates Aug 30; burn night Sep 5. ROADMAP,
  PROCUREMENT, and glossary aligned.
- **New tentative plan:** Ben may spend ~Jul 20-31 in TN fleet-testing the ~70
  boards at Steve's (production-firmware mesh effects + presence, indoors if
  enclosures lag), back for the container unload. Recorded in ROADMAP Phase 6 +
  TODO.
- **RGBW top-up is now PLANNED** (not conditional) -- "definitely buy more,
  they're cheap"; sizing waits on the chandelier mix.
- Team brief updated to match (figures rebuilt with collision-proof callout keys,
  bench-work timeline entries added, spend donut added, Nevada City dates).
- Second review round (same day): gobo MVP direction recorded (flat discs likely;
  cones add complexity/brittleness -- few or none; enclosure/README + glossary);
  bridge-directed multicast show concept recorded in BACKGROUND (DJ/MIDI/mic at
  the bridge streaming a sound-reactive show); brief polish (battery "two-year
  life" rephrased -- it is ADR 0002's reuse requirement, not a measured lifetime;
  throw distances restated as 7-10 ft deployment / crisp to 15 ft tested; ToF
  window drawn in the hat; bridge topology panel; sun-to-moon battery-meter state
  diagram; bigger spend donut with icon legend).

## 2026-07-08 - Ben + Claude - Documentation housekeeping: fleet plan recorded (150-152, four classes), ADRs 0024-0029, procurement ledger, stale-doc sweep, team write-up

Pre-crunch documentation reconciliation (~6 weeks to Aug 20). The live docs were
current but the canonical docs had frozen in mid-June: "100 fixtures" everywhere,
COTS-vs-custom framed as future (it resolved), procurement (~$12.7k committed)
recorded nowhere, and several settled decisions had no ADR. Interviewed Ben for the
ground truth, then swept.

Fleet plan recorded (tentative until installation; placement is free because the
design is fungible/wireless): 72 hanging downlights (RGBW + gobo, 7-10 ft) + 38-40
perimeter HEX on 5 ft shepherd hooks + 24 uplights (RGBW, no gobo, battery may fill
the bamboo cylinder, gasketed USB-C at a base "boot") + 16 chandelier lights
(HEX/RGBW mix TBD, scope/ownership loose) = 150-152. Canonical living counts table
added to SYSTEM.md; everything else points at it. Uplight/chandelier power is the
big open decision: off-light 5 W panel vs solar-free 20 Ah LFP (batteryspace #6832,
2 samples; bench test gates a ~40-cell buy) vs budgeted 6 Ah.

New ADRs (retroactive records of decisions made 06-11 through 07-08):
- 0024 production architecture lock: COTS PowerFeather V2 at ~150 in four classes;
  68 boards bought 06-11, 82 more invoicing 07-10 (ships same day per Elecrow rep);
  custom PCBA deferred to a 2027 option. Spares thin (~8 bench boards) -- risk-registered.
- 0025 battery vendor: fullbattery 32700 6 Ah qualified n=2; Palowextra rejected;
  175 cells bought (75 on 06-11, same day the first sample qualified; 100 on
  07-07); 20 Ah option OPEN.
- 0026 panels: Voltaic ETFE P105 5 W (110) / P126 2 W (50) + 160 pigtails, bought
  06-24, measured 06-29; role map P105->downlights, P126->perimeter.
- 0027 sensors: MSA311 + multizone ToF by class (TMF8820-mini downward on
  downlights; VL53L5CX outward on perimeter; uplights/chandelier none, tentative);
  fused IMUs rejected (per-device cal); 150 accel + 148 ToF + 60 protective covers
  ordered 07-07.
- 0028 power-management bus integrity: the 100 kHz rule + dedicated bus on custom
  PCBA + no power-mgmt I2C from core-0 tasks under WiFi (the reboot-epidemic
  conviction, sealed by the 46 h soak).
- 0029 LED electrical drive: HEX on the switchable 3V3 rail (decided); boost
  shelved with its complete numbers + revival spec (decided); RGBW production feed
  OPEN -- rail-wired today (V+/GND/A0 JST-XH), with the measured-better VBAT
  option (+33 % fringed white) and its conversion plan/costs recorded.
Backward Status annotations added on 0002/0004/0008/0012/0013/0014/0016/0017/0018
(append-only, both-ways links).

Ops docs: NEW `ops/PROCUREMENT.md` (orders ledger with Ben's real dates -- five
orders placed 07-07: MSA311+STEMMA cables, VL53L5CX, ToF covers, TMF8820-mini, 100x
6 Ah; panels+pigtails 06-24; PowerFeathers 06-11; LED buy 06-17; HEX/NeoHEX samples
05-10; plus small-order history, to-buy queue, lead-time risks backward from Aug 20,
vendor directory). `ops/bom.md` rewritten as shared-core + four per-class tables +
fleet-totals/spares math (flags: PF spares thin; RGBW tight at 100 bought vs up to
~104 needed depending on chandelier mix). `ops/README.md` refreshed.

Stale-doc sweep: README (fleet, repo tree to reality, status), AGENTS (superseded
Decided entries annotated, new decisions added, ESP-NOW claim re-hedged at 150),
BACKGROUND (fleet, chandelier update, mandala program PULLED -- new plan is in-house
+ generative bamboo-leaf patterns per species; 05-10 R&D section banner),
SYSTEM.md (fleet table, sensors as production intent, noisemaker line, 0028/0029
design rules, measured solar results, open-gates refresh), ROADMAP (Phase 3 stamped
RESOLVED, done items checked with evidence, Phase 1b/1c workstream sections added,
risk register updated -- battery/boost/COTS-supply risks retired; 82-board schedule
risk added), glossary (mid-word truncation at "PCB Assem" fixed; ~40 entries added
for the current stack), docs/README (+research/), firmware README/ARCHITECTURE
(target-vs-reality framing, dead boards removed, IS31 section replaced, OTA aligned
to shared-WiFi maintenance, 0028 task constraint + sensor_task), hardware README
(0028/0029 custom-PCB constraints, test list updated), enclosure README (uplight
boot stub for Steve, ToF apertures), two never-filled test docs banner-marked
SUPERSEDED, TODO reconciled (boost/battery/networking items closed with evidence,
INV_2026_00401 retired -- identity unclear, probably the Bamboo Pure invoice; new
items: 82-board tracking, cabling/USB-C buys, 20 Ah bench test, sensor-allocation
confirm, uplight boot, chandelier mix, ESP-NOW 150 re-run).

Also produced: a team-facing scrolling write-up (shareable artifact + HTML copy
committed under docs/) covering the system story -- what's proven with dates,
architecture, network topology, the power/day-night state machine, OTA, sensors,
noisemakers, procurement status, and the road to Aug 20.

Next: the interview left two factual TBCs for Ben to backfill in
`ops/PROCUREMENT.md` (delivery confirmations for the June orders, Elecrow batch-2
invoice landing 07-10).

## 2026-07-08 - Codex - Noted production dusk gate separate from charger input

Ben observed that the field-cycle lights came on earlier than desired for production.
Captured a TODO to add a real production dusk/dawn light-enable gate instead of using the
current bench shortcut where "charger input disappeared" means dark. Current net_bench
field-cycle uses panel/supply voltage plus useful input or battery charge current; it
does not use lux or a sustained dusk classifier for the state transition.

## 2026-07-08 - Codex - Flashed ADR23 bridge and OTA'd 9F26F8 peer

Deployed the `net-bench-2026-07-08.1` ADR23 latched field-cycle build. USB-flashed the
COM4 serial bridge (`9E5AB8`) from:

- `firmware/net_bench/build/serial-bridge-20260708-adr23-latch-tail/net_bench.ino.bin`

Restarted the dashboard/logger and then used targeted shared-WiFi maintenance for
`9F26F8`. Maintenance discovery found the peer at `192.168.4.38`; OTA uploaded:

- `firmware/net_bench/build/field-cycle-peer-20260708-adr23-latched/net_bench.ino.bin`

`ops/bench/field_cycle_ota.py` verified no-button recovery: OTA ack succeeded, the peer
rejoined ESP-NOW with `fw=net-bench-2026-07-08.1`, `reset_reason=software`, and field
phase `draw`. The live deployment logger is:

- `ops/bench/data/ca/2026-07-08-ca-field-cycle-9F26F8-adr23-deploy.jsonl`

## 2026-07-08 - Codex - Restarted field logger and built ADR23 latched field-cycle firmware

Restarted the standalone JSONL logger for the live 9F26F8 field-cycle run:

- `ops/bench/data/ca/2026-07-08-ca-field-cycle-9F26F8-hexload-v2-resumed-latchparser.jsonl`

Then prepared `net-bench-2026-07-08.1` for the next field-cycle OTA. The new build aligns
the bench low-voltage policy with ADR 0023's measured 32700 LFP curve: dim at 3.00 V
loaded, protect/LED-off at 2.95 V after 60 s confirmation, and critical protect at
2.90 V. Protect is now latched until a timer wake sees real battery charge current from
solar/USB, so resting-voltage rebound in the dark will not restart the HEX draw load.

Added `fcdim=` / `fclat=` bridge/dashboard/logger fields plus `/telemetry` booleans
`field_load_dimmed` and `field_protect_latched`. Build artifacts:

- `firmware/net_bench/build/field-cycle-peer-20260708-adr23-latched/net_bench.ino.bin`
- `firmware/net_bench/build/serial-bridge-20260708-adr23-latch-tail/net_bench.ino.bin`

## 2026-07-07 - Codex - Hardened field-cycle JSONL logging before next OTA

The 2026-07-05 HEX-load field-cycle logger stopped around 2026-07-07 05:03 PT because
the host UDP receive buffer was still 1024 B while the `nb-peer` line had grown with
field-cycle/BQ telemetry. Increased `ops/bench/net_bench_log.py` to receive full-size
UDP datagrams so the next MPPT-tail OTA/logging pass does not trip the same Windows
`WSAEMSGSIZE` failure.

Also recorded a bench TODO for one clean sunrise-to-sunrise solar-cycle capture with
known fixture placement, sensor orientation, disk headroom, and the corrected logger.

## 2026-07-07 - Ben + Claude - speaker_demo: STEMMA speaker (#3885) percussion-synth bench; noisemaker shootout status

Noisemaker shootout status from Ben's informal crowd testing (small n, treat as
directional): piezo too soft (and needs solder); vibration motor reads as a
cell phone; SparkFun Qwiic Omron relay has a great click (more click than
clack) but $18/unit kills it at fleet scale; Modulino buzzer and RedBot
speaker read as buzzers; 8002A amp is loud but its square-wave "nintendo
sounds" were disliked. The consistent signal across testers: SQUARE-WAVE TONES
feel harsh / at odds with the bamboo-tree aesthetic. Relay clicks got mixed
reviews (some loved, some meh -- possibly a failure to imagine 150 rippling
through the tree rather than a verdict on the sound). Two candidates remain:
(A) Adafruit STEMMA speaker #3885 ($4.76, PAM8302 analog amp + mini speaker),
(B) Adafruit MOSFET driver + push-pull solenoid mallet striking the bamboo
itself (idea stage, see TODO).

New `firmware/speaker_demo/` for candidate A, on the PowerFeather V2 demo unit,
repurposing the LED header: speaker V+ on the GPIO4-gated 3V3 rail, GND, signal
on A0/GPIO10. Since the #3885 is an ANALOG amp, the sketch skips tone() square
waves entirely and runs a small fixed-point synth: 16 kHz GPTimer sample ISR
mixing up to 12 voices (decaying sine partials via 1024-entry LUT, xorshift
noise with one-pole LP, optional pitch glide) into a 78.125 kHz / 10-bit LEDC
carrier, duty written by direct LEDC register access (driver calls are not
ISR-safe; S3 needs the conf0.low_speed_update latch strobe). DC bias slews
0<->512 over ~32 ms (no pop into the AC-coupled amp input) and auto-mutes to
PWM-low after 3 s of silence. Sound palette targets "organic": bamboo
knock/tock/tick (sine partials + contact-noise chiff), shaker, marimba (random
A-minor pentatonic), chime (inharmonic partials, ~1.3 s tail), water drip
(rising glide) -- plus one square "beep" kept as the A/B baseline. "Ripple"
plays a ~2.4 s 20-knock cascade and "Grove" free-runs sparse random knocks
(exponential gaps, adjustable events/min) -- single-fixture previews of the
150-fixture idea. Dashboard at `speakerdemo.local` (sway_demo plumbing: BubbyNet
STA + AP fallback, /update OTA, guarded charging + solar guard, telemetry);
"Amp power" button cuts the 3V3 header rail itself -- the production-style
software kill-switch.

Fixed-point lesson worth keeping: voice amplitude at Q15 with a Q30 decay
multiply truncates ~0.5 LSB/sample, which flattens long quiet tails -- the
1.3 s chime died at ~250 ms, matching the truncation math exactly. Amplitude
now Q23 (mix does one more 64-bit multiply); with wall-clock polling of
/state the three chime partials now die on their designed 0.45/0.8/1.3 s
schedule. (First measurement looked 2.5x fast even after the fix -- that was
curl latency inflating a sleep-based poll loop, not firmware. Measure with
timestamps.)

Verified on hardware tonight: USB flash of .1, then .2 via /update OTA (works,
mutes audio during flash); PF SDK up, guarded charging found the cell at
3.38 V and enabled 500 mA (SOC ~98 % on USB, solar guard active); boot knock,
full palette, ripple, grove all trigger via HTTP; bias wake/mute observed.
NOT yet judged: how it actually sounds (Ben's ears, then crowd), idle vs
playing current draw, and acoustics through the lantern/hat geometry.

Bring-up addendum (.3-.5, all OTA'd on battery, no tether): the speaker was
silent and the rail read 0 V. Three separate faults, each with a lesson:

1. **3V3 header at 0 V with everything commanded on** -- Ben found the rail
   came alive after reseating/fixing the harness; consistent with the header
   load switch folding back into a short in the custom JST hookup. Debug aid
   that settled the software half remotely: .3 exposes the actual GPIO4 pad
   level (`rtc_gpio_get_level`) as `en3v3` in /state -- pad read 1 while the
   header read 0 V, proving the fault electrical, not firmware.
2. **SDK rail-pin traps found while chasing (1)** (POWERFEATHER_NOTES updated):
   with the SDK up, EN_3V3/VSQT are RTC-HELD pads -- raw pinMode/digitalWrite
   on GPIO4 is silently ignored (and remuxes the pad); `Board.enable3V3()`
   try-locks and can fail silently -- check the Result and retry. speaker_demo
   .3 does both; raw GPIO4 writes now only in the init-failed fallback.
3. **A0 never drove: ledcAttachChannel(78125 Hz, 10-bit) fails** -- silently
   (bool return, warning only on the unread serial). 78125 x 1024 = 80 MHz
   exactly, and the LEDC auto-clock picked the 40 MHz XTAL, where that needs an
   impossible 0.5 divider. The 16 kHz sample ISR ran perfectly (bias/voice
   state machine all consistent over HTTP) while writing duty into a
   never-started timer -- the pin floated, and the PAM8302's input-bias leakage
   read ~30 mV at SIG, which is what finally pointed at the pin. .5 carrier is
   39062 Hz / 10-bit (divider 2.0 at 80 MHz, 1.0 at 40 MHz XTAL -- accepted
   either way) with lower fallbacks; /diag dumps the live LEDC registers
   (attach ok, timer res/div, counter moving, latched duty) + ISR-rate counter
   so the pin-drives-or-not question is answerable without a scope. Measured
   after .5: ISR 16007 Hz, timer running, duty 8192 = the 512<<4 awake
   midpoint. Lesson: on S3, always CHECK ledcAttach* returns, and don't design
   a carrier at exactly the clock ceiling.
4. **Audible whine at 5.2 kHz (+ ~21 kHz on a phone spectrum app) once sound
   worked** -- intermod beats between the 39062.5 Hz carrier and the 16 kHz
   sample clock, which were not integer-related: |3fc-7fs| = 5187 Hz and
   |fc-fs| = 23.06 kHz (folds to ~21 kHz at the phone's 44.1 kHz Nyquist).
   Numbers matched Ben's measured peaks. .6 integer-locks the clocks: carrier
   78125 Hz @ 9-bit (LEDC divider exactly 1.0 on the 40 MHz XTAL, no
   fractional-divider dither) with the sample ISR at exactly fc/4 = 19531.25 Hz
   (GPTimer 40 MHz / 2048, same crystal) -- every beat product now lands at
   n x 19.53 kHz or DC, out of the audible band. Also stopped re-strobing the
   LEDC duty registers when the value is unchanged, so the idle carrier is
   untouched between notes. Verified via /diag: attach ok, 9-bit, div=256,
   fc/fs = 4.00. Lesson for ANY future PWM-audio (or PWM-LED-near-audio)
   design: pick carrier and sample clocks integer-related and dither-free, or
   the difference tones end up in ears.
5. **Residual whine after the clock lock (~5-14 kHz peaks; source still OPEN;
   one diagnosis made and RETRACTED the same evening).** A "whine tracks the
   pitch slider" observation (x0.5 -> 5.8 + 9.7 kHz; x0.26 -> 7.4 + 14 kHz)
   briefly pointed at quantization limit cycles on the decaying tails, but
   Ben could NOT reproduce it and identified the confound: the speaker's
   MOUNTING changed between measurements (bare desk vs coaster -- which also
   audibly changed the richness), so every phone-app spectrum from tonight
   has placement as an uncontrolled variable. What remains established:
   (a) the whine rides the awake-carrier window and stops at the idle mute,
   so the energy source is our carrier/output path, not amp/rail background;
   (b) the .7 156250 Hz/8-bit A/B played grossly distorted -- a gross effect,
   robust to the placement confound -- consistent with exceeding the
   PAM8302's ~250 kHz internal modulator's input Nyquist; mode removed, keep
   PWM carriers ~40-80 kHz for this amp. Candidate sources, none confirmed:
   quantization limit cycles (tonal, would pitch-track), PAM oscillator beats
   (analog, chip-dependent), speaker/mount electromechanical resonance
   demodulating carrier energy (placement-dependent -- the one the confound
   observation actually favors). .8 ships TPDF dither (+-1 duty LSB in the
   ISR), carrier 39062.5 Hz @ 10-bit integer-locked at exactly 2 x fs (fault
   4 cannot return; halves the LSB), and a half-LSB voice-kill floor --
   cheap regardless of verdict, and they eliminate the limit-cycle candidate
   outright. Controlled re-listen pending, complicated by (6). Lesson:
   pin the MECHANICAL setup before trusting acoustic spectra.
6. **Bench casualty + an acoustics finding.** The #3885's tiny trim pot lost
   its wiper top mid-adjustment; the board now passes signal only with the
   pot pressed or its pads bridged. Fix: solder-bridge the pad pair Ben
   already identified with tweezers (pins the gain; the firmware volume
   slider becomes the volume control -- which production wants anyway) and
   order a spare #3885 for the crowd test. Fleet lesson: an exposed trim pot
   is a liability at playa scale (dust, vibration, fingers) -- bridge/epoxy
   it in production, or evaluate the MAX98357A I2S amp (no pot, true DAC
   path, ~same price, 3 data wires) if the PWM path keeps fighting. Genuine
   finding from the confound: mounting coupling is a first-order acoustic
   variable (coaster >> bare desk for richness) -- the bamboo lantern is a
   resonant tube, so test the speaker coupled into the lantern geometry.
7. **Controlled carrier A/B (fixed placement, fixed pot bridge) settled fault
   5's open verdict: the AMP-OSCILLATOR BEAT theory is the one that fits.**
   On fw .8 (dither active in both modes): 39 kHz/10-bit carrier -> strong
   ~6 kHz whine; 78 kHz/9-bit -> no whine, just louder broadband hiss with a
   slight ~5.2 kHz bump. Dither had already removed the limit-cycle
   candidate, and fixed placement removed the mount-resonance candidate --
   what remains, and fits the asymmetry, is the PAM8302's free-running
   class-D oscillator beating the carrier's odd harmonics: with this unit's
   oscillator near ~200 kHz, the 39 kHz carrier's strong 5th harmonic
   (195.3 kHz) beats at ~6 kHz, while the 78 kHz carrier's nearest strong
   harmonics beat at 33-45 kHz (ultrasonic). The broadband hiss was the .8
   dither itself, +6 dB at 9-bit. fw .9: default carrier 78125 Hz/9-bit;
   sample rate doubled to 39062.5 Hz (still exactly fc/2, locked); dither is
   now HIGH-PASS-SHAPED TPDF (energy pushed toward fs/2 = 19.5 kHz, off-ear
   and off-speaker) and GATED on voice activity (idle-awake carrier runs
   mathematically clean; in-note dither hides under the program). FLEET
   CAVEAT (important): the PAM oscillator is loose and varies chip to chip,
   so NO carrier choice is universally safe across ~114 amps -- some unit
   will always land a harmonic near its oscillator. Per-unit A/B button is
   the bench probe; the robust production fixes are an inline RC low-pass
   (~1k + 10 nF) on SIG or an I2S amp (MAX98357A). Estimates of this unit's
   oscillator are rough (phone-app "ish" numbers); the mechanism call rests
   on the controlled A/B contrast, not the exact frequencies.

VERDICT (end of session, Ben's ear): fw .9 is CLEAN -- no whine, hiss
"basically inaudible and pleasant" with ear against the speaker. Remaining
wish: more loudness. Headroom notes for next session: (a) the bench tests ran
at vol 60 = 0.36 of full scale (the slider's square-law taper) -- vol 100 is
+9 dB and was set at session end; (b) per-sound amplitudes sit at 0.7-0.95 --
a "loud mode" with >1 drive into a soft clipper could buy several more
perceived dB at some percussive edge cost; (c) the #3885 is running from the
3V3 rail; the PAM8302 makes ~2x the power at 5 V, but the PowerFeather has no
5 V rail in the field, so don't count on that; (d) the big free lever is
ACOUSTIC: the coaster observation says coupling/baffling dominates -- mount
the speaker against/into the bamboo lantern tube before judging loudness
(also the artistically right answer: the lantern becomes the instrument).
Still open: idle + playing current draw for the night power budget.

Direction check from Ben at session close: even with candidate A clean, he
still leans toward PHYSICAL noisemakers -- relay clicks (mixed crowd reviews,
but he suspects listeners couldn't imagine 150 of them rippling through the
tree) or the completely untested MOSFET + solenoid mallet (candidate B). So
candidate B's first bench test rises in priority; the speaker stands as the
proven fallback/complement (and its synth knock is the preview instrument for
selling the ripple concept to the team).

Ben's overnight hang test (constrained rig, n=1): accel bubble tracked hand
tilt well; sway barely moved the bubble but the light effects read nicely --
consistent with the pendulum-degeneracy prediction (accel-only tilt can't see
swing direction while hanging; note also the sketch's 0.4 s gravity low-pass
partially tracks a ~2 s swing, so some bubble motion under big pushes is filter
leakage, not disproof). Sensor-selection discussion concluded: keep MSA311-class
accel for production (sway energy, ~zero power, no per-unit cal -- BNO055/085
per-device calibration effectively disqualifies them at fleet scale), and get
true tilt GEOMETRICALLY from the downward multizone ToF that every downlight
carries anyway. Deployment context recorded: ~74 hanging at ~10 ft, ~40 on 5 ft
perimeter shepherd hooks.

sway_demo .3/.4 (OTA'd, no tether):
- VL53L5CX (vendored driver copied from presence_bench; additionally disabled
  signal_per_spad -- loop-idiom sketch, the per-frame read must stay short on
  the shared 100 kHz Wire1) at 4x4 @ 10 Hz. Robust LS plane fit over the zone
  ranges -> geometric tilt vs ground + height at nadir; per zone we keep the
  FARTHEST valid target (ground sits behind a person). Web UI: cyan ToF ring
  next to the accel dot on the bubble display, zone heatmap, height readout;
  /tofraw debug endpoint dumps the raw two-target frame.
- Fit math verified on host: synthetic planes 0-40 deg recovered exactly, incl.
  a one-zone occluder rejected by the outlier pass.
- NOT yet verified on real geometry: the bench unit is lying face-down, every
  zone returns a valid target at 0-13 mm (correctly rejected by the >30 mm
  floor) plus status-4/13 multipath ghosts (correctly rejected). Needs aiming
  at the floor from >0.5 m to light up the fit.
- Charging: Ben's demo cell accidentally ran the fixture overnight, and .1/.2
  (bench default, charger OFF) meant USB was NOT recharging it (found at 13%
  SOC, ma=-3 on USB). .3 ports the presence_bench guarded one-shot: charging
  stays off unless the warmed-up gauge reports a plausible cell (2.5-4.4 V),
  then 500 mA LFP-profile charge + solar guard. Verified live: +381 mA into
  the cell immediately after OTA.

.5 (same day): Ben's jury-rig holds the ToF off-level and hard to shim, so the
Re-zero button now also zeros the ToF: it captures the fitted plane as the
mount reference (auto on first good fit). Verified live on the propped rig:
16/16 zones valid, 15 used (one outlier auto-rejected), mount captured at
15.7 deg -> relative tilt near zero, height 0.62 m. The full accel-vs-ToF chain
is now verified on real geometry.

.6 (same day): Ben reports the rig SPINS hard when swung -> re-derived the
geometry. Result: the mount-zero survives spin (the offset is rigid in the
sensor frame), but (a) reported tilt DIRECTION is in the spinning body frame --
unavoidable without a yaw reference -- and (b) .5's component-wise subtraction
added a spin-phase flutter (~0.5 deg on a 10 deg swing at the 15.7 deg mount;
synthetic test). .6 stores the zero as a unit normal and reports the exact 3D
angle acos(n . n0): synthetically spin-invariant to 0.000 deg at every spin
phase, any mount tilt. Also noted: heavy spin corrupts the ACCEL side more
(centripetal ~1 g at 3 rev/s a few cm off-axis pollutes bubble + sway env) --
production implication: mount the accel near the spin axis. OTA'd to the unit
battery-only (USB came loose during rig fiddling -- cell draining ~130-180 mA,
37% SOC; charging re-arms when USB returns).

Next: eyeball accel-vs-ToF bubbles on the hang rig (prediction: sway pulses the
accel dot's color while the cyan ToF ring actually swings); consider a ToF-tilt
LED mode once trusted; then the outdoor lantern test.

## 2026-07-06/07 - Ben + Claude - 32700 SHOOTOUT: Palowextra "7.2Ah" busted (5,643); fullbattery qualified n=2 (5,752); power-policy thresholds derived (ADR 0023)

**The Amazon "7.2 Ah" 32700 (Palowextra) is a ~5.6 Ah cell in a big wrapper, and it
loses to the $1-cheaper fullbattery 6 Ah where it counts.** Head-to-head full-discharge
coulomb runs (both cells virgin, HEX37 val224 ~0.86 A, 79.9 °F, INA219 truth, Nitecore-only
charging): P delivered **5,643 mAh to 2.5 V (78 % of label)** vs F's **5,752 (96 %)** —
near-tie to a lab cutoff, but P's **2.3× IR (136 vs 60 mΩ)** pulls its knee through the
3.0 V product floor 1.4 h early: **4,342 vs 5,139 mAh usable (−15.5 %)**. Corroborated by
weight parity (136/138 g) and Off-Grid Garage's 5,450 on a cycler. **Ben's decision: buy F,
skip P; cycle-2 rig swap deliberately cancelled** (rig confound ~8 % can't close 15.5 %).
Report with charts: `docs/tests/BATTERY_32700_SHOOTOUT_REPORT_2026-07-06.html`.

Free findings: (1) **F reproduced June 11 within +0.5 %** — production cell qualified
n=2, 75-unit purchase validated at $0.89/Ah; (2) **MAX17260 +8 % bias is a chip trait**
(+9.3/+8.1 % on two boards, replications 7-8 — /1.08 is universal); (3) both cells end in
a brownout rattle only below ~2.6 V (31/35 resets — state-difference artifact, earlier
"P cascades / F fades" read was wrong); (4) gauge SOC swept 98→0 %, plateau-blind as ever;
(5) MAX17260 won't cold-POR off a ~2.8 V cell (looks like no-cell; self-recovers on
precharge — POWERFEATHER_NOTES).

Tooling (commit 3668554): `afk_discharge.py --ina-file/--no-ina` + `ina_logger.py` tee =
two rigs share one 4-ch INA monitor; `ina_mapcheck.py` green-pulse wiring diagnostic
**caught 3 reversed shunts + 1 crossed rig label before they touched data** (also: KB2040
QT-rail short = red/black-swapped adapter; run mapcheck on any new rat's nest). Boards
9E5B0C/9E5AF0 flashed power_bench no-charge floor-2.3 for the runs, restored to
charge-ma 500 / floor 2.90 after.

**ADR 0023** distills the F curve into production dim/off/sleep thresholds (standard
tier: dim 3.00 / off 2.95 / sleep 2.90 under full load — LED holds full brightness to
2.70 V, first instability 2.69 V at 99 % delivered, overnight+OTA reserve is only
~50 mAh; hysteresis + coulomb-primary hybrid required). Re-derivation recipe in the ADR.

## 2026-07-06 - Codex - Built safe field-cycle MPPT perturb firmware, not deployed

Designed and built `net-bench-2026-07-06.1` for tomorrow's solar-cycle experiment.
The new `--field-mppt` field-cycle option preserves the initial OTA listen window, then
samples 4.6/4.8/5.0 V VINDPM candidates during healthy charge wakes. It records status,
skip/run reason, run count, active/best/last VINDPM, and candidate panel powers as a new
heartbeat tail plus `/telemetry` JSON. Safety policy for this first bench build: 4.6 V
remains the only persistent/rescue default; the peer clamps back to 4.6 V before sleep
or maintenance unless a future `--field-mppt-hold` build is explicitly used.

Host updates: `ops/bench/net_bench_dashboard.py` and `ops/bench/net_bench_log.py` parse
the new `mppt*` suffix. The heartbeat now exceeds the old 128 B bridge buffer, so the
matching serial bridge must be flashed before an MPPT peer is deployed.

Built but did not deploy:

- `firmware/net_bench/build/field-cycle-peer-20260706-mppt-safe/net_bench.ino.bin`
- `firmware/net_bench/build/serial-bridge-20260706-mppt-tail/net_bench.ino.bin`

## 2026-07-06 - Codex - Low-VBAT OTA boundary TODO

Added a TODO to bracket the true low-VBAT OTA boundary on the current shared-WiFi
path with historical confounders removed. The clean successes so far are around
3.10 V loaded battery-only, 2.901 V solar-assisted, and 2.496 V USB-assisted, while
the lower-voltage "failures" in the logs are contaminated by wrong maintenance paths,
stale WiFi secrets, deprecated AP-mode images, or pre-upload failures. Future tests
should record separate bounds for battery-only, solar/VDC-assisted, and USB-assisted
OTA using known-good credentials and targeted `U<id>` maintenance.

## 2026-07-06 - Ben + Claude - sway_demo: MSA311 tilt/sway drives the RGBW point source, with a web verifier

First motion-reactive lighting bench app: `firmware/sway_demo/`. An MSA311
(Adafruit STEMMA-QT) on Wire1 feeds a 50 Hz gravity low-pass (tilt vs a
calibrated rest pose) + high-pass delta -> fast-attack/slow-decay envelope
(sway), mapped onto the single 4 W SK6812 RGBW on GPIO10. Three web-selectable
mappings: sway (default; hue amber->violet + brightness with motion energy, W
flash on big spikes), tilt (hue = lean azimuth, brightness = lean angle), both.
The built-in web app (http://swaydemo.local/) draws a bubble level + sway pulse
ring + 30 s strip chart, painted in the exact RGBW the LED is showing, so the
mapping is verifiable by eye. Patterns reused: led_studio (LED/web/OTA),
presence_bench (SDK init with charging OFF, VSQT power-cycle, EN_HIZ clear,
Wire1 pinned at 100 kHz per the bus-integrity rule).

Bench verification (USB flash to /dev/ttyACM1, then the .2 tweak via WiFi OTA):
MSA311 found at 0x62, |g| ~= 0.96-1.00 flat on the bench, idle noise floor
env ~0.005 g (2% of the default 0.22 g full scale -- rock-stable resting
color), sway/tilt/color all tracked motion as intended. One real finding: the
resting base brightness of 30 landed in the SK6812 gamma dead-zone
(rgbw=1,0,0 -- POWERFEATHER_NOTES low-end issue), so .2 raised the default to
60. Charging is OFF in this sketch; port the led_studio charger + solar-guard
config before using it with a cell unattended.

Housekeeping: removed a stray `<<<<<<< HEAD` merge-conflict marker that had
been committed at the top of this file.

Next: hang the sensor+light in a lantern and tune the mapping against real
pendulum dynamics (envelope decay vs swing period, sensitivity default), and
decide whether tilt-hue or sway-hue reads better through the gobo.

## 2026-07-05 - Ben + Claude - 46-hour continuous battery soak seals the 100 kHz fix; ended by honest cell exhaustion

The Test-A configuration quietly kept running: **boot#136 (fw `.31`, full
five-sensor bench, Wire1 at 100 kHz) ran 46.2 h continuously** (2026-07-03
18:10 -> 2026-07-05 16:20), mean discharge 209 mA, ending only when the 7.2 Ah
LFP hit the cliff (bv 2.51 -> 2.31 V in the final samples, then dark). The
July-2 configuration averaged ~60 s per boot on battery; this ran ~2770x
longer and died of an empty cell. Bus-clock fix considered SEALED.

Honest bookkeeping: (1) a watcher fired a stale "bus-speed story FALSIFIED"
template on the end-of-discharge reboots -- WRONG, it was speaking outside its
15-minute design window; the JSONL shows one 46 h boot then cliff-voltage
cycling. (2) The integrated 5830 mAh "delivered" is NOT a clean capacity number:
mid-run bv reached 3.54 V, so USB was attached for part of the window (charging
while running). (3) Gauge SOC (42 -> 0) advisory as always on LFP. Cell needs a
recharge; board revives on USB.

## 2026-07-05 - Codex - Field-cycle OTA helper guardrails

Reviewed `ops/bench/field_cycle_ota.py` as the bench-specific wrapper for targeted
field-cycle OTAs. Kept the scope intentionally narrow, but added two guardrails from
the 2026-07-05 `9F26F8` OTA:

- `--maint-resend-s` re-sends targeted `U<id>` during discovery so a sleeping field
  peer has multiple chances to catch the maintenance command.
- `--ip` now validates `/telemetry` fixture identity before upload unless explicitly
  overridden with `--trust-ip`.

Also added a TODO to later extract the generic OTA workflow primitives into a shared
Python module once production firmware/deployment tooling needs them. For now,
`field_cycle_ota.py` remains a field-cycle pit-crew helper, not a general deployment
framework.

## 2026-07-05 - Codex - Aggressive HEX-load field-cycle v5 OTA

Ben freed disk space and asked to OTA the higher-information solar cycle build. Bumped
`firmware/net_bench/net_bench.ino` to `net-bench-2026-07-05.1` and built a peer image
for outdoor node `9F26F8` with the same state thresholds but a heavier dark-period
HEX load:

```
--field-low-mv 3100 --field-critical-mv 3000 --field-low-confirm-s 60
--field-led-load --drawdown-lit 18 --drawdown-brightness 128
```

Targeted `U9F26F8` maintenance worked, but the peer disappears from the ESP-NOW
dashboard while it is in WiFi-only maintenance, so the IP must be discovered from the
shared WiFi side. A subnet `/telemetry` scan found `9F26F8` at `192.168.4.71`.
`ops/bench/net_bench_ota.py --reboot comms` uploaded the v5 image successfully; the
peer rejoined ESP-NOW as `fw=net-bench-2026-07-05.1`, `reset_reason=software`,
`maint_status=0`, then resumed field-cycle charge/sleep behavior.

Started a fresh 48 h logger:

- `ops/bench/data/ca/2026-07-05-ca-field-cycle-9F26F8-hexload-v2-aggressive.jsonl`

First post-OTA charge sample showed about `battery_v=3.395 V`, `supply_w=0.94 W`,
`battery_w=0.59 W`, `lux=18079`, BQ charge current target near `1480 mA`, and
`field_phase=charge`. Between 5-minute charge wakes the dashboard peer row will age
out; that is expected for this experiment, not by itself a dropout.

## 2026-07-03 - Codex - HEX-load field-cycle v4 OTA after bridge/peer swap

Swapped the bench roles so the PowerFeather with the soldered HEX header (`9F26F8`)
became the outdoor field-cycle peer and the former solar peer (`9E5AB8`) became the
USB serial bridge on COM4. The bridge is running the serial-bridge master image; the
field peer is logging under:

- `ops/bench/data/ca/2026-07-03-ca-field-cycle-9F26F8-hexload-v1.jsonl`

First field-cycle HEX load build (`net-bench-2026-07-03.1`, 18 pixels at brightness
96) successfully OTA'd and entered draw mode, but the dashboard showed only one fresh
post-OTA packet before going stale. The root cause was likely command timing, not a
radio brownout: the sustained targeted `U9F26F8` maintenance command was still being
sent while the freshly OTA'd peer rebooted, so the new image was immediately caught
back into maintenance. Maintenance telemetry later showed the peer alive at
`192.168.4.42` and still drawing about 400 mA because the field-cycle HEX load was not
blanked before entering maintenance.

Patched `firmware/net_bench/net_bench.ino` to `net-bench-2026-07-03.2` so
`enterMaintenance()` explicitly blanks/cancels bench loads before switching to WiFi
OTA. Built and OTA'd a gentler night-run image for `9F26F8` after waiting for the
sustained `U` window to end:

```
--field-low-mv 3100 --field-critical-mv 3000 --field-low-confirm-s 60
--field-led-load --drawdown-lit 9 --drawdown-brightness 64
```

OTA ack succeeded via `ops/bench/net_bench_ota.py` using the shared-WiFi maintenance
path. The peer rejoined ESP-NOW with `fw=net-bench-2026-07-03.2`,
`reset_reason=software`, `field_phase=draw`, no supply, and a stable load around
0.74-0.76 W (`battery_v` about 3.20 V, `battery_ma` about -232 to -236 mA) after a
short soak. This should produce a more useful overnight drawdown than the 1.4 W
short-blast configuration.

## 2026-07-03 (cont.) - Ben + Claude - Housekeeping: June brownout docs re-graded under the unified bus-integrity story

Ben's call: with the story straight, purge the stale hypotheses. Done, with
evidence-honest framing (June was never instrumented at the BQ register level,
so the unification is best-supported inference, clearly labeled):
- `BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md` gains a RETRO-ANALYSIS header
  re-grading every hypothesis: H2 (connectors/"marginal connection") RETIRED as
  leading explanation -- no confirmed connector kill in either dataset, and the
  June re-seating observations are equally explained by the IS31/STEMMA seat
  changing bus loading; H5 load-stacking and H3 boost-mode DEAD; H4 bulk-cap
  RETIRED as remedy (caps fix sag, not switch-openings); H1 TX-transients
  subsumed as the noise source, not the load. The best-aged June sub-result:
  "VSQT shed didn't help, only physical disconnection did" -- the bus-integrity
  mechanism announcing itself.
- ADR 0018: dated addendum refining "not a general bus property" -- the general
  property IS the bus; the NeoDriver result proved that device benign, not the
  bus robust. Decision unchanged (strengthened).
- TODO brownout section: header updated to RESOLVED + UNIFIED; retired items:
  reflow-board-1-joints, VSYS-bulk-cap (x2), NeoDriver-overnight; the
  field-watch item reframed to the July telemetry lens (rr=poweron on battery
  => suspect the bus first; port the boot-counter/reset-reason/breadcrumb idiom
  to production firmware).
Droopy-battery / bad-connector language survives nowhere as an active suspect;
connector hygiene remains noted as ordinary good practice only.

## 2026-07-03 - Ben + Claude - CASE CLOSED: the 400 kHz Wire1 clock was the killer; 100 kHz full bench rock-solid on battery

Morning wrap of the reboot hunt. Overnight `.28` soak: 7.3 h on battery, ZERO
deaths (the no-SDK config). Then the decisive Test A: **`.29` = the FULL
firmware (sensor task, SDK round-robin, breadcrumbs, all five sensors + mux)
with ONE change -- Wire1 at 100 kHz instead of 400 kHz -- ran 900+ s on battery
on the WORST board (the spare, ~60 deaths of history), in the old crossover
band, at -318 mA full load.** Against `.23` (same STA-only radio profile,
400 kHz) dying in 10-160 s: only the clock differed. Bus speed convicted;
POWERFEATHER_NOTES' "keep the SDK's bus speed" guidance vindicated -- our
"measured exception" was the root cause all along.

Refined mechanism (and two corrections for honesty): the shared Wire1 also
carries the BQ25628E -- the chip the battery current flows through. At 400 kHz
under WiFi TX noise, corrupted transactions near the power path's control
registers (BATFET/ship/EN_HIZ class) open the battery switch outright: no sag,
no brownout detector, straight to reset_reason=poweron; USB immune because VBUS
bypasses the BATFET. Retro-explains the early -290 mA-discharge-on-USB anomaly
(a stray EN_HIZ set, later cleared). CORRECTIONS: (1) cont. 10's "core-0"
attribution was inference, not measurement -- the bisect varied WHAT ran, never
WHERE; the round-robin was simply the only 400 kHz talker to the charger. Core
interaction remains at most an aggravator (optional Test B if we ever care).
(2) The XM125's ~5% read errors were cited as 400 kHz-marginality evidence --
RETRACTED: they persist at 100 kHz (the XM's own protocol quirk, already on
the TODO). The conviction rests on the controlled A/B, not the tea leaves.

Cost of the fix: sensor cadence 0.8 -> 0.6 Hz, VL53 blob 2.7 -> 9.4 s. Nothing.
`.30` makes 100 kHz the compiled default; README bus section rewritten;
POWERFEATHER_NOTES gains a hard-won section with the rules ("never raise the
clock on any bus shared with the charger/gauge"; dedicated power-management bus
on the custom PCBA; treat battery-only poweron resets as possible power-path
register upsets). Note the June IS31 brownout rhymes (shared-power-bus
disturbance, ADR 0018) -- the general pattern is now documented.

Remaining follow-ups: reflash led_studio onto the desk board (r10 pending);
label both boards; optional Test B (400 kHz from core 1) for mechanism rigor;
XM125 decode session; and back to the actual mission -- walk-under datasets +
the lantern-rig splay-occlusion session.

## 2026-07-02 (cont. 10) - Ben + Claude - CONVICTED: the PowerFeather SDK battery round-robin from the core-0 task

Final bisect verdicts, same night: `.26` (task ON, SDK calls gated) ran 380+ s
stable on battery; `.27` (task ON, battery round-robin ON, charge-enable gated)
**died within seconds, repeatedly**. Combined with `.24` (everything on) dying
and `.25` (no task) stable: **the killer is the SDK battery round-robin -- one
PowerFeather SDK read (getBatteryVoltage / getBatteryCurrent / getBatteryCharge
/ getSupplyVoltage / checkSupplyGood, one field per 800 ms) issued from the
FreeRTOS task pinned to CORE 0 -- concurrent with the WiFi stack that lives on
the same core.** Charge-enable acquitted (and timeline-consistent: it did not
exist before `.15` yet `.13/.14` already died). NVS acquitted earlier.

The mechanism puzzle for tomorrow: led_studio issues the IDENTICAL SDK calls at
the IDENTICAL cadence from core 1's loop() and is historically stable on
battery, and the loadgen reads the gauge every heartbeat from loop() likewise.
So the variable is not the reads -- it is WHICH CORE / what concurrency they run
under. Working hypotheses to test with a scope or targeted builds: (a) SDK I2C
transactions from core 0 delay/preempt WiFi-stack timing so the radio's power
state machine misbehaves mid-TX (VSYS spike alignment); (b) an SDK-internal
critical section / interrupt-disable window that is benign on core 1 but toxic
on core 0 during TX; (c) Wire1 clock-stretch or bus stall interacting with
core-0 interrupt latency. Production takeaway REGARDLESS of mechanism: on this
platform, keep PowerFeather SDK / power-management I2C OFF core 0 (or off a
core-0-pinned task) when WiFi is active -- direct constraint on ADR 0005's
task-architecture design.

Overnight: ~10 min of `.27` death samples accumulated for the record, then
`.28` (= the stable no-SDK config, version-bumped for log clarity) auto-flashed
for an 8 h battery soak -- morning silence = long-horizon confirmation.
Tomorrow: implement the fix (battery reads from loop()/core 1, led_studio-style,
with explicit Wire1 serialization vs the sensor task -- or a core-1 task),
restore full presence_bench, reflash led_studio onto its board, re-run the
formal 10-min confirmations, label boards.

## 2026-07-02 (cont. 9) - Ben + Claude - The reboot hunt goes firmware-side: hardware fully exonerated, bisect narrowing on the core-0 sensor task

The elimination cascade, in one evening: A0 jumper pulled -> still dies; holder
replaced with a FULL 7.2 Ah LFP on soldered welded-tab leads -> still dies (at
bv 3.50-3.55, killing the crossover theory -- clean buck region); SoftAP
disabled (.23 STA-only) -> still dies; **cell + firmware moved to the
led_studio desk board -> DIES THERE TOO** (110 s at 3.55 V). Hardware is out.
Then the anchor test, Ben's idea: **June's `power_bench --loadgen` on the same
board + cell survived 10 min on battery INCLUDING its heavy phase (200x512 B
UDP/s + LED at bv 3.33)** -- an order of magnitude more TX than presence_bench
generates. presence_bench firmware convicted by contrast. RSSI spread across
boots (-22 vs -52) explained by the June Eero-latching finding (each boot
re-associates to a different node) -- consequence, not cause.

Bisect ladder (each build = ONE variable, battery-run on the desk board):
- `.24` = .23 minus the 10 s NVS breadcrumb writes -> **DIED. NVS acquitted**
  (despite being suspect #1 and the classic ESP32 flash-write-brownout story).
- `.25` = .24 minus the core-0 sensor task entirely (no I2C/SDK/charging after
  setup; WiFi + WebServer + mDNS remain) -> **~300 s stable, no deaths**
  (provisional; 10-min confirmation queued for tomorrow). Sensor task
  provisionally convicted.
- `.26` = .24 with the task ON but its SDK calls gated out (PB_TASK_NO_SDK: no
  battery round-robin, no charge-enable; probes/init machinery intact) --
  **RUNNING OVERNIGHT.** Survival => "PowerFeather SDK I2C from core 0"
  convicted (note led_studio does the same calls from core 1's loop() and is
  stable -> the variable would be WHICH CORE / concurrency, directly relevant
  to ADR 0005's FreeRTOS production architecture). Death => the task's
  probe/scheduling side.

Instrumentation kept honest: bisect builds without batteryTick report bv/ma=0
(watcher survival rule switched to uptime-only; Ben controls the unplug).
All state samples in `ops/bench/data/presence/2026-07-02_rebootwatch.jsonl`;
loadgen heartbeats in `2026-07-02_loadgen_udp.log`. Flags added: build.sh
--no-breadcrumb / --no-task / --task-no-sdk. NOTE: the desk board needs
led_studio reflashed when the hunt concludes (r10 pending).

## 2026-07-02 (cont. 8) - Ben + Claude - CORRECTION: the spare is a PRISTINE board (not June's board 1); suspects re-ranked

Ben inspected: the spare has NO hand-soldered JST (June board 1's marker) --
nothing but the A0 flying jumper added hours ago. So cont. 7's "board-1
signature" reads as the same FAILURE CLASS, not the same board, and the June
doc's n=3 twist now cuts differently: June's pristine boards were stable on
battery **with a soldered cell**; tonight's pristine board loops **with a 26650
HOLDER**. Re-ranked suspects: (1) holder/pigtail loop impedance (H2 relocated
to the holder -- spring holders are the classic variable-milliohm offender);
(2) the A0 flying jumper (only non-stock element, installed hours before the
reboots were noticed -- electrically innocent on paper, mechanically a bench
confound; 30-second test: remove it); (3) the AP+STA + HTTP radio profile on a
marginal loop (June's stable runs were STA+UDP loadgen -- fix ladder if so:
--wifi-lowpower -> STA-only -> localhost dashboard + sparse updates, the
net_bench recipe); (4) stock-V2 thin VSYS margin under AP+STA (June H4, the
never-tested bulk-cap insurance). Also noted from the June doc: a LIT panel on
VDC buffers spikes like USB (BQ power path supplements from input when a supply
is present) -- explains the solar peer's daytime immunity; a dark panel is
near-negligible, its nights survive on the sleep-dominated ESP-NOW profile.
NEXT ladder (cheapest first): pull the A0 jumper + battery run; then Ben's
plan: run June's own `power_bench --loadgen` (STA+UDP, no AP/HTTP) on this
board+cell -- stable => radio profile implicated; loops => bypass the holder
with tabbed/soldered leads (or the INA screw terminals as the fat path) to
split holder-R from H4.

## 2026-07-02 (cont. 7) - Ben + Claude - Decisive: reboots persist with the ENTIRE sensor chain unplugged -- WiFi alone kills it; this is June's board-1 signature

Ben physically unplugged the whole STEMMA/Qwiic chain: the board still
reboot-cycles on battery (rst=1, 10-30 s uptimes, bv 3.31, ~-120 mA = ESP +
WiFi AP+STA only). Sensor load fully exonerated. This matches the UNRESOLVED
June board-1 brownout point for point (LOG 2026-06-04: 794-reboot loop
overnight on battery, poweron resets, healthy bv 3.24-3.46, lightest load,
dying at WiFi association; hypothesis H2 = marginal battery solder joint; the
"inspect/reflow board 1's battery + VDC joints" TODO was never executed).
Consolidated theory: THIS board's battery input path has a resistive/cold
joint; WiFi TX spikes (~300-500 mA sub-ms) dip it below the power path's
undervoltage cutoff; USB is immune because VBUS bypasses the battery input.
Whether the spare (old net_bench master 9F2690) IS June's board 1 or a second
specimen of the same defect class is unknown -- worth identifying.

Fleet reassurance: validated fleet boards ran radio-on-battery at LOWER
voltages all June without this. Next: two 5-minute swap tests -- (1) same
board + different cell/leads (persists -> board joint, reflow it); (2) same
cell/holder + different board (moves -> harness). The config-ladder deaths in
cont. 6 remain valid data but their load-correlation now reads as "more TX/load
peaks = more dice rolls against a marginal joint," not converter crossover.

## 2026-07-02 (cont. 6) - Ben + Claude - Battery-reboot investigation: config ladder run; load-correlated but not load-gated; verdict needs the INA

Instrumented hunt (NVS boot counter + reset reason + 10 s pre-death voltage
breadcrumb + a host watcher appending every sample to
`ops/bench/data/presence/2026-07-02_rebootwatch.jsonl`). ALL deaths are rst=1
(full power loss -- panic/watchdog would read 4-6, the ESP brownout detector 9);
USB runs indefinitely; battery runs die at plateau voltage. Death uptimes by
firmware config (cell drifting 3.33 -> 3.28 V across the session -- a real
confound, noted):

| Config (battery, ~bv 3.28-3.33) | Death uptimes (s) |
|---|---|
| all 5 sensors                  | 11, 11, 11, 10, 103 |
| no MLX / no XM (3 ToFs)        | 173, ~161 |
| MLX on / XM off                | 184, 123, 40, 63 |
| TMF8821 only                   | 81, 20, 30 |

Reading (hedged, n small, drift confound): the full-stack 11 s mode = death at
the moment the 5th sensor comes up and load peaks -- strongly load-correlated.
But lighter configs still die on minute scales, including TMF-only where the
only frequent transmitter left is WiFi itself (bench runs AP+STA -- SoftAP
beacons every ~100 ms). Working model: a MARGINAL BATTERY LOOP (cell IR + holder
+ pigtail R) on the LFP plateau, where instantaneous current peaks (radar
bursts, WiFi TX) transiently dip the BQ's battery undervoltage lockout ->
BATFET opens -> full power-off. The TPS631013 crossover band (3.25-3.35 V --
exactly where the plateau parks) remains an unexcluded co-suspect. Echoes r8's
lesson from this morning's boost bench: harness/loop resistance is a
first-class design parameter.

Production framing: a deployed lantern runs ESP-NOW + deep-sleep duty cycles,
NOT an AP+STA dashboard -- this exact workload is bench-shaped. The transferable
lessons: (1) battery-loop R budget matters; (2) LFP plateau parks the converter
in its crossover band for most of discharge -- deliberate stability margin
needed; (3) the boot-counter + reset-reason + pre-death-breadcrumb pattern is a
cheap, effective field-diagnostics idiom worth porting to production firmware.

NEXT (the decisive steps): (a) clamp a SEN0291 INA219 into the battery lead
(screw terminals, the r10 method) and catch the collapse waveform at death --
separates loop-R sag from converter misbehavior; (b) optional software rung
first: STA-only + WiFi modem power-save build (kills the beacon TX cadence);
(c) fatten/shorten the battery leads and re-run the ladder. Also fixed tonight:
build.sh was missing --no-l1x (added); dashboard pitch-drag inverted (fixed,
`.22`); flip-H no longer mirrors the 3D orbit direction.

## 2026-07-02 (cont. 5) - Ben + Claude - Reboot mystery: cell is a 4Ah LFP 26650 (sag ruled out); old image was OVERCHARGING it; crossover-band + contact hypotheses live

Ben identified the presence-bench cell: **26650 4 Ah LFP**. That kills the
"drained Li-ion sags" theory (the observed ~257 mA battery draw is 0.06C -- a
healthy 26650 cannot sag from that) and RE-DATES the early 4.19/4.12 V readings:
gauge voltage is chemistry-profile-independent, so those were REAL terminal
volts -- **the old net_bench master image (Li-ion charge profile) was charging
the LFP toward 4.2 V on USB = overcharge**. Cell has been relaxing back to
plateau all session (4.12 -> 3.68 -> 3.29-3.33 V); LFP is forgiving, cell
presumed fine, but this is a fleet-level hygiene flag: image chemistry profile
MUST match the attached cell (POWERFEATHER_NOTES "chemistry flash order" gotcha,
now with a live specimen). presence_bench's LFP profile + 3.65 V ceiling was
already correct.

Reboot status: on battery the board now power-on-resets (rst=1, NOT
panic/watchdog -- software crash ruled out by reset-reason) about every 30-60 s
at bv 3.29-3.33 V, and runs indefinitely on USB. Two hypotheses, in tension:
(1) **intermittent battery contact** (holder/pigtail -- the June H2 ghost);
(2) **TPS631013 buck-boost CROSSOVER-BAND instability under bursty load** --
the cell is parked at exactly the documented 3.25-3.35 V mode-hunting band
while the bench draws radar bursts every 300 ms + WiFi. The June stability runs
were at 3.18-3.24 V (clean boost) and never tested parked-in-crossover with
spiky loads; if (2) is real it is PRODUCTION-RELEVANT (an LFP spends most of
its discharge on that plateau). Discriminating experiment queued: USB-charge to
full (rest ~3.4 V, above the band), run on battery, and watch the new
10-s-resolution pre-death breadcrumb (`.17`: boot log prints "previous run:
up=Xs bv=Y.YV") -- deaths clustering in 3.25-3.35 V and stopping above it
indict the converter; deaths at any voltage / correlated with bumping the rig
indict the contact. Cautious framing: n=1 board, bench wiring quality unverified,
the two causes can coexist.

## 2026-07-02 (cont. 4) - Ben + Claude - First eyeball verdicts + .14/.15 fixes (flip toggle, boot counter, charging)

Ben's first live dashboard session, verbatim verdicts worth keeping: **thermal
"simply beautiful, so much information"; TMF8821 "likely the sweet spot for this
project"**; VL53L1X works but single-zone makes him nervous vs multizone's
robustness to dust/self-occlusion (agreed -- that IS the $3-vs-$10 question the
rig session will quantify); VL53 multizone was mirrored horizontally (fixed:
flip-H/V display toggles, `.14`); XM125 distance-app output hard to correlate
with motion (expected: the DISTANCE app reports all static reflectors -- desk and
wall are permanent peaks; the PRESENCE app is the motion-tuned firmware; also its
peak-strength decode returns a 0xEEEEEE00 sentinel and peaks 2+ read beyond the
configured 5 m window = decode bug, queued). Radar x-y blobs are physically
impossible on the A121 (1 TX / 1 RX, ~+-35 deg cone, range-profile only) -- if 2D
radar matters, add an LD2450 (~$10, tracks 3 targets with real x/y) to the kit
order. UI blank-outs + all Qwiic LEDs blinking = the board IS rebooting
(VSQT-cycle signature). Instrumented + mitigated: NVS boot counter + reset reason
in the status strip (`boot#N rst=R`; 1=power/brownout, 4-6=crash), and gentle
charging (500 mA, 3.65 V LFP ceiling = safe undercharge for any chemistry) now
turns on via a deferred one-shot once the gauge reads a plausible cell (`.15`
verified: "battery 3.32V present -> charging ON"; boot-time reads give a false
0.00 V). If boot# still climbs with rst=1, next suspect is the BQ's USB
input-current-limit negotiation.

## 2026-07-02 (cont. 3) - Ben + Claude - ALL FIVE presence sensors live behind the mux; bench complete

Ben wired the TCA9548A per plan (main chain = MLX + TMF + XM125 + mux; VL53L5CX ->
port 0, TOF400C -> port 1); the armed watcher auto-OTA'd `.13` on reappearance and
the full bench initialized in 11 s: mux detected, **all five sensors ok**, both
0x29 chips coexisting behind their own ports with zero address changes. Data
sanity: MLX 25.9-31.4 C scene; VL53 8 zones with 2 valid targets (near ~135 mm
bench clutter + far ~1.5 m); TMF 9 results; XM distance app 3 peaks; **VL53L1X
1612 mm status-0 sig-1784 -- the ~$3 production candidate is ranging**, and its
distance agrees with the VL53's far targets on the same scene. The presence bench
is feature-complete: 5 sensors side-by-side, wireless dashboard w/ baseline/
occlusion/PRESENT tiles, /api/log, JSONL logger. Next: Ben's dashboard eyeball
pass + first walk-under runs, then the lantern-rig occlusion session (usable-zone
counts = the deliverable). Open: XM peak-strength decode (bogus sentinel; peaks 2+
also look noisy -- distances beyond ~5 m window), XM ~5% read errors, MLX at
~1 fps effective (knob to 8 subpages/s available).

## 2026-07-02 (cont. 2) - Ben + Claude - VL53L5CX address-change is a silicon dead end (reproducible zombie); pivot to TCA9548A mux

Ben soldered the XSHUT header (continuity-verified; the on-board diag flipped to
"gated=F002 released=... jumper works" -- the earlier NO CHANGE verdict was the
unsoldered pin, as suspected). That unblocked the relocation dance and exposed the
real boss fight: **changing the VL53L5CX's I2C address bricks it until power
cycle**, reproducibly, in BOTH orders tried: (a) SparkFun `setAddress()` after full
init, and (b) raw ST-equivalent register writes (page 0x7fff=0, reg 0x4=0x2A) at
POR before any init. In every case the chip MOVES (bus scan ACKs at 0x2A, 0x29
empty) but answers 0000 to all register reads and times out every DCI op --
per-step logging isolated it to CANNOT_SET_RESOLUTION with a healthy F002 id read
seconds earlier at 0x29. Matches multiple ST community reports ("address changes
but comms fail until power cycle"); ST's official multi-VL53L5CX recipe uses the
LPn pin, which the SparkFun breakout does not wire to anything controllable.
Conclusion: software relocation ABANDONED (left in the sketch behind
PB_VL53_RELOCATE=0).

**Resolution: Ben's SparkFun TCA9548A Qwiic mux.** Wiring: main chain keeps MLX +
TMF + XM125 + mux (0x70); VL53L5CX -> mux port 0, TOF400C/VL53L1X -> mux port 1
(BOTH 0x29 residents must be behind ports -- one open channel at a time).
Firmware `.13` (built, awaiting the rewired board): mux auto-detect at boot,
select-before-use in the two ToF paths, no address games at all; without the mux
it falls back to `.12` behavior (L5CX direct at 0x29, L1X XSHUT-gated). `.12`
verified before teardown: all four original sensors ok incl. VL53 multi-target.

Debug-tooling lessons this arc (all now in the sketch): identity probes (device-id
reads) beat bare ACK probes -- arduino-esp32 3.x phantom-ACKs zero-length write
probes and its `endTransmission(false)` is deferred, so bare-read probes can also
mislead; ticks must never run on ERROR state (an un-begun driver "recovered" by
reading garbage from the WRONG chip at a shared address); reset_reason in the boot
log line; per-step init logging.

## 2026-07-02 (cont.) - Ben + Claude - TOF400C/VL53L1X added as 5th sensor; XSHUT jumper diagnosed NOT conducting (hardware fix pending)

Ben wired the TOF400C's XSHUT to A0/GPIO10 and chained it onto the Qwiic bus.
Firmware `.6/.7` adds the full 5th-sensor integration: XSHUT gated LOW from the
first lines of setup, VL53L5CX relocates itself 0x29 -> 0x2A after init, then the
L1X is released to own 0x29 (`reinit=vl53` re-runs the whole dance); 5th dashboard
tile + panel (distance sparkline w/ baseline line), SparkFun VL53L1X lib (gotcha:
`begin()` returns 0 on SUCCESS).

**Hardware verdict from the new on-board diagnostic:** `id@0x29 gated=0000
released=0000 -> NO CHANGE: jumper likely NOT conducting`. The raw device-id bytes
at 0x29 are identical with XSHUT driven low vs high, and read 0000 (wired-AND of
two chips fighting the bus) instead of the L5CX's F0/02 -- the L1X is squatting on
0x29 regardless of the gate. Until the wire is fixed (check: continuity GPIO10 ->
module XSHUT pad; check the pad isn't strapped to VCC on the module -- some
TOF400C clones tie it high), BOTH ToFs are down (contention); MLX/TMF/XM run fine.
After a wire fix the bench self-recovers via reprobe+backoff (worst ~2 min) or
instantly via the dashboard "Reinit all".

Also fixed this round (found via the fake-recovery it caused): sensor ticks were
allowed to run in ERROR state, so the never-initialized L1X object "read" garbage
from whatever sat at 0x29 and self-promoted to ok. Ticks now run only in OK;
ERROR recovers exclusively through reprobe -> full re-init. First-soak data (pre-
L1X): MLX/TMF/VL53 0 errors over 5 min at 400 kHz; XM ~5% read-error rate
(intermittent, seq still advances -- open item); VL53 froze when the un-gated
TOF400C was hot-plugged mid-soak (the collision, live) -- now covered by both the
gating and a mid-session stall self-heal.

The presence-sensing research track (Elliot ask, 2026-06-12 note) got its bench.
Ben's 4 SparkFun Qwiic modules (MLX90640 thermal 32x24, VL53L5CX 8x8 ToF imager,
TMF8821 multizone ToF, XM125 radar -- co-facing, one Qwiic chain) now stream to a
wireless dashboard from the repurposed spare PowerFeather V2 (was an old net_bench
master, id 9F2690). New: `firmware/presence_bench/` (single .ino, led_studio-derived
WiFi/mDNS/OTA + PROGMEM dashboard), `ops/bench/presence_logger.py` (JSONL to
`data/presence/`), README with API + bring-up notes.

**Working end-to-end (verified `.5`):** all four sensors init in <9 s from sensor
POR; i2c scan shows exactly the six expected devices (4 sensors + gauge 0x36 +
charger 0x6A); MLX streams clean thermal (25-36 C desk scene); XM125 probe
identified Ben's module as running the Acconeer DISTANCE app (not presence) --
its multi-peak list is genuinely useful for the splay/floor/person separation;
TMF8821 reports 2 objects/zone off the desk; **VL53L5CX multi-target WORKS: the
vendored driver (2 targets/zone) returned 10 zones with two valid returns each
(e.g. zone 0: T0 115 mm + T1 269 mm, both status-5)** -- the instrument the
splay-self-occlusion question needs. Dashboard: 4 live panels (thermal heatmap,
tap-a-zone ToF grids, radar depth strip), browser-side baseline capture / delta
view / occlusion masking / PRESENT tiles / event log; detection logic deliberately
lives in JS so thresholds tune live and rules re-run offline against logged frames.

**Architecture notes:** all I2C (sensors + battery round-robin) in ONE task on core
0, HTTP serves caches on core 1 -- proved its worth when a wedged sensor task left
OTA fully functional. Shared SDK Wire1 retuned to 400 kHz (deliberate, documented
exception to POWERFEATHER_NOTES' 100 kHz; BQ25628E + MAX17260 are 400 kHz parts) --
the VL53 blob uploads in 2.7 s and nothing errored in the first soak. Sensor rail
(VSQT) is POWER-CYCLED at boot: it stays up across ESP reboots and stale sensor
state from a previous image cost an hour of debugging (below). `/api/log` ring
buffer added because native-USB serial kept eating boot banners -- wireless
debugging paid for itself immediately.

**Bugs found + fixed (the debugging story):**
1. **Two infinite loops in ST's VL53L5CX ULD** (vendored into the sketch precisely
   so we could patch -- see `src/vl53l5cx/VENDORED.md`): `_vl53l5cx_poll_for_answer`
   never exits if the device goes mute (timeout only ORs an error and keeps
   spinning), and `vl53l5cx_stop_ranging` sets ERROR on its 5 s timeout but never
   breaks -- stop-on-a-non-ranging-device spins forever. The second one wedged the
   whole sensor task on first boot (`begin()` fine in 2.7 s, then the sketch's
   stop-before-start config path hung). Both patched with breaks; sketch also
   tracks ranging state and never stops a non-ranging device.
2. **Stale sensor state across reboots**: VSQT stays powered through ESP
   soft-resets, so the VL53 carried a half-stopped state through reflashes and
   "ranged" silently (init ok, zero frames). Fix: VSQT off/on at boot + a
   self-heal reinit if a sensor is ok-but-silent for 8 s.
3. **arduino-esp32 3.x zero-length-write probes phantom-ACK** half the address
   space (scan returned ~40 bogus devices on a healthy bus). Probe by 1-byte read
   instead -- scan now exact.
4. Failed sensor inits now back off exponentially (a failing VL53 init costs
   MINUTES of blocked bus because the ULD doesn't early-exit between steps --
   without backoff it starved the other three sensors).

**Open / next:** browser dashboard needs Ben's eyeball pass (walk-under test +
baseline capture); 5-min soak at 400 kHz running at session end (err counters
clean so far); XM125 peak-STRENGTH decode returns a bogus sentinel (-286331392)
-- distance values are fine, strength only sizes dots, investigate later; MLX
effective ~1 fps at the default 4-subpages/s knob (8 available); the spare
board's battery telemetry is inconsistent (bv 4.12 then 3.68, ma -290 then 0;
gauge was configured LFP by this sketch, cell chemistry unverified, charging
deliberately OFF) -- Ben should confirm what cell is physically attached. Ben also
has a TOF400C/VL53L1X (the original $3 production candidate): it collides with
the VL53L5CX at 0x29 (the "0x52" in its docs is the 8-bit notation, not a radar
conflict) -- add via XSHUT-gate on GPIO10 + relocate one of them, ~a session.

Full narrative report of today's boost A/B campaign, written for future-us and for
Steve (plain language, no session shorthand). Every claim carries an evidence grade
(REPLICATED / MEASURED ONCE / STRONG EVIDENCE / HYPOTHESIS / OPEN) -- the session's
own corrected-mid-stream claims (the "flaky STEMMA cable", r4's "+27% from the
contact fix") are used as the worked examples of why the grading matters. Includes
five figures (HEX A/B bars, RGBW brightness ladders, the full-power topology matrix,
efficacy with measurement-plane caveats, and the raw partial-brightness
current-instability trace), regenerable via ops/bench/report_figs_boost_ab.py.
One in-session claim is explicitly downgraded in the report: "W-die 3x efficacy vs
RGB-white" was computed from unstable current readings; the defensible number is
~1.4x (bare, full brightness), and boosted RGB-white efficacy was never cleanly
measured. Open-questions table mirrors the TODO items.

## 2026-07-02 - Ben + Claude - r9 completes the matrix: boosted-VBAT-fat hits 3044 lux with NO wall; both predictions land

Final cell: TPS63802 4.2 V boost fed straight from VBAT on the larger-gauge wiring
(no INAs; lux + gauge bv). Both ladders linear end to end, no aborts:

  wonly:    129 / 259 / 513 / 766 / 1016   (prediction was ~1040-1060: hit, -3 %)
  rgbwhite: 396 / 785 / 1554 / 2305 / 3044 (prediction ~2900-3000: hit)
  Cell sag at rgbwhite-255 (~1.3-1.4 A draw): bv 3.299 -> 3.203, ~100 mV. Comfortable.

THE COMPLETED MATRIX (usual aim, bri=255, gamma 0, ~SOC 63-75 LFP):

  config                     W-only (clean white)   RGB-white (fringed)
  bare, rail-fed                    470                1310  (no wall)
  bare, VBAT + fat wire             448                1746  (no wall)
  boosted, rail-fed                1044                wall at bri=128 (rail limit)
  boosted, VBAT + thin harness    ~1060 aim-corr       wall at bri=128 (harness R)
  boosted, VBAT + fat wire         1016                3044  (NO WALL)

Campaign conclusions (RGBW 4 W point source):
- **The "wall" was never the architecture.** Rail regulator first, instrumented-
  harness resistance second; with VBAT + proper wire the module delivers its full
  ~4 W: 3044 lux, 1.74x the bare-VBAT rgbwhite and ~2.3x anything rail-fed.
- **Boost value, final form (VBAT-fed, good wiring): clean white 448 -> 1016 lux
  (2.3x); max fringed white 1746 -> 3044 (1.7x).** Efficacy tax ~25-30 % (battery
  plane, from the r7 accounting) -- boost converts efficiency into output ceiling,
  consistently, in every topology tested today.
- **Production topology, if the RGBW ships with or without boost: LED power from
  VBAT (downstream of the gauge shunt!), fat conductors, ESP rail untouched.** The
  rail-fed path gives up 33 % of bare rgbwhite and walls any boost; VBAT-direct is
  simpler AND better. EN->GPIO for the kill; connector quality is worth 25 % of
  top-end light (the day's thrice-learned lesson).
- Bare remains Ben's production GO (bare-VBAT rgbwhite 1746 lux is plenty per the
  eye test); the boost option file is complete and shelved with real numbers at
  every operating point it could be revived for.

## 2026-07-02 - Ben + Claude - Audit: gamma bug invalidates NOTHING (verified per-file); r8 blindness re-attributed to an I2C bus wedge + board reboot

Ben challenged two claims in the r8 entry; both corrections below are evidence-based.

**Gamma audit (Ben's worry: "huge repercussions -- how much does this invalidate?"):
answer NOTHING, verified against every file, not from memory.** All 31 capture files
log /state rows; scanning every row: gamma=0 in ALL runs through r7 and in r8c;
gamma=1 ONLY in r8 (optically blind anyway; its salvaged claim -- full rgbwhite at
~40 mV cell sag -- rests on the bri=255 step, where gamma8(255)=255 is identity) and
r8b (flagged at capture; its bri=255 points match r8c within 0.5 %). Every HEX suite,
every RGBW ramp r1-r7, and every verdict built on them: gamma=0 throughout, zero
impact. Render-path check: setRGBWpix applies gamma8 AFTER brightness scaling, so
gamma distorts sub-255 bri steps as ~bri^2.2 and is exactly identity at bri=255 --
matching the observed r8b curve.

**Gamma mechanism correction: not "left on from eye-testing" -- it is the BOOT
DEFAULT.** `gGamma = true` in led_studio; the PowerFeather rebooted during the
rewiring window (fingerprint in the state rows: r7 shows the all-day session state
lit=12/speed=38, r8 shows boot defaults lit=18/speed=30 -- battery feed interrupted
while working at the VBAT header). The ramp's mode-set never touched gamma, so the
default survived into r8/r8b. Ramp tool already pins gamma=0 now; the boot default
itself is a bench trap for any future /set-driven capture that assumes session state.

**r8 blindness correction: Ben is right that the STEMMA cable was likely never
flaky -- but it was not a port jump either.** Evidence: the post-r8 probe of
/dev/ttyACM2 returned live ina_monitor output with a fresh 5-hour-uptime timestamp,
so the ramp HAD been reading the correct device (the port jump to ttyACM1 happened
later, at the reseat, when the KB2040 re-enumerated and its uptime reset to ~3.5
min). During r8 the monitor was emitting almost nothing (~1 line per 2 s where ~20
expected) with 0x41 stuck present-but-ERR and the still-attached VEML undetected.
Best-fit mechanism: **wedged I2C bus** -- the INA harness was unplugged mid-session
from an actively polling monitor (classic SDA-held-low), stalling every transaction
into timeouts: slow loop, unreachable VEML, ERR spam all explained. The fix was the
KB2040 REBOOT during the reseat handling (Wire re-init cleared the wedge); the cable
reseat itself was probably incidental. Hedge: a marginal contact cannot be fully
excluded, but the sparse-output signature favors the wedge.

Hardening TODO queued: ina_monitor should clear a channel's present flag after N
consecutive ERRs and attempt I2C bus recovery (9 SCL pulses + Wire re-init) when the
whole bus errors, so a mid-session unplug cannot blind the monitor until a reboot.

## 2026-07-02 - Ben + Claude - r8 bare-VBAT fat-wire: the wall was bench wiring, and VBAT-direct beats the rail by +33% on RGB-white

Production-similar test per Ben: RGBW V+ direct from the VBAT header pin, larger-gauge
JST-XH, GND via the split cable, NO boost, NO INA instrumentation (the dupont-wired
INA harness was the suspect). Instrumentation = VEML lux + gauge bv only; the ramp
tool gained a gauge-bv abort floor for uninstrumented runs. Protocol notes: the first
run (r8) was optically blind -- the STEMMA cable to the VEML had loosened during
rewiring (reseat fixed it; KB2040 re-enumerated to ttyACM1); r8b then produced a
superlinear ladder because the UI's GAMMA toggle had been left on from eye-testing
(identity at bri=255, so full-brightness points remain valid; ramp tool now pins
gamma=0). r8c is the clean run. r8b/r8c bri=255 agreement: 0.5 %.

r8c (bare, VBAT-direct, fat wire, gamma 0, "usual" aim per the W anchor):
  wonly lux:    59 / 116 / 226 / 338 / 448   (linear; ==rail-fed 470 within mount noise)
  rgbwhite lux: 225 / 444 / 882 / 1317 / 1746 (linear; NO WALL, all steps completed;
                cell terminal sag ~40 mV at full per the r8 gauge rows)

Findings:
- **The rgbwhite collapse was the bench wiring.** On fat wire from VBAT, full
  RGB-white runs clean -- no abort, no sag worth naming. The r7 "wall" was ~0.3 ohm
  of instrumented-harness loop resistance, confirmed by its absence here.
- **VBAT-direct beats the rail path by +33 % on RGB-white** (1746 vs 1310 lux at the
  same aim): under load the 3V3 rail delivered ~2.97-3.1 V at the die while VBAT +
  fat wire holds ~3.25-3.3 V -- the starved green/blue dies convert every extra
  100 mV into light. W-only is unchanged (448 vs 470: the W die is equally starved
  either way). 1746 lux is the BRIGHTEST white of every configuration tested today,
  boosted ones included -- fringed/warm, but free.
- This quietly revalidates the old ADR 0008 topology (LED direct from VBAT) for the
  RGBW: simpler, brighter at full, and it inherits r7's proven ESP decoupling.
- **Production caveat (important): tapping VBAT at the header BYPASSES the fuel
  gauge's current shunt** (r7/r8 finding: gauge ma blind to the whole LED branch).
  A production VBAT-fed LED rail must tap downstream of the gauge sense resistor
  (trivial on a custom PCBA; needs schematic check on the PowerFeather COTS path)
  or SOC/coulomb telemetry undercounts the dominant load.

Matrix now (usual aim, bri=255): bare-rail W 470 / rgbw 1310; bare-VBAT W 448 /
rgbw 1746 no-wall; boosted-rail W 1044 / rgbw walls at 128; boosted-VBAT-thin W
~1060 aim-corr / walls at 128 (harness). Remaining cell: boosted-VBAT on fat wire
(r9) when Ben re-adds the boost -- prediction: W ~1040-1060, rgbwhite runs past the
old wall and lands ~2900-3000 if the ladder stays linear (~10.2 lux/bri at usual aim
x3.98), cell sag permitting.

## 2026-07-02 - Ben + Claude - r7 VBAT-fed boost: ~11% battery-side saving, ESP fully decoupled, wall becomes a wiring problem

Ben rewired the boost input to VBAT (VBAT header pin + GND borrowed via 2-pin JST-XH
split from the free VDC/solar port; 0x41 INA moved into the VBAT->boost branch).
Board topology surprise: the VBAT tap bypasses the 0x45 shunt, so 0x45 now reads the
BOARD's own draw only -- a dead-constant 116-118 mA at every step (cleanest ESP+radio
overhead number of the day); total system = 0x41 + 0x45.

r7 (VBAT-fed), W ladder lux: 169/335/665/992/1347; W-full stable: 225 mA @ 3.212 V =
0.723 W -> 1346.5 lux. rgbwhite: 380 @32, 759 @64; HARD ABORT at 128 (branch node
2.588 V). Board never blinked.

Predictions graded:
- **Aim moved again**: whole W ladder is a UNIFORM 1.26-1.29x vs r6 (clean geometry
  factor this time, unlike r4's kink) -- the rewiring session re-seated the module at
  the "favorable" aim, same ~+27 % magnitude as the r4 outlier; the rig plausibly has
  two quasi-stable seatings. Aim-corrected die output == r6 (~1060 vs 1044 lux):
  same 4.2 V at the die either way, as physics requires.
- **Efficiency: PASS after fixing my reference-plane sloppiness.** The 0.62-0.65 W
  prediction wrongly treated r6's 0.731 W (measured on the 3V3 RAIL, already
  once-converted, ~0.81 W at the battery) as battery-plane. Honest battery-plane
  comparison at matched die output: two-stage ~0.81 W -> single-stage 0.723 W =
  **~11 % saving**, consistent with deleting a ~90 %-efficient stage. Aim-corrected
  efficacy tax vs bare (also converted to battery plane, ~0.23 W): **~37 % -> ~28 %**.
- **"No rgbwhite wall": FAIL on this harness, but the mechanism changed.** The wall
  is no longer the 3V3 rail regulator -- it is ~0.3 ohm of harness loop resistance
  (measured: 72 mV sag at 225 mA on the W-full step; loop = VBAT pin -> dupont ->
  module -> LED -> borrowed-GND JST-XH split -> VDC port) plus cell/protection sag,
  collapsing the branch node at ~1 A demand. On a production VBAT feed (PCB traces,
  proper connectors) this wall is a wiring spec, not an architecture limit.
- **ESP decoupling: PROVEN.** Board draw stayed at 116-118 mA through every step
  INCLUDING the branch collapse; /state clean after, no reset. In this topology LED
  transients structurally cannot brown out the controller -- the radio-burst-during-
  LED-load concern is dead where it matters.

Boost option file (still shelved -- bare remains the GO): if the field test at height
ever demands the 2.2x clean white, the production shape is VBAT-fed single conversion
on the adapter PCB: ~28 % efficacy tax, ESP immune to LED transients, EN->GPIO +
pull-down for the software kill, and connector/trace quality worth ~25 % of top-end
light (today's recurring lesson, three different ways).

## 2026-07-02 - Ben + Claude - r6 GOLD STANDARD: boost verdict settles at 2.2x clean white / ~37% efficacy tax; r4's +27% was aim, not electronics

Root cause found by Ben while simplifying the boost wiring: **two blown-out female
duponts**. The RGBW's 3-pin JST cable-to-cable pins are oversized for female duponts
-- forcing them in splays the socket, and it then makes a poor friction fit on normal
header pins (the boost PCB). That is the physical mechanism behind the flaky boost
path. Rewired simply with fresh duponts, all snug; r6 run as the gold standard.

r6 boosted, default ladder:
  wonly lux: 135 / 265 / 525 / 786 / 1044   (r1: 133/263/521/777/1033 -- MATCHES r1)
  wonly @255 stable: 229 mA / 0.731 W -> 1044 lux = 1428 lux/W
  rgbwhite: 323 @32, 641 @64; HARD ABORT at 128 (2.516 V) -- wall replicated 3rd time

Interpretation (corrects the r4 entry):
- **Electrical fix confirmed and quantified**: r6 matches r4's current draw (229 vs
  227 mA at W-full) vs r1's 243 -- the bad contact wasted ~6 % input power. That is
  the WHOLE electrical story.
- **r4's +27 % light was an aim outlier**, not the contact fix: r6 has r4's
  electrical numbers with r1's optical numbers. The r4 entry's "2.7x / 20 % tax"
  claim is RETRACTED; the kinked r4 ladder (low-bri matching r1, high-bri +27 %)
  remains unexplained -- fixed geometry cannot be bri-dependent; a thermal-mechanical
  tilt of that particular mount under high drive is the surviving speculation. Logged
  as a mystery, not a finding.
- Mount-to-mount aim statistics across the day: r1/r2/r5/r6 all land within ~1-3 %
  of each other (the taped outline works); r4 was a single +26 % outlier; RGB-die aim
  separately moved -11 % once (r5). Absolute lux carries this seating uncertainty;
  within-mount ratios do not.

**GOLD STANDARD RGBW boost verdict** (r6 boosted vs r5 bare, usual seating):
  bare W-full 470 lux @ 0.208 W (2260 lux/W); boosted W-full 1044 lux @ 0.731 W
  (1428 lux/W) -> **boost = 2.2x the clean white at ~37 % efficacy tax**; boosted
  rgbwhite rail-walls at bri=128 every time; bare rgbwhite-full ~1310 lux is the
  free bright-white option (color fringe/tint tradeoff). The original r1-era numbers
  were right all along -- the day's detours bought their confirmation plus the
  dupont root cause. Bench rule going forward: NEVER mate JST cable-to-cable pins
  into female duponts; use proper JST pigtails or crimp housings.

## 2026-07-02 - Ben + Claude - Repeatability due diligence (bare r5): electrical perfect, W-die aim perfect, RGB dies -11% at identical current

Ben (rightly) disliked that a reseat moved numbers 27 %, so: bare remount, exact
ladder, compare to bare r2. Result splits into three clean findings:

1. **Bare electrical path: perfectly repeatable.** Stable-step currents identical
   across the remount (W-full 64 vs 64 mA, rgbwhite-full 263 vs 264 mA; power within
   1 %). The bare config has no reseat sensitivity.
2. **W-die optics: perfectly repeatable.** Whole W lux ladder identical to 0.1 %
   (470.5 vs 470.0 at full). The hot-swap procedure holds the W die's aim through the
   tube essentially exactly. This retroactively CLEANS UP the r1-vs-r4 boost analysis:
   W-die geometry is proven stable across mounts, so r4's kinked ladder (+5 % low,
   +27 % high, current DOWN 7 %) is pure electrical -- the lossy-contact story
   survives due diligence, and the boost-path connection is confirmed as the sole
   large variance source in the whole rig.
3. **RGB dies: -11 % lux at IDENTICAL current, uniform across the ladder** (0.89x at
   every step, r5 vs r2). Same drive, same watts, less light, W unmoved. Two candidate
   explanations, unresolved: (a) per-die aim shift -- the 4 dies sit mm apart in the
   package, and a small module rotation about the W-die axis changes the RGB dies'
   throw through the tube (the same die-offset physics as Ben's color-fringing
   observation); (b) RGB die degradation from the collapse/abort events (r1/r3/r4
   pushed high current through the RGB dies; uniform -11 % across all three from
   brief events seems less likely, but not excluded). DISCRIMINATING TEST when
   curious: nudge/rotate the module slightly and re-check rgbwhite lux at one step --
   recovery = aim; no recovery = degradation. (No RGBW per-channel singles baseline
   exists from the r2 era -- the ramps only ran wonly + rgbwhite -- so the singles
   looks in rgbw_boost_ramp.py can only characterize the current state, not compare
   backward. If degradation is suspected, capture singles NOW as the go-forward
   baseline.)

Current-seating headline numbers (r4 boosted + r5 bare, adjacent mounts):
bare W-full 470 lux @ 0.21 W; boosted W-full 1315 lux @ 0.73 W (2.8x, ~20 % efficacy
tax); bare rgbwhite-full 1310 lux @ 0.84 W (was 1475 at r2 aim -- absolute rgbwhite
lux carries the per-die aim factor, ratios within a mount do not).

## 2026-07-02 - Ben + Claude - r4 after reseat: r1 was the sick mount; healthy boost = 1315 lux clean white at only ~20% efficacy tax

Ben questioned whether the INA had settled; the within-step drift analysis that
followed found something better: partial-brightness current medians are UNRELIABLE in
ALL runs (bare included -- so not the boost module), stable only at bri=255. Mechanism:
the 3V3 rail regulator (TPS631013) burst-modes at light-mid loads and the INA219's
68 ms averaging window aliases the bursts into slow apparent wander; at full drive the
regulator runs continuous PWM and readings are rock-stable (sd ~2 mA). Lux (VEML,
100 ms integration) stays clean throughout -- trust lux everywhere, trust mA only on
stable steps. rgbw_boost_ramp.py now flags unstable steps automatically.

r4 = exact r1 replication after Ben reseated the boost connections:

  wonly lux ladder:  139 / 275 / 544 / 982 / 1315   (r1: 133 / 263 / 521 / 777 / 1033)
  wonly @255 (stable): 227 mA / 0.726 W in -> 1315 lux = 1811 lux/W
  rgbwhite: 359 @32; 716 @64 (branch 2.846 V, at the soft-floor edge);
            HARD ABORT at 128 (2.52 V) -- r1's wall REPLICATED (r3's cliff-at-72
            does not reproduce; that was the bad contact).

Decomposition: low-bri steps match r1 within 4-5 % (at light drive even a lossy
contact reaches die regulation -> bounds the optical/geometry shift at ~+5 %); high-bri
steps are +26-27 % with 7 % LESS input power. Conclusion: **r1's boost path had
contact/series resistance from the start** -- it starved the die at high drive and
burned input power in the connection. The reseat fixed it. All r1/r3 boosted absolute
numbers were depressed at high drive; bare runs unaffected (no boost path).

Updated headline (healthy mount, geometry-matched approximately):
  bare W-full ~470-490 lux @ 0.21 W (~2250-2350 lux/W)
  boosted W-full 1315 lux @ 0.73 W (~1810 lux/W)
  -> the boost buys ~2.7x the clean white at only ~20 % efficacy tax (was stated as
  2.2x / 40 % tax off the sick mount). The RGBW boost case is STRONGER than reported.
Caveats: geometry shifted ~+5 % across the re-matings, so cross-era absolute lux
carries that error; a fresh bare run at current geometry would clean up the pair.
Reliability lesson for production: the boost path connection quality is worth ~25 %
of top-end light -- connectorization/solder, not hand-wired jumpers.

## 2026-07-02 - Ben + Claude - Boost re-mount r3: electrical operating point shifted; rgbwhite cliff moved; curiosity answered

Boost re-mounted after the bare r2 run (Ben flagged a possible LED bump). The r3
alignment ladder says the change is NOT (mainly) optical seating -- the boosted
electrical operating point itself moved:

- W-only @ full: r1 243 mA / 0.776 W / 1033 lux vs r3 199 mA / 0.639 W / 968 lux.
  Current -18 % at the same commanded look; lux only -6 %; lux-per-mA UP 14 %
  (consistent with lower current density, not geometry). A pure bump cannot change
  die current.
- rgbwhite fine ladder (curiosity run): @32 120 mA / 318 lux and @64 227 mA / 633 lux
  -- HALF of r1's branch current at 64 (481 mA) for nearly the same lux -- then
  HARD ABORT at bri=72 (branch bus 2.58 V). The wall moved from somewhere in
  (64, 128] down to (64, 72], at half the current of r1's healthy 64-step.
  Nonmonotonic collapse at lower current = smells like boost-module input
  instability (IR-dip -> UVLO/foldback oscillation), not a simple rail ceiling.

Prime suspect: contact/series resistance in the re-mated boost path (input or output
connector), possibly a nudged wire/jumper; module damage from the r1 collapse event
not excluded. ACTION: reseat/meter the boost module connections (output should read
4.2 V unloaded), then re-run `--config boosted --runtag r4-align --looks wonly` and
compare to r1. Treat ALL r3 numbers as suspect for A/B purposes. Also a production
data point: a hand-wired boost path showed ~18 % current shift across one re-mating
-- connectorization/soldering quality matters if a boost ever ships.

Curiosity question (can boosted rgbwhite beat bare's 1475 lux before collapse?):
answered NO even for the healthy r1 mount by slope -- ~10.2 lux/bri projects ~1300
lux at bri=128, which is already past the wall. Bare rgbwhite wins on raw output
because the starved dies self-limit: full duty cycle, no conversion loss, no wall.
Boosted W-only (~1033 lux healthy) remains the brightest CLEAN white.

## 2026-07-02 - Ben + Claude - RGBW bare ramp r2: the boost EARNS ITS KEEP on the W die (+120% light)

Bare RGBW ladder, same harness position, same aborts (none triggered -- bare never
approaches the rail wall):

  look      bri  led_V  led_mA  led_W  batt_mA   lux
  wonly      32  3.280    11    0.036    131      60
  wonly      64  3.276    18    0.059    138     119
  wonly     128  3.268    27    0.088    153     236
  wonly     192  3.264    46    0.150    181     353
  wonly     255  3.260    64    0.209    186     470
  rgbwhite   32  3.264    39    0.127    161     185
  rgbwhite   64  3.244    70    0.227    198     369
  rgbwhite  128  3.232   127    0.410    264     737
  rgbwhite  192  3.200   192    0.614    330    1106
  rgbwhite  255  3.188   264    0.842    397    1475

Bare vs boosted, same looks (the opposite of the HEX story):
- **W-only @ full: bare 470 lux / 64 mA vs boosted 1033 lux / 243 mA = +120 % light.**
  The W die is severely current-starved at the ~3.26 V rail at EVERY brightness (it
  passes ~26 % of its boosted current; its Vf stack is the tallest on the module).
  Both ladders are linear in bri -- the starvation is a constant current ceiling, not
  compression at the top.
- rgbwhite @ 64: bare 369 vs boosted 654 lux (+77 %). Boosted rgbwhite is rail-limited
  to bri<=64-96; bare rgbwhite runs clean to FULL (264 mA, 3.19 V, no wall) because
  the starved green/blue dies cap their own draw.
- Bare rgbwhite @ full = 1475 lux = the brightest white measured through the tube --
  but presumably golden/warm (green/blue starved; VEML cannot see chromaticity).
  EYE CHECK WANTED: bare rgbwhite-255 color vs boosted wonly-255.
- Efficiency: bare W 2249 lux/W vs boosted W 1331 lux/W -- the boost still costs ~40 %
  lumens/W (same tax as HEX). The boost does not create efficiency; it buys OUTPUT
  CEILING: max clean white 470 lux bare -> 1033 boosted (2.2x).

Verdict shape for the RGBW (differs from HEX): criterion B is YES -- +120 % is
unmistakable to the eye; criterion C is still NO (~40 % efficacy tax). So the boost
decision reduces to an optics/artistic question: does the gobo role need more white
than the bare W die's ceiling? If bare W-full is bright enough at height, skip the
boost (best lumens/W on the fixture); if not, the boost is the only clean way to 2x
(bare rgbwhite is brighter still but color-compromised). Field test at projection
distance decides. Caveats: n=1 module/position; absolute lux is rig-specific;
chromaticity unmeasured.

## 2026-07-02 - Ben + Claude - RGBW boosted ramp r1: W-die is the efficiency star; RGB-white hits the rail wall

RGBW 4 W point source (led_studio mode=1, single SK6812 RGBW px on GPIO10) hot-swapped
into the tube harness, boosted (TPS63802 4.2 V fed from the 3V3 header). New
`ops/bench/rgbw_boost_ramp.py`: stepped brightness ladder per look with live
rail-droop aborts (hard floor 2.60 V on the LED branch, soft 2.80 V, /state
reachability check), one JSONL per run, LEDs blanked on any exit.

Boosted r1 (W-only then RGB-white, ladder 32/64/128/192/255):

  look      bri  led_V  led_mA  led_W  batt_mA   lux
  wonly      32  3.264    39    0.127    163     133
  wonly      64  3.248    67    0.218    178     263
  wonly     128  3.224   103    0.332    247     521
  wonly     192  3.208   194    0.622    302     777
  wonly     255  3.192   243    0.776    373    1033
  rgbwhite   32  3.176   174    0.553    346     339
  rgbwhite   64  3.104   481    1.493    684     654
  rgbwhite  128  HARD ABORT: branch bus hit 2.452 V mid-step; LEDs blanked; board
                 survived (no reset -- /state kept the script's values)

Findings (boosted config, n=1):
- **W-only is rail-safe to full**: 0.776 W input at bri=255, branch droop only
  3.26 -> 3.19 V. Lux tracks bri linearly (no compression) -- the die holds constant
  current across the ladder.
- **RGB-white hits the wall between bri 64 and 128**: 481 mA input at 64 was fine
  (3.10 V); the 128 step collapsed the branch to 2.45 V. Usable boosted RGB-white
  domain ~bri<=64-96. Rail ceiling between ~0.5 and ~1 A demand, consistent with
  Ben's ~1 A rating.
- **W-die efficacy crushes RGB-mixed white**: 1033 lux / 0.776 W = ~1330 lux/W vs
  654 / 1.49 = ~440 lux/W at rgbwhite-64. Phosphor white beats RGB mixing 3x for
  white gobo throw -- W-only should be the default white look on RGBW fixtures.
- Cross-module ballpark (same rig, different emitter geometry -- caveat): W-only
  full = 1033 lux vs HEX center-px white full = 216 lux, ~5x.

Next: swap to bare RGBW, same ladder (`--config bare --runtag r2`). The real boost
question this time is the W ladder: the W die's Vf stack may genuinely starve at the
bare ~3.2 V rail -- if bare W-only compresses/plateaus where boosted stayed linear,
the boost earns its keep on the RGBW; if not, same verdict as HEX.

## 2026-07-02 - Ben + Claude - June discharge data settles it: the pixel really saw ~2.97 V at show loads

Ben recalled the June harness metered BOTH nodes -- confirmed, the "conflation"
hypothesis from the previous entry is dead. `2026-06-10-discharge-1357.jsonl` logs
`batt_ina_bus_v` (0x45) AND `led_bus_v` (0x41, post-regulator at the LED), same
channel convention as today's boost harness. Binned medians at full-RGBW bri=255:

  batt_ina_v  led_bus_v  led_ma  batt_ma
     3.00       2.964      292     456
     2.95       2.968      292     459     <- most of the plateau lived here
     2.90       2.974      291     463
     2.85       2.859      291     468     <- LED V+ converges to VBAT: boost out of
     2.80       2.807      291     479        headroom at this load ("battery dying")

So the June "goldening at 2.8-2.95 V LED rail" was DIRECTLY MEASURED at the pixel,
provenance solid. The regulated rail delivers ~3.2 V at 42-122 mA (today's data) but
only ~2.97 V at ~290 mA show load (droop + harness IR, split not separable across the
two harnesses). Ben's slow-decline recollection is in the data too: led_ma holds ~291
constant while led_bus_v drifts down -- constant current, slowly starving voltage =
slow dimming, no cliff until VBAT < ~2.9 where LED V+ tracks the battery down.

Consistency check with today's boost verdict: intact and enriched. Single-px gobo
look (~42 mA) sits at ~3.2 V un-starved -> boost buys nothing (measured +1.6 %).
Show-class loads sit at ~2.97 V mildly starved -> boost buys a little (+6.9 % at
ring1) and would buy more at full show loads -- but the gobo role never goes there
(Ben). Ben also recalls evidence that a RADIO BURST while LEDs hog the ~1 A rail can
brown out -- consistent with the rail being the shared choke point; relevant to the
RGBW step-0 rail characterization, where the June curve already gives an anchor
(~2.97 V at ~290 mA).

Doc hygiene: "off the I2C bus" ambiguity fixed in the living docs (AGENTS.md,
POWERFEATHER_NOTES.md -- exact wording: data on a free GPIO, V+ from the regulated
3V3 header, NOT on the I2C bus) and a clarification APPENDED to ADR 0018 rather than
editing it -- the append-only rule holds, ambiguity fixed by addendum.

## 2026-07-02 - Ben + Claude - Correction: hex V+ is the regulated 3V3 rail, not VBAT; verdict rationale updated

Ben corrected a topology assumption threaded through the verdict entry below: the hex
V+ is fed from the PowerFeather's regulated 3.3 V buck-boost rail (switchable header),
NOT from VBAT. LFP at 3.1-3.2 V terminal just means the TPS631013 runs in boost mode.
What the pixel sees is the regulator setpoint minus harness/switch IR: bare runs read
~3.20 V at the branch INA at 42-122 mA (modest ~0.2 ohm apparent drop). The boosted
runs read lower (3.05-3.13 V) at the same point, but that node feeds the TPS63802's
switching input -- do not treat those as a clean impedance measurement.

What changes (the measured verdict does NOT -- it is photons vs watts):

- The "LFP plateau picks your operating point" framing was wrong in mechanism. The
  regulator picks the operating point; harness IR does the sagging. The pixel-level
  conclusion stands: ~3.2 V effective at the pixel is a mildly-undervolted sweet spot,
  blue's -5 % marks the knee edge.
- The low-SOC caveat WEAKENS: the bare config is already SOC-invariant by
  construction, because the rail regulates until deep discharge (bb_efficiency data
  showed the rail carrying much larger loads at VBAT 2.9-3.05). A low-SOC verdict
  flip is now unlikely; the residual check is rail droop under load at low VIN --
  still a 10-minute spot-check when a drained cell is around, but demoted.
- June's "goldening at 2.8-2.95 V LED rail" under show loads: PROVENANCE NOW UNCLEAR
  (correction within this session: the feed was never STEMMA -- always 3V3 + GND +
  GPIO; and per Ben, brightness back then was measured with the serial-USB PAR
  sensor). bb_efficiency notes say the LFP *terminal* sagged to ~2.9-3.05 V under
  show loads -- the June note may have conflated battery terminal with pixel V+. If
  the regulated rail actually held ~3.2-3.3 V, the goldening mechanism needs a
  re-look (rail droop near the ~1 A ceiling under ESP+LED show load is the leading
  candidate). Directly answerable now: with the bare hex mounted, ramp ring2/all at
  rising bri and log 0x41 bus_v vs branch current = the rail droop curve, ~5 min.
  Same measurement doubles as step 0 of the RGBW rail-capability check.
- The boosted config as tested was a double conversion (VBAT -> 3V3 boost -> 4.2 V
  boost); the battery-side numbers already include that tax, so the efficiency
  verdict is if anything generous to the boost.
- RGBW A/B design sharpens: today's HEX data never exceeded 0.21 A on the ~1 A rail
  (clean, uncontaminated by any limit), but 4 W white rail-direct wants ~1.2 A at
  3.3 V -- the BARE config hits the rail ceiling too. The RGBW experiment is really a
  topology question (rail-fed vs VBAT-fed boost) with a rail-capability
  characterization as step 0. TODO updated.

## 2026-07-02 - Ben + Claude - Boost A/B verdict (HEX, single-px gobo regime): boost NOT worth it at healthy SOC

Boosted r3 remount closed the loop: every r3 number matches r1 to <1 % across a
physical swap (white 217.0 vs 216.7 lux, ring1 596.8 vs 596.0). Combined bound on
seating error across the full bare/boosted/bare/boosted series: <=2 %, usually <1 %.

Consolidated results, center-anchor looks, LFP bench cell at SOC ~97 (rail 3.12-3.20 V
under load), VEML7700 at the tube exit (only ratios are portable):

  look              bare lux    boosted lux   delta     LED branch W (bare->boosted)
  white 1 px full   211.5-215.6 216.7-217.4   +1.6 %*   0.134 -> 0.216  (+60 %)
  red single        33.3        33.2-33.5     ~0        0.065 -> 0.113
  green single      128.6       129.4-129.5   +0.7 %*   0.065 -> 0.113
  blue single       60.4        63.4-63.6     +5.1 %    0.062 -> 0.113
  ring1 7px bri128  557.9       596.0-596.8   +6.9 %    0.388 -> 0.62-0.63
  (* within the +-2 % seating noise)

Verdict against Ben's three "worth it" criteria, for the HEX in its production role
(Ben's product call: >1 full-white px washes out the gobo, so single white px full --
or color-separated singles distributing the same load -- IS the operating point; the
heavy-load ladder is moot for HEX):
  A) install effort: NOT justified by B/C below.
  B) visible lumens/color gain: NO. White +1.6 % is inside the noise; blue +5 % is
     below brightness JND. No goldening on bare at this load -- the drivers are not
     meaningfully starved at plateau voltage with a single pixel.
  C) lumens/W: boost is ~40 % WORSE (white ~1594 -> ~1006 lux/W); the extra 60 %
     branch power becomes constant-current-driver heat, not photons.

Why this likely holds in production: LFP spends most of the night at 3.2-3.3 V
terminal, and a single-px look (~170 mA system) barely sags it -- today's rail IS the
plateau operating point. The 2026-06-12 goldening lived at 2.8-2.95 V under multi-px
show loads the gobo role never uses. Boost gain visibly grows with load (+7 % at just
7 px half) exactly as the dropout physics predicts -- the effect is real, the
production HEX just does not operate where it pays.

Caveats / remaining: n=1 hex pair, board, cell; single SOC. The one surviving boost
case for HEX is the low-SOC end (knee, ~3.0 V open) -- worth a cheap repeat on a
run-down battery before final BOM removal. The 4 W RGBW point source is a separate
question (different LED, own Vf stack) and inherits none of this verdict.

## 2026-07-02 - Ben + Claude - Bare r2 suite: swap reproducibility ~2%; boost gain grows with load

Bare hex remounted (swap 2). Third protocol gotcha closed for good: led_studio only
redraws static frames on an actual VALUE change -- re-sending identical values does
not render, so the r2 white capture caught a dark hex despite the "poke" (artifact
file kept). boost_ab_log.py now wiggles bri by 1 count and back, forcing two real
renders; the r2b redo is the valid bare white run.

Cross-swap reproducibility (the geometry error bound Ben's back-and-forth was for):
bare white 215.6 -> 211.5 lux across two physical swaps (~2 %); red single 33.3 vs
boosted 33.5, green 128.6 vs 129.5 (<1 % where no optical gain is expected). Seating
noise ~ +-1-2 %.

Bare r2 vs boosted r1, same sliders (diffs above the noise floor in bold):
  white 1 px full: 211.5-215.6 vs 216.7-217.4 lux (+1-3 %, marginal) at +60 % LED power
  red single: 33.3 vs 33.5 (nil)   green single: 128.6 vs 129.5 (nil)
  **blue single: 60.4 vs 63.6 (+5.3 %)** -- blue is the highest-Vf channel; mild
    starvation at ~3.2 V rail is visible exactly where physics says it should be
  **ring1 7 px bri=128: 557.9 vs 596.0 (+6.8 %)** at 0.388 vs 0.631 W branch power
Trend: boost gain grows with load (1 px white ~+1-2 %, blue single +5 %, 7 px +7 %) --
consistent with the dropout/sag hypothesis; the decisive regime (ring2/all-37, low
SOC, where bare sags to ~2.8-2.95 V) is still unprobed. Efficiency so far: boost
costs ~+60 % LED-branch power for single-digit optical gains at all probed points.
Next: boosted r3 remount (boosted-side reproducibility), then design the heavy-load
ladder carefully (boosted all-37 at bri=128 projects to ~1.1 A into the 3V3 header --
brownout risk; step up with live sag watch, abort below ~2.8 V input).

## 2026-07-02 - Ben + Claude - First boosted captures: +60% LED power, +1% light at the single-px test point

Boosted hex (TPS63802 4.2 V inline on V+) swapped in, same taped position. Two protocol
gotchas caught first: (1) a run captured a BLANK hex -- led_studio only pushes static
frames on change, so a hex hot-swapped after the last render stays dark until the next
/set; boost_ab_log.py now pokes a no-op render before every capture (the artifact file
`092726_boosted-center-rgbwhite-full-r1.jsonl` is kept as a record). (2) W-channel-only
is dark: the NeoHEX is RGB-only SK6812, so W drops out of the HEX protocol (still
relevant for the 4 W RGBW point source).

Headline (60 s runs, center px RGB white full, SOC ~97, battery ~3.2-3.3 V open):
bare 215.6 lux @ 0.134 W LED branch; boosted 216.7-217.4 lux @ 0.215 W LED branch =
**+60 % electrical, +<1 % optical, lumens/W drops ~40 % (1608 -> ~1010 lux/W)**.
Battery total 0.537 -> 0.635 W. Interpretation (single test point, n=1): a SINGLE pixel
at a healthy battery is NOT in dropout -- the SK6812 constant-current drivers were
already at regulated current at ~3.19 V, so the 4.2 V headroom burns in the drivers as
heat. The boost's claimed value regime (blue/green dropout, goldening) needs the
heavy-load rail sag (~2.8-2.95 V) and/or low SOC -- not yet probed. Do NOT generalize
to "boost is worthless" from this point alone.

New `ops/bench/boost_ab_suite.sh <config> <runtag>` runs the per-mount battery:
white-full 60 s, R/G/B singles 30 s, ring1 (7 px) white bri=128 30 s, restores the
look. Boosted r1 suite: red 33.5 lux / green 129.5 / blue 63.6 (channel branch powers
nearly equal at 0.112-0.114 W; singles sum to 226.6 vs white 216.7, additive within
~5 %), ring1-half 596 lux @ 0.631 W branch, battery 1.07 W, boost input bus sagged to
3.05 V. Next: swap to bare, `boost_ab_suite.sh bare r2`, then keep alternating
(Ben's plan: multiple back-and-forth mounts to bound the seating/geometry error), then
push into the heavy-load/low-SOC regime where the boost hypothesis actually lives.

## 2026-07-02 - Ben + Claude - Boost A/B harness live; bare-hex baseline captured

Ben's harness: desk | bare HEX | upside-down 3D-printed lantern proto (cylindrical
tube) | VEML7700 taped at the tube exit, ~6 in from the hex. Look: center pixel only,
r=g=b=255, w=0, bri=255, PowerFeather battery-only (sv=0.02). New
`ops/bench/boost_ab_log.py` merges the KB2040 'ina'/'lux' serial stream with
led_studio /state into labeled JSONL (ops/bench/data/boost_ab/).

INA channel map CONFIRMED (corrects the earlier idle-based guess, which had it
backwards): **0x41 = LED power out, 0x45 = battery (charge-positive, so discharge
reads negative)**. Three independent cross-checks: (1) 0x41 = 42.1 mA at center-white
== the known 41.8 mA single-px number from 2026-06-11; (2) 0x45 = -170 mA vs the fuel
gauge's -175 mA; (3) LED-off floor: 0x41 drops to 8.3 mA (= 37-px dark quiescent),
0x45 stays ~-135 mA (ESP + WiFi overhead). Budget closes: 42 (LED) + ~128 (system) ~=
170 (battery).

Bare-hex baseline (60 s, `2026-07-02_091851_bare-center-rgbwhite-full.jsonl`):
**215.6 lux** (sd 0.22) at the tube exit; LED 42.1 mA @ 3.19 V = **0.134 W**; battery
170 mA @ 3.16 V = **0.537 W** system. Ambient-dark reference (bri=0, same position):
**2.3 lux** floor -- LED dominates the reading; look restored and verified after.

Caution flag: a quick 12 s glance ~10 min before the logged run read 167.3 lux at the
SAME LED current (42.3 mA) -- a 29 % optical difference with an unchanged electrical
operating point, almost certainly geometry (final taping happened in between), not the
LED. Protocol consequences for the A/B: (1) nothing moves once positioned -- mark the
hex outline on the desk so bare and boosted hexes seat identically under the tube;
(2) take an ambient-dark reference each session; (3) run bare vs boosted back-to-back
at similar SOC; (4) compare same-sliders AND matched-lux. n=1 harness, unshrouded room
light -- treat absolute lux as position-specific, only ratios are portable.

## 2026-07-02 - Ben + Claude - ledstudio.local live on the desk board + lux channel on the monitor

Two bench-tooling steps for the TPS63802 4.2 V boost experiment:

- `firmware/led_studio/` now sets hostname + mDNS (`http://ledstudio.local/`) and was
  USB-flashed to the desk PowerFeather `9E5B0C` (ttyACM1), replacing
  `power-bench-2026-06-11.1` -- that image predates the `/update` endpoint, so there
  was no OTA path off it; the board was already tethered. Verified live:
  `ledstudio.local` resolves (192.168.4.76), the LED Studio UI serves, and the new
  image has `/update`, so future studio tweaks go over OTA per the standing preference.

- `firmware/ina_monitor/` gains an optional photopic lux channel on the same QT chain:
  TSL2591 (0x29) and VEML7700 (0x10) are auto-detected at boot, on 'r', and by a 5 s
  background re-probe (hot-plug friendly), and stream as `lux` lines interleaved with
  the `ina` lines -- light + electrical power in one timestamped serial stream. Fixed
  low-gain / 100 ms configs sized for LED-bench levels; a `sat=1` flag marks
  saturation (move the sensor back rather than re-gaining mid-comparison). KB2040
  reflashed no-touch via the 1200-baud bootloader touch; verified INAs still stream
  and both lux probes correctly report MISSING with nothing plugged in.

Sensor rationale (Ben's question: switch from PAR?): yes, for the boost verdict use a
photopic lux sensor as the primary light metric. The decision criteria are lumens and
lumens/W as perceived, and the channels the boost should recover are blue/green -- a
photopic sensor weights them like the eye, while the PAR meter's flat 400-700 nm
quantum response over-credits blue (it counts blue photons ~1:1 that the eye weights
~0.05). Keep the PAR meter (ttyACM0) logging as a spectrum-robust cross-check and for
continuity with plot_par_vs_draw data. For A/B ratios, absolute calibration is moot;
linearity + not saturating + fixed geometry are what matter. VEML7700 is the cleaner
photopic instrument; TSL2591 has more dynamic range -- either works, both supported.

Tentative INA channel labels from the live stream after the led_studio flash: 0x41
carries the system load (~44-45 mA with the ESP awake) = battery side; 0x45 idles at
1-2 mA = LED power out with LEDs off. n=1, unlabeled wiring -- confirm by lighting
pixels and watching which channel jumps before logging real runs.

## 2026-07-02 - Ben + Claude - KB2040 flashed as the INA monitor for the boost bench

The Metro that ran `firmware/ina_monitor/` is now on noisemaker duty, so the monitor role
moves to an Adafruit KB2040 (RP2040) for the TPS63802 4.2 V boost experiment (bare vs
boosted HEX, INAs on battery and LED power out). The sketch needed zero code changes:
`Wire.begin()` default pins are the STEMMA-QT port on both boards (KB2040 = GPIO12/13),
and `Serial.printf` works on the arduino-pico core. Compiled with
`rp2040:rp2040:adafruit_kb2040` and flashed by UF2 drop onto the RPI-RP2 bootloader
drive; header comment now documents both targets and the KB2040 flash path.

Bench note: the KB2040 initially did not enumerate at all (no lsusb entry despite power).
BOOTSEL-hold + reset brought up the RPI-RP2 bootloader on the same cable, so the cable
was fine; whatever firmware was previously on the board was not exposing USB. After the
UF2 drop it enumerates as 239a:8105 with CDC serial (ttyACM2 on this host; the Apogee PAR
meter is ttyACM0 and the desk PowerFeather 9E5B0C is ttyACM1). Verified live: probe found
the two SEN0291s at 0x41 and 0x45, i2c scan clean, both streaming at 10 Hz (~3.2-3.3 V
bus, ~7-10 mA idle on each -- which INA is battery vs LED rail still needs a load test to
label). Next: wire the boost hot-swap and run the eye test + PAR/INA comparison per the
TPS63802 TODO section.

## 2026-07-02 - Ben + Claude - Retired the 120 mAh/night budget floor

Removed the old ~120 mAh/night nightly-budget number as a reference point in SYSTEM.md,
AGENTS.md, and TODO.md (ADR 0021 left as-is, append-only). It was napkin math from before
hardware testing -- low-current ESP32-C3, very dim 1-3 pixel ambient assumptions -- and
the gobo work since shows crisp projection needs far more LED power than it assumed, so
keeping it around even as a "floor" invited anchoring. The production budget will be
derived bottom-up: measured LED draw (400-500 mA at full on HEX/RGBW) x a realistic show
duty cycle, minus measured harvest at MPP. The TODO item to compute it stays open.

## 2026-07-01 - Codex - Field-cycle v2 deployed for multi-day solar run

Upgraded `firmware/net_bench` to `net-bench-2026-07-01.1` for the next multi-day
`9E5AB8` solar-cycle experiment.

Firmware changes:

- Added field-cycle v2 summaries to the heartbeat while keeping the packet at 128 bytes:
  charge/discharge Wh, peak panel/charge/draw W, soft-low debounce seconds, and phase
  duration minutes.
- Added soft-low debounce for field-cycle drawdown: critical floor still sleeps
  immediately, but soft low must persist for `NB_FIELD_LOW_CONFIRM_S`.
- Added optional `--field-led-load` for draw phase, reusing the direct-GPIO HEX/SK6812
  load with separate `--drawdown-lit` and `--drawdown-brightness` controls.
- Extended `ops/bench/net_bench_dashboard.py` and `ops/bench/net_bench_log.py` to parse
  the v2 summary suffixes.

Deployed build on the outdoor solar peer `9E5AB8` via targeted shared-WiFi OTA:

```
./build.sh --role peer --channel 11 --field-cycle \
  --field-charge-s 300 --field-wait-s 300 --field-protect-s 900 \
  --field-wake-ms 8000 --field-cold-ms 30000 \
  --field-low-mv 3150 --field-critical-mv 3050 --field-low-confirm-s 30 \
  --field-led-load --drawdown-lit 18 --drawdown-brightness 128 \
  --chem lfp --cap 6000 --charge-ma 1500 --maintain 4.6
```

OTA path: targeted `U9E5AB8` caught the peer in shared-WiFi maintenance at
`192.168.4.64`; upload returned `Update complete. Rebooting.` The peer rejoined ESP-NOW
with `fw=net-bench-2026-07-01.1`, reset reason `software`, then resumed charge-mode
timer sleeps. The COM7 serial bridge/master was USB-flashed to `.1` too, so it can decode
and emit the new field summary tail.

Live verification after restart:

- Dashboard backend running on `http://127.0.0.1:8765/`, COM7 bridge `.1`.
- New logger run: `ops/bench/data/ca/2026-07-01-ca-field-cycle-9E5AB8-v2.jsonl`.
- First v2 rows show `field_charge_wh`, `field_peak_panel_w`, `field_peak_charge_w`,
  `field_low_s`, and phase-minute fields. Example: charge phase, `field_charge_wh=0.4`,
  `field_peak_panel_w=3.15`, `field_peak_charge_w=2.59`, `field_charge_min=10`.

Next: let this run through several day/night cycles, then analyze whether the 18-pixel
load reaches protect nightly, whether `3.15 V` + 30 s debounce avoids false cutoffs, and
whether panel Wh/day covers the configured night load.

## 2026-07-01 - Codex - Added Modulino Buzzer and Vibro I2C controls

Extended `firmware/clacker_demo/` for Ben's Arduino Modulino Buzzer and Modulino Vibro
boards on the Metro ESP32-S3 STEMMA/Qwiic bus. The dashboard now shows Buzzer and Vibro
detection status next to the Omron relay, has `Scan I2C`, adds `Modulino buzzer` as a
selectable tone output, and adds Vibro `Pulse`, `Buzz`, `Soft buzz`, and `Vibro off`
buttons.

Implemented the Modulino protocol directly from the Arduino libraries instead of adding
another dependency: Buzzer receives 8-byte little-endian frequency/duration packets at
7-bit address `0x1E` (firmware address `0x3C`), while Vibro receives 12-byte
frequency/duration/power packets at `0x38` (firmware address `0x70`, with a fallback probe
for the `0x3A`/`0x1D` address listed in some docs). Existing beep/sweep/Moonlight playback
now routes through the Modulino Buzzer when selected.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. The live
bench detected the SparkFun relay at `0x18`, Modulino Buzzer at `0x1E`, and Modulino Vibro
at `0x38`. Verified a short Modulino Buzzer chirp and a short Vibro pulse via the API,
then restored the amp output selection and left all relays/audio/vibro off with default
`420 ms` gap / `70 ms` pulse settings.

## 2026-07-01 - Codex - Added Qwiic Omron relay dashboard controls

Extended `firmware/clacker_demo/` so the Metro ESP32-S3 dashboard can click/clack the
SparkFun Qwiic Omron relay on the STEMMA/Qwiic port. Added an `Omron Qwiic click` button,
Qwiic scan/status display, and an `Start Omron` repeat-clack mode using the same gap and
pulse-width sliders as the existing relay controls. Starting Omron repeat mode stops the
A/B relay auto mode so the audible timing stays easy to compare.

The first implementation targeted the newer TCA9555-based SparkFun Qwiic Relay Line at
`0x20`/`0x21`, but the connected board did not detect there. Added support for the older
SparkFun Qwiic Single Relay protocol at `0x18`/`0x19` as well; the bench board detected as
`single` at `0x18`. Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on
`/dev/ttyACM1`, verified the dashboard API at `http://clacker.local/`, exercised a short
Qwiic pulse plus a short repeat-clack run, then restored defaults (`420 ms` gap, `70 ms`
pulse) and left all relays/audio off.

## 2026-07-01 - Codex - Added selectable D5/D6/D7 noisemaker outputs

Updated `firmware/clacker_demo/` after Ben wired a passive piezo to Metro `D6`/GPIO6 and
a SparkFun RedBot buzzer to `D7`/GPIO7 while keeping the 8002A amp/speaker on `D5`/GPIO5.
The dashboard now has a noisemaker selector plus quick chirp buttons for amp, piezo, and
RedBot; all existing beep, sweep, and melody controls play through the selected output.
The firmware detaches the prior LEDC/PWM pin before moving playback to another output so
only one noisemaker is driven at a time. The large alarm remains intentionally unpowered.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
the board rejoined as `http://clacker.local/` / `192.168.4.57`, exercised short chirps on
all three outputs via the API, muted playback, and left the selected output back on the
8002A amp.

## 2026-07-01 - Codex - Extended Moonlight to first high melody entrance

Extended the `Moonlight` button in `firmware/clacker_demo/` so it continues past the
opening triplet setup into the first high G#4 melody entrance ("duh duh-duh" piano-line
moment Ben called out). Because the bench output is still monophonic square-wave PWM, the
G#4 entrance is exaggerated as separated longer hits rather than layered over the arpeggio
like the real piano score.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/tune?id=moonlight` starts playback and muted with `/api/tune?id=stop`; final state
reported `tune="none"`.

## 2026-07-01 - Codex - Routed sweep buttons through melody scheduler

Ben reported the three sweep buttons were still silent while the Moonlight melody worked.
Changed `firmware/clacker_demo/` so `Sweep up`, `Sweep down`, and `Laser sweep` are now
explicit stepped note sequences run through the same proven monophonic scheduler as the
working melody buttons, instead of the separate continuous-retune sweep state machine.
This should avoid the silent behavior from rapid frequency retuning.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/sweep?id=up`, `/api/sweep?id=down`, and `/api/sweep?id=laser` each report the
expected active tune state, then muted with `/api/tune?id=stop`; final state reported
`tune="none"`. Acoustic confirmation still depends on Ben's bench listen.

## 2026-07-01 - Codex - Corrected Moonlight opening from referenced MIDI

Downloaded Ben's reference MIDI (`https://bitmidi.com/uploads/16752.mid`) to inspect the
opening. It is format 1 with 8 tracks and 120 ticks/quarter; the initial tempo is about
50 BPM, making the opening triplet notes about 400 ms apart. The recognizable opening
texture repeats the G#3-C#4-E4 triplet cell eight times before moving, whereas the prior
bench melody compressed each harmony into one ascending gesture and climbed too quickly.

Updated `firmware/clacker_demo/` so `Moonlight` is now a short monophonic reduction of
the first 16 triplet groups from the MIDI-derived opening pattern. This remains square-wave
single-voice playback on Metro `D5`/GPIO5, not a piano/PCM arrangement, but it preserves the
repeated triplet texture. Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on
`/dev/ttyACM1`, triggered `/api/tune?id=moonlight`, and then muted with
`/api/tune?id=stop`; final state reported `tune="none"`.

## 2026-07-01 - Codex - Reworked clacker sweeps and Moonlight melody

Updated `firmware/clacker_demo/` after Ben reported the three sweep buttons were silent
and the Moonlight sequence was too fast / not recognizable. Replaced the sweep playback
path with direct LEDC frequency control instead of rapid queued `tone()` calls, which is a
better fit for continuously changing frequencies. Also lowered and slowed the Moonlight
sequence into a more recognizable opening-arpeggio approximation, still monophonic square
wave rather than piano/PCM audio.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Exercised
`/api/sweep?id=up`, `/api/sweep?id=laser`, `/api/tune?id=moonlight`, then muted with
`/api/tune?id=stop`; API state returned to `tune="none"`. Actual acoustic quality still
needs Ben's ears at the bench.

## 2026-07-01 - Codex - Added sweep and melody buttons to noisemaker dashboard

Extended `firmware/clacker_demo/` speaker controls with nonblocking frequency sweeps
(`Sweep up`, `Sweep down`, `Laser sweep`) plus a simple monophonic Moonlight-style
arpeggio sequence. The new controls still use the existing `tone()`/PWM path on Metro
`D5`/GPIO5; this is not PCM or polyphonic audio, just note/sweep scheduling over square
waves.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
the page contains the new buttons, exercised `/api/sweep?id=laser` and
`/api/tune?id=moonlight`, then muted with `/api/tune?id=stop`.

## 2026-07-01 - Codex - Fixed clacker dashboard slider persistence

Fixed the `firmware/clacker_demo/` dashboard sliders snapping back during the 1 Hz state
refresh. Slider changes now push timing values immediately to a new `/api/settings`
endpoint, and the browser suppresses slider rewrites while a drag/update is in flight.
Added `Cache-Control: no-store` on dashboard/API responses so the browser reloads the
new JavaScript after reflashing.

Rebuilt and reflashed the connected Adafruit Metro ESP32-S3 on `/dev/ttyACM1`. Verified
`/api/settings?interval=760&pulse=115` persisted through `/api/state`, then restored the
bench defaults to `interval=420` and `pulse=70`.

## 2026-07-01 - Codex - Added WiFi dashboard for relay/speaker noisemaker bench

Onboarded against the repo read order and fetched `origin/main`; local `main` was current
with `origin/main` (`0 0` ahead/behind), with pre-existing local changes in `LOG.md` and
untracked `firmware/clacker_demo/` preserved.

Reworked `firmware/clacker_demo/` from an automatic two-relay pulse sketch into an Adafruit
Metro ESP32-S3 WiFi dashboard for Ben's lantern noisemaker bench. The dashboard connects to
the shared bench AP via ignored `wifi_secrets.h`, serves at `http://clacker.local/`, drives
relay modules on Metro `A0`/`A1`, supports one-shot relay clicks plus adjustable A/B
auto-clack timing, and drives the 8002A amp/speaker signal from Metro `D5`/GPIO5 with
simple tone/melody buttons. Added a local build helper that uses a dedicated Arduino
`--build-path` to avoid shared-cache collisions.

Compiled the sketch for `esp32:esp32:adafruit_metro_esp32s3`, uploaded it to the connected
Metro on `/dev/ttyACM1`, and verified the dashboard API at `http://clacker.local/api/state`
with the board reporting IP `192.168.4.57`.

## 2026-06-30 - Codex - Updated clacker sketch for two-relay comparison

Updated `firmware/clacker_demo/` for Ben's A/B relay sound comparison: the sketch now
drives relay modules on Metro `A0` and `A1`, assumes high-trigger modules, and alternates
short pulses through slow, medium, and double-tap patterns. Reflashed the connected
Adafruit Metro ESP32-S3 on `/dev/ttyACM1` after compiling with a dedicated Arduino build
path.

## 2026-06-30 - Codex - Added relay clacker bench sketch

Added `firmware/clacker_demo/`, a small Arduino sketch for Ben's relay/noisemaker
experiment using a cheap Songle-based relay module. The sketch toggles Metro D13 through
slow, medium, and double-tap patterns so active-low and active-high relay boards can be
heard without changing firmware. The README records the initial 3V3/GND/D13 wiring and
notes that common SRD-05VDC relay modules may need USB 5 V on VCC while keeping D13 as
the logic input.

## 2026-06-30 - Codex - Hungry 6 Ah cell pulled near-nominal solar power

Ben moved the nearly-depleted 6 Ah 32700 LiFePO4 cell back onto the solar `9E5AB8`
PowerFeather after briefly proving that the same cell would take high power from an
Anker USB bank on the bench rig. The next fresh solar wake showed the earlier sub-watt
behavior was not a hard panel/charger ceiling:

- `battery_v=3.404..3.458`, `battery_ma=1000..1020`, and `battery_w=3.46..3.47`.
- `supply_v=4.859`, `supply_ma=774..792`, and `supply_w=3.76..3.85`.
- Panel-side INA reported about `5.16 V` and `0.79..0.81 A`, or about 4.1 W by
  magnitude (`ina_panel_w=-4.09..-4.16`; sign is wiring direction).
- BQ telemetry still showed `bq_vindpm_mv=4800`, `bq_ichg_ma=1480`,
  `bq_vreg_mv=3600`, charge enabled, HIZ false, VBUS source detected, and no fault.

Interpretation: the P105/5 W-class panel and BQ path can source roughly 4 W in direct hot
sun with a charge-hungry LFP cell. The earlier low-watt plateau was likely a transient
combination of very-low-VBAT charger behavior, battery acceptance/surface-voltage state,
solar input qualification/VINDPM interaction, and/or simply not yet enough sun. The brief
USB charge may have lifted the cell/charger out of a low-voltage regime, making this a
good candidate for a repeatable "recover from below 3.0 V" characterization rather than
evidence of a failed solar path.

## 2026-06-30 - Codex - 7200 mAh cell swap restored multi-watt solar harvest

Ben swapped the 7200 mAh 32700 LiFePO4 cell from the disconnected `9E5AF0` setup into
the solar-powered `9E5AB8` PowerFeather while the panel was connected. This produced
the expected harvest jump:

- Before the swap, low-cell / USB-rescue samples were around `supply_v=4.887`,
  `supply_ma=94..104`, and `battery_ma=36..38`; earlier solar-only samples with the
  depleted cell were roughly 0.3-0.6 W input and near-zero battery current after the
  OTA threshold event.
- After the swap, `9E5AB8` reported `battery_v=3.571`, `battery_ma=774`,
  `supply_v=5.554`, `supply_ma=542`, and `ina_panel_mv=5832`, `ina_panel_ma=-565`.
  That is about 3.01 W at the charger telemetry and about 3.30 W by panel-side INA
  magnitude, with about 2.76 W into the battery.
- BQ telemetry showed `bq_vindpm_mv=4800`, charge enabled, HIZ false, BATFET normal,
  VBUS adapter/source detected, charge-state 2 (CV/taper bucket), and no fault.

Interpretation: the panel/charger path can harvest multi-watt power in this setup. The
earlier sub-watt behavior was not a simple panel/MPP ceiling; it was dominated by the
deeply depleted cell's charge-acceptance/precharge/power-path state and/or the source
interaction. `9E5AB8` still has `cap=6000` in NVS after the physical 7200 mAh swap; leave
it alone for a clean short harvest comparison, then set targeted capacity to 7200 mAh
before relying on gauge/SOC accounting.

During the swap the COM7 serial bridge briefly USB-disconnected/rebooted (Windows eject
sound; dashboard raw log shows a fresh `.7` boot banner at about 2026-06-30 14:14
America/Los_Angeles). It came back on COM7. The dashboard backend and logger remained
alive; the browser page may need a refresh because its event stream can stale after a
USB reconnect.

## 2026-06-30 - Codex - BQ charger telemetry OTA added during USB-rescue test

Added `net-bench-2026-06-30.7` charger telemetry while `9E5AB8` was recovering from
low VBAT on an Anker USB bank with the solar panel disconnected. The change appends a
new heartbeat tail with BQ25628E VINDPM, charge-current limit, CV limit, raw
control/status/fault registers, and dashboard/log decodes for `CHG_EN`, `EN_HIZ`,
BATFET control, VBUS state, and charge state. Updated `ops/bench/net_bench_dashboard.py`,
`ops/bench/net_bench_log.py`, and this README path's telemetry docs.

Builds/flash:

- Built peer image at
  `firmware/net_bench/build/field-cycle-peer-20260630-v7/net_bench.ino.bin`.
- Built and USB-flashed the COM7 serial bridge/master to `.7`.
- Used targeted `U9E5AB8` so older drawdown peer `9E5AF0` was not pulled into
  maintenance.
- `9E5AB8` entered shared-WiFi maintenance at `192.168.4.40` and accepted OTA to `.7`;
  `net_bench_ota.py` recorded `t_ack_s=6.21`, recovered true, no button.

First `.7` post-OTA heartbeat:

- `battery_v=2.938`, `battery_ma=36`, `supply_v=4.887`, `supply_ma=104`.
- `bq_vindpm_mv=4800`, `bq_ichg_ma=1480`, `bq_vreg_mv=3600`.
- `CHG_EN=true`, `EN_HIZ=false`, BATFET normal, VBUS adapter state, charge-state CC
  bucket, `fault0=0`.

Interpretation: the charger/power path is healthy; the low Anker wattage is not ship
mode, HIZ, or a BQ fault. It is ordinary charge regulation/source behavior with the
board near a 4.8 V VINDPM point and the cell around 2.94 V. Logger continuation now
writes to `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8-v7-bq.jsonl`.

## 2026-06-30 - Codex - USB bank masked by higher solar input during field-cycle rescue

During the `9E5AB8` low-VBAT field-cycle run, Ben connected an Anker USB battery while
the solar panel was still attached. The Anker did not detect a load, while dashboard
telemetry still showed the PowerFeather supply at about `6.2 V` from the panel. After
Ben disconnected the solar panel, then disconnected/reconnected the Anker, the board
accepted USB input. A fresh wake at 2026-06-30 13:25 America/Los_Angeles showed:

- `supply_v=4.887`, `supply_ma=92`, `supply_good=true` from the charger telemetry.
- `battery_v=2.916`, `battery_ma=38`, `ina_batt_ma=34`, so the battery was charging
  slowly rather than disconnected.
- `ina_panel_mv=4788`, `ina_panel_ma=0`, confirming the panel path was no longer the
  active source.

Interpretation: a 5 V USB bank will not necessarily source current while the solar/VDC
input is already sitting above it. Once the solar input is removed, USB works, but this
build's `--maintain 4.8` solar VINDPM setting leaves little headroom on a 5 V bank
(`4.887 V` observed at the board) and likely throttles input current. Low-VBAT
precharge/trickle behavior may also be limiting cell current around 2.9 V. Follow-up:
add direct BQ25628E charger status/fault telemetry and consider a USB-rescue policy
that lowers VINDPM toward 4.6 V when the source is a USB/power-bank input rather than
a solar panel.

## 2026-06-30 - Codex - Solar-only low-VBAT OTA succeeded at 2.901 V

The armed field-cycle watcher caught `9E5AB8` on a fresh solar wake at
2026-06-30 11:58:58 America/Los_Angeles with `battery_v=2.901`, `supply_v=6.217`,
`supply_ma=76`, and `supply_good=true`. Because the peer was still running `.4`, the
watcher intentionally sent one last bare `U`, observed maintenance telemetry at
`192.168.4.40`, and uploaded the `.6` field-cycle image:

- `ops/bench/net_bench_ota.py` wrote `t_ack_s=5.08`, `ack="Update complete. Rebooting."`,
  `recovered=true`, `button_press_required=false`, notes
  `9E5AB8 .6 solar-only low-VBAT OTA at >=2.90V`.
- The peer rejoined ESP-NOW as `net-bench-2026-06-30.6`.
- The `.6` rail-restore change worked: lux, SHT31 panel temperature/RH, and onboard INA
  telemetry returned after sleep. Live sample after the OTA showed `lux=sat`,
  `ptc=45.5`, `prh=19`, `ipv=6456`, `ipa=-71`, `ibv=2888`, `iba=0`.

This validates the "low VBAT + external solar panel" OTA stress path at about 2.90 V
loaded/charging. Future maintenance on `.6` peers can use targeted `U9E5AB8`, so parallel
drawdown tests no longer need to be disturbed by single-peer OTAs.

## 2026-06-30 - Codex - Targeted maintenance command added for single-peer OTA

Added the non-universal maintenance command Ben asked for before starting parallel
drawdown tests. `net-bench-2026-06-30.6` keeps bare `U` as the sustained fleet
maintenance wake, but also supports `U<id>` such as `U9E5AB8`:

- Firmware: added `NB_TARGET_ENTER_MAINT`; peers enter maintenance only when the
  packet's 3-byte target id matches their short id. The master serial handler sustains
  either bare fleet `U` or targeted `U<id>` for 35 s so it can catch timer-wake windows.
- Dashboard: validates `U[0-9A-Fa-f]{6}` and changes `Peer maint` to send targeted
  maintenance for the selected peer instead of broadcasting to every awake peer.
- Sensor rail restore: on boot, PowerFeather 3V3 and VSQT/STEMMA rails are explicitly
  re-enabled before env/INA probing, with a short settle delay. This should restore
  panel/battery INA telemetry after a field-cycle rail-off sleep.

Built and USB-flashed the COM7 serial bridge/master to `.6`; built the `.6` field-cycle
peer OTA artifact. Because outdoor peer `9E5AB8` is still running `.4`, the solar-only
low-VBAT migration to `.6` must use one last bare `U`; after that, targeted `U9E5AB8`
can be used without disturbing separate 6 Ah vs 7.2 Ah drawdown experiments.

At 2026-06-30 10:37 America/Los_Angeles, `9E5AB8` was alive on solar-only `.4`, charging
around 2.79 V with about 0.48 W supply input. A single `.6` watcher remains armed to
trigger the solar-only low-VBAT OTA at a fresh wake with `battery_v >= 2.90` and
`supply_good=true`; an accidentally leftover `.5` watcher was stopped so only one upload
can fire.

## 2026-06-30 - Codex - Field-cycle lifecycle mode implemented and deployed to 9E5AB8

Graduated the low-VBAT stress-test path into a first production-ish lifecycle mode inside
`firmware/net_bench` rather than starting a new sketch, preserving the proven ESP-NOW
bridge, shared-WiFi OTA, PowerFeather solar guard, and dashboard tooling.

Implemented `--field-cycle`:

- Peer state machine: `charge` on external supply/solar -> rail-cut timer sleep while
  charging -> `wait-dark` when full-ish -> always-awake `draw` in dark using the normal
  1 Hz radio load -> `protect` timer sleep at low/critical LFP voltage.
- Sleep paths blank the pixels, cut both PowerFeather switchable rails, and use timer
  wake so the board remains recoverable. Solar/USB does not have to electrically wake the
  ESP32; the charger works while the ESP32 sleeps and the next timer wake observes supply.
- Added append-only heartbeat tail: `fc`/`fcr`/`fcc`/`fce`/`fcchg`/`fcdis`/`fcmin`/`fcmax`
  for lifecycle phase, transition reason, cycle count, phase elapsed seconds, rough
  charge/discharge mAh, and cycle voltage bounds.
- Bumped the ESP-NOW receive buffer from 96 to 128 bytes and fixed append-tail length
  checks so a `.4` bridge can still parse older `.2`/`.3` peers.
- Updated `build.sh`, `firmware/net_bench/README.md`, `ops/bench/net_bench_dashboard.py`,
  and `ops/bench/net_bench_log.py` for the new mode/telemetry.

Verification/deployment:

- Compiled field-cycle peer:
  `--role peer --channel 11 --hb-hz 1 --field-cycle --chem lfp --cap 6000 --charge-ma 1500 --maintain 4.8`.
- Compiled and USB-flashed COM7 bridge/master to `net-bench-2026-06-30.4`
  (`--role master --channel 11 --serial-bridge`). Dashboard restarted on
  `http://127.0.0.1:8765/` and showed bridge `.4`.
- Shared-WiFi OTA uploaded the field-cycle peer image to `9E5AB8` at `192.168.4.40`
  from about 2.67 V VBAT, USB bank supply good. Upload acked in 4.45 s with no button.
- `9E5AB8` rejoined as `net-bench-2026-06-30.4`, emitted `fc=2` (`charge`),
  `fcr=2`, `fcc=1`, `fce=305`, `fcchg=3`, `fcmin=2675`, `fcmax=2678`, then entered
  the 5-minute charge sleep with rails cut.
- `9E5AF0` was not OTA-updated; it was resumed from maintenance and then targeted-parked
  for 21600 s to stop draining while `9E5AB8` runs the field-cycle test.
- Started long JSONL logger:
  `ops/bench/data/ca/2026-06-30-ca-field-cycle-9E5AB8.jsonl`
  (`--duration 172800`, notes `9E5AB8 field-cycle .4 first day/night lifecycle`).

Next: let the logger run through the next wake/charge/dark/protect transitions, then
summarize charge recovery, sleep cadence, drawdown duration, cutoff reason, and whether
the full/taper heuristic needs adjustment.

## 2026-06-30 - Codex - Low-VBAT remove-from-bank behavior check

Checked the live COM7 serial-bridge dashboard and current `firmware/net_bench` code for
Ben's question about removing a very low `9E5AB8` from a USB battery bank.

Live dashboard snapshot at about 2026-06-30T14:16Z:

- `9E5AB8`: `net-bench-2026-06-30.3`, COMMS mode, 2.54 V VBAT, 0% SOC,
  +35 mA into the battery, `supply_good=true`, 4.875 V / 92 mA supply, about 0.36 W
  running load, `drawdown_active=false`.
- `9E5AF0`: `net-bench-2026-06-30.2`, 3.15 V loaded, about -165 mA, no supply.

Conclusion: the current ordinary net_bench COMMS image does **not** automatically enter
deep sleep just because VBAT is low or external supply disappears. Low-voltage sleep
exists only in specific paths: manual/broadcast `S`, targeted `P<id>[:seconds]`,
`--sleep-cycle`, `--autosleep`, and the targeted drawdown helper's soft/hard floors
(3.18 V / 3.05 V). The maintenance power check is advisory by default and protects OTA
entry reporting, not normal runtime. At 2.5 V, removing USB supply without first parking
the peer is expected to run the board at roughly always-on peer load until voltage
collapses, likely ending in shutdown/brownout behavior rather than graceful sleep.
Recommended bench action before unplugging: targeted park, e.g. `P9E5AB8:21600`, then
let it charge/recover later.

## 2026-06-30 - Ben + Codex - Low-VBAT charging OTA stress pass on 5AB8

Ran a targeted low-voltage charging OTA stress test on `9E5AB8` after the overnight
run-down rescue. No AP maintenance mode was used.

Starting bridge state at 2026-06-30T14:05Z:

- `9E5AB8`: live at 2.461 V, +37 mA into the battery, 0% SOC,
  `supply_v=4.863 V`, `supply_ma=92 mA`, `supply_good=true`,
  `net-bench-2026-06-30.1`.
- `9E5AF0`: live at 3.150 V loaded, -168 mA, `net-bench-2026-06-30.2`.

Bumped the peer test image to `net-bench-2026-06-30.3` solely to make the OTA proof
unambiguous, then built the peer image for channel 11 / LFP / 6000 mAh /
1500 mA charge limit. Binary string check before upload found `BubbyNet` and
`maintenance WiFi up`; `ResonanceMaint`, `Brandon Springs`, and old `.1`/`.2`
firmware markers were absent.

Sent the shared-WiFi maintenance command `U` through the COM7 bridge. At the maintenance
endpoint, `9E5AB8` was at about 2.496 V, +36.2 mA into the battery, 4.875 V USB supply,
and `supply_good=true` on `192.168.4.40`.

Uploaded only to `9E5AB8`:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-lowvbat-charging-5AB8-peer-bubbynet\net_bench.ino.bin --nodes 9E5AB8=192.168.4.40 --jobs 1 --reboot comms`

Result file:
`ops/bench/data/ca/2026-06-30-low-vbat-charging-5AB8-ota-results.jsonl`.

The upload acked in 5.88 s with no button:
`Update complete. Rebooting.` `9E5AB8` rejoined ESP-NOW as
`net-bench-2026-06-30.3`, `reset_reason=software`, about 2.50 V, still charging
from USB (`supply_good=true`, about 92 mA supply current). The bounded monitor ran to
`ops/bench/data/ca/2026-06-30-5AB8-low-vbat-charging-ota-monitor.jsonl`; final sample
showed 2.507 V, +33 mA battery current, `supply_good=true`, `.3`, and fresh ESP-NOW
heartbeats.

Operational note: because `U` is still broadcast-only, `9E5AF0` also entered the
shared-WiFi maintenance window. The HTTP `/resume` request to `192.168.4.39` timed out
at the client, but the peer was verified fresh on the bridge again at 3.151 V,
`net-bench-2026-06-30.2`, no button.

## 2026-06-30 - Ben + Codex - Overnight run-down rescue and single-peer OTA pass

Ben accidentally ran the two wireless peers down overnight, which produced a useful
recovery/OTA boundary test. Baseline from the COM7 dashboard:

- `9E5AB8`: stale by about 35k seconds, last heartbeat at 2.381 V, -173 mA,
  no supply, `net-bench-2026-06-30.1`.
- `9E5AF0`: live at about 3.151 V loaded, -169 mA, SOC 8%,
  `net-bench-2026-06-30.1`.

Started a rescue monitor at
`ops/bench/data/ca/2026-06-30-5AB8-usb-revive-monitor.jsonl`, then Ben turned on the
USB battery feeding `9E5AB8`. The transition was immediate:

- 2026-06-30T13:58:20Z: still stale, 2.381 V, -173 mA, `supply_good=false`.
- 2026-06-30T13:58:21Z: fresh heartbeat, 2.388 V, +34 mA into the battery,
  `supply_v=4.855 V`, `supply_ma=88 mA`, `supply_good=true`.
- By 2026-06-30T14:04:10Z after HTTP `/resume`, it was back on ESP-NOW at 2.449 V,
  +35 mA, supply good, no button/USB data cable required.

For the other peer, bumped the net-bench version to `net-bench-2026-06-30.2` solely to
make the OTA proof unambiguous, then built a peer image for channel 11 / LFP / 7200 mAh
with `ARDUINO_BUILD_PATH` set only to keep a stable artifact path. `build.sh` already
uses a unique Arduino build path by default, so the cache-collision protection remains
inside the helper. Binary string check before upload: `BubbyNet` and `maintenance WiFi
up` present; `ResonanceMaint`, `Brandon Springs`, and old `.1` version absent.

Sent shared-WiFi maintenance command `U` through the bridge. Both awake peers joined
BubbyNet maintenance because the current command is broadcast-only:

- `9E5AF0` -> `192.168.4.39`, about 3.156 V, no supply.
- `9E5AB8` -> `192.168.4.40`, about 2.435 V, USB supply good.

Uploaded only to `9E5AF0`:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-overrun-5AF0-peer-bubbynet\net_bench.ino.bin --nodes 9E5AF0=192.168.4.39 --jobs 1 --reboot comms`

Result file:
`ops/bench/data/ca/2026-06-30-overnight-5AF0-ota-results.jsonl`.

The upload acked in 5.06 s with no button. `9E5AF0` rejoined ESP-NOW as
`net-bench-2026-06-30.2`, `reset_reason=software`, about 3.15 V loaded. `9E5AB8` was
returned from maintenance via `http://192.168.4.40/resume` and rejoined ESP-NOW while
continuing to charge from the USB battery. No AP maintenance mode was used.

## 2026-06-29 - Ben + Codex - OTA failure interpretation softened after clean low-voltage pass

Interpretation update after the official shared-WiFi low-voltage OTA pass: the earlier
maintenance failures should not be described as proven low-VBAT instability. The clean
successful run updated both peers at about 3.10 V loaded (`9E5AB8`) and 3.27 V loaded
(`9E5AF0`) over shared WiFi with no AP and no button. That makes stale WiFi credentials
and AP-contaminated builds the more likely root causes of the confusing pre-upload
failures:

- local `wifi_secrets.h` targeted `Brandon Springs Activity Guest` while the laptop was
  actually on `BubbyNet`;
- `9E5AF0` still had a deprecated `NB_MAINT_AP` image and advertised
  `ResonanceMaint-9E5AF0`;
- old pre-`.5` images could also trip the task watchdog during maintenance entry because
  the WiFi join loop was not feeding it.

Low battery is still a stressor and a boundary variable for production policy, but the
2026-06-29 evidence does not prove that low VBAT caused the earlier OTA instability.
Treat 3.10 V loaded as a proven successful shared-WiFi OTA point, and treat the older
2.95-3.03 V failures as ambiguous / wrong-path pre-upload failures rather than voltage
cutoffs.

## 2026-06-29 - Ben + Codex - Official shared-WiFi low-voltage OTA passed on two peers

Ran the official low-voltage OTA test on the two wireless peers with the COM7 serial
bridge back on USB. This was the fleet path only: shared WiFi (`BubbyNet`) plus
`ops/bench/net_bench_ota.py` parallel uploads. No peer self-AP was used.

Pre-test live baseline from the dashboard:

- bridge `9F26F8`: COM7 serial bridge, channel 11, `net-bench-2026-06-29.4`.
- peer `9E5AB8`: `net-bench-2026-06-29.5`, about 3.098 V loaded / -156 mA, SOC 9%;
  INA battery about 3.100 V / -123 mA. This is below the `.5` advisory LFP OTA floor.
- peer `9E5AF0`: `net-bench-2026-06-29.5`, about 3.274 V loaded / -162 mA, SOC 30%.

Bumped `firmware/net_bench/net_bench.ino` version string to
`net-bench-2026-06-30.1` solely to make the OTA proof unambiguous, then built a non-AP
peer image with isolated build path:

`--role peer --channel 11 --hb-hz 1 --chem lfp --cap 6000 --charge-ma 1500`

Binary string check before upload: `ResonanceMaint` absent, `maintenance WiFi up`
present, `BubbyNet` present, stale `Brandon Springs` absent, `.1` version present, old
`.5` version absent.

Sent dashboard command `U` for sustained ESP-NOW `ENTER_MAINT`; both peers joined shared
WiFi maintenance and exposed `/telemetry` on BubbyNet:

- `9E5AF0` -> `192.168.4.30`, still `.5`, mode 1, battery 3.278 V.
- `9E5AB8` -> `192.168.4.33`, still `.5`, mode 1, battery 3.102 V.

Ran:

`python ops\bench\net_bench_ota.py --bin firmware\net_bench\build\ota-20260630-lowvoltage-official-peer-bubbynet\net_bench.ino.bin --nodes 9E5AF0=192.168.4.30,9E5AB8=192.168.4.33 --jobs 2 --reboot comms`

Results written to `ops/bench/data/ca/2026-06-30-official-low-voltage-ota-results.jsonl`:

- `9E5AB8`: upload ack `Update complete. Rebooting.`, `t_ack=4.46 s`, recovered true,
  no button.
- `9E5AF0`: upload ack `Update complete. Rebooting.`, `t_ack=4.96 s`, recovered true,
  no button.

Post-OTA ESP-NOW verification via the bridge:

- `9E5AB8`: rejoined with `firmware_rev=net-bench-2026-06-30.1`,
  `reset_reason=software`, age < 1 s, battery about 3.09-3.10 V loaded.
- `9E5AF0`: rejoined with `firmware_rev=net-bench-2026-06-30.1`,
  `reset_reason=software`, age < 1 s, battery about 3.27 V loaded.
- WiFi scan after the run showed only `BubbyNet`; no `ResonanceMaint-*` SSID.

Interpretation: current shared-WiFi OTA path is proven on two wireless peers at a lower
successful LFP voltage of about 3.10 V loaded (external INA around 3.10 V on `9E5AB8`).
This is a successful lower bound, not a final hard production threshold below which OTA
must be blocked.

## 2026-06-29 - Ben + Codex - Shared-WiFi OTA lower-bound attempt exposed AP-contaminated peer

Attempted the requested low-battery OTA pass on the two live peers using the shared-WiFi
fleet path only. Did not connect to or upload through any peer self-AP.

Pre-attempt state from the serial-bridge dashboard/log:

- bridge `9F26F8`: `net-bench-2026-06-29.4`, channel 11, serial bridge on COM7.
- peer `9E5AB8`: around 3.02-3.03 V loaded, SOC 4-5%, INA battery about 3.01-3.03 V
  and roughly -125 to -160 mA, still heartbeating.
- peer `9E5AF0`: around 3.258 V loaded, SOC 28%, still heartbeating before the command.

Built a non-AP peer image from current source (`net-bench-2026-06-29.5`) with an isolated
Arduino build path:

`--role peer --channel 11 --hb-hz 1 --chem lfp --cap 6000 --charge-ma 1500`

Sent dashboard command `U`, which is the sustained ESP-NOW `ENTER_MAINT` broadcast while
the master stays in serial-bridge comms. A shared-subnet scan found no peer `/telemetry`
endpoints, so no OTA upload occurred. The laptop was on `BubbyNet`; the checked-in local
`firmware/net_bench/wifi_secrets.h` currently targets `Brandon Springs Activity Guest`,
so the bench WiFi endpoint discovery was not on a proven same-SSID setup.

More importantly, `ResonanceMaint-9E5AF0` appeared in the OS WiFi scan after `U`. That
means peer `9E5AF0` is still running a deprecated `NB_MAINT_AP` image, despite the desired
test being shared-WiFi only. Treated that peer as AP-contaminated and did not use its AP.
It will need USB flashing or a deliberate one-off recovery decision before it can
participate in a true shared-WiFi lower-bound test.

Peer `9E5AB8` did not expose a shared-WiFi endpoint either. It rebooted during the
maintenance-entry window and came back with `reset_reason=task_watchdog` at about
3.03 V, then continued heartbeating around 3.01-3.02 V. This establishes a practical
lower-bound result for the old running image: around 3.02-3.03 V loaded is too low for
reliable shared-WiFi maintenance entry on that image. It is a pre-upload failure, not an
OTA transfer failure. After capturing the evidence, sent targeted sleep
`P9E5AB8:21600`; its heartbeat age climbed, confirming it parked instead of continuing
to drain the low LFP cell.

Follow-up USB cleanup: corrected the local, gitignored `firmware/net_bench/wifi_secrets.h`
from the stale `Brandon Springs Activity Guest` SSID to `BubbyNet`, matching the laptop's
current shared WiFi. Built and USB-flashed COM4, which enumerated as `9E5AB8`
(`D8:85:AC:9E:5A:B8`), with the same `.5` non-AP peer image. Binary string check:
`ResonanceMaint` absent, `maintenance WiFi up` present, `BubbyNet` present, stale Brandon
Springs SSID absent, `.5` version present. Serial boot banner confirmed
`net-bench-2026-06-29.5`, role peer, channel 11, node `9E5AB8`, LFP 6000 mAh / 1500 mA,
env/INA sensors present, and direct entry to `COMMS (ESP-NOW)`.

Important caveat: the visible self-AP was `ResonanceMaint-9E5AF0`; the USB board just
flashed was `9E5AB8`. Therefore `9E5AB8` is now known-clean for shared-WiFi maintenance,
but `9E5AF0` should still be treated as AP-contaminated unless it times out back to comms
and is positively identified as a non-AP build, or is USB-flashed too.

Second USB cleanup: Ben connected `9E5AF0` as COM6. USB serial HWID/MAC identified it as
`D8:85:AC:9E:5A:F0`; flashed the same verified `.5` BubbyNet non-AP peer image. Serial
boot banner confirmed `net-bench-2026-06-29.5`, role peer, channel 11, node `9E5AF0`,
LFP 6000 mAh / 1500 mA, no env/INA sensors on this board, watchdog enabled, and direct
entry to `COMMS (ESP-NOW)`. A fresh WiFi scan showed only `BubbyNet` and no
`ResonanceMaint-*` SSID. Both previously live peers (`9E5AB8` and `9E5AF0`) are now
known-clean non-AP `.5` builds for the next shared-WiFi OTA test.

Next shared-WiFi lower-bound test should start only after:

- both target peers are confirmed non-AP builds (no `ResonanceMaint-*` SSID can appear);
- `wifi_secrets.h` matches the bench SSID the laptop is actually on, or a dedicated
  portable router SSID;
- at least one peer is already on `.5` or newer so maintenance-entry watchdog feeding and
  immediate OTA-start failure resume are present;
- the test records the pre-`U` loaded voltage and INA voltage/current, then runs
  `net_bench_ota.py --reboot comms` only against discovered shared-WiFi `/telemetry` IPs.

## 2026-06-29 - Codex - OTA maintenance-entry hardening after flaky low-battery attempts

Onboarded against the current repo context and traced the recent OTA failures against the
validated 2026-06-08 path. Read: standard OTA + rollback remains validated; the recent
failures happened before firmware transfer, while entering/discovering maintenance mode
from a low or poorly powered peer.

Updated `firmware/net_bench` to `net-bench-2026-06-29.5`:

- peers now report a maintenance-entry power preflight before leaving ESP-NOW. Advisory
  floors are LFP >= 3.20 V, Generic_3V7 >= 3.60 V, or an accepted supply current >=
  250 mA, but enforcement defaults OFF (`NB_MAINT_POWER_ENFORCE=0`) so the low-voltage
  OTA lower bound can be measured instead of guessed. A below-advisory peer sends one
  heartbeat with `mt=2` before attempting maintenance.
- if WiFi/AP OTA startup fails, the peer immediately resumes ESP-NOW instead of waiting
  for the long maintenance timeout.
- the OTA upload route now feeds the task watchdog during POST/upload handling, and AP
  startup feeds the watchdog too. The earlier `.4` WiFi-association watchdog fix remains.
- heartbeat/bridge telemetry gained maintenance status (`mt=`), and the dashboard/log
  parser records it. Dashboard marks `OTA power warn` / `OTA start failed` when present.

Verification: compiled an LFP peer image and a serial-bridge master image with isolated
Arduino build paths; both compile. `python -m py_compile` passes for
`net_bench_dashboard.py`, `net_bench_log.py`, and `net_bench_ota.py`.
Promoted the Arduino parallel-compile/cache collision and deprecated `--maint-ap`
warnings into the top of `AGENTS.md` so future sessions see them during onboarding, and
changed `firmware/net_bench/build.sh` to use a unique temporary Arduino build path per
run so the safe behavior is built into the script.

Interpretation for field reliability: the old success case was real -- software-reset OTA
and rollback worked. The current task is to retire AP-mode confusion, use the shared-WiFi
parallel OTA path, and measure the real lower-voltage OTA boundary before turning any
voltage threshold into a blocking production policy.

## 2026-06-29 - Ben + Codex - Outdoor peer reflashed to parallel OTA path

Attempted a low-battery OTA setup on outdoor solar peer `9E5AB8` around 2.95 V. The
peer stopped normal ESP-NOW heartbeats after `U`, but no reachable maintenance AP or
shared-WiFi peer IP was found, so no OTA upload occurred. Treat this as a lower-bound
maintenance-entry/credentials-path failure, not a firmware-transfer failure.

Ben USB-plugged the outdoor peer as `COM4`; flashed `net-bench-2026-06-29.3` peer image
with LFP chemistry, 6000 mAh capacity, 1500 mA charger cap, channel 11, and 1 Hz
heartbeat. A first flash accidentally included the per-device maintenance AP fallback;
reflashed immediately without `NB_MAINT_AP`, using the local WiFi secrets include path so
maintenance mode remains the scalable shared-WiFi path. Boot banner confirmed node
`9E5AB8`, `net-bench-2026-06-29.3`, sensors present, and ESP-NOW comms. Binary string
check confirmed the image contains `maintenance WiFi up` and not `ResonanceMaint` or the
maintenance-AP path.

Added targeted bench config commands so dashboard row focus can become an address instead
of only a view filter: `C<id>:<mah>` targets capacity/gauge config and `G<id>:<mA>`
targets charger-current config, while bare `C<mah>`/`G<mA>` remain serial console fleet
broadcasts. Updated the dashboard to require one selected peer before sending capacity or
charge changes, and documented that `--maint-ap` is an emergency single-board recovery
mode only, not the fleet OTA path. Master compile and dashboard `py_compile` pass; the
USB bridge still needs to be flashed to `.3` before the new targeted UI commands can work
through the dashboard.

Follow-up after Ben plugged the bridge back in: reflashed COM7 (`9F26F8`) as the `.3`
serial bridge with `NB_SERIAL_BRIDGE=1`, channel 11, and 1 Hz default frame rate, then
restarted the dashboard at `http://127.0.0.1:8765/` and sent `R1`. Verified the new
targeted config path with no-op `G9E5AB8:1500`; the bridge printed
`target SET_CHARGE_MA 9E5AB8 1500 mA` and both peers stayed online. Live state then
showed `9E5AB8` around 3.03 V / 7% SOC with solid RSSI, but panel current still 0 mA and
the battery discharging roughly 0.5 W, so the current physical solar/charger condition is
not net-positive.

Promoted the bright-sun PowerFeather/BQ25628E input-latch gotcha from documented bench
knowledge to a firmware baseline. Added shared `firmware/powerfeather_solar_guard.h`:
it force-sets `REG0x17[0] VBUS_OVP=1` at charger init, watches for the stuck signature
(`supply_v` near panel Voc, `supply_good=false`, near-zero input current), and toggles
`EN_HIZ` to re-run input qualification without a physical unplug. Wired it into the
solar/charging Resonance sketches (`net_bench`, `power_bench`, `led_studio`) and updated
the firmware notes/TODO so future solar firmware treats the guard as mandatory baseline
practice. Remaining gate is bright-sun hardware validation of an automatic clear.

Added firmware-revision visibility to the net_bench dashboard. `net_bench` `.4` now
appends a fixed `fw_rev` tail to peer heartbeats, emits `fw=` on bridge master lines, and
bumps the ESP-NOW receive buffer from 64 to 96 bytes so the larger heartbeat is accepted.
`ops/bench/net_bench_dashboard.py` parses and renders firmware under each peer ID plus in
the master panel. Reflashed COM7 bridge `9F26F8` to `.4` and verified the live dashboard
shows `net-bench-2026-06-29.4` for the bridge; existing `.3` peers correctly show `fw ?`
until they are updated.

Attempted shared-WiFi maintenance entry again before peer OTA. No `/telemetry` endpoints
were reachable; outdoor peer `9E5AB8` came back with `task_watchdog`, indicating the old
`.3` peer can trip the 8 s watchdog while blocked in the 20 s WiFi-join loop. Patched
`.4` to feed the watchdog during WiFi association in both maintenance and master WiFi
joins. Peer OTA is deferred until USB flash or a known-good quick maintenance join puts
at least one peer on the watchdog-safe image.

## 2026-06-29 - Ben + Codex - Repo text normalized to ASCII

Normalized tracked text files to ASCII equivalents to avoid Windows/codepage mojibake
when agents or shell tools print project docs. Replaced Unicode punctuation and symbols
with plain forms such as `--`, `->`, `>=`, `<=`, `deg`, `ohm`, `u`, `x`, and ASCII tree
drawing. Kept binary assets and live bench data out of the mechanical rewrite.
Added an `AGENTS.md` style note asking future agents to keep Markdown/docs ASCII-only
unless Unicode is project-critical.

Verification: `rg -nP "[^\x00-\x7F]"` over tracked text now returns no hits, literal
mojibake glyph scan returns no hits, `git diff --check` is clean aside from CRLF warnings,
and all changed `ops/bench/*.py` scripts pass `python -m py_compile`.

## 2026-06-29 - Ben + Codex - Dashboard radio-rate and solar-nap controls

Added live dashboard controls for reducing ESP-NOW bench overhead while a solar peer is
trying to recover from a near-empty LFP. `ops/bench/net_bench_dashboard.py` now exposes
quick `R1`/`R2`/`R5`/`R10` buttons, a custom `R<hz>` heartbeat-rate input, and a selected
peer `Nap` control that sends `P<id>:seconds`. The dashboard command validator now accepts
bounded `R1..R100` and targeted `P<id>[:seconds]` commands.

Updated `firmware/net_bench/` to `net-bench-2026-06-29.2`: the serial bridge can now set
a direct radio/frame rate with `R<hz>`, and peers that have the matching build can accept
a targeted `NB_TARGET_SLEEP_FOR` packet and enter timed deep sleep while other peers keep
running. Documented both commands in `firmware/net_bench/README.md`.

Built both master and peer images. Flashed the COM7 bridge (`9F26F8`) with the new master
image, restarted the dashboard at `http://127.0.0.1:8765/`, and sent `R1`. Live telemetry
confirmed the outdoor solar peer `9E5AB8` still reports around 2.93 V / SOC 0% with net
positive charge, but its awake load remains roughly 0.35 W; lowering heartbeat cadence is
not the same as sleeping the MCU/radio. The bigger recovery lever is `P9E5AB8:3600`, but
that requires flashing the outdoor peer to the new peer image first, preferably over USB or
after it has enough charge for a safe maintenance window.

Bench note: after the bridge work, current telemetry showed indoor peer `9E5AF0` no longer
actively drawing down (`drawdown_active=false`, `drawdown_mah=0.0`) while still alive around
45% SOC. Treat the earlier 7200 mAh HEX drawdown run as interrupted/invalidated unless a
separate JSONL review says otherwise.

## 2026-06-29 - Ben + Codex - Multi-peer dashboard focus polish

Updated `ops/bench/net_bench_dashboard.py` so the local net_bench dashboard behaves
cleanly when multiple peers with different telemetry capabilities are online. Added an
All/peer focus selector, metric source labels, and capability-aware top-card selection:
panel and charger cards now stay sourced from the peer that actually has panel/supply
telemetry, while All view shows net battery power across fresh peers. Selecting the
indoor HEX drawdown peer now shows stable "no panel telemetry" instead of flickering
between panel data and missing fields as heartbeats alternate.

Condensed the peers table into grouped `link` / `battery` / `supply` / `panel` / `state`
cells so both the outdoor solar peer and indoor drawdown peer fit together without a
horizontal scrollbar at the normal dashboard viewport. Restarted the COM7 dashboard and
verified the live page in-browser with both peers present; the drawdown logger and peer
continued running.

## 2026-06-29 - Ben + Codex - Targeted 7200 mAh HEX drawdown started

Onboarded against the current repo context, then prepared the Amazon 7200 mAh LFP /
PowerFeather / HEX stack for tomorrow's P105 full-sun demand-limit test. Found the old
LED Studio image on the LAN at `192.168.4.30`; ARP mapped it to MAC
`d8:85:ac:9e:5a:f0`, so the net_bench node id is `9E5AF0`. Brief LED Studio probes
showed the battery around 3.30 V under the red-ring load and about `-0.75 A` with the
HEX at all-white brightness 128.

Added a targeted `net_bench` drawdown command, `D<nodeid>[:mah]`, so the serial bridge can
start a HEX load on one peer without disturbing other live peers. The peer integrates
discharge current in firmware, advertises `dd`/`ddb`/`dda` in the bridge line, stops on a
mAh budget or guarded LFP voltage floor, explicitly blanks the SK6812 frame, cuts rails,
and timed-sleeps for 12 hours. Updated the dashboard command whitelist/parser and
`net_bench_log.py` so JSONL captures `cap`/`chg` and drawdown fields.

Flashed the COM7 serial bridge (`9F26F8`) with `net-bench-2026-06-29.1`, then OTA-flashed
`9E5AF0` from LED Studio to:

`--role peer --channel 11 --chem lfp --cap 7200 --charge-ma 1500 --hb-hz 1 --maint-ap`

Started logger
`ops/bench/data/ca/2026-06-29-ca-lfp-7200-hex-drawdown-9E5AF0.jsonl` and sent
`D9E5AF0:3500`. Initial drawdown telemetry: `bv` about 3.22 V loaded, `ima` about
`-0.84 A`, `drawdown_active=1`, with existing solar peer `9E5AB8` unaffected. Expected
end condition is either about 3500 mAh delivered or the guarded loaded-voltage floor, then
12 h sleep to preserve the hungry battery for the next full-sun P105 run.

## 2026-06-29 - Ben + Codex - Voltaic ETFE outdoor MPP comparison

Ran the local power dashboard against the PowerFeather solar telemetry peer on `COM7`
with the Voltaic 5 W ETFE panel (`P105`) and the smaller Voltaic ETFE panel (`P126`),
both into a 2 Ah LFP that was hungry enough to accept real charge current. Data was logged
to `ops/bench/data/ca/2026-06-29-ca-lfp-6000-net-solar-telemetry-1hz-2118.jsonl`
(run label still says `lfp-6000`; the live peer config was changed to `C2000` for the
2 Ah pack).

Findings:

- `P105` 5 W ETFE: with about 15 deg tilt, best observed region was around `m46`/`m48`.
  At `m48`, panel-side INA was about 5.1-5.3 V and 0.73 A, roughly 3.8-3.9 W. Charger
  input was about 3.47 W and battery-side charge about 3.1-3.2 W. Raising toward `m52`
  lost power. This is less surprising against the P105 datasheet expected `Vmp` near
  4.69 V than against the storefront headline values. Remaining caveat: the 5 W run may
  still be battery-acceptance-limited; LFP near 3.55-3.6 V can enter CV/taper or hit
  terminal-voltage limits early, especially on a smaller/higher-IR cell.
- `P126` smaller ETFE: all results were with about 15 deg tilt. Best observed region was
  around `m58`; panel-side INA reached about 6.1 V and 0.31 A, roughly 1.89 W, and
  charger input was about 1.66-1.68 W. `m60`/`m62` fell off. The panel is proportionally
  close to its nominal 2 W rating in real hot/late-day conditions.
- MPP matters materially for both panels. The 5 W panel gained roughly 0.4 W charger-side
  from a poor/higher setpoint to best; the 2 W panel gained roughly 0.2 W from `m48` to
  best. As a daily-energy term, that is about 1-2 Wh/day over a 5-full-sun-hour heuristic.

Interpretation: the Voltaic ETFE panels look promising for the BOM, especially the small
panel for HEX fixtures. Use panel-side INA as panel-capability truth when available; use
charger input/battery current as system truth. For a cleaner P105 verdict, re-run with the
larger 6-7.2 Ah LFP intentionally discharged to a mid-SOC/hungry voltage region so charger
taper and cell IR are less likely to cap demand. The stair-step sweep results also make a
simple periodic software MPPT/hill-climber worth implementing and measuring.

## 2026-06-29 - Codex - Re-onboarded and reviewed OTA/stuck-device failure modes

Re-read the session-start project context (`README.md`, `LOG.md`, `TODO.md`,
`docs/block-diagram/SYSTEM.md`, the OTA/LED/PowerFeather ADRs, `POWERFEATHER_NOTES.md`,
`net_bench` docs, and the brownout/networking test notes) after context compaction.
Current state is consistent with the earlier onboarding: PowerFeather V2 remains the
validated reference, direct-GPIO LEDs remain the production LED interface, battery-only
standard OTA with rollback is feasibility-green, and the hardening work is now about
guardrails around power state, charger input qualification, rollback health, low-battery
maintenance entry, and field recovery operations.

Reliability read: the recent low-battery maintenance-AP experiment should be treated as a
boundary warning, not a refutation of the validated OTA path. It showed that an
always-awake, deeply depleted peer can brownout during AP/maintenance transition, and that
a single-WiFi laptop cannot both join the peer AP and keep Codex/backend connectivity. The
production answer should keep the default shared-router OTA path for parallel updates,
retain self-AP as a one-device recovery lane, gate maintenance on voltage/current/supply
state, and keep USB/pogo or at least external USB-power recovery as the guaranteed last
resort.

## 2026-06-29 - Ben + Codex - Recovered solar peer over USB with USB-safe VINDPM

After the low-battery maintenance-AP experiment stranded peer `9E5AB8`, unplugged the
USB serial bridge and connected the peer directly as `COM4`. Direct-flashed the updated
`net_bench` peer image (`--role peer --channel 11 --maint-ap --chem lfp --cap 6000
--charge-ma 1500 --hb-hz 1`) with `--maintain 4.6` instead of the prior 5.2 V default.
Flash succeeded and verified on MAC `d8:85:ac:9e:5a:b8`.

Rationale: a 5 V USB power bank/USB source can be blocked or heavily current-limited when
the charger's input-regulation/maintain setpoint is above the source voltage. Keep the
boot default USB-recovery-safe (about 4.6 V) and raise VINDPM live with `m<v10>` only
during panel MPP testing, or implement a persisted setting with a USB/supply-voltage clamp.

## 2026-06-29 - Ben + Codex - Low-battery maintenance-AP OTA boundary test

Tried to push the current `net_bench` peer image to solar telemetry board `9E5AB8`
while it was intentionally deep in the low-battery region. Starting point before the
maintenance command: about 2.57 V, SOC 0 %, and ~0.45-0.50 W net load. The peer heard the
bridge's sustained `ENTER_MAINT` and stopped fresh ESP-NOW telemetry. It advertised the
expected `ResonanceMaint-9E5AB8` AP briefly enough for Windows to connect when given an
explicit profile, but the Codex laptop cannot stay reachable to the backend while its only
WiFi interface is joined to the peer AP.

After returning the laptop to BubbyNet, the bridge showed the useful boundary result:
the peer had brownout-reset, emitted only two fresh post-brownout heartbeats at about
2.33 V (`rr=brownout`, uptime ~3.3 s), and then went stale. This does **not** prove that
a full OTA upload cannot ever complete at low battery, because the single-WiFi laptop
constraint prevented the upload attempt. It does show that this starting point is below a
comfortable maintenance-entry floor for the always-awake bench image: AP startup /
maintenance transition alone was enough to hit a brownout-adjacent state. Retest the full
upload with external power or a second host network interface. The temporary Windows
maintenance-AP profile was deleted and BubbyNet auto-connect was restored.

## 2026-06-29 - Codex - Onboarding pass

Read the session-start orientation path (`README.md`, `LOG.md`, `TODO.md`,
`BACKGROUND.md`, `docs/block-diagram/SYSTEM.md`, ADRs 0001-0022, root `ROADMAP.md`,
and the PowerFeather/networking/Voltaic notes) before taking on new work. Current state:
PowerFeather V2 is the validated COTS/reference architecture; ESP-NOW, battery-only OTA
with rollback, watchdog recovery, rails-off sleep, and the solar charge path are green.
The active gates remain role-specific energy sizing, BQ25628E VBUS_OVP/HIZ guard,
Voltaic P105/P126 outdoor tests, HEX/RGBW type mix and placement, HEX 4.2 V boost,
mock-hat RF, sealed-hat thermal behavior, and production firmware hardening.

Noted existing uncommitted WIP adding runtime `net_bench` battery capacity and
charge-current config (`C<mah>` / `G<mA>`) plus peer timed sleep and dashboard support;
left that work intact.

## 2026-06-28 - Ben + Codex - Runtime battery capacity and charge-current config for net_bench

Added NVS-backed bench config to `firmware/net_bench`: `C<mah>` broadcasts a battery
capacity update to peers, persists it, and reboots them so `Board.init()` applies the new
MAX17260 gauge capacity; `G<mA>` broadcasts/persists the charger current cap and applies
it live. Heartbeats now carry `cap=`/`chg=` so the serial bridge/dashboard can verify the
peer's active config after OTA. Chemistry remains build-time because the charge-voltage
profile is safety-critical.

Updated the local power dashboard with capacity/charge controls and parser support. This
is primarily for swapping the 2 Ah bench LFP and the fullbattery.com 32700 6 Ah LFP during
solar-panel tests without rebuilding firmware for a simple constant.

## 2026-06-20 - Codex - Onboarding pass and PowerFeather SDK 2.1.1 review

Read the current repo orientation path (`README.md`, `LOG.md`, `TODO.md`,
`BACKGROUND.md`, `docs/block-diagram/SYSTEM.md`, and the active PowerFeather/LED/OTA
ADRs) before reviewing the PowerFeather-SDK 2.1.1 release. Current state remains:
PowerFeather V2 is the validated COTS/reference path; ESP-NOW, battery-only OTA +
rollback, watchdog recovery, and solar charge path are feasibility-green; active gates are
bottom-up role-specific energy sizing, BQ25628E VBUS_OVP/HIZ charger guard, Voltaic panel
tests, HEX/RGBW placement, boosted-HEX characterization, mock-hat RF, thermal, and
production firmware hardening.

PowerFeather-SDK 2.1.1 is a narrow MAX17260 time-estimate fix plus version bumps. The
MAX17260 driver now preserves raw `0xFFFF` for time-to-empty/time-to-full so
`Mainboard::getBatteryTimeLeft()` returns `Result::NotReady` instead of a bogus large
estimate. Resonance impact is limited to `firmware/power_bench/` telemetry
(`time_left_min`) and the older `powerfeather_demo_port` UI; `net_bench`, `led_studio`,
charger/VINDPM behavior, OTA, sleep, LED control, and mesh feasibility are not touched.
Recommendation: update bench machines from SDK 2.1.0 to 2.1.1 when convenient, but no
architecture or firmware changes are required.

## 2026-06-17 - Codex - Reconciled stale architecture docs before commit

Cleaned up stale overview context that still pointed at the early ESP32-C3/CN3058/AP2112K
and IS31-primary direction. Added ADR 0022 to record the LED fleet decision from the gobo
session: use both HEX and 4 W RGBW point-source modules by optical role, with type mix and
placement still open. Rewrote the canonical system architecture/power-budget doc around
PowerFeather V2, BQ25628E/MAX17260/TPS631013, direct-GPIO LEDs, role-specific panel sizing,
and the still-open energy/thermal/RF gates. Updated the hardware README, BOM skeleton,
roadmap, references, glossary, README status, and TODO entries to match the current state.

## 2026-06-17 - Codex - Onboarding pass

Read the repo orientation path (`README.md`, latest `LOG.md`, `TODO.md`, `BACKGROUND.md`,
`docs/block-diagram/SYSTEM.md`, key ADRs, current test notes, and bench/tool README files)
to re-establish the live state before taking on implementation work. Current mental model:
PowerFeather V2 remains the validated COTS/reference architecture; networking, solar path,
and battery-only OTA/rollback are feasibility-green; the active gates are panel/cell sizing,
LED role split and placement, VBUS_OVP/HIZ charger guard, mock-hat RF/thermal, and production
firmware hardening. Noted existing uncommitted work on Voltaic ETFE testing, PowerFeather
SOC cautions, net_bench docs, and the new serial-bridge dashboard; left that WIP untouched.

## 2026-06-15 - Ben + Codex - Travel maintenance AP committed; Voltaic ETFE panel prep captured

Remote travel bench update. `net_bench` now has a `--maint-ap` option for client-isolated
networks: the normal/parallel path remains shared WiFi maintenance mode, but a field peer
can now enter maintenance by advertising `ResonanceMaint-<nodeid>` and serving OTA at
`192.168.4.1`. The master serial-bridge path stays useful on USB for telemetry and the
field peer can remain pure ESP-NOW until maintenance is requested. Also widened live
`SET_MAINTAIN` (`m<v10>`) to the PowerFeather SDK range, 4.0-16.8 V, so high-Vmp panels
such as Voltaic P126 can be swept without a reflash.

Captured tomorrow's Voltaic ETFE test prep in
`docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md`: P105 5 W and P126 2 W source specs,
derived size/weight/cost comparisons, P105-vs-P126 BOM read, and a concrete outdoor run
shape for the COM7 serial bridge plus INA-instrumented peer. Key warning for the run:
both panels have Voc above the BQ25628E default low input-OVP threshold, so a no-charge
result may be the known bright-sun input qualification latch until the VBUS_OVP/HIZ-kick
firmware item is handled.

## 2026-06-12 (cont. 3) -- Ben + Claude -- Interactivity/presence sensing: option space mapped (Elliot ask)

Elliot (project lead) saw the 06-11 LED demo and asked for presence detection /
interactivity -- "what makes people spend quality time at the tree." Full landscape in
`docs/research/PRESENCE_SENSING_INTERACTIVITY_2026-06-12.md`; headlines: it's ART not
security (false positives are benign -> ~80 % reliability = success); the product is the
MESH choreography (PRESENCE event + ripple, the packet layer already fits); primary
candidate = downward VL53L1X ToF eye (sway-robust, ~$3, ~zero power, but needs a port
next to the gobo aperture -- Steve); radar = through-enclosure (no dust exposure) but
LED-show-class power unless duty-cycled + self-sway artifacts (IMU veto); the FREE
experiment = mesh-RSSI presence (bodies attenuate 2.4 GHz ~20 dB, already in every
heartbeat). Bench kit ~$10, test plan Steve-compatible, TODOs queued.

## 2026-06-12 (cont. 2) -- Ben + Claude -- HEX 4.2 V boost direction (TPS63802); revised HEX budget; Steve-runnable bench TODO pushed

Remote session while Ben travels. Two outcomes, both queued as Steve-runnable TODOs
(Steve has duplicate components + Claude Code on his end; data site code `tn`):

**Revised HEX budget -- the gobo looks are cheap.** Ben's verdict: HEX looks best as
1 px white or 3 px single-channel (plus trails). Measured: 1 px full = 41.8 mA
(~0.12 W rail), 3 px ~105 mA (~0.3 W) -> ~0.4-0.6 W battery-side with overhead =
**all-night on the 3 W panel, in-tree**. Yesterday's 2.1 W HEX row was the all-37 case
only; in its actual role the HEX lantern runs as cheap as the RGBW one.

**4.2 V V+ boost (TPS63802) -- why and how.** At the sagged ~2.9 V rail the SK6812
blue/green drivers are in dropout (Vf 3.0-3.2 + ~0.5 V headroom needed) -> starved
channels = the goldening + ~25-30 % current deficit. A regulated 4.2 V V+ should give
**~+40-60 % white lumens** (blue/green recovery, V(lambda)-weighted), restore color
balance, and make looks **SOC- and fixture-invariant** (also quietly solves the
Community-Mandala brightness-normalization concern). Key constraints discovered:
- **4.2 V, NOT 5 V**: WS-data VIH = 0.7 x VDD; 5 V supply -> 3.5 V threshold breaks
  3.3 V GPIO data. 4.2 V -> 2.94 V = in spec, no level shifter.
- **TPS63802 module** (TI buck-boost, 1.3-5.5 Vin, 2 A, output-select solder jumpers):
  re-bridge 3V3->4V2 (fully open 3V3 first; meter unloaded). Cheap boards don't break
  out EN (tiny pad only) -> bench version feeds from the **switchable 3V3** (kill-switch
  inherited via GPIO4); the **VBAT-fed + EN-on-GPIO** single-conversion variant is the
  production architecture, to live on the NeoHEX adapter PCB rev. PS pad = power-save
  mode select; leave default (PFM efficiency matters at dim ambient).
- **Count-cap required on boosted builds**: all-37 white at regulated 4.2 V ~ 2 A out --
  far beyond module + rail. Firmware n-cap before anyone maxes "all".
- Rail-vs-pixel bottleneck clarified: at 1-3 px the limit is per-pixel undervolt (boost
  fixes); the converter/rail limit only binds at high counts.
- LM2596-class = buck, wrong direction; MT3608-class = acceptable fallback (pot-set,
  drift risk -- preset jumpers preferred for fleet).

Decision data wanted from the bench (Steve): PAR + INA lumens-per-system-watt, 2.9 vs
4.2 V, 1/3 px, per-channel, at two SOCs. TODO has the full procedure.

## 2026-06-12 (cont.) -- Claude -- BQ25628E datasheet read: the Voc ceiling is a REGISTER BIT; "6V" panel class unblocked

Datasheet (SLUSFA4C) electrical characteristics resolve yesterday's bright-sun latch and
the panel voltage window:
- **V_VBUS_OVP is selectable**: VBUS_OVP=0 (POR default) -> 6.1/6.4/6.7 V rising;
  VBUS_OVP=1 -> 18.2/18.5/18.8 V. Our connect-time Voc 6.15 V tripped a min-spec part at
  the default setting. Chip operating range is 3.9-18 V (26 V abs max) -- the ~6 V ceiling
  was configuration, not silicon.
- **Input qualification is EDGE-triggered** ("power up from input source" sequence runs at
  insertion): explains why shading to 4.7 V did NOT recover but full VBUS removal did.
  EN_HIZ toggle should synthesize a fresh edge (8.3.4.3) = the firmware re-qual kick.
- Other gates are non-issues for panels: poor-source test = >=3.6-3.75 V at <=10 mA;
  sleep-exit = VBUS > VBAT + 0.115-0.34 V; UVLO rising 3.2-3.5 V.
- Bonus confirmations: chip charging defaults VREG 4.2 V / ICHG 320 mA (the exact
  led_studio-uninitialized LFP hazard, now in writing); chip-level VINDPM floor is 3.8 V
  (the 4.6 floor is the SDK clamp); VINDPM_BAT_TRACK = VBAT+400 mV dynamic floor option.
- **Supersedes the "narrow viable window" panel conclusion from earlier tonight**: with
  VBUS_OVP=1 + the requal kick (now a procurement-prerequisite TODO), the spec is
  Vmp(STC) >= ~5.4 V, Voc <= ~16 V -- the standard 6 V class (incl. Voltaic's 6 V ETFE
  line) is fully in-window. Current was never a constraint (2 A charge ceiling; charger
  draws only what it needs).

## 2026-06-12 -- Ben + Claude -- Gobo verdict: BOTH LED types, by role; full-brightness budget sketch

**Gobo session result (Ben, inverted-lantern rig, dark):** both modules are excellent for
DIFFERENT roles -- the LED-axis answer is a MIXED FLEET, not a winner.
- **HEX (37x SK6812):** beautiful animations, dancing patterns, the color-channel
  separation (Split) modes shine -- but it reads best within ~6 ft; at 10-15 ft the color
  washes out and patterns lose crispness. The intimate/close-range module.
- **4 W RGBW point source:** crisp and beautiful even at 15 ft; the color fringing acts
  like a Venn diagram -- overlap regions mix into NEW colors, far richer than plain
  R/G/B edge fringing. The long-throw/gobo module.
- Direction: lanterns of both types. Feeds ADR 0018 (update it to record both-by-role
  and the placement question: which heights/positions get which module).

**Full-brightness budget sketch** (gamma off, bri 255; measured LED-rail draws + 0.2 W
assumed production overhead, /0.85 converter; harvest = derated effective-solar-hours
estimate pending the dawn-dusk log): HEX-full ~2.1 W battery-side; 4W-module RGB-full
~1.1 W; W-only ~0.45 W. Sustainable hours/night on the 3 W panel in-tree (unshaded):
HEX 1.8-3.0 (2.4-3.6); RGB 3.6-6.0 (4.8-7.3); W-only all night. 5 W panel scales x1.67:
HEX 3-5 h, RGB ~whole-night. The 32700 (18 Wh usable) banks 3-5 nights of sustainable
show -> single nights can splurge and repay. 5 W buys storm-recovery margin more than
capability. Caveats: shading factor dominant unknown; production overhead unmeasured;
HEX "full" is rail-limited (stiffer cell = brighter AND hungrier).

**Panel-shopping spec (from the BQ25628E limits + bench):** buck-only charger ->
panel hot loaded Vmp >= 4.6 V (= the SDK's VINDPM floor; sub-4.6 setpoints are silently
REJECTED -- which re-explains the 06-11 "4.4 V collapse": those points measured a stale
setpoint, not 4.4; LOG cont. 2/3's below-the-knee story is corrected accordingly, and the
4W-cam panel's "flat no-knee curve" below 4.6 was the same artifact). Voc ceiling: input
qualification failed at ~6.05-6.15 V and latched (the bright-sun gotcha), accepted 5.43 V
-> as-configured ceiling ~6 V-ish; datasheet ACOV verification is now PROCUREMENT-GATING
(if fixed ~6.3 V, the standard "6V" panel class (Voc 6.8-7.4) can never qualify at
open-circuit and no firmware kick saves it; viable window narrows to Voc(STC) <= ~5.8 =
the Seeed's class). Current is a non-issue (BQ = 2 A charge max; charger draws only what
it needs). Voltaic P139 (Voc 2.76) = boost-ecosystem class, unusable on a buck charger.

## 2026-06-11 (cont. 3) -- Ben + Claude -- 32700 VERDICT: 5726 mAh (95 % of rating) = the production cell passes; "4 W" camera panel = a 1 W panel in a trench coat

**32700 6 Ah LFP capacity (the production-cell gate): 5,726 mAh clean to a 2.473 V cutoff
over 7.16 h, ZERO resets.** Stitch: run 1 981 + ~6 (gap at ~124 mA idle after run 1's
parse crash) + run 2 4,739; **7 corrupt-but-parseable INA samples ablated** (-256 to
-343 A class; the raw script integral read 10.9 Ah -- reconcile-before-believing, again).
Cross-check: the gauge's own run-2 integral came out **+8.5 % above the clean INA** --
the MAX17260 current bias replicating for the **6th consecutive session** (+8 +-1 %,
both directions, both cell types; the /1.08 software correction is now very solid).
**Verdict: PASS at $5.10 ($0.89/delivered-Ah)** -- 95 % is ratings-tolerance territory on
a first cycle with a conservative cutoff. Qualify a 2nd sample from the batch before the
100-unit order (n=1), but this is the production cell unless that surprises. Notable:
under the fading HEX load the cell rode the whole tail gracefully (load self-dimmed
605 -> ~250 mA as the rail sagged) -- zero brownout resets, vs the mule's 44-reset
cascade under the stiffer RGBW point-source load.

**"4 W" ring-camera panel bake-off (Ben's economy-of-scale candidate): rejected, with
numbers.** Voc only ~5.45 V hot at the connector (10-cell panel + blocking diode -- Ben
visually confirmed diode-only in the housing). Flat-mounted: a dead-flat ~0.28 W from
VINDPM 4.6 down to 4.0 (current-source-starved, ~65 mA -- no knee at all). Tilted
square to the sun at 4.6 V: 0.579 W in ~57 klux -- scaled to full sun ~1.0-1.1 W real
capability = **~4x overrated**, plus bezel self-shading when flat (tilting doubled
output, more than geometry alone explains) and the diode tax. Apples-to-apples the
Seeed 3 W delivers ~4x the real harvest. The 10-minute sweep harness is now the
panel qualifier: any candidate (incl. the ETFE panels) earns its place through it.
(Sweep-tooling fixes from the session: anchor-all-zero ZeroDivision guard; "restore
5.5 V on exit" bit us twice when the next panel's window sat below 5.5 -- the live
check is `sgood=1` + `sma=0` = setpoint above the panel's window, send `m46`.)

Misc: lux-sensor bump mid-session produced a fake 30x light drop (the 3.5 klux
"shade" reading) -- worth a mount for the TSL. Sun-angle context for today's numbers:
3:40 pm flat-mount cosine loss ~18 % + cell ~65 deg C temp derate ~16 % fully explains
"2 W from a 3 W panel" -- the Seeed performs AT rating once physics is applied.
Tomorrow: cool-AM Seeed sweep (Vmp(T) -> MPPT decision), then a dawn-to-dusk harvest
log = measured effective-solar-hours (the fixture-specific derate of Ben's 5-h
heuristic; pre-derate estimate ~2-3 h flat, ~1.5-2.5 in-tree).

## 2026-06-11 (cont. 2) -- Ben + Claude -- FIRST WIRELESS MPP SWEEP: hot-panel optimum 4.6-4.7 V = ~3x the default harvest; bright-sun input-latch gotcha; 32700 verdict pending

**The harvest question (the sizing campaign's last unmeasured term) now has its hot-panel
answer.** Full instrument cluster on the outdoor peer -- TSL2591 (lux; saturated in full
sun, IR ch1 used as the normalization channel), SHT31 taped to the panel back
(60-61 deg C; IR gun front 155-157 deg F ~ 68 deg C -> ~8 deg C front-to-back offset, cell ~64-66 deg C),
and Ben's idea of SEN0291 INAs on the peer's OWN STEMMA bus (panel + battery leads,
heartbeat tail 3, fw 2026-06-11.2) -- zero outdoor tether, all data over ESP-NOW.

- **Curve (bright sun ~3:40-4:15 pm, panel ~60 deg C back):** 5.5 V -> 0.59 W; 5.2 -> 1.18;
  5.0 -> 1.45; 4.9 -> 1.55; 4.8 -> 1.66; 4.7 -> 1.69; **4.6 -> 1.73 W (best, both sweeps)**.
  The default 5.5 V harvests **31 % of optimum (3.2x available)** -- worse than the
  06-08 cloud-confounded ~2.6x hint. Knee-bracket re-sweep reproduced 4.6 as peak
  (anchor drift 5 % with the improved 4.9 anchor).
- **Panel INA vs BQ telemetry: the BQ under-reports harvest ~10 %** (1.91 W panel-side
  vs 1.73 BQ-side at the peak) -- input-stage loss the sizing math must include. The
  self-instrumentation cross-check earned its keep on day one.
- **Below the knee:** sweep 1's 4.4 V point COLLAPSED (0.55 W, input parked near Voc --
  VINDPM below the hot panel's knee has no stable operating point when stepped to from
  near-idle); the re-sweep's 4.4 read 1.53 W but DEMAND-LIMITED (4.9/4.5/4.4 all
  identically 1.53 W late-session -- battery filling toward ~50 % and/or charger thermal
  foldback in the heat caps demand, making VINDPM moot). Conservative rule: **fixed
  setpoint >= 4.6 V hot, approach setpoint changes from above; run sweeps on a hungry
  battery.** Cool-AM session pending for Vmp(T) -> the fixed-vs-temp-comp-vs-P&O call.
- **NEW FIELD GOTCHA (production-relevant): connect/boot under bright sun latches the
  charger's input fault** -- panel sat at Voc ~6.0-6.2 V, sgood=0, zero draw; a hand-
  shade to 4.7 V did NOT clear it; only full VBUS removal (face-down/unplug) re-ran
  qualification. Captured in POWERFEATHER_NOTES + firmware-guard TODO (playa bring-up
  hazard at 100-fixture scale). Anchor methodology fix: anchor at 4.9 not 5.5 (the 5.5
  point is load-noise-dominated; first sweep's 25 % "drift" was that artifact).

**32700 capacity run (in progress at write time):** three CORRUPT-BUT-PARSEABLE INA
samples (-256 to -343 A, physically impossible) inflated the live integral -- caught by
reconciling the integral against instantaneous currents; clean re-integration =
**~4.8 Ah by the knee region, final pending the fading tail**. (An earlier in-chat
"~7 Ah, above rating" read was glitched data -- retracted within the hour. The 06-10
lesson again: reconcile integrals before believing them.) All four INA host scripts now
drop both mangled lines AND beyond-range values (the INA's +-4 A) at ingest. Notable
along the way: the HEX load FADES gracefully as the rail sags (zero resets in 5.6 h,
vs the RGBW point source's 44-reset brownout cascade on the mule) -- the two LED
architectures fail differently at end-of-charge.

Also: 9F2690 reflashed as serial-bridge master; mpp_sweep gained ir-ch1 fallback
(TSL2591 saturates in full sun even at min gain -- expected); peer INA address
convention: both-DIP-off = 0x40 = panel, both-on = 0x45 = battery (0x44 = SHT31).

## 2026-06-11 (cont.) -- Ben + Claude -- Pushed the HEX to the cliff: visual failure sequence mapped; protect latch validated live; 32700 charging for capacity test

**The aggressive ramps (Ben watching, ~20 % SOC mule cell).** After the guarded runs,
floors were dropped near hardware limits and the value ramp walked 37-px white from
141 mA up. Results, all INA ground truth:
- **Sustained ceiling ~480 mA** (val 208) at ~20 % SOC -- far above the morning's
  conservative floors; rail rode at 2.7 V (min 2.53) for whole steps without electrical
  failure. The step toward ~500 mA (val 224) ended it: **the firmware battery-floor
  protect latched mid-step** (rail CUT to 0 V, LEDs unloaded, WiFi off, no self-rejoin --
  the designed endpoint, needing a button; the brownout-reset path self-recovers). Every
  guard layer fired in design order across the night: script floors first, fw protect as
  the backstop, zero bricks.
- **Hot-step vs ramp asymmetry, quantified:** an idle->290 mA hot-step (n=10 @ full)
  brownout-reset the board instantly (rr=poweron) at the same SOC where a gradual ramp
  survived 480 mA -- a ~1.7x margin difference. "Ramp gently / no full-white hot-steps"
  is now a measured production rule, not folklore. (The danger zone also slid ~100 mA
  down as SOC fell 98 %->20 % -- the current cap must be SOC/voltage-aware or worst-case.)
- **Visual failure sequence (Ben, thick packing foam as diffuser -- too bright naked-eye):
  (1) subtle flicker (onset before any electrical flag), (2) subtle "goldening" of white
  (blue channel -- highest Vf -- starves first as the rail sags), (3) uneven lighting where
  the brightest CONTIGUOUS run of pixels jumps around every few seconds** (WS-protocol
  data corruption: pixels keep whichever frame last latched cleanly), then (4) the
  protect cut. All graceful-degradation modes -- nothing alarming below the cliff, which
  supports dim-don't-die low-battery behavior.
- First aggressive attempt (file `0549`) also caught the n=10@255 hot-step brownout live
  (board uptime reset mid-ramp; script bug fixed: failed /set now prints + retries once).

**32700 6 Ah LFP candidate -- charging overnight.** Board 9F2690 (the former bridge
master) flashed `power_bench --led none --cap 6000 --chem lfp` BEFORE cell connect (the
LFP flash-order rule), then the 32700 attached: charging at **+515 mA** (USB input-
limited), bv 3.328, supply_good. Gauge says SOC 99 -- ignore (un-learned, cycles=0,
plateau-blind); the REAL capacity test is tomorrow's full charge -> INA-coulomb
discharge (the validated 06-10 methodology). At fullbattery.com bulk ~$5.10 (~$0.85/Ah)
it's the leading production cell if it makes rating (ADR 0017 direction). Note: this
board's Wire1 scan shows an extra mystery device at 0x2A (others have only 0x36/0x6A) --
harmless so far, noted in case the board ever behaves oddly.

**Bench state for tomorrow's MPP sweep:** mule 2000 cell DISCONNECTED at ~20 % SOC
(ideal bulk-charge precondition); 9E5B0C powered off; a spare board still needs the
serial-bridge master flash (9F2690 got the new master fw tonight but was immediately
repurposed for the 32700); TSL2591 + SHT31 arrive early PM. The 32700 discharge can run
on the bench INAs in parallel with the outdoor sweep -- load/wiring decided in the
morning (HEX board swap vs RGBW on 9F2690).

## 2026-06-11 -- Ben + Claude -- Loose-ends night: RGB-3W = RGBW-4W on RGB; STEMMA cable verdict; HEX ground truth + the "all-on-max" instability explained

Bench session while waiting on the TSL2591/SHT31 delivery. Three loose ends closed.

**1. RGB-3W vs RGBW-4W: identical RGB top-end.** The new 3 W RGB module (no W channel)
at full r=g=b drew **256.5 mA** (3 cycles, +-0.3 mA) vs the 4 W RGBW's RGB-full
**257.3 mA** at the same ~2.8 V sagged rail -- 0.3 % apart. The "3 W vs 4 W" rating
difference is entirely the W channel (+66.5 mA standalone, +34 mA on top of RGB under
combined sag). W pattern on the new LED: ~0 mA (no channel, and the 4-byte RGBW frame
drives a 3-byte pixel fine -- first three bytes land). Shunt NOT backwards (Ben's worry):
both INA channels kept the original sign convention. Caveats: n=1 of each module;
tonight USB+charging vs 06-10 battery (rail sag happened to match, making it fair).
Data: `2026-06-11-afk-sweep-0028.jsonl` + `-power/gauge.png`.

**2. Metro STEMMA port: the CABLE was the whole story (port healthy).** The Metro
ESP32-S3's QT port is the same I2C bus as the headers (SDA=47/SCL=48, 10 k pullups, no
power gate; 0x36 on the bus = the Metro's own onboard MAX17048). STEMMA QT (GND,VCC,
SDA,SCL) and Gravity PH2.0 (VCC,GND,SCL,SDA) are **pairwise-inverted**, so a
straight-through adapter lands ALL FOUR pins wrong -- power reversed (the 06-09
dead-short/USB-kill incident, explained) AND SDA/SCL swapped. Ben re-matched the leads
-> all 4 INAs + the MAX17048 found and streaming **through the QT port** (which also
proves the port survived the 06-09 short). `ina_monitor` gained `s` (I2C scan) / `r`
(re-probe) serial commands for future bus debugging.

**3. HEX (37x SK6812) with INA ground truth -- and the "all-on-max instability" is a
BATTERY-SAG ceiling, not an LED/data failure.** power_bench `/set` gained `n=` (light
first n pixels; fw 2026-06-11.1, OTA'd battery-only -- another no-touch flash) and
`ops/bench/hex_ramp.py` ramps count (1->37 @ full) then value (n=37, 16->255) with
host-side abort guards (gauge-V floor primary, INA means secondary, board-reset/HTTP
the real detectors), backing off BEFORE the board browns out:
- Single pixel (INA, battery-only): **41.8 mA full white** (17.3 @ 64, 23.0 @ 128,
  34.2 @ 192). Ground-truth replacement for the gauge-based HEX numbers.
- Count ramp @ 255: safe through **n=10 (288 mA)**; n=14 (372 mA) tripped the gauge
  floor (3.008 V). Value ramp @ n=37: safe through **val 64 (261 mA)**; val 96
  (358 mA) tripped (gauge 2.980 V). Convergent: **~350-400 mA of LED draw pulls the
  bench cell's terminal to ~3.0 V even at 98 % SOC** (LED + ~150 mA WiFi system ->
  ~0.5 A battery draw; matches the 06-10 finding that brownout cascades start
  ~2.97 V under load). Per-pixel current self-limits as the rail sags (41.8 -> ~27
  mA/px at n=14), so all-37-full would NOT hit 37x41.8 -- but the cell dives first.
- Implications: (a) Ben's observed all-on-max instability = battery sag to the
  brownout zone; rail/data stayed fine to the guard floors (rail mean >= 2.79 V).
  (b) The ceiling is a CELL property (high effective IR incl. harness, ~0.7 ohm at
  0.5 A) -- the production 32700 ~6 Ah cell lifts it substantially. (c) Production
  firmware needs a **current cap** (brightness x lit-count) for burst modes --
  reinforces the existing cap-brightness TODO / ADR 0013 failsafe. We deliberately
  never drove it to an actual reset; the guards stop at early-warning floors, and the
  visual-flicker threshold (if lower) is a separate observation.
- Gauge current bias replicated again: **+7.4 %** this session (and +8.8 % on the
  CHARGE side in the morning run) -> ~+8 +-1 % across 4 sessions, both directions;
  the /1.08-ish software correction is solid.
Data: `2026-06-11-afk-sweep-0119.jsonl` (+pngs), `2026-06-11-hex-ramp-0128.jsonl`
(0126 = aborted first try whose floors were miscalibrated to transient WiFi dips --
kept for the record).

## 2026-06-10 (cont. 2) -- Ben + Claude -- MPP sweep goes fully wireless: TSL2591 lux + SHT31 panel-temp ride the heartbeat

Ben flagged the sweep's weak point: the Apogee PAR sensor is USB-tethered, so logging
light outdoors meant a laptop or a dedicated rpi at the panel. Three options weighed:

- **PowerFeather USB-C as USB-host to the Apogee: rejected.** The S3 silicon can do OTG
  host, but this stack runs TinyUSB in device mode, the Apogee is an FTDI-class device
  (needs a vendor VCP host driver under ESP-IDF, not Arduino), and the V2's USB-C is a
  charge/device input that doesn't source VBUS -- the sensor wouldn't even power up. A
  research detour, not a bench fix.
- **TSL2591 I2C lux module (arriving ~06-11): ADOPTED as the primary light channel.**
  Chained on the peer's STEMMA-QT, auto-probed at boot, lux appended to the heartbeat
  (append-only tail 2, `NB_PROTO_VER` unchanged -- same pattern as the supply fields) ->
  light data arrives over ESP-NOW with **zero outdoor tether**. Note "lux vs PAR": neither
  matches the panel's silicon spectral response -- both are *relative* normalization
  channels, which is all the sweep needs (absolute W comes from the anchors agreeing).
  The TSL2591's raw ch0/ch1 (full+IR) are logged too. **Saturation caveat:** full sun
  (~100k+ lux) can exceed its range even at min gain/integration; firmware detects and
  reports `lux=sat`; the fix is a paper/PTFE diffuser (fine for relative use). The Apogee
  remains an optional host-side cross-check for the indoor dry run (`--par-port`).
- **SHT31-D taped to the panel BACK: ADOPTED for continuous panel temp** (back-surface
  contact ~ cell temp - a couple deg C in sun; standard PV practice) -> `ptc=`/`prh=` in the
  heartbeat. The IR gun stays as the front-surface spot-check at anchors (the script
  still prompts). **Battery NTC** (the V2's 103AT thermistor on the charger TS pin) is
  exposed too (`btc=`) but **opt-in** (`--batt-ntc`): enabling TS with no thermistor
  attached makes the BQ apply JEITA to a floating pin and can SUSPEND CHARGING -- gotcha
  captured in POWERFEATHER_NOTES. With the NTC taped to the cell it doubles as hardware
  LFP charge-temp protection (a thermal-track freebie).

Implementation (compiled both roles; on-hardware validation when the sensors arrive):
`net_bench` fw 2026-06-10.1 -- env auto-probe + 1 Hz cache (TSL2591 read blocks ~120 ms,
so high-rate heartbeats reuse the cache), heartbeat tail 2, master bridge prints
`lux=/ch0=/ch1=/ptc=/prh=/btc=`; `net_bench_log.py` + `mpp_sweep.py` + `mpp_analyze.py`
parse them (host tooling re-validated end-to-end against a simulated master emitting the
new tokens). Sweep flags generalized: `light-saturated`, `light-unstable`, `no-light`.

## 2026-06-10 (cont.) -- Ben + Claude -- MPP-sweep tooling ready (next bench test); buck-boost show-load finding from existing data

**Decision: the next bench test is the clean full-sun MPP sweep** (the open TODO from
06-08 cont. 10). Rationale: with capacity, idle, and LED draw now measured, harvest is the
last unmeasured term in the battery/panel sizing equation -- and the dirty 06-08 sweep
suggests the default VINDPM 5.5 V may give up ~2x vs the hot-panel MPP (~4.85 V), i.e. a
potential ~2x panel-sizing error at 100 units, plus it settles the MPPT firmware decision.
Runner-up was the gobo session (evening-compatible, doesn't compete for sun).

**Tooling built + validated (no hardware in the loop yet):**
- net_bench master `m<v10>` -- explicit SET_MAINTAIN setpoint (e.g. `m48` -> 4.8 V) next to
  the bare-`m` cycle; range-checked to the peer's 40-58 accept window. Compiles; reflash
  the DESK master over USB -- the outdoor peer needs nothing.
- `ops/bench/mpp_sweep.py` -- guided session: anchor (5.5 V) re-visited every 3 points so
  light/temp drift is measured rather than silently corrupting the curve (the 06-08
  lesson); 3x re-send of the unacked SET_MAINTAIN broadcast; Apogee PAR sampled each
  heartbeat + IR-temp prompts; dark-panel + PAR-instability flags with a redo offer;
  restores 5.5 V on exit; relays nb-* to UDP so net_bench_log co-records. Validated
  end-to-end against a pty-simulated master (recovered a synthetic IV peak at 4.8 V).
- `ops/bench/mpp_analyze.py` -- PAR-normalized P-vs-VINDPM per session, anchor-drift
  report, best-setpoint + "what fixed 5.5 V gives up" ratio, Vmp shift cool-vs-hot.
Procedure (also in TODO): SOC <~60 % first (charger must stay in bulk/CC), indoor
window dry run, then cool-AM + hot-midday sessions on a stable-sun day.

**Buck-boost finding from EXISTING data** (`ops/bench/bb_efficiency.py` on the 06-10
full-discharge JSONL; closes part of the "efficiency vs VBAT" TODO without bench time):
at full-RGBW show load the LFP **terminal** voltage sags to ~2.9-3.05 V, so the TPS631013
ran in **boost for the entire pre-brownout discharge -- the 3.25-3.35 V buck/boost
crossover was never visited under load.** Overhead (ESP+WiFi+converter, not separable
with this instrumentation) ~0.48-0.52 W and roughly flat; P_led/P_batt lower bound
0.61-0.64. Reframes the chemistry-tax concern: no crossover/mode-hunt tax at show loads;
the residual open regime is the production **ambient** load (tens of mA), where the
plateau terminal V (~3.2-3.3 V) does sit near the crossover. Caveats: n=1 cell/board/
load; fine structure vs VBAT may be time-confounded (WiFi activity); plot
`data/ca/2026-06-10-discharge-1357-bb-eff.png`.

## 2026-06-10 -- Ben + Claude -- Full discharge: bench LFP is AT/ABOVE its 2000 mAh rating (capacity vindicated); gauge learn cycle + brownout failure mode

**Capacity, finally measured (gauge-independent).** A full charge->empty discharge on battery
(`afk_discharge.py`, full-RGBW ~467 mA load, INA 0x45 coulomb integration) delivered **~2077 mAh
to a 2.5 V cutoff over 280 min, SOC 98->0 %** -> ~ **2119 mAh** at 100 %. The bench "2000 mAh" 18650
LFP is **at/above its rating** -- every earlier under-capacity claim (the 06-09 "~760 mAh" slice,
the older "~1000 mAh / 2x overrated") is **dead.** Ben's skepticism + the reputable-dealer prior
were right; the low numbers were entirely the un-learned, plateau-fooled gauge + my slice
extrapolation. (Production targets a different cell -- LFP 32700 ~6000 mAh -- so this is methodology
validation, not a product sizing number.)
(Data note: one spurious INA-0x45 sample -- a -21 A I2C/serial glitch at 138 min -- had inflated the
logged integral to 2144 mAh; `afk_analyze` now ablates it + re-integrates -> 2077 mAh. LED & gauge
were normal at that instant, so it was a lone read glitch, not a real transient.)

- **Usable under full LED load: ~1971 mAh** before the first brownout (first reset, bv 2.97; LED
  held full to ~2045 mAh, bv 2.80). The brownout cascade is confined to the last ~100 mAh.
- **Gauge vs INA (this run IS the learn cycle):** current bias **+8.3 %** high (median, |INA|>50 mA);
  coulomb **+7.9 %** (gauge 2241 vs clean INA 2077 mAh) -- now consistent with the instantaneous
  bias (the glitch had masked it at +4.5 %). Gauge SOC hit 0 % at ~1977 mAh with ~100 mAh (~5 %)
  still left -- mildly pessimistic at the tail but respectable for an un-learned LFP gauge.
  **DesignCap 2000 is ~correct** (measured ~2119) -- the SOC flakiness was UN-LEARNED gauge, NOT a
  wrong DesignCap (retracting the 06-09 "set DesignCap ~760"). This discharge + the recharge = a
  full learn cycle; re-check SOC accuracy on the NEXT cycle.
- **Gauge SOC shape (Ben's read):** SOC held at **1 % across the whole voltage knee** (where
  dV/dQ steepens), and the **1 %->0 % step coincided almost exactly with the brownout onset** -- a
  usable "really empty now" signal even though the flat plateau hides SOC elsewhere.
- **Failure mode (intended, aggressive):** 44 brownout-reboots in the deep knee -- under the
  ~467 mA full-RGBW load, once the cell sagged below ~2.97 V the board couldn't hold ESP+LED+WiFi
  -> reboot cascade (draw fell to ~145 mA). **LEDs went unstable ~2.7 V but the board kept running
  to ~2.5 V.** Bounded by the `--batt-floor 2.3` build + the script's 2.5 V cutoff; recovered fine
  on USB (charger precharge/trickle at 2.56 V). **Production lesson: set the low-battery cutoff
  well ABOVE the heavy-load brownout point.**

Tooling: `afk_discharge.py` (fixed-load coulomb run, reset-tolerant, waits-for-unplug),
`build.sh --batt-floor`, `afk_analyze.py` (constant-load runs + robust median gauge bias + glitch
ablation/re-integration). Plot: `ops/bench/data/ca/2026-06-10-discharge-1357-gauge.png`.

## 2026-06-09 (cont.) -- Ben + Claude -- SEN0291 wattmeter read 10x low (0.1 vs 0.01 ohm shunt); fixed, cross-checked, AFK gauge-cal sweep launched

**The "400 mA (power_bench) vs 36 mA (wattmeter)" mystery was a units bug, not a measurement
conflict.** Same current, 10x apart: `ina_monitor` computed `ma = shunt_mv / INA_RSHUNT_OHMS`
with `INA_RSHUNT_OHMS = 0.1` (the INA219 *reference* shunt), but the **DFRobot SEN0291 hardware
shunt is 10 mohm (0.01)**. Every current it ever printed was **10x low**. The gauge was right.

**Evidence (convergent):**
- Datasheet: SEN0291 = "10 mohm alloy shunt", +/-8 A, **1 mA resolution** -- and 1 mA = INA219's
  10 uV LSB / 0.01 ohm. The resolution spec only closes at 0.01 ohm, not 0.1.
- Live reconcile (`ops/bench/reconcile_ina_pf.py`, W-full): INA reported 6.7 mA -> x10 = 67 mA;
  PF battery-current delta = 81 mA -> ratio **~12x** (datasheet says 10; excess = WiFi-TX bursts
  in the gauge average + sagging rail + a noisier INA on the sag). Order of magnitude confirmed.
- Battery cross-check (INA 0x45 = battery line vs gauge): off -121 vs -138 mA; RGBW-full
  -461 vs -502 mA.
- `SYSTEM.md` already had RGBW at 400-500 mA; and this day's own puzzling "wake ~ 11 mA ... far
  under the 168 mA RX" -> **x10 = ~110 mA**, resolved.

**Fix:** `ina_monitor.ino` -> `INA_RSHUNT_OHMS 0.01` (Metro reflashed). **All prior INA numbers/
plots x10** -- incl. the "11 mA/0.6 s wake" (-> ~110 mA) and the led-ina-sweep PNGs (regenerated
x10). Sub-mA sleep floor stands (still below range; relabel only). At 0.01 ohm, PG=/1 = +/-4 A range,
1 mA/LSB (the old "caps ~400 mA" comment was the 0.1 ohm artifact). Raw `shunt_mv` was always
logged, so historical JSONL is recoverable by x10 without rewriting it.

**Tooling for the AFK gauge-cal run:**
- power_bench gained `/set?r&g&b&w&bri&gamma` (arbitrary single-pixel drive, per-channel gamma)
  + an unattended **battery-floor guard** (on battery, sustained <2.90 V -> cut the 3V3 LED rail +
  WiFi; non-bricking, reset/USB recovers). Reflashed via USB. (Gotcha: the post-flash RTS reset
  left the 3V3 rail off -- needed a physical reset, per POWERFEATHER_NOTES.)
- `ops/bench/afk_sweep.py`: loops {RGB,W,RGBW}x{gamma 0,1}xlevels logging INA 0x41 (LED), INA
  0x45 (battery) and gauge telemetry per point, with a **coulomb-budget cutoff** (sag-immune; a
  voltage floor false-trips -- the cell sags to 2.99 V at 460 mA even at 33 % SOC). Launched
  battery-only, 200 mAh budget.

**Run results** (`afk_sweep.py` -> `afk_analyze.py`; 814 pts, 54.8 min, 13 cycles, battery
33%->9% SOC; stopped on the 200 mAh coulomb budget; plots `*-power.png` / `*-gauge.png`):
- Corrected LED draw: W-full ~63 mA, RGB-full ~250 mA, RGBW-full ~290 mA. Full-scale is
  RAIL-SAG-limited: the LED bus droops to ~2.84 V under load (on USB *and* battery alike) so the
  SK6812 channels lose headroom; current was flat-to-slightly-rising over the run (254->259 mA RGB)
  i.e. NOT SOC-limited here. Gamma cleanly separates the mid-range (RGBW lvl 64: 70->9 mA).
- **Gauge current bias** (n=814): gauge = 1.080*INA + 2.4 mA, mean ratio 1.094 -> reads **~+9 %
  high** vs INA ground truth -> software-correct gauge current by x0.926 (or trim the MAX17260
  sense-R). Instantaneous gauge battery_ma is noisy/laggy; INA 0x45 is steady.
- **Coulomb**: gauge integrated 200 mAh vs INA 183 mAh over the run (gauge +9 %, matching the
  current bias).
- **Capacity: NOT determined -- the earlier "~760 mAh" was an overreach (Ben pushed back, rightly).**
  Gauge SOC fell 33->9 % over 183 mAh (INA), but the **resting voltage stayed flat at 3.190->3.186 V**
  (LED-off, ~120 mA) the whole run -- we never reached the LFP knee. So we have NO read on remaining
  capacity: 183 mAh could be ~24 % of a small (~760 mAh) cell OR ~9 % of the rated 2000 mAh with a
  gauge that over-drops SOC on the flat plateau -- a mid-plateau slice can't distinguish them, and
  the un-learned LFP gauge (cycles=0) can't be trusted to either. Cell is BatterySpace (reputable,
  rated 2000); no basis to call it bad, and the user's larger cells aren't testable yet (no holder).
  Plausible too (Ben): a freshly-charged gauge may pin SOC near 100 % before dropping, so DeltaSOC over
  a slice misrepresents charge. **Resolve with a clean full-charge -> full-discharge INA-coulomb run**
  (now possible -- charging re-enabled); leave DesignCap at the 2000 rating until then. The earlier
  README/SYSTEM "~1000 mAh, 2x overrated" is likewise unverified.

Caveats: the +9 % gauge current-bias fit is tight (n=814) but single-session. Post-run the cell
idled ~120 mA; I cut it to ~66 mA via WiFi-off (`q`). Follow-up (this session) adds a recoverable
timer-wake deep-sleep-on-floor + charge-enabled recovery so an unattended low cell can't be stranded.

## 2026-06-09 -- Ben + Claude -- Rails-cut idle win; 4-channel INA219 monitor built; ground-truth shows idle is tiny (gauge over-read it)

**Rails-off A/B (the sleep-current fix).** Hypothesis from cont. 10/11: the ~20 %/night
sleep-cycle drain was the two switchable 3V3 rails left on during deep sleep, not the wakes.
Added `Board.enable3V3(false) + enableVSQT(false)` before `esp_deep_sleep_start()`
(`net-bench-2026-06-09.1`) and ran a battery-only A/B vs the rails-on overnight baseline:
**rails-on ~1.7 %/h -> rails-off ~0.5 %/h, a ~3-4x cut** (~ 20 %/night -> ~ 5 %/night). The
rails were the dominant idle draw, as hypothesized. (Ratio is robust; the gauge only moved
~1 SOC count in 2 h, so the absolute is coarse -- see below.) **Captured as a gotcha in
POWERFEATHER_NOTES** so we don't relearn it. V2 keeps the gauge alive with VSQT off (separate
power-mgmt I2C), so telemetry survives the rail-cut.

**Built a 4-channel ground-truth power monitor** (`firmware/ina_monitor/`): Adafruit Metro
ESP32-S3 reading 4x DFRobot SEN0291 (INA219) at 0x40/41/44/45, separate-monitor topology
(reads a board-under-test's current through its deep sleep -- the thing the on-board gauge
can't). Direct register reads (bus V + raw shunt mV -> current; calibration-independent),
streams `ina ...` lines. Saga worth noting: (1) a STEMMA<->Gravity cable that **swapped
VCC/GND** dead-shorted + briefly killed USB on the Metro -- *all four INA boards survived* the
reversal; (2) Metro defaults to USB-OTG/TinyUSB which re-enumerates on sketch start (no
serial) -- flash with `USBMode=hwcdc,CDCOnBoot=cdc` like the PowerFeathers; (3) the hub works
direct-wired to the Metro's SDA=47/SCL=48 headers (bypass the bad cable). A reverse-polarity
JST also scared us but the PowerFeather + INA both survived.

**First ground-truth measurement (INA in the peer's battery lead).** Caught the ~30 s wake
as a current bump: **wake ~ 11 mA for ~0.6 s, sleep ~ 0** (below the ~0.2 mA PGA floor). So
the duty-cycled **battery** drain is **sub-mA** -- far below the gauge A/B's ~0.5 %/h (~4-5
mA). Reconciliation (vindicates Ben's gauge-distrust): the gauge's ~0.5 %/h was within its
own 1-count noise on the flat LFP plateau; the real drain was simply too small for it to
resolve, and the INA finally does. **Idle is negligible -- now ground-truth, not inferred.**
Caveats: 10 Hz may undersample a brief (<100 ms) radio-init spike (40 mV bus sag hints at
one) -> a fast-sample capture is the next step to nail per-wake energy; sub-0.2 mA sleep is
below this PGA range (sharpen by dropping the range); the ~11 mA wake being far under the
~168 mA always-on RX wants understanding (likely boot/init-dominated, not full RX).

**Walked back the LFP capacity claim** (Ben was right): cont. 11's "~1000 mAh / overrated 2x"
was too strong. The 06-03 drain delivered >=617 mAh but stopped *mid-plateau* (not empty), on
an un-learned gauge -> true capacity is unknown, likely a normal ~1-1.5 Ah 18650 LFP. Needs a
clean full->empty coulomb run on a learned gauge or external meter. Softened in README /
SYSTEM.md.

## 2026-06-08 (cont. 11) -- Ben + Claude -- Drawdown aborted (redundant); LFP capacity looks ~half rated; sleep-cycle idle budget negligible

Ben flagged the running always-on LFP drawdown as redundant -- correct. The 2026-06-03
overnight reboot-loop drain (`...is31-loadgen-overnight.jsonl`) already has the LFP
discharge curve at a similar load (mean -145 mA): SOC 92->30 %, **flat ~3.25 V throughout**
(min 3.234), 4.25 h -- the "LFP plateau -> V-SOC useless -> coulomb-count" lesson. Aborted the
new run; switched the board to the sleep-cycle test instead.

**Capacity finding (from the existing 06-03 data):** it integrates to **~617 mAh delivered
for a 62 % SOC drop -> real usable capacity ~ ~1000 mAh, not the 2000 mAh rating.** The
"2000 mAh" 18650 LFP looks **overrated ~2x** (physically, 18650 LFP are ~1000-1500 mAh;
2000+ is Li-ion-class). This **~halves the assumed battery budget.** Caveat: LFP gauge SOC is
shaky on the plateau -- **confirm with a clean full->empty coulomb-counted run** (USB top-up
first). Partly answers the "compare LFP sample vs rated capacity" TODO.

**Sleep-cycle duty-cycled average (computed):** sleep-cycle validated on hw (lean wake
~250 ms to HB + ~400 ms maint-listen ~ **0.65 s radio-on per 30 s cycle**). The MAX17260
can't catch the sub-second wake spike (reads ~0 mA), so computed from trusted pieces:
avg ~ (0.65 s / 30 s) x 168 mA (the always-on radio draw) + sleep floor ~ **~4 mA at a 30 s
wake interval** (~2 mA @ 60 s, ~1 mA @ 300 s). **Takeaway: the idle/sleep budget is
negligible** (~48 mAh/night @30 s on a ~1 Ah cell ~ a few %); **sizing is LED-show- and
harvest-bound, not idle-bound.** Caveats: active current is the separately-measured always-on
figure, sleep floor is estimated -- a precise per-wake/sleep number needs an **external
ammeter (SEN0291 / multimeter)**; the gauge fundamentally under-samples brief-pulse loads.
**Field concern:** that under-sampling means a sleeping fixture's gauge SOC can read
optimistically high -> low-battery logic must cross-check **voltage** (reinforces existing
TODO). Sleep-cycle left running overnight battery-only as a gauge-vs-pulse cross-check.

## 2026-06-08 (cont. 10) -- Ben + Claude -- Solar/sizing session: sleep-cycle + OTA-wake, idle floor, MPP sweep (cloud-caveated), drawdown started

Long bench session toward battery/panel sizing. New firmware `net-bench-2026-06-08.9`
(all validated on hardware via OTA) + several findings.

**Firmware:**
- **Sleep-cycle** (`--sleep-cycle --sleep-s N`): deep-sleep duty cycle (wake -> telemetry
  heartbeat -> brief maint-listen -> deep-sleep). Validated: `rr=deepsleep`, ~32 s cycle.
  Trimmed the USB-CDC `delay(1500)` on deep-sleep wakes so the wake is lean.
- **`U` sustained ENTER_MAINT** (~35 s): no-touch OTA-recovery of a **sleeping** board --
  the normal `u` burst misses a board awake only ~400 ms/30 s. Validated: a deep-sleeping
  peer caught it on a wake window and joined WiFi for OTA. The field **fleet wake-for-
  maintenance** primitive.
- **`SET_MAINTAIN`** (master `m`): runtime VINDPM/charger-maintain set over ESP-NOW (no
  reflash) -- the MPP-sweep actuator + future P&O MPPT primitive.

**Idle-load floor (battery-only, clean):** an always-on ESP-NOW peer draws **~168 mA /
~0.55 W**, and killing the WiFi scanning barely moved it -- the load is **radio-RX-
dominated**, not scanning. 168 mA flattens a 2 Ah cell in ~12 h, so **always-on is
unsustainable on battery -> deep-sleep duty-cycling is mandatory** (quantifies the "be
quiet during sunshine" instinct).

**Harvest (full sun):** Seeed 3 W panel, flat at ~2 pm, **lux 127 k (~1000 W/m^2 = full
sun)**, panel **150 deg F / ~65 deg C** (IR; glass epsilon~0.9, so true temp ~equal or a hair higher).
Measured **~1.0-1.2 W** at the default VINDPM 5.5 V (SOC 34-58 %, bulk-charging). 3 W is
STC; heat (-15-18 % + Vmp droop) + flat angle explain <3 W -- but see MPP.

**MPP sweep -- finding + caveat:** swept VINDPM 5.5->4.4 V. **Peak power at ~4.85 V -- matches
the hot-panel Vmp prediction; 5.5 V is well past the IV knee** (power craters above ~5.0 V).
BUT a **cloud rolled in mid-sweep (127 k->37 k lux)** with the panel temp drifting, so the
absolute watts (0.14-0.37 W) and the apparent 2.6x are **NOT a clean full-sun number** (the
start/end 5.5 V points disagreed, 0.138 vs 0.215 W = intra-sweep drift). **Robust:** MPP
~ 4.85 V hot, fixed 5.5 V is wrong when hot. **TBD:** the actual full-sun gain (no full-sun
MPP point captured). **MPPT verdict: green-light to MEASURE properly (clean full-sun sweep
+ simultaneous lux/IR-temp at 2 panel temps), not yet to commit it's worth ~2x.**

**Drawdown (started, cloudy evening):** brought inside, panel disconnected, always-on
~157 mA battery-only discharge from ~SOC 60-76 % (gauge jumpy on the LFP plateau -- trust
the coulomb count). `--autosleep` deep-sleeps at brownout to protect the cell. Logging
overnight -> LFP discharge curve, gauge accuracy, delivered capacity, cutoff voltage
(`ops/bench/data/ca/` + `/tmp/nb_drawdown_raw.log`; results next session). NOTE: this used
the always-on load; the **sleep-cycle duty-cycled average** (the low overnight budget
number) is still un-measured.

## 2026-06-08 (cont. 9) -- Ben + Claude -- Conclusions: WiFi hypothesis settled (moving-board artifact) + stress-test framing

Wrap-up of the day's two device tests.

**WiFi drop -- hypothesis settled (high confidence).** The board latches to one Eero BSSID
at association and **does not auto-roam** (ESP32 has no 802.11k/v/r); carried from indoors
to the yard, it clings to the now-weak indoor node instead of hopping to the strong (-46
dBm) nearer one -> the link collapses while a good AP sits right there (the scan is the
smoking gun). Fix is cheap and already partly in place: **a reset, a software reset, or a
firmware "re-associate on link loss" guard** forces a fresh scan-and-associate, which
picks the strongest beacon (our maintenance-OTA path already does a fresh `WiFi.begin()`,
which is why OTA worked from the bad spot). **Framing (Ben):** this is a **bench artifact
of a *moving* board** -- deployed fixtures are stationary and won't walk away from their
Eero, so we're unlikely to hit this in the field. Logged as a **gotcha** (see
POWERFEATHER_NOTES) + a firmware-guard TODO, not a blocker.

**Panel 0.12 V -> 5.55 V swing -- explained:** Ben **reseated the solar connector** mid-check;
that's the swing, not a mystery intermittent. Takeaway for production: **mechanically
secure/strain-relieve the panel pigtail** (a loose connector = silent zero-harvest), and
item (a) now makes a dark panel obvious live (`supply_good=0`, `supply_v~0`).

**Stress-test framing (important for reading the numbers):** this run **highly activated the
radio (continuous ESP-NOW + 15 s all-channel WiFi scans) WHILE harvesting** -- a deliberate
worst case. Even so the cell net-charged in decent light. **In the field the fixture will
be asleep / quiet during sunshine**, so real harvest-vs-load is *more favorable* than these
bench numbers -- i.e. the bench load figures are conservative, not representative. Next
focus: a **sizing-oriented** solar run (realistic sleep/duty-cycle load, harvest across
sun/cloud/shade) to actually spec the cell + panel.

## 2026-06-08 (cont. 8) -- Ben + Claude -- Item (a): supply/panel telemetry over ESP-NOW -- built + VALIDATED on hardware

Built the solar-telemetry half of the plan (item (a)): carry the **supply (panel) side**
over ESP-NOW so it logs from anywhere without WiFi-STA. Threaded `supply_mv`/`supply_ma`/
`supply_good` end-to-end -- peer reads `Board.getSupplyVoltage/Current/checkSupplyGood`
(cached ~1 Hz in `readBattery`), **appended** to `NbHeartbeat` (kept `NB_PROTO_VER=1`;
append-only + length-checks -> no flag-day, a pre-supply master still reads the battery
fields of a supply-capable peer; new master reads old peer via `offsetof` guard), stored
in `NbPeerStat`, emitted as `sv=/sma=/sgood=` on the `nb-peer` bridge line. Host
`net_bench_log.py` parses them (optional regex group) and derives `supply_w` (panel
harvest), `battery_w`, and `load_w = supply_w - battery_w` into the JSONL. fw
`net-bench-2026-06-08.7`.

Deployed via the maintenance round-trip (master `u` -> peer rejoined WiFi -> OTA `.7`
supply build -> reflash master over USB). **Works end-to-end:** `sv=5.56 sma=160 sgood=1`
-> **panel ~0.89 W**, battery flips to **net-charging +140 mA** under the (heavy) scan
load; harvest swings 0.5-0.9 W with the clouds, all logged.

**Solved the earlier "net-discharge at noon" puzzle:** while the peer was briefly in
maintenance mode its `/telemetry` showed **`supply_v=0.123` -- the panel was essentially
dark** (shaded/mis-oriented in-hand, or a loose connector). So the discharge was simply
**zero harvest**, not a battery/load problem. Once the panel saw light again, `supply_v`
jumped to 5.55 V and it charged. Lesson: **harvest is very orientation-sensitive** -- a
real sizing finding, and exactly the thing item (a) now makes continuously visible.

**Caveat for sizing:** the derived `load_w ~ 0.39 W` here is the *diagnostic firmware's*
load (radio always on + 15 s WiFi scans), NOT a fixture budget -- don't size the cell to
it. The **panel-harvest V/I is the directly-useful output**; the load side still needs
the bottom-up fixture duty-cycle budget (existing TODO). Boards left running on ch 11,
logging to `ops/bench/data/ca/2026-06-08-ca-lfp-2000-net-master-multicast-rNA-1946.jsonl`.

## 2026-06-08 (cont. 7) -- Ben + Claude -- WiFi coverage diagnostic VALIDATED on hardware (2 boards, OTA) + PDR seq-bug fixed

Took (cont. 6)'s firmware to hardware. Flashed the **serial-bridge master** (`9F2690`)
over USB on ACM1 (`--serial-bridge --no-charge`, ch 11) -- boots into "SERIAL BRIDGE (no
WiFi)" and streams `nb-*` to USB as designed. Then **OTA'd the scan-report peer onto the
live solar board** `9E5B0C` (the only wireless Resonance board -- found by sweeping the
LAN for `/telemetry`; note `192.168.4.73` is an unrelated "Grow Light", NOT ours, left
untouched). Built the OTA with **`--chem lfp --cap 2000 --maintain 5.5`** to match the
board's LFP cell + solar panel (Li-ion profile would overcharge the LFP -- the
POWERFEATHER_NOTES gotcha).

**Worked end-to-end, first try.** Post-OTA the peer left WiFi, rejoined as an ESP-NOW
peer (`rr=software`, LFP 3.33 V, still solar-charging ~40 mA), and streamed the **2.4 GHz
coverage map to the desk with zero WiFi-STA on the field board** -- resolving the **3
BubbyNet Eero nodes separately by RSSI** (`...a3:06`/`...9c:06` @ -44, `...40:c6` @ -62, all ch
11) plus neighbors on chs 1/6/11. The two things flagged as load-bearing-but-unverified
in (cont. 6) -- async `WiFi.scanNetworks()` coexisting with ESP-NOW, and the post-scan
channel re-pin -- **both hold**.

**Found + fixed a real bug:** heartbeats and scan-AP packets shared one tx sequence
counter, so each scan batch's N sends read as N phantom heartbeat *gaps* at the master
(uplink PDR showed a bogus 0.65). Gave heartbeats their **own contiguous seq** (`hbSeq`
in `sendHeartbeat`). Re-OTA'd the fix via the **maintenance round-trip** (master serial
`u` -> peer rejoined BubbyNet -> OTA -> both back to comms, no touch -- also validates that
path). After: `gaps=0` through scans, `pdr=1.0` with an honest occasional `gaps=1`.
(Downlink `dlpdr~0.8` is expected: the peer is deaf to the master's 10 Hz frames during
its own ~2.5 s scan window -- informative, not a fault.)

Net: **item (b) is validated on hardware.** Still TODO: the actual **yard walk** (carry
`9E5B0C` out, watch the per-Eero-node RSSI fall off -> the coverage-at-distance map +
where to place a field maintenance AP) and write that note. Tooling to capture it
(`net_bench_serial_bridge.py` -> `net_bench_log.py` `nb-scanap` rows) is ready but a
background log wasn't started this session. Boards left running on ch 11.

## 2026-06-08 (cont. 6) -- Ben + Claude -- WiFi coverage diagnostic, reworked as a wireless ESP-NOW bridge (firmware done, untested on hw)

Picked up the solar-telemetry/range handoff plan, item (b) -- the WiFi range diagnostic.
Started on the standalone tethered sketch (`firmware/wifi_diag/`: associates, streams
RSSI/BSSID/channel + a 2.4 GHz scan, flags a *missed-roam* when a stronger same-SSID Eero
node wasn't chosen). Then Ben pushed back on the laptop tether and proposed a better
setup: an **ESP-NOW "wireless serial" bridge** to his desktop. That's the right call --
it's the *same* architecture item (a) needs, so building it once serves both.

**Reworked (b) as scan-only over an ESP-NOW bridge** (extends `firmware/net_bench/`):
- **`--serial-bridge`** (a master): does NOT join WiFi; stays pinned to `--channel` and
  relays everything it hears (`nb-master`/`nb-peer`/`nb-scanap`) to **USB serial**, so a
  desk-tethered board logs the whole field fleet -- no laptop in the yard.
- **`--scan-report`** (a field peer): async-scans 2.4 GHz (**never associates**), then
  broadcasts the strongest `--scan-max` APs (BSSID/RSSI/ch/SSID) as a new `NB_SCANAP`
  packet. Because it never associates, the radio is **ours to pin to `--channel`** (no
  Eero-channel coupling -- the key insight; an *associated* board is locked to the Eero's
  channel and ESP-NOW rides that). Radio is re-pinned to `--channel` after each scan;
  ESP-NOW TX is suppressed while the scan hops.
- Host: `ops/bench/net_bench_serial_bridge.py` relays the bridge's serial -> UDP:54321 so
  the **existing** `net_bench_log.py`/`net_bench_monitor.py` work unchanged; `net_bench_log.py`
  gained an `nb-scanap` row (per-AP coverage -> JSONL).

Why this answers (b): the plan's own stated smoking gun is "a scan showing a closer node
with better RSSI it didn't pick" -- a **scan needs no association**, so scan-only delivers
the per-Eero-node RSSI coverage map from anywhere in the yard (and tells us where to put
the field maintenance AP). The empirical roaming-*decision* test stays in the tethered
`wifi_diag` probe.

**Status: all 4 net_bench variants compile clean (28% flash); NOT yet run on hardware**
(no board on USB this session -- ACM0 is the PAR sensor). Cautions before trusting any
map: async `WiFi.scanNetworks()` + ESP-NOW coexistence on the S3 is assumed-fine but
unverified, and the post-scan channel re-pin is the load-bearing line. Next (Ben): flash
2 boards on a shared `--channel`, walk the field peer, confirm `nb-scanap` updates from
the yard, then write the RSSI map + AP-placement note here. Details: updated
`SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md` (end) + `firmware/net_bench/README.md`.

## 2026-06-08 (cont. 5) -- Ben + Claude -- A/B rollback VALIDATED (bad image auto-reverts) + the recipe

Tested A/B rollback with a bad image (battery-only LFP). **PASS:** pushed a power_bench
build whose self-test hook reports unhealthy (`extern "C" bool verifyOta(){return false;}`,
gated by `-DRES_OTA_FAIL_SELFTEST`); on first boot the Arduino core (`initArduino`, before
`setup()`) saw the image `PENDING_VERIFY`, called `verifyOta()`->false ->
`esp_ota_mark_app_invalid_rollback_and_reboot()` -> bootloader **reverted to the last-good
image automatically, no touch** (board came back on `ota1`; the bad image never reached
setup/WiFi). `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is in the arduino-esp32 3.3.7 build.

**Gotcha (caught the first try):** `verifyOta()` is a **C-linkage** weak hook (defined in a
.c core file). A plain C++ override is name-mangled, silently does NOT override, the default
(returns true) runs, and the bad image **sticks** (no rollback). Must use `extern "C"`.

**Production recipe (the safety net):** implement `extern "C" bool verifyOta()` with a real
self-test (power chip init + radio + fuel-gauge reachable) -> return false on failure for an
auto-revert. **Limitation:** this only catches self-test FAILURE; an image that passes
verifyOta then crashes/hangs LATER in setup()/loop() is already marked valid -> could brick.
Robust pattern: `verifyRollbackLater()=true` to DEFER the mark-valid, run extended checks +
the watchdog, and mark valid only after proving stable for N s -- so a late crash/hang trips
the watchdog while still PENDING_VERIFY and rolls back next boot. power_bench keeps the
gated `RES_OTA_FAIL_SELFTEST` fixture as a reusable rollback test.

## 2026-06-08 (cont. 4) -- Ben + Claude -- Battery-only OTA validated on worst-case LFP (the field-reset requirement)

Per Ben (correctly): battery-only OTA with NO physical access is a hard requirement (can't
take lanterns off the tree), so battle-test it now. Did 3 consecutive OTAs to the LFP board
**battery-only (no USB), at ~3.2 V (the buck-boost-crossover, hardest regime), over WiFi**:
**3/3 recovered cleanly, no button**, each via software reset (`rr=software`), and the new
image confirmed running (`fw` flipped to `power-bench-2026-06-08.ota1` after OTA#1 -- a real
update, not a rollback). With the ~14 battery-only peer OTAs earlier this session that's
**~17/17, zero failures.** Conclusion: **battery-only field OTA is trustworthy** -- the
"never touch a deployed lantern" requirement is met.

Key clarification (resolves the earlier confusion): the flaky/stranding resets were the
**USB-JTAG hardware reset** (esptool's RTS path during *USB* flashing) + the no-battery
brownout -- neither exists in field OTA, which uses `ESP.restart()` (software reset), reliable
every time. "Use USB" was bench-iteration convenience, not a trust statement.

Caveats (refinements, NOT blockers; a failed OTA is safe -- stays on / A-B rolls back to the
known-good image, never bricks): (1) tested over GOOD WiFi (the field model = a local AP near
the tree for a maintenance window); OTA over a MARGINAL link is untested (TCP retries, but a
bad link could fail the upload -> no update). (2) A/B rollback not yet explicitly tested (push
a deliberately-broken image -> confirm auto-revert) -- worth doing as the ultimate safety net
alongside the watchdog + autosleep recovery.

## 2026-06-08 (cont. 3) -- Ben + Claude -- Solar path validated (net-positive in weak light) + LFP bring-up + a brownout root-cause

Moved to solar feasibility (power_bench, not net_bench). Switched to the LFP 2000 mAh cell --
flashed `power_bench --chem lfp --cap 2000 --maintain 5.5` (Seeed 3W panel: Vmp 5.5 / Voc
8.2 / Imp 540 mA) BEFORE connecting the cell (LFP charges to ~3.6 V, not Li-ion's 4.2 V --
flashing the LFP profile first keeps the charger safe). `Board.init(2000, Generic_LFP) Ok`.

**Brownout root-caused (clean):** on USB with NO battery, the board crash-looped (USB-CDC up
~1 s then reset). Cause = `--maintain` (VINDPM) 5.5 V > USB 4.92 V -> the charger *rejects*
USB (won't pull its input below the 5.5 V setpoint) -> with no battery to source VSYS, it
brownouts; and enabling charging into a missing battery is the trigger point. Connecting the
cell fixed it instantly (battery sources VSYS). Unifies the earlier brownout work: it was
`maintain > supply voltage` + no buffer, not "no battery" per se. (Firmware guard TODO: don't
enable charging if no battery detected; and `maintain` must be <= the supply you're on.)

**Solar result (partly cloudy, ~10:18 am Oakland, through a window):** panel **5.56 V x
~66 mA ~ 0.37 W**, VINDPM holding the panel steady at 5.5 V, **battery_ma +~10 mA -- net
POSITIVE charge** into the LFP (3.31 V / 33%, safe) *even with WiFi running* (the radio eats
~56 of the 66 mA; ~10 mA banks). Path validated end-to-end. Extrapolations: asleep, ~all
66 mA would bank; full sun -> ~540 mA (~8x) -> the ~120 mAh/night budget closes with margin.
Solar essentially de-risked (it's what the board is built for).

Also: ESP-NOW reached the back fence but WiFi-STA couldn't hold the yard -- expected, not a
bug (different destination = router vs office-master, and WiFi assoc+TCP needs far more
margin than ESP-NOW's loss-tolerant broadcast). It WiFi-reconnected fine once close -- no
instability. Next (do on USB so reflash/tune is safe): full-sun board-asleep harvest number
+ `--maintain` sweep (5.5/5.0/4.6) for the shaded canopy.

## 2026-06-08 (cont. 2) -- Ben + Claude -- T3 range walk: clean V, link held through house+yard+oak

Walked the cup board (`9F2690`) out the back door, across the yard to the fence (behind a
big oak), and back, slowly, with the 3 stationary boards as controls. New tooling:
`ops/bench/net_bench_walk.py` (continuous per-peer RSSI/PDR logger, run in background) +
`net_bench_walk_plot.py` (Pillow V plot) + live landmark markers. Result: a clean V/bathtub
(-19 dBm office -> -80..-87 floor at fence/oak with a few brief dropouts -> -30 back), 152
samples / 328 s. **Findings:** (1) the **house doorway dominated** (~50 dB in the first ~30
steps); open-yard distance added little; (2) the **oak trunk caused the deepest dips**,
recovering at the fence past it; (3) RSSI is **path-asymmetric** (door -69 out / -47 back --
multipath); (4) the **3 reference boards stayed flat** -> the swing is real, environment
stable (good control). The link **held ~100 steps through a house door + full backyard +
behind an oak** -- far harsher than the tree (open air + bamboo, no doorway), so a strong
deployment result. Data/graph: `ops/bench/data/ca/2026-06-08-rangewalk.{jsonl,png,-markers}`.
Live RSSI also viewable via `net_bench_monitor.py`. (Still un-measured: pure open-field
clean-LoS cliff distance -- the house doorway masked the distance falloff here.)

## 2026-06-08 (cont.) -- Ben + Claude -- Obstruction mapping: enclosure ~RF-transparent, solar panel is the attenuator

Used the identify/locate blink to label peers placed in different obstructions (10 Hz, all
held ~99-100% PDR at bench range): 3D-printed lantern cylinder (board inside) -15 dBm;
ceramic cup -29; metal laptop in a metal+glass cabinet -31; **glass+metal solar panel on a
box -52** (~25-35 dB hit). **Two build-relevant findings:** (1) the **lantern enclosure is
~RF-transparent** -- the printed/plastic housing won't detune or block the mesh; (2) the
**solar panel is the one real attenuator (~25-35 dB)** and it sits over the antenna in the
hat -- the antenna-keepout concern made concrete (still 100% PDR / ~38 dB margin at bench
range). Caveats: placement+obstruction combined (not pure material deltas), RSSI approximate,
short range. Worst case = panel attenuation + full tree distance stacked -> the mock-hat RF
test (Steve). Also flagged: identify's 8 s blink is too short for human-in-the-loop / field
use (Ben missed a single blink waiting on chat latency) -- make it ~30 s or toggle-until-stop.

## 2026-06-08 -- Ben + Claude -- Fuel-gauge false-low after charge (SOC needs voltage cross-check)

Morning: one peer (`9E5AF0`, 10050 mAh) was blinking 4 Hz (LED "<10%"), but **bv=4.188 V
= fully charged** -- the cell charged fine; the gauge is misreading 1%. Extends yesterday's
cap-reseed finding: after the `DesignCap` change the MAX17260 re-seeded per-board to
*different wrong* values (`9F26F8`->~100%, `9E5AF0`->~1%), and the overnight charge didn't
fix it because the board ran the whole time (~100-200 mA) so the charger likely never hit
the clean **termination** event the gauge uses to anchor 100%. Lessons for production: (1)
gauge SOC is untrustworthy after a cap change / without a real learn cycle; (2) an
always-awake fixture solar-charging may never anchor its gauge (the duty-cycled CA design
helps -- low load during charge); (3) **low-battery logic must cross-check voltage** -- a
false 1% could trip a needless shutdown, a false 100% could over-discharge. Action: add a
voltage sanity-check to the battery LED (bv>4.0 V => never show "critical").

**Done (v07.5, OTA'd to all 5):** the battery LED now floors the displayed level by a
loaded-Li-ion voltage estimate, so a false-low gauge can't show "critical" -- `9E5AF0` now
shows SOLID (gauge still reads 1% but bv 4.19 V vetoes it). Ben's field-vs-bench insight:
this false-low is likely a **bench artifact** -- deployed fixtures sleep + trickle-charge
from solar under near-zero load, so the charger reaches termination and the gauge anchors
(and gets a real cycle daily); the always-pinging bench run is the pathological case.
Friction noted: each firmware OTA needs per-board cap bins (cap is a build flag) -- a
follow-up could store cap in NVS / make it runtime-settable so one bin serves all.

## 2026-06-07 (cont. 6) -- Ben + Claude -- Rate sweep PASS: ESP-NOW scales to ~100 nodes

Ran the broadcast-rate sweep (new `ops/bench/net_bench_ratesweep.py`, drives the master's
`+`/`-` over serial + measures per-rate PDR from the bridge), 1->50 Hz, master + 4 peers,
co-located, Li-ion. **Aggregate uplink PDR >=97% across the whole range, no collapse:**
1Hz 100%, 10Hz 99.5%, 20Hz(100 pkt/s) 99.1%, 50Hz(250 pkt/s) 97.2%. Clean airtime fit
`loss ~ 1.05e-4 x pkt/s` -> **100 nodes @ 1-2 Hz/node ~ 98-99% PDR**. Strong GREEN for the
"can we base 100 fixtures on this" question. (Tooling fix: the naive worst-peer knee was a
small-sample artifact -- one lost packet of ~60 reads as 98%; switched the verdict to
aggregate loss.) Caveats: 5-node small-N (no hidden-node at scale), co-located (range is
T3/T4 next), Li-ion (re-verify on LFP). T5 parallel-OTA already passed; T3/T4/T6/T7 remain.

## 2026-06-07 (cont. 5) -- Ben + Claude -- Identify/locate command; per-board cap; MAX17260 re-seed finding

Added an on-demand **identify/locate** command (master `i`/`I` -> target board blinks a
distinct `..-` on the onboard LED for 8 s; the data-center chassis-ID pattern) and used it
to map board<->battery without plugging in: master 2200, `9F2690`/`9E5AB8` 4400,
`9E5AF0`/`9F26F8` 10050 mAh. OTA'd each board with its correct `--cap` (fw v07.4; all 5
recovered no-button -- cumulative OTA reliability still 100%).

**Fuel-gauge finding:** changing the MAX17260 `DesignCap` re-inits the gauge and **resets
learned SOC** -> a transient bad reading (`9F26F8` 10050 mAh: 27% @3.73 V with cap=2000 ->
**100% @3.72 V** after re-seeding to 10050; true ~50%). So: **set DesignCap once at first
boot, charge to full to anchor 100%, let the gauge learn over a cycle; don't change cap in
the field.** More critical on LFP (flat OCV). Folds into T6 prep (fully charge cells
first). Also shipped a `/resume` re-init fix (v07.3) in the same firmware.

## 2026-06-07 (cont. 4) -- Ben + Claude -- net_bench first light: ESP-NOW works, OTA validated, battery-LED deployed

Flashed the fleet (1 master USB + peers on Li-ion). **First light, ch 11:** master +
**3 peers** up, uplink/downlink **PDR ~99.5%** at 10 Hz co-located, RSSI -25 to -33 dBm,
**0 send-fail** -- ESP-NOW works. (One flashed peer never booted -- a silent no-boot the
watchdog can't catch since it never reached loop(); post-flash boot flakiness or flat
cell.) Added a **battery-level onboard LED** (GPIO46: >50% solid, 25-50% 1 Hz, 10-24%
2 Hz, <10% 4 Hz) and **OTA-deployed it** (v07.2) to master + 2 reachable peers via the
maintenance-mode cycle. **T5 effectively PASS** -- all recovered via *software reset, no
button* (master via /telemetry, peers via ESP-NOW rejoin with rr=software).

Two findings: (1) `net_bench_ota.py` false-FAILED the peers -- they reboot OFF WiFi into
comms, so /telemetry polling can't see them; fixed with `--reboot comms` (the OTA
"complete/Rebooting" ack + software reset IS the success signal; confirm rejoin via the
bridge). (2) **Brownout de-risk:** the ~4%-SOC peer dropped out entering maintenance --
the WiFi-association inrush on a near-empty Li-ion cell is the brownout failure mode; at
100x we must gate OTA/maintenance on SOC (or lean on the autosleep guard). Next: charged
cells on all boards, then the rate sweep + range/obstruction matrix.

## 2026-06-07 (cont. 3) -- Ben + Claude -- net_bench: first ESP-NOW firmware + 5-node feasibility harness

Built the project's **first ESP-NOW firmware** to de-risk basing ~100 fixtures on the
PowerFeather V2 (networking/radio/stability axis). New `firmware/net_bench/` (forked from
power_bench): broadcast-only ESP-NOW (unencrypted FF:FF -- the 100-node-scalable pattern;
encrypted peers cap at ~17), **master** role (broadcasts SHOW_FRAME + WiFi-STA-bridges
per-peer stats to the host over UDP:54321) and **peer** role (pure ESP-NOW on battery,
HEARTBEAT with seq/battery/downlink-PDR). Per-source seq-gap PDR. **Maintenance-mode
switch** (ESP-NOW metadata -> peers join AP -> standard WiFi OTA, ADR-0010 compliant; no
firmware over ESP-NOW). **Watchdog** added (esp_task_wdt -- net-new, closes the open
field-reliability TODO) + `--wdt-hangtest`. Autosleep guard ported.

Host harness: `ops/bench/net_bench_log.py` (master bridge -> JSONL), `net_bench_ota.py`
(parallel OTA + auto-recovery/no-button assertion), `net_bench_summary.py` (per-peer
PDR/RSSI + scale-extrapolation loss knee). Test plan + acceptance targets:
`docs/tests/NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`.

**Bench-validated on 1 board (9E5B0C):** boots, Board.init Ok, ESP-NOW up, heartbeats
broadcasting (0 send-fail); **watchdog recovery PASS** (induced hang -> task-WDT reset ->
reboot, post-reset reason `task_watchdog`, no human); master WiFi-join + host JSONL
capture PASS. **Channel-lock confirmed real:** home AP "BubbyNet" is ch 11, so building
with `--channel 6` made the master warn and every send fail (`Peer channel != home
channel`). **Action for Ben: build all 5 boards with `--channel 11`** (= the AP channel)
to run the multi-node matrix. All battery results will be Li-ion (JST-PH) -- asterisked to
re-verify on LFP (LFP plateau sits on the buck-boost crossover, the harder regime).
Multi-node T0-T7 pending Ben's 5 boards on a matched channel. Plan approved; this is the
implementation of that plan.

## 2026-06-07 (cont. 2) -- Ben + Claude -- Second Split style (rotate-about-center) + ping-pong spiral

Two small LED Studio refinements:
- **Split RGB is now 3-state (Off / Triad / Rotate).** Triad = the original local R/G/B
  offset cluster (spread/rotate). **Rotate** = R at the point, G/B the same point
  rotated 120 deg /240 deg about the grid center -> a 3-fold rotationally-symmetric color
  split (collapses to white at the exact center; shines with a moving spiral/orbit
  head). Both validated on hardware.
- **Spiral now ping-pongs** (out to the edge, then retraces inward) instead of jumping
  from the outer tip back to the center -- no per-frame discontinuity. Orbit still wraps
  seamlessly (closed ring). Verified: spiral order-index steps by <=1 the whole cycle.

## 2026-06-07 (cont.) -- Ben + Claude -- Merged LED Studio (HEX + RGBW + RGB), Split-as-toggle

Merged `hex_studio` + `rgbw_studio` into one **`firmware/led_studio/`** with a UI mode
toggle that hot-swaps between three LED options on the same A0/GPIO10 data pin -- no
reflash -- by reconfiguring the NeoPixel type/length at runtime
(`updateType`/`updateLength`): **HEX grid (37px RGB)**, **RGBW point (1px)**, and a
new **RGB point (1px)** for the high-power RGB LED (same as the RGBW minus the white
die -- same render path, 3-byte strip, W ignored). Removed the two now-superseded
single sketches. Confirmed harmless to mismatch mode vs physical module (both SK6812):
worst case is wrong colors / one LED until refreshed; strip is blanked on each switch.

Per Ben's request, **Split-RGB is now a toggle modifier, not its own animation** -- so
the separated R/G/B triad follows the selected path: Static (parked at the anchor,
Step+ to move it), Spiral, Orbit (sweeps the triad along the path with trail), and
Breathe (pulses the triad). Spread/rotate tune the fringe width. Validated on hardware
across all three modes + the split paths.

Process note (field-reliability data): the **USB-JTAG flash flakiness recurred twice**
this session -- the port dropped after one upload (needed a replug) and a write failed
with "Error during build" before succeeding on retry. Reinforces the TODO that the
deployed lantern must never depend on the USB/RTS reset path (software reset + watchdog
+ the autosleep recovery instead). Recovering the IP after a reset still needs the
pyserial RTS pulse (native USB-CDC) -- see `firmware/POWERFEATHER_NOTES.md`.

## 2026-06-07 -- Ben + Claude -- Two findings: 3V3-rail-needs-enabling (GPIO4) + 8-bit gamma low-end dead-zone

**1) PowerFeather V2 switchable 3V3 rail must be enabled (GPIO4 / EN_3V3).** The
studio sketches drove the HEX/RGBW off the 3V3 header but didn't run the SDK, so the
header read **0 V** -- the rail is a load switch gated by GPIO4 (active HIGH), which
`Board.init()` normally turns on. Fix: non-SDK apps drive GPIO4 HIGH in setup()
(`pinMode(4,OUTPUT); digitalWrite(4,HIGH)`). Added to both studios, reflashed RGBW,
rail + LED came up. Bonus: since the LEDs are on the *switchable* rail,
`digitalWrite(4,LOW)` is a free LED kill-switch (the "software-cuttable 3V3"
pixel-power option). Captured this + the other recurring PowerFeather gotchas
(V2 board flag, native-USB reset/IP recovery, keep LEDs off the I2C bus) in a new
**`firmware/POWERFEATHER_NOTES.md`** best-practices doc, linked from
`firmware/README.md`.

**2) 8-bit + gamma kills the low brightness end (relevant to ambient).** With gamma
ON, the LED goes fully dark below ~brightness 24; gamma OFF lights it at very low
levels. Mechanism: gamma correction linearizes *perceived* brightness via
`out = (in/255)^2.6 * 255`, but Adafruit's gamma8 table maps **input 0..23 -> 0**
(then 1 for 24..35, 2 for 36..43...) -- the bottom ~9% of the range quantizes to off
because 8-bit PWM has no codes for the sub-1 values the curve demands. Tradeoff:
gamma-on = smooth perceived dimming mid/high but a dead-zone + coarse steps at the
bottom; gamma-off = usable ultra-dim but non-linear ramp. This matters because the
lantern's ambient spec ("1-3 LEDs at ~10%") sits right in the dead-zone. Noted for
later; fixes to consider when tuning the ambient look: dim-floor (`max(1,gamma8(x))`),
gentler gamma, gamma-on-color-only, or temporal dithering. No change made now.

## 2026-06-06 -- Ben + Claude -- RGBW Studio: interactive web app for the 4 W RGBW point source

Built `firmware/rgbw_studio/` -- sibling of hex_studio for the single high-power
SK6812 RGBW pixel (Adafruit 5163, 4 W). Validated on hardware (PowerFeather ACM1,
RGBW data on GPIO10): boots, joins WiFi, serves UI; all endpoints exercised OK
(W-only, hue cycle, candle, off) and the board stayed alive through the animations.
Came up at http://192.168.4.209 (same DHCP lease as the HEX session).

The RGBW is a point source (crisp gobo) with a dedicated W die, so this studio is
all about color + temporal modulation (no geometry): R/G/B/**W** sliders + color
picker, gamma toggle; white/warmth presets (W-only, RGB-white, RGBW-full, warm amber)
+ a warmth crossfade slider (RGB-white <-> W); and color animations -- **Hue cycle**,
**Breathe**, **Candle** (smoothed random-walk flicker of the chosen color), **Fade**
(crossfade to a Color-B picker). Settings readback for recording good combos.

Reminder from the LED findings: at 3.3 V the RGBW is voltage-starved (dim, non-linear
mid-range) -- fine for judging color/shadow geometry on the bench, but use 5 V for true
brightness characterization. Next: run it through the inverted-lantern gobo rig
alongside hex_studio to settle point-vs-area (and W-vs-RGB-white) by eye.

## 2026-06-04 (cont. 11) -- Ben + Claude -- HEX Studio: interactive web app for HEX aesthetics + gobo dial-in

Built `firmware/hex_studio/` -- a standalone WiFi web app to dial in the SK6812 HEX
look through the gobo, separate from `power_bench` (which is brownout/telemetry
scaffolding). Validated on hardware: flashed to the PowerFeather (ACM1, HEX data on
**GPIO10**, 3V3 + GND), boots, joins WiFi, serves the UI. Boot prints confirm the
HEX37 geometry (`ring sizes 1/6/12/18`); all HTTP endpoints exercised OK (`/state`,
`/set`, `/off`). Drove it red/center, then split-mode -- the R channel pixel computed
onto index 19, confirming the triad geometry.

Features: brightness + R/G/B sliders (+ color picker), gamma toggle for smooth
low-end dimming; shape selector (center / +inner ring / +two rings / all, computed
from the real hex rings, center = px 18); animations -- **Spiral** (single pixel
outward, trail slider), **Orbit** (single pixel around a chosen ring = the gobo
*moving-shadow* test), **Breathe**, **Twinkle**; **Freeze + Step+** to park a moving
pixel and read off its index; and **Split RGB** (Ben's ask) -- pure R/G/B on three
pixels in a triad around an anchor, with **spread** (fringe width) + **rotate**
sliders, anchor walked by Step+ -- to deliberately throw *wide separated color
fringes* through the gobo (vs the tight fringe of co-located channels). The page reads
back the exact current settings (rgb/hex, bri, shape, anim, lit pixel, split anchor/
spread) so a good-looking combo can be recorded precisely.

Bench wiring confirmed this session: **ACM1 = PowerFeather MCU, ACM0 = Apogee PAR
meter**, HEX on **pin 10**. Flash: `./build.sh --pin 10 --port /dev/ttyACM1`. The S3
is native-USB-CDC, so the boot banner (with the IP) only appears on a reset -- pulse
RTS via pyserial (or just re-flash) to recover the IP; this session it came up at
192.168.4.209 (DHCP, may change). Next: Ben drives it through the inverted-lantern +
flat-filter rig (source on desk, shadow on ceiling) to compare point vs area vs
split-fringe looks and record what reads well.

## 2026-06-04 (cont. 10) -- Ben + Claude -- AMENDMENT: LED axis NOT resolved; RGBW undervolting is viable; gobo testing queued

Walking back two overstatements from the cont. 8/9 entries below. Those entries
stand as the record of what was measured, but their *conclusions* were too strong:

1. **"LED axis resolved / SK6812 HEX direct-GPIO is the BOM front-runner" -- overstated.**
   The LED module is **not decided**. IS31-out is firm, but the HEX-direct and the
   4 W RGBW are **roughly tied in viability** and serve **different, complementary
   roles**, not the same one:
   - **SK6812 HEX direct-GPIO** = distributed / area source -> **washes out the gobo**
     (good for general ambient glow), or animate by moving a single lit pixel around
     the hex (the cast-shadow-in-motion idea -- untested, want to try it).
   - **4 W RGBW** = single **point source** -> the only candidate that throws **crisp
     mandala shadows** through the gobo. A multi-LED array can't do that geometry.
   Because the gobo wants a point source and the ambient mode wants an area source,
   the "winner" may be **application-dependent** rather than one part. No frontrunner
   until gobo testing says so.

2. **"4 W RGBW needs 5 V" -- overstated.** It is **voltage-starved at 3.3 V in this
   bench run** (non-monotonic mid-range current near its Vf), but Ben is fairly
   convinced from prior experience that **undervolting it is viable -- 5 V is NOT
   required**, with caveats. What we actually have is a poorly-characterized low-V
   curve, not a hard 5 V requirement. **Open work:** properly map the RGBW's 3.3 V
   behavior -- usable dimming range, color balance, max brightness -- before deciding
   whether any boost is warranted.

Also flagging that the **PAR/mA efficiency ranking is muddied** by testbeds run at
different SOC/load (each LED run sat at a different buck-boost operating point -- see
the Field-reliability "buck-boost efficiency vs VBAT" item), so the HEX-vs-NeoHEX
~1.6x and HEX-vs-RGBW comparisons are *system* efficiency at as-measured conditions,
not a clean intrinsic ranking. Re-rank at a fixed VBAT before trusting the slopes.

**Next:** basic gobo testing (point vs area source, crisp-shadow vs wash, the
single-moving-pixel animation idea) + a clean RGBW low-voltage characterization.
TODO + ADR 0018 amended to drop the single-winner framing. ADR 0018 rewrite should
record "IS31 out; HEX-direct and RGBW both live" -- not a decided module.

## 2026-06-04 (cont. 9) -- Ben + Claude -- 4W RGBW characterized + full efficiency ranking (LED axis resolved)

Tested Adafruit 5163 (4 W addressable RGBW NeoPixel) direct-GPIO. At 3.3 V it's
**voltage-starved** -- Vf ~3.0-3.2 V, and the rail sags into that band under load
(bv->3.11 V at full), so current is non-linear and it only reaches ~half its rated
output (~430 mA vs ~800 mA at 5 V). Diagnostic: `rgbw-undervolt.png`. **It needs 5 V**
(unlike the hex, which under-volts gracefully). Cleaner re-run via `--wifi-lowpower`.

Final PAR-vs-draw efficiency ranking (`led-par-vs-draw.png`, slope = PAR/mA):
- **RGBW 4 W: steepest + highest PAR (~38)** -- brightest and most efficient *at high
  brightness*; but poor/non-linear dimming at 3.3 V and a single point source; wants 5 V.
- **HEX-direct ~0.07**, **HEX/NeoDriver ~0.055**, **NeoHEX ~0.04** (least efficient, out).

**Warm-white-only (RGBW W channel only, `--rgbw-white`):** the ultra-low-power "vibes"
mode -- **~78 mA at full but dim (PAR 8)** at 3.3 V (W channel under-driven; brighter at
5 V). Efficient (~0.09 PAR/mA) but low absolute output. Cleaner data this run (45 s
dwell + 100% cell) confirmed the earlier low-brightness "PAR>0, mA~0" was the measurement
floor (small LED current swamped by WiFi-baseline jitter), not real zero current. A clean
all-channel re-run (longer dwell) **agrees with the noisy one at the endpoints** (full
white ~430 mA / PAR 40, reproducible) and fixed the br=60 under-read (14->190 mA), **but the
mid-range stayed non-monotonic** (br=160 drew less current than br=100 yet more light) --
i.e. the messiness is the 4 W RGBW operating unstably *at its Vf on 3.3 V*, NOT measurement
noise. PAR (light) is monotonic; current is erratic. Confirms: the 4 W RGBW **needs 5 V**
for a clean/characterizable curve; at 3.3 V only the full-white point is trustworthy. So **LED
draw is a knob ~80 mA (dim warm) -> ~430 mA (full RGBW); the artistic brightness target
picks the point.** Added flags `--rgbw-white`, `--step-ms`.

**LED axis resolves to a use-case choice:** distributed dimmable glow -> **SK6812 HEX,
direct-GPIO @ 3.3 V** (no boost); single ultra-bright beacon -> **4 W RGBW, needs 5 V
boost**; ultra-low-power warm ambient -> **RGBW warm-white-only ~80 mA**. IS31 ruled out
(shared-bus brownout). Tooling added today: `--bright-sweep`,
`--sweep-max`, `--brightness`, `--pixel-pin`, `--wifi-lowpower`; `led_efficiency_sweep.py`
(+reboot-abort), `plot_led_eff.py`, `plot_par_vs_draw.py`, `plot_rgbw_diag.py`; Apogee
SQ-420 PAR reader.

## 2026-06-04 (cont. 8) -- Ben + Claude -- Direct-GPIO HEX validated; 3-way efficiency: direct-GPIO SK6812 wins

Soldered a 4-pin header on board 2 (3V3 * QON-NC * GND * A0=GPIO10) and drove the HEX
(SK6812) **direct from GPIO10** -- no NeoDriver, off the I2C bus. Validated working
(`--led neohex --pixel-pin 10`). Then a capped efficiency sweep (`--sweep-max`, new flag)
overlaid on the NeoDriver curves (`led-eff-3way.png`):
- **Efficiency order: hex-direct >= hex(NeoDriver) > neohex.** Direct-GPIO HEX is ~10% more
  light/mA than HEX-via-NeoDriver (no passthrough/overhead loss), and both SK6812 beat the
  WS2812C NeoHEX (~1.6x).
- **Direct draws ~1.7-1.8x current+PAR per brightness setting** vs NeoDriver (br=60: 362 mA/
  PAR27 vs 215 mA/PAR15) -- because the NeoDriver's Vin->pixel **passthrough drops voltage**
  and direct gives the LEDs the full 3.3 V (current is very VCC-sensitive near the WS2812/
  SK6812 low-V knee). Gap widens with current.
- **Confirmed by the 4-way 2x2** (`led-eff-4way.png`): NeoHEX shows direct~NeoDriver (low
  current -> negligible passthrough drop), while the high-current HEX shows the 1.7x gap -- so
  efficiency is a chip property (HEX 1.6x), and the path-difference is current-dependent.
- **BOM front-runner: SK6812 HEX, direct-GPIO** -- most efficient, fewest parts, brownout-safe
  by construction. Caveats: WS2812 latch their last frame (must send an explicit all-off to
  blank); connect/bring-up gently (full-white inrush browns the rail); higher VCC = browns a
  marginal cell sooner (run on a healthy pack / cap brightness).

Process findings logged: (1) board 2's USB-JTAG **auto-reset is flaky** -- after flashing, tap
the physical reset if the green LED doesn't come up (chip is healthy; verified via esptool
flash_id). (2) **SOC is trustworthy while the cell stays connected** (held 91->92% across a
USB->battery unplug, only bv relaxed ~0.3 V) -- the big SOC jumps earlier were from **cell
hot-swaps** resetting the gauge's coulomb state, not from USB power. New tooling: `--sweep-max`,
reboot-abort in `led_efficiency_sweep.py`, `ops/bench/plot_led_eff.py`.

## 2026-06-04 (cont. 7) -- Ben + Claude -- CORRECTION: NeoDriver does NOT boost pixel power (only the data signal)

Per Adafruit (product 5766): the NeoDriver's 5 V charge-pump is **only for the data
signal** ("clean 5 V signal even on 3 V boards") -- it does **NOT** power/boost the
NeoPixels. *"No way the STEMMA QT port can provide that much current... need external 5 V
on the terminal blocks."* Pixel power = whatever feeds Vin (3-5 V), passed through.
- **Corrects** the earlier (cont. 3/5) claim that the NeoDriver "boosts Vin->5 V,
  self-contained." It does not.
- Explains the "dimmer on 3V3": pixels run at **3.3 V (under their 3.7-5 V spec)** ->
  under-driven, not a boost current cap (the draw-vs-brightness curve doesn't plateau,
  confirming under-voltage scaling, not a current limit). On board 2's USB-hub 5 V the
  pixels got full 5 V -> "blindingly bright."
- **BOM consequences:** (1) full brightness needs a real ~5 V pixel supply -- battery
  (3.2-4.2 V) and 3V3 are below 5 V, so add a **5 V boost** for max brightness, or accept
  reduced brightness under-volted; (2) for dim/<=1 A operation under-volted is fine (matches
  the budget); (3) VBAT (<=4.2 V Li-ion) > 3V3 (3.3 V) for brightness without a boost;
  (4) the NeoHEX-vs-HEX efficiency was measured at 3.3 V (under-volt) -- SK6812 tolerates
  low V better, so re-check the 1.6x edge at the actual ship voltage.
- Plot of the comparison: `ops/bench/data/ca/led-eff-compare.png` (via new
  `ops/bench/plot_led_eff.py`).

## 2026-06-04 (cont. 6) -- Ben + Claude -- NeoHEX vs HEX efficiency: HEX (SK6812) ~1.6x more light/mA

Built brightness-sweep tooling: fw `--bright-sweep` (steps brightness {0,5,15,30,60,100,
160,255}, 30s each, light-WiFi held constant, reports `br=` in heartbeat; br=0 = LEDs off
for a clean baseline) + `--brightness` flag + `ops/bench/led_efficiency_sweep.py` (reads
Apogee SQ-420 PAR on USB + board `ima` over WiFi, groups by br, prints PAR-per-LED-mA).
Setup: 6" tube, PAR sensor at top pointing down, module at base, NeoDriver Vin from 3V3.

- **Result: HEX (SK6812) ~ 1.6x more light-efficient than NeoHEX (WS2812C-2020)** --
  PAR/LED-mA: NeoHEX ~0.040-0.045 (flat), HEX ~0.062-0.072, consistent across all
  brightness steps. At matched ~384 mA draw: NeoHEX PAR 15 vs HEX PAR 26 (~1.7x). HEX
  reaches higher max (PAR 30 @ 491 mA vs 16 @ 384 mA). **For the power budget, HEX wins.**
  Data: `ops/bench/data/ca/led-eff-{neohex,hex}.json`.
- Both SK6812/WS2812C are 37-px RGB (GRB), Grove->NeoDriver, no reflash to swap.
- **Caveats:** PAR is photon flux, not lumens (spectra differ, so perceived-brightness
  ratio may shift -- but 1.6x is consistent across 6 levels); 6" low-SNR geometry (dim
  steps noisy, mid-high solid); color/dimming-smoothness not measured (visual call, also
  tends to favor SK6812). Full-white NeoHEX/HEX off 3V3 = 384/491 mA LED -- within 1 A.
- Found + fixed a baseline bug: `setBrightness(0)` doesn't blank NeoPixels, so br=0 must
  set ledOn=false (color 0) for a true LED-off baseline.

## 2026-06-04 (cont. 5) -- Ben + Claude -- LED decision: IS31 ruled out, NeoHEX (via NeoDriver) leading; NeoHEX-vs-HEX + RGBW queued

- **3V3-powered NeoDriver works on battery:** board 1 (the brownout-prone unit) + NeoDriver
  fed from the **3V3 header** (dim, brightness 30 -> ~0.5 A from 3V3, under the 1 A limit),
  STEMMA for I2C, on battery + WiFi -> **no brownout** (Ben observed). Dim-30 is still
  "pretty bright." Added `--brightness` build-flag.
- **DECISION: IS31FL3741 13x9 ruled out for the V2 battery product.** Cause: its presence
  on the V2's shared charger/gauge I2C bus + WiFi reliably browns out on battery
  (well-proven, IS31-specific). Caveats noted: (a) untested mitigations -- VSYS bulk cap, or
  moving it to the *second* I2C bus (GPIO35/36, not the shared bus) -- might rescue it; (b)
  it's a 13x9 grid vs the hex form. **Revisit only if the grid aesthetic is a hard
  requirement.** Supersedes ADR 0018 (IS31 as primary module) for the battery build --
  flag ADR 0018 for an update.
- **Leading LED path: NeoHEX (WS2812C-2020) via Adafruit NeoDriver** -- no brownout, no
  solder on the I2C side, self-contained (NeoDriver boosts 3-5 V Vin -> 5 V + level-shifts
  data). Continue stability testing.
- **Queued tests:** (1) **NeoHEX (WS2812C-2020) vs HEX (SK6812)** head-to-head -- color
  quality, dimming smoothness (low-end PWM), power efficiency vs brightness, low-V behavior
  (SK6812 generally better at low V / finer PWM; WS2812C-2020 smaller/denser). (2) **single
  high-power RGBW LED.** (3) LED-current measurement at field brightness (folds into #1).

Fixed the brick-risk that ate ~1 h today (no-wake deep sleep stranded board 2, needed
BOOT+RESET download-mode + `esptool erase_flash`). fw `power-bench-2026-06-04.2`:
- **Never deep-sleep while external supply present** (USB/VDC) -- root cause of the
  stranding; on supply the board stays flashable/recoverable and there's no brownout
  risk anyway. `lgSupplyPresent()` = `getSupplyVoltage > 4.0 V`.
- **Timer wake** (15 min) instead of indefinite, via `esp_sleep_enable_timer_wakeup`.
- On a timer wake **still on battery -> re-sleep** (protect cell); **on supply -> run/
  charge**. So plugging USB self-recovers within one interval; can't brick.
- Unified `lgEnterDeepSleep()` (loop-break, coulomb-budget, lowbatt-knee, maxrun all
  route through it; LED-clear guarded for IS31/NeoPixel/NeoDriver). Compiles clean for
  all LED variants.
- **VALIDATED LIVE** (3 mAh budget / 60 s wake, `--budget-mah`/`--wake-s` flags): on USB
  ran continuously w/o sleeping (charging, mah=0); on battery hit the 3 mAh budget ->
  SLEEPING announce -> deep sleep; 124 s of timer-wake/re-sleep silence on battery; then
  USB plug -> recovered on the next wake (fresh boot, ima=+438 charging) with **no
  BOOT+RESET download-mode needed**. Brick-risk resolved.

## 2026-06-04 (cont. 3) -- Ben + Claude -- NeoDriver (I2C) is STABLE: brownout is IS31-SPECIFIC, not the bus

Built a `--led neodriver` variant (Adafruit NeoDriver 5766, SeeSaw I2C -> WS2812, on the
STEMMA bus; added Adafruit_seesaw lib + seesaw_NeoPixel in lgApplyLed). Drove a NeoHEX
full-white, **LED 5 V from an external USB hub** (LED current off the battery; the
NeoDriver boosts 3-5 V Vin -> 5 V and level-shifts data, per its silkscreen).

- **Result: STABLE** -- board 2, NeoDriver on the same shared I2C bus, battery + WiFi,
  full-white -> **371 s+, 0 reboots, through the heavy-WiFi phase**, bv steady 3.25. Same
  board/cell/bus/WiFi that **looped the IS31 within ~1 min**.
- **Verdict: the brownout is IS31-SPECIFIC**, not "any I2C device on the power-mgmt bus."
  Since the IS31 browns out even LEDs-off (presence alone), it's the IS31FL3741 chip's
  electrical behavior on SDA/SCL (back-current/loading during WiFi spikes), not LED
  current and not a general bus property. Matches Ben's hypothesis, isolated to the part.
- **LED-axis implication:** I2C LEDs are NOT categorically out. **NeoDriver + WS2812
  (NeoHEX) is a strong no-solder, self-contained LED path** (bright, onboard 5 V boost +
  data level-shift, no extra parts) that does NOT brown out the V2 on battery.
- **Caveats:** n=1, ~6 min; the IS31 was *intermittent* (stable for minutes before
  failing overnight), so the NeoDriver needs an **hours/overnight** run to trust. And
  that needs the **auto-sleep wake-source fix first** (brick-risk; on TODO) -- today the
  no-wake deep sleep + download-mode recovery cost ~1 h and corrupted board 2's WiFi
  (fixed via `esptool erase_flash`).

## 2026-06-04 (cont. 2) -- Ben + Claude -- IS31 presence on the I2C bus is NECESSARY for the brownout (clean A/B)

Decisive test: board 2, same deep-cycled cell, on battery, **IS31 physically unplugged**
-> **stable 365 s+, 0 reboots, through light AND heavy WiFi** (bv 3.27, soc 93). Versus
the same board+cell **with** the IS31 -> brownout loop. Only variable changed = the IS31
on the STEMMA/I2C bus.

- **The IS31's presence on the shared I2C bus is necessary.** Rules out cell+WiFi alone
  (stable) and WiFi-association-inrush alone (stable). Loops occurred in phase 0 with
  **LEDs off**, so it's **not LED current** -- it's the chip on the bus. Matches Ben's
  back-current / I2C-disturbance hypothesis.
- **Still open:** (a) IS31 *actively* misbehaving (spikes/back-current on SDA/SCL) vs
  (b) *any* I2C device loading the shared charger/gauge bus tips VSYS under WiFi.
  Next test: Adafruit NeoDriver (5766, I2C SeeSaw) on the same bus, NeoPixels powered
  externally -> also brownouts => (b); clean => (a). Needs a SeeSaw NeoPixel driver in fw.
- **Procurement note:** an I2C LED module on the V2's shared power-management bus is a
  real risk for the battery product; nudges toward a non-shared-bus (GPIO/SeeSaw-with-
  external-power) LED path, or bus isolation / bulk cap mitigation.
- Aside: board 2's WiFi wedged after the brownout/deep-sleep/download-mode gauntlet;
  recovered only via full `esptool erase_flash` + reflash + clean reboot (corrupted
  PHY/NVS). The loop-breaker's no-wake-source deep sleep also needed manual BOOT+RESET
  download-mode to reflash -- both reinforce the wake-on-USB fix already on the TODO.

## 2026-06-04 -- Ben + Claude -- Brownout CAME BACK overnight (794-reboot loop); guard flaw fixed; SOC/voltage thesis confirmed

Left board 1 on the loadgen on battery overnight (coulomb-budget auto-sleep at 91%
SOC). Morning: a **794-reboot loop over 4.25 h** -- every reset `poweron` (VSYS
collapse), at **healthy bv 3.24-3.46 across SOC 98%->30%**, in the **lightest** phase
(LEDs off, light WiFi), boots dying ~5-9 s in (around WiFi association). The first
boot ran 112 s, then a steady ~100 reboots / 30 min.

- **The brownout is real + intermittent on board 1.** Yesterday's "non-reproduction"
  (n=3 boards stable, capstone, wiggle) was the fluke; it drifts marginal over
  hours/temperature. Strengthens **H2 (marginal connection on board 1)**; per-boot
  trigger looks like the **WiFi-association current spike**, not load-stacking
  (lightest load) and not depletion (healthy V at every SOC).
- **Guard flaw (Ben called it):** coulomb-budget + max-runtime + low-V auto-sleep are
  all RAM state that resets each reboot, so a tight loop defeats them (`mah_used`
  never passed 1.4 of the 1000 mAh budget). It only bled slowly (92%->30%) because
  each short boot draws little. **Fix:** NVS-persisted boot counter (`--autosleep`) --
  clean start (USB/SW reset) zeroes it, `poweron` boots increment, >=25 sub-survival
  boots => deep sleep before WiFi.begin; a boot surviving 120 s clears it. fw
  `power-bench-2026-06-04.1`. Heartbeat now also carries `soc=` and `mah=`.
- **SOC/voltage thesis confirmed hard:** bv pinned at ~3.24 V for 4 h while gauge SOC
  drained 92%->30% -- LFP voltage is useless for SOC, but the gauge's coulomb count
  tracked the drain (it's the *voltage* that's untrustworthy, not the gauge number).
  Plots via new `ops/bench/plot_soc_v.py`:
  `2026-06-02-ca-liion-4400-soc_v.png` (Li-ion, usable slope) vs
  `2026-06-03-ca-lfp-overnight-soc_v.png` (LFP, near-vertical plateau). Logger:
  `ops/bench/loadgen_log.py` (JSONL + inline reboot flags + LED-current A/B).
- **Now running (2026-06-04):** same cell+grid on **pristine board 2**, multi-hour
  with the fixed guard -- board-specificity test (loop like board 1, or run clean?),
  and if stable it finally captures the LED-current A/B + LFP V-SOC discharge curve.

### 2026-06-04 (cont.) -- board 2 ALSO loops (NOT board-specific); loop-breaker validated

- **Board 2 (pristine) brownout-looped too** -- first boot 356 s (reached phase 1,
  grid lit), then collapsed on the USB->battery unplug (Ben watched the grid cut out at
  the instant of unplug = the first brownout), then looped (poweron, healthy bv ~3.23,
  soc ~72). So the brownout is **NOT board-1-specific** -- overturns the "board 1 solder
  joint" read. Common factors across all looping cases: the **cell** (deep-cycled
  overnight), the **IS31 grid + cable**, firmware.
- **Loop-breaker FIRED (fix validated in the wild):** board 2 deep-slept itself out of
  the loop. Logger saw only 8 reboots but the firmware NVS counter counts every boot --
  including the sub-association boots that die before sending any UDP -- so it hit 25 and
  slept while staying silent to the logger. Cell protected at ~72%/3.23 V.
- **Temperature ruled out** (Ben: office 72.5 deg F now, ~74 when it worked, 79 max -- too
  narrow to matter).
- **Leading hypotheses now:** (Ben) the **IS31 driver latching into a bad state** ->
  back-current/spikes on SDA/SCL (fits: IS31-unplugged always stable; `enableVSQT(false)`
  never helped = I2C back-power); vs the **deep-cycled cell's raised ESR** exposing the
  IS31+WiFi load. Next: (1) unplug IS31 + rerun same cell (presence necessary?), (2) GPIO
  WS2812 vs IS31 (I2C-specific vs load), (3) fresh cell + IS31 (cell-ESR).

## 2026-06-03 (cont. 2) -- Ben + Claude -- Brownout does NOT reproduce on n=3 boards; supersedes the "load-stacking" conclusion

**Walk-back of the entry below.** We lifted n=1->n=3 by moving the **same LFP cell,
same IS31 grid, same STEMMA cable** across three boards (only the board changed), then
re-tested the original board. Result: the brownout reproduces on **none** of them.

- **Board 2** (pristine): stable, light + heavy WiFi, 0 resets, bv to 3.19 V.
- **Board 3** (pristine): stable, light + into heavy, 0 resets, bv to 3.20 V.
- **Board 1** (the one that browned out earlier, capstone re-test, identical setup):
  **stable**, 4 min, 0 resets, bv 3.24 V.
- **Wiggle test** on board 1: 30 s of hard mechanical stress on the leads/connector
  **plus STEMMA hot-replugs** (the action that caused an instant reset earlier) ->
  **0 resets / 0 dropouts over 200 s**. Could not re-induce the collapse by any means.

**So both earlier conclusions are wrong/superseded:** not a platform "load-stacking"
property (boards 2/3 fine), not "board 1 anomalous" (board 1 now fine too). With board,
cell, grid, and cable all held constant, the only thing that changed across the
afternoon is **repeated unplug/re-seat of connectors** -> leading explanation is now
**H2: a marginal physical connection** (soldered battery joint and/or STEMMA seat) that
re-seated. **Inferred, not confirmed** -- we showed the brownout *stopped*, not *why*,
and could not reproduce it even deliberately. Also notable: stable while in **active
boost** at 3.24 V (the *harder* regime) argues against H3 (low-LFP/boost instability).

**Bottom line for procurement (unblocked):** three V2 boards run IS31 + continuous WiFi
on battery with zero brownouts down to ~3.2 V, so we **cannot** call V2 + IS31 unsafe on
battery. We also **cannot** claim full root-cause understanding (non-reproducible). Carry
a **VSYS bulk cap as cheap insurance** and watch for recurrence in the field. Full
write-up (Status, board-swap table, superseded sections) in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`. Lesson logged: we wrote a firm
conclusion twice today and were wrong both times -- n=1 + a single connection was not
enough.

## 2026-06-03 (cont.) -- Ben + Claude -- Brownout cause isolated: IS31-on-bus + WiFi (load-stacking) [SUPERSEDED by the entry above]

On a SOLID soldered LFP connection (the spring splice had confounded earlier runs)
and with cleaned-up instrumentation (uptime-based phase, no NVS write, `reset_reason`
+ battery V/I in the UDP heartbeat), the brownout reproduced cleanly and we isolated
it. Full write-up + open questions in
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`.

- WiFi off (any LED): stable. WiFi on + IS31 **unplugged** (light or heavy TX):
  stable (9 min, 0 resets, bv to 3.24 V). WiFi on + IS31 **connected**: `poweron`
  brownout ~7-17 s.
- **Cause:** load-stacking -- needs BOTH WiFi active AND the IS31 module physically on
  the STEMMA/I2C bus; neither alone does it. `reset_reason=poweron` (VSYS collapse) at
  healthy bv -> not depletion / connector / chemistry. Modem sleep did not fix it.
- **Sub-result:** firmware VSQT power-shed (`enableVSQT(false)`) did NOT fix it (~21
  resets / 7 min) -- only physically unplugging the module stops it. Candidate
  mechanism: I2C back-powering (IS31 stays on SDA/SCL off the main 3V3). Unproven.

Implications (firming, not final; n=1 board): **VSYS bulk capacitance** is the
mechanism-independent fix (bench-validate next); an **I2C LED module can't be
software-shed** (back-power) whereas a **GPIO WS2812** could; OTA-on-battery shouldn't
rely on VSQT-shed for the IS31 (use bulk cap / daytime solar / a GPIO module).

Also: ported demo gained an **Input Current Limit (IINDPM) slider** -- confirmed the
~500 mA USB charge cap is the **BC1.2/USB-C source-detection default** (not a port
bug; the SDK sets IINDPM=3200 but USB-C advertises current via CC, not D+/D-).
Doesn't affect solar/VDC charging. Tooling: loadgen heartbeat now carries
phase+uptime+bv+reset_reason+lb+sqt, low-batt backoff, and a `--loadgen-shed` mode.

## 2026-06-03 -- Ben + Claude -- Battery-brownout investigation: tooling, plan, ported demo (ONGOING, no conclusions yet)

Investigating the precise conditions under which the PowerFeather V2 takes a full
power-on reset on battery while running fine on USB. Observations so far are
partial and several are **confounded** (a marginal spring-splice test connection
on the bare LFP, battery type switched mid-investigation, stacked loads), so this
entry records **tooling and a plan, not findings**. Plan, hypotheses, and the open
test matrix are in `docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`.

Added bench tooling to `firmware/power_bench` (via `build.sh` flags):
- `--loadgen`: WiFi load generator (no HTTP server) emitting a UDP heartbeat with
  phase + uptime + battery voltage for remote outage/reset detection; auto-sweeps
  {light/heavy WiFi} x {LED off / full grid}. Phase persisted in NVS so it advances
  past (not retries) a phase that reboots the board.
- `--batt-stress` / `--batt-stress-full`: radio OFF, LED-panel heartbeat (center or
  full grid) -- radio-off baselines.
- `--wifi-lowpower` (modem sleep + 8.5 dBm), `--charge-ma`, `--ota` (wireless flash).

Ported PowerFeather's official ESPUI web-telemetry demo to V2 / SDK 2.x / core 3.x
(`firmware/powerfeather_demo_port`): SDK 1.x->2.x API (mV->V floats, maintain-voltage
units), `Generic_LFP`, and the ESP32Async core-3.x library stack. Compiles, boots,
and brings up the `PowerFeather_Demo` AP on V2 (verified on USB); web UI + on-battery
behavior still to exercise with a phone + a solid battery connection.

Next: re-run the matrix on a solid (soldered) LFP connection at known SOC.

## 2026-06-02 -- Ben + Claude -- PowerFeather V2.R2 power-bench bring-up (Phase A)

PowerFeather V2.R2 arrived. Stood up an Arduino-based power-telemetry bench
harness on it. New firmware `firmware/power_bench/` forked from `smoke_test`,
adding PowerFeather-SDK telemetry and a JSON `/telemetry` endpoint for WiFi data
collection across the three test axes (battery, LED option, solar panel).

Toolchain confirmed: FQBN `esp32:esp32:esp32s3_powerfeather`, board macro
`ARDUINO_ESP32S3_POWERFEATHER`, ESP32 core 3.3.7, PowerFeather-SDK 2.1.0
(namespace `PowerFeather`, singleton `Board`, `<PowerFeather.h>`). LED libs already
installed.

Battery chemistry is firmware-only (no jumpers): `Board.init(capacity_mAh,
BatteryType)` -- `Generic_3V7` for Li-ion (current), one-line swap to `Generic_LFP`
for LiFePO4. Note the SDK leaves charging DISABLED by default; the firmware now
calls `enableBatteryCharging(true)` with a conservative 200 mA cap (configurable).

Flashed and validated against the SDK validation plan (board `9E5AB8`, fixture on
WiFi at `192.168.4.185`), with a 4400 mAh PKCell Li-ion (2x18650), a 1 W panel on
VDC, and the IS31FL3741 13x9 on STEMMA-QT:
- Phase 1: I2C scan of Wire1 (STEMMA-QT, GPIO47/48) shows MAX17260 gauge (0x36),
  BQ25628E charger (0x6A), and IS31 (0x30) -> confirmed V2 hardware. The STEMMA-QT
  bus is shared by the power ICs and the LED module; the IS31 driver uses `Wire1`.
- Phase 2: `Board.init(4400, Generic_3V7)` returns `Result::Ok`; charging enabled
  at 200 mA cap; no SDK errors.
- Phase 3: `/telemetry` JSON serves correct values over WiFi -- `battery_v` 3.60 V,
  `battery_ma` +204 mA (charging at the cap), `supply_v` 4.665 V, `supply_ma`
  ~236 mA, `supply_good` true. Power balances: ~1.1 W in, ~0.73 W into the cell.

Two findings:
1. BUG (fixed): the float telemetry fields were one-position shifted due to C++
   unspecified argument-evaluation order -- the SDK getter was inlined as a function
   argument alongside the out-param it writes. Sequenced the getter before the JSON
   append (matching the integer-field pattern). Confirmed against the SDK's stock
   `SupplyAndBatteryInfo` example, which read correctly the whole time.
2. ROOT CAUSE FOUND + FIXED: `soc_pct/health_pct/cycles/time_left_min` returned
   `InvalidState` because the SDK selects the fuel-gauge IC at COMPILE TIME --
   MAX17260 (V2) only if `POWERFEATHER_BOARD_V2`/`CONFIG_ESP32S3_POWERFEATHER_V2`
   is defined, else the V1 `LC709204F`. In an Arduino build neither is set, so it
   defaulted to the V1 gauge and `probe()` failed on the wrong IC (the stock SDK
   example fails the same way for the same reason). A power-cycle did not help -- it
   was never a learning issue. Fix: build with `-DPOWERFEATHER_BOARD_V2=1` (now in
   `firmware/power_bench/build.sh`, with a `#error` guard in the sketch). With the
   flag: gauge = MAX17260, probe ok, `soc 7%`, health 100%, cycles 0, time_left,
   `telemetry_errors []`. Also added an init retry for the post-flash boot transient.

Also noted: mode `q` (quiet baseline) stops WiFi, so the WiFi logger must use mode
`0` (LEDs off, radio on) as its baseline. And the 200 mA charge current dominates
LED-current deltas, so clean LED measurement wants `-DRES_PF_ENABLE_CHARGING=0`.

Phase B done: `ops/bench/power_logger.py` (WiFi poller -> site-partitioned JSONL),
`power_summary.py`, `ops/bench/data/{ca,tn}/`, ADR 0020, and
`docs/tests/POWER_BENCH_HARNESS_2026-06-02.md`. Logger + summary validated against
the live board. Firmware variant builds (IS31/NeoHEX/RGBW) all compile.

## 2026-05-20 -- Ben + Codex -- PCBWay assembly quote revised toward J5-only

PCBWay's first assembly quote identified J1 / M5Stack A118 as the expensive and
slow part: about $32.82 for five assembled boards and 7-10 working days of
component lead time. Revised the PCBWay packet to match the practical prototype
path:

- Keep J1 pads in the Gerbers for later hand-solder/fit testing.
- Mark J1 DNP for assembly so PCBWay does not source the A118 connector.
- Use J5 as the assembled LED output through the Grove-to-STEMMA-QT cable.
- Keep C2 DNP.
- Update PCBWay notes and BOM to six placed SMD parts: J2, J3, J4, J5, R1, C1.

PCB fabrication counts remain 46 SMT pads and 14 drill holes. Assembly counts
are now six SMD components, zero through-hole components, and DNP parts J1/C2.

## 2026-05-18 -- Ben + Codex -- PCBWay packet prepared for NeoHEX adapter

Created `hardware/led-adapter/neohex-passive-rev-a/manufacturing/pcbway/` with
a self-contained quick-turn PCBA upload packet:

- `neohex-passive-rev-a-gerbers.zip` with Gerbers plus drill file.
- `bom-pcbway.csv` with only populated parts: J1, J2, J3, J4, J5, R1, C1.
- `neohex-passive-rev-a-pos-pcbway.csv` with C2 filtered out as DNP.
- `neohex-passive-rev-a-pos-all.csv` as a full centroid reference.
- `ORDER_NOTES.txt` and `README.md` with PCBWay settings, DNP notes, solder
  jumper notes, and pad/hole counts.
- `drc.rpt` showing zero violations and zero unconnected items.

For the PCBWay enquiry, use 46 SMT pads and 14 drill holes if they mean board
fabrication counts; use 7 SMD components and 0 through-hole components if they
mean assembly placement counts.

## 2026-05-18 -- Ben + Codex -- NeoHEX adapter gained JST-SH fallback output

Added a second LED-output receptacle to the NeoHEX passive adapter starter PCB:

- Kept J1 as the local M5Stack A118 HY2.0-4P SMD candidate.
- Added J5 as a stock JST-SH 4-pin SMT receptacle intended for an Adafruit
  4528-style Grove-to-STEMMA-QT cable.
- Wired J5 in parallel with J1 so Rev A can use either output without solder
  rework; the unused output should be left unplugged.
- Mapped J5 as `1 GND`, `2 VLED`, `3 NC`, `4 DATA_OUT`, matching the NeoHEX
  signal on the Grove yellow/SCL-position conductor.
- Updated the design packet, BOM, netlist, KiCad README, and TODOs.

`kicad-cli pcb drc` reports zero violations and zero unconnected items after
adding J5. Remaining risks are physical cable/footprint verification, J2 power
harness verification, and schematic capture/back-check.

## 2026-05-18 -- Ben + Codex -- NeoHEX adapter moved toward SMT PCBA

Ben preferred a PCBA-friendly adapter because the board will sit inside the
enclosure and should not see meaningful cable forces. Reworked the NeoHEX
adapter starter PCB away from through-hole populated connectors:

- Added local footprint library `hardware/led-adapter/neohex-passive-rev-a/kicad/resonance.pretty/`.
- Added local `M5Stack_HY2.0-4P_SMD_A118` candidate footprint for J1, based on
  the M5Stack A118 HY2.0-4P SMD connector dimensions.
- Replaced J2 with stock SMT
  `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal`.
- Grew the starter board to 72 mm x 35 mm so the larger SMT connector bodies,
  routing, and labels remain easy to inspect.
- Updated the J1 silkscreen label to `J1 HY2.0 SMD` next to the connector.

`kicad-cli pcb drc` reports zero violations and zero unconnected items after the
SMT conversion. The design is still not order-ready: physically verify J1
against the actual M5Stack Grove/HY2.0 cable, verify J2 against the chosen power
lead, and capture/back-check the schematic before sending to assembly.

## 2026-05-18 -- Ben + Codex -- Smoke mode 1 changed to max center

Changed COTS smoke firmware mode `1` from dim warm-white center to max-white
center for each board class:

- IS31FL3741: `LEDscaling=0xFF`, `globalCurrent=0xFF`, center pixel white.
- NeoPixel-backed boards: global brightness remains `255/255`, center pixel is
  now `(255, 255, 255)`.

Bumped firmware to `smoke-2026-05-19.1`, updated the smoke README and COTS mode
dashboard label to `1 Center Max`, built all four variants, and OTA-flashed:

- `192.168.4.248` / fixture `E41B2C` / C6 + IS31FL3741.
- `192.168.4.249` / fixture `570D32` / FeatherS2 Neo.
- `192.168.5.32` / fixture `1B5108` / Atom Matrix.
- `192.168.4.27` / fixture `55BA78` / Atom + NeoHEX.

All four boards reported `smoke-2026-05-19.1` and mode `1 center_max_white`
after flashing. Atom + NeoHEX needed a throttled OTA retry
(`curl -H 'Expect:' --limit-rate 40k ...`) after normal multipart upload attempts
failed.

## 2026-05-18 -- Ben + Codex -- KiCad 10 starter PCB for NeoHEX adapter

Ben upgraded KiCad from the Ubuntu 22.04 package to KiCad 10 via the KiCad PPA.
Verified `kicad-cli` is now available and reports `10.0.3`; the `pcbnew`
Python module also reports `10.0.3`.

Added a KiCad starter project at
`hardware/led-adapter/neohex-passive-rev-a/kicad/`:

- `neohex-passive-rev-a.kicad_pro` -- KiCad 10 project file.
- `neohex-passive-rev-a.kicad_pcb` -- routed 60 mm x 35 mm starter layout.
- `generate_starter_pcb.py` -- reproducible generator for the starter PCB.
- `README.md` -- KiCad-specific caveats and validation commands.

The starter layout keeps Rev A passive: external `VLED` injection, shared
ground, selectable STEMMA/GPIO data input, 330 ohm data resistor, local
decoupling, optional `SJ4` STEMMA_V+ bridge marked for low-current testing only,
and test pads. `kicad-cli pcb drc` reports zero violations and zero unconnected
items, and Gerber/drill export succeeds into `/tmp/res-neohex-kicad/`.

Important caveat: J1 is still a placeholder JST-PH 1x04 2.0 mm footprint standing
in for the exact M5Stack Grove/HY2.0 socket, and no schematic has been captured
yet. Do not order this board until J1 is replaced with the exact connector
footprint, cable pin order is verified, and the schematic/PCB are back-checked.

## 2026-05-18 -- Ben + Codex -- NeoHEX passive adapter Rev A design packet

Started a small PCB workstream for a no-solder-ish HEX/NeoHEX adapter board as both an educational PCB exercise and a possible 100-unit assembly aid.

Added `hardware/led-adapter/neohex-passive-rev-a/`:

- `README.md` -- design intent, schematic, connector pinouts, layout guidance, assembly variants, bring-up checklist, and open questions.
- `bom.csv` -- first-pass BOM for Grove/HY2.0 output, external LED power input, STEMMA/QT data input, optional generic GPIO input, data resistor, decoupling, jumpers, and test pads.
- `netlist.csv` -- explicit nets for KiCad capture.

Rev A is intentionally passive: connectors, shared ground, power injection, one data-source solder jumper, 330 ohm data resistor, and optional bulk capacitance. It does not include a boost regulator or constant-current driver. Added TODO items to capture the board in KiCad and order quick-turn boards.

## 2026-05-18 -- Ben + Codex -- Planned iso-current LED brightness test

Added `docs/tests/ISO_CURRENT_LED_BRIGHTNESS_TEST_2026-05-18.md` after visual smoke testing showed large brightness differences between full-low modes: roughly `FeatherS2 Neo >> NeoHEX ~= IS31FL3741 > Atom Matrix`, with the Atom Matrix diffuser likely contributing.

The new test plan separates electrical normalization from optical/gobo evaluation. It defines current targets, pattern classes, measurement setup with SEN0291 wattmeters, fixed-camera optical procedure, and result tables. Added a TODO item to run the test once the SEN0291 wattmeters are available.

## 2026-05-18 -- Ben + Codex -- Standalone Atom recovered on new subnet

The standalone Atom Matrix + DFRobot DFR0559 stack appeared unreachable from the dashboard at its old address `192.168.4.250`. After Ben moved it from the DFR0559 output to direct USB, serial confirmed it was healthy and connected to `BubbyNet`, but DHCP had assigned `192.168.5.32`.

Serial report:

- Board: `m5stack_atom`
- MAC: `F8:B3:B7:1B:51:08`
- Fixture ID: `1B5108`
- Reset reason: `poweron`
- Previous firmware: `smoke-2026-05-15.7`
- WiFi IP: `192.168.5.32`

OTA-updated the Atom to `smoke-2026-05-18.2` at `192.168.5.32` and updated the local COTS mode dashboard from the stale `192.168.4.250` address. The board was warm while powered from the DFR0559 even with LEDs off; no firmware fault was visible over USB. Follow up with SEN0291 current measurements on the DFR0559 5 V output before leaving that stack powered unattended.

## 2026-05-18 -- Ben + Codex -- NeoHEX center-cluster mapping adjustment

Ben observed that Atom + NeoHEX mode `3` appeared as a single seven-LED column. The placeholder NeoHEX crop used contiguous indices `15..21`, which confirms the NeoHEX chain appears to be indexed by hex columns rather than by a rectangular 3x3 layout.

Updated the Atom + NeoHEX crop for `smoke-2026-05-18.2` to use a first-pass center hex cluster around center index `18`: `11, 12, 17, 18, 19, 24, 25`. Built the Atom + NeoHEX variant and OTA-flashed `192.168.4.27`; the board came back as `smoke-2026-05-18.2`, and `/mode?m=3` succeeded.

Network scan found the reachable smoke boards at `192.168.4.27`, `192.168.4.248`, and `192.168.4.249`. The standalone Atom + DFRobot DFR0559 stack at prior address `192.168.4.250` remains unreachable; likely next checks are DFR0559 ON jumper position, battery/output recovery via BOOT, supply stability, and then USB serial recovery if needed.

## 2026-05-18 -- Ben + Codex -- Atom + NeoHEX smoke-test variant

Fourth COTS prototype connected over USB: M5Stack Atom Matrix v1.1 on an Atomic Battery Base, connected to M5Stack Unit NeoHEX over Grove.

Added a compile-time smoke-test variant for Atom + NeoHEX:

- Build flag: `--build-property compiler.cpp.extra_flags=-DRES_ATOM_GROVE_NEOHEX=1`
- Board name: `m5stack_atom_neohex`
- NeoPixel data pin: GPIO26, matching the Atom Grove yellow signal wire.
- Pixel count: 37.
- Initial center index assumption: 18.

USB-flashed the new Atom over `/dev/ttyUSB0`. It reported MAC `14:08:08:55:BA:78`, fixture ID `55BA78`, and joined home WiFi at `192.168.4.27`. The OTA web page reports `smoke-2026-05-18.1`, board `m5stack_atom_neohex`, and mode `0`. Verified `/mode?m=2` then `/mode?m=0` over HTTP.

Also OTA-updated the reachable C6 + IS31FL3741 board and FeatherS2 Neo board to `smoke-2026-05-18.1`. The original standalone Atom Matrix at `192.168.4.250` was not reachable during this pass and remains to be updated when powered/reconnected.

Updated the local COTS mode dashboard to include Atom + NeoHEX, and added the new stack to the LED measurement worksheet. The existing C6, FeatherS2, and regular Atom smoke-test builds still compile.

## 2026-05-15 -- Ben + Codex -- Brightness calibration fix for smoke-test modes

Ben observed that several LED measurement modes were effectively invisible, especially on the Atom Matrix: `4` full-low was invisible, `5` capped full-array was extremely faint, and `1` center was too dim. Root cause was double dimming on NeoPixel boards: low RGB component values were also being multiplied by low `Adafruit_NeoPixel::setBrightness()` values, causing integer scaling to round many channels down to 0 or 1. The IS31FL3741 full-low mode also used RGB values below RGB565's low-end quantization threshold.

Updated `firmware/smoke_test/` to `smoke-2026-05-15.7`:

- NeoPixel measurement modes now use `setBrightness(255)` and control current with explicit low raw RGB values.
- IS31FL3741 modes now avoid RGB565 values that quantize to black.
- Mode `1`, `3`, `4`, and `5` brightness levels were raised while keeping capped full-array modes conservative.

Built and OTA-flashed `.7` to all three unplugged boards over WiFi. All three returned to mode `0`, and `/mode?m=5` then `/mode?m=0` succeeded on C6 + IS31FL3741, FeatherS2 Neo, and Atom Matrix.

## 2026-05-15 -- Ben + Codex -- Static COTS mode dashboard

Added `ops/bench/cots-mode-dashboard.html`, a local static dashboard for the three active smoke-test boards:

- C6 + IS31FL3741 at `192.168.4.248`
- FeatherS2 Neo at `192.168.4.249`
- Atom Matrix at `192.168.4.250`

The page sends `/mode?m=<mode>` commands by iframe navigation rather than `fetch()`, so it works from a local `file://` page without requiring CORS headers from the ESP web server. It includes per-board and all-board controls for modes `0`, `1`, `2`, `3`, `4`, `5`, and `q`, plus embedded board status iframes.

## 2026-05-15 -- Ben + Codex -- OTA and USB flash timing benchmarks

Ben ordered 12 DFRobot SEN0291 I2C digital wattmeters, so manual USB power-meter experiments are on hold until they arrive. Added a TODO item to integrate the wattmeters into the power-test harness/worksheets.

Ran first flash timing benchmarks on `smoke-2026-05-15.6`; details are in `docs/tests/OTA_FLASH_BENCHMARKS_2026-05-15.md`.

Results:

- Strict sequential OTA, waiting for each board to be reachable again: 44.123 s for 3 boards.
- Parallel OTA batch: 18.291 s for all 3 boards to upload and become reachable again.
- USB upload, excluding compile time: C6 7.109 s upload / 10.188 s ready; FeatherS2 Neo 13.047 s upload / 16.218 s ready; Atom Matrix 14.287 s upload / 17.515 s ready.

FeatherS2 had one failed USB reset/upload attempt (`Errno 71`) that left it in the ESP32-S2 bootloader; a recovery USB upload succeeded, and a subsequent normal USB upload also succeeded. All three boards are back online at `smoke-2026-05-15.6`, mode `0`.

## 2026-05-15 -- Ben + Codex -- LED measurement firmware loaded on COTS smoke boards

Extended `firmware/smoke_test/` into a deterministic LED measurement harness and bumped it to `smoke-2026-05-15.6`.

New serial/HTTP measurement modes:

- `q` -- quiet baseline: stop OTA/WiFi and clear LEDs.
- `0` -- LEDs off, current WiFi/OTA state unchanged.
- `1` -- center dim warm white.
- `2` -- 3-pixel RGB fringe.
- `3` -- center 3x3 dim warm white.
- `4` -- full-array very-low white.
- `5` -- full-array capped white, brief measurements only.

The OTA status page now shows the active mode and exposes `/mode?m=<mode>` links, so the USB current meter workflow can use either serial commands or `curl` while WiFi OTA is active. Added `docs/tests/COTS_LED_MEASUREMENTS_2026-05-15.md` as the worksheet for current and optics readings.

Built and uploaded `smoke-2026-05-15.6` over HTTP OTA to all three connected boards:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- M5Stack Atom Matrix: `192.168.4.250`

All three served `Version: smoke-2026-05-15.6`, accepted `/mode?m=1`, and were left in mode `0` with LEDs off and OTA still available. LED-current readings are still open; record them in the new worksheet.

## 2026-05-15 -- Ben + Codex -- Home-WiFi web OTA validated on all three COTS smoke boards

Committed and pushed the initial smoke-test baseline as `f36595e Add COTS smoke test firmware`.

Added station-mode web OTA support to `firmware/smoke_test/`:

- `wifi_secrets.h` is now ignored by git.
- `wifi_secrets.h.example` documents the local secrets format.
- Serial command `w` connects to configured WiFi and starts the same web updater.
- Serial command `o` still starts temporary AP OTA mode.
- `RES_WIFI_AUTO_CONNECT` allows bench firmware to enter WiFi OTA maintenance mode on boot.
- The web updater page now reports board, fixture ID, and firmware version.

Created a local ignored `wifi_secrets.h` for Ben's home WiFi and USB-flashed `smoke-2026-05-15.3` to all three boards as the WiFi-enabled OTA baseline. All three connected to the home WiFi and started web OTA:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- M5Stack Atom Matrix: `192.168.4.250`

Then built `smoke-2026-05-15.4` and uploaded the app binaries over HTTP OTA to all three boards:

- `curl -F firmware=@/tmp/res-c6-ota/smoke_test.ino.bin http://192.168.4.248/update`
- `curl -F firmware=@/tmp/res-feathers2neo-ota/smoke_test.ino.bin http://192.168.4.249/update`
- `curl -F firmware=@/tmp/res-atom-ota/smoke_test.ino.bin http://192.168.4.250/update`

All three returned `Update complete. Rebooting.` and reconnected, serving `Version: smoke-2026-05-15.4` from their OTA web pages.

Open follow-up: `RES_WIFI_AUTO_CONNECT` is convenient for bench testing but should stay off in committed examples and production-like firmware. Production should enter OTA only in explicit maintenance mode.

## 2026-05-15 -- Ben + Codex -- COTS smoke firmware built, flashed, and serial-verified

Added `firmware/smoke_test/`, an Arduino CLI smoke-test sketch for the first three COTS prototypes. It builds for:

- `esp32:esp32:adafruit_feather_esp32c6:CDCOnBoot=cdc,PartitionScheme=min_spiffs`
- `esp32:esp32:um_feathers2neo:PartitionScheme=min_spiffs`
- `esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs`

The sketch prints a serial boot report, MAC-derived fixture ID, reset reason, heap, OTA partition labels, board pin summary, I2C scan results, and a conservative LED test. It also includes a serial-command-triggered temporary AP web updater (`o` command) for future OTA smoke testing without hard-coded WiFi credentials.

Installed Arduino libraries needed for the smoke pass: Adafruit IS31FL3741 Library 1.2.3, Adafruit BusIO 1.17.4, Adafruit GFX Library 1.12.6. Existing Adafruit NeoPixel 1.15.4 is used for the built-in 5x5 matrices.

All three boards were flashed and serial-verified:

- Adafruit Feather ESP32-C6 + IS31FL3741: firmware `smoke-2026-05-15.2`, MAC `58:E6:C5:E4:1B:2C`, fixture ID `E41B2C`, I2C devices `0x30` (IS31FL3741) and `0x36` (likely onboard battery monitor), IS31 initialized, OTA partition `app0`.
- FeatherS2 Neo: firmware `smoke-2026-05-15.2`, MAC `48:27:E2:57:0D:32`, fixture ID `570D32`, built-in 25-pixel matrix on GPIO21, no I2C devices found, OTA partition `app0`.
- M5Stack Atom Matrix: firmware `smoke-2026-05-15.2`, MAC `F8:B3:B7:1B:51:08`, fixture ID `1B5108`, built-in 25-pixel matrix on GPIO27, no I2C devices found, OTA partition `app0`.

Notes:

- Arduino builds should not be run in parallel against the same sketch/cache; mixed RISC-V/Xtensa objects corrupted the Arduino cache. Sequential builds with explicit `--build-path` work.
- The smoke LED test intentionally limits both total lit pixels and PWM/global brightness. This matches the gobo/patterned-aperture direction and avoids M5Stack Atom Matrix full-brightness stress.
- End-to-end OTA upload through the temporary AP is implemented but not yet tested from a browser/client.

## 2026-05-15 -- Ben + Codex -- First COTS prototype USB inventory and interim C6 matrix path

Three COTS prototype boards arrived and were connected over USB for first bench bring-up:

- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 RGB LED matrix over STEMMA-QT. This is an interim substitute for the delayed PowerFeather matrix stack, useful for IS31FL3741 I2C, LED-current, OTA, and gobo/optics testing, but not a substitute for PowerFeather `VSQT`, LiFePO4 charging, fuel-gauge, sleep-current, or solar telemetry validation.
- M5Stack Atom Matrix with built-in 5x5 LEDs, USB-powered for now.
- UnexpectedMaker FeatherS2 Neo with built-in 5x5 LEDs, USB-powered for now.

USB/serial inventory on Ben's Linux bench:

- `/dev/ttyACM0` -- UnexpectedMaker FeatherS2 Neo, USB VID:PID `303a:80b5`, serial `84722E75D023`, Arduino FQBN `esp32:esp32:um_feathers2neo`.
- `/dev/ttyACM1` -- Adafruit Feather ESP32-C6 via Espressif USB JTAG/serial, USB VID:PID `303a:1001`, serial `58:E6:C5:E4:1B:2C`, Arduino FQBN `esp32:esp32:adafruit_feather_esp32c6`.
- `/dev/ttyUSB0` -- M5Stack Atom Matrix via FT232, USB VID:PID `0403:6001`, serial `8D529F3938`, Arduino FQBN `esp32:esp32:m5stack_atom`.

Local tool state: Arduino CLI is installed with `esp32:esp32` core 3.3.7. No repo firmware exists yet beyond architecture docs. No firmware was flashed during this inventory pass.

Immediate test direction: create a small USB smoke/OTA bring-up firmware before broader firmware architecture work. It should print board ID, MAC-derived fixture ID, reset reason, build version, LED driver status, I2C scan results where applicable, and OTA status. Use LiPo-only DFRobot DFR0559 tests for now and do not connect LiFePO4 to LiPo-only boards.

## 2026-05-11 -- Ben + GPT -- PowerFeather SDK 2.0.0 release confirms V2 support path

PowerFeather-SDK 2.0.0 was released shortly after the PowerFeather V2 hardware/schematic review. This is a strong positive signal that the PowerFeather developer is active and that V2 is far enough along to have first-class software support.

Key release-note items relevant to Resonance Lighting:

- Adds PowerFeather V2 board support selectable through ESP-IDF Kconfig or `POWERFEATHER_BOARD_V2`.
- Adds MAX17260 fuel-gauge support, including battery current, health, cycles, time estimates, alarms, learned-state restore, LiFePO4 mode, and custom MAX17260 battery profiles.
- Adds a shared fuel-gauge abstraction for LC709204F and MAX17260, which should let Resonance firmware support V1/LiPo fallback and V2/LiFePO4 paths behind one interface.
- Adds `BatteryType::Generic_LFP`, directly matching the project's preferred LiFePO4 chemistry.
- Adds `Board.init()` for no-battery operation and `Board.init(const MAX17260::Model&)` for custom battery profiles.
- Adds `updateBatteryFuelGaugeTemp()` overload that reads the board thermistor and updates the fuel gauge.
- V2 keeps the power-management I2C bus available while `VSQT` is disabled. This matters because Resonance wants to turn off external LED modules / STEMMA-QT loads while preserving housekeeping telemetry.
- Charger settings can be retained across RTC-preserving warm boots when battery/profile configuration still matches.
- Custom profiles now apply profile charge voltage and termination current to the charger.
- Initialization safety was improved: charger part validation, POR/watchdog recovery, profile-change detection, and full policy reapplication.
- MAX17260 LFP configuration, profile loading, learned-parameter handling, voltage alarms, and fuel-gauge reinitialization were fixed.
- Missing/open/shorted battery temperature sensors now get sanity checks.
- I2C fault latency was reduced with bounded transfer timeouts and the newer ESP-IDF I2C master driver.
- ESP-IDF requirement is now >=5.2, <=5.5.

Interpretation:

PowerFeather V2 is no longer just an attractive schematic. It now has explicit SDK support for the exact features Resonance cares about: LiFePO4 fuel-gauge mode, MAX17260 telemetry, thermistor integration, custom profiles, power-domain behavior with `VSQT` off, and improved recovery from charger/gauge initialization edge cases.

Action:

- Treat PowerFeather V2 + PowerFeather-SDK 2.x as the primary COTS LiFePO4 prototype path.
- On first hardware arrival, verify the boards are truly V2 by visual chip ID and I2C scan.
- Build first firmware with ESP-IDF >=5.2 and PowerFeather-SDK 2.x, not the older 1.x docs/examples.
- Add a small compatibility layer in Resonance firmware so PowerFeather telemetry can be consumed by the normal battery/power telemetry interface.
- Capture telemetry from BM 2026 fixtures if this platform or a PowerFeather-derived custom board is used; this data should inform BM 2027 solar/battery sizing.

Open questions:

- Does the Elecrow stock currently shipping as "ESP32-S3 PowerFeather V2" contain V2 hardware, or could it be V1 stock/listing ambiguity?
- Will the developer share V2 KiCad layout files, or only schematic/3D model?
- How well has V2 been tested with actual LiFePO4 cells under solar/VDC input?
- Does the SDK expose enough raw charger/fuel-gauge telemetry for long-term logging without significant custom driver work?

## 2026-05-10 -- Ben + ChatGPT -- PowerFeather V2 / COTS R&D update

Second-pass architecture update after COTS search, purchases, and schematic review.

### What changed

- **PowerFeather V2 is now the leading COTS/reference architecture.** It appears to match the project unusually well: ESP32-S3-WROOM-1, onboard PCB antenna, BQ25628E charger/power-path, LiFePO4 support in V2, MAX17260 fuel gauge, TPS631013 buck-boost 3.3 V rail, switchable VSQT/STEMMA-QT rail, solar/DC input, and rich power telemetry. V2 status is still preliminary until hardware arrives and is verified.
- **PowerFeather V1 remains LiPo-only as a board-level system.** V1 uses BQ25628E, but the board-level fuel gauge and regulator choices make it unsuitable for LiFePO4 production use. It may still be a strong LiPo fallback.
- **PowerFeather V1/V2 schematic diff completed.** V1 and V2 both use BQ25628E. V2 swaps the 3.3 V regulator from XC6220 LDO to TPS631013 buck-boost, swaps the fuel gauge from LC709204F to MAX17260, adds a 20 mohm current-sense resistor, and adds I2C power-domain isolation around the STEMMA-QT rail.
- **COTS purchases made.** Ben bought the R&D candidates discussed in the COTS survey except USB power meters, which are already on hand. Elecrow PowerFeather boards were ordered despite possible ambiguity about whether the listing is V2 or V1. Ben also contacted the PowerFeather creator about V2 availability and KiCad files.
- **LED module plan narrowed.** The Adafruit IS31FL3741 13x9 RGB matrix is the leading plug-and-play STEMMA-QT LED module for PowerFeather. M5Stack NeoHEX is promising optically but is WS2812/Grove, not STEMMA-QT/I2C, and likely needs a GPIO data line plus a 5 V or otherwise suitable LED rail. M5Stack Atom Matrix is a compelling all-in-one fallback with ESP32 + 5x5 LEDs + USB-C.
- **Battery sourcing narrowed.** Prefer one larger LiFePO4 cell per fixture, ideally 18650 1500-2000 mAh, instead of multiple 14430 cells in parallel. 14430 cells are easy to find and cheap, but packs of many small cells add contacts, matching, wiring, assembly, and QA risk.
- **Solar-panel plan clarified.** Square/rectangular 1-5 W panels are fine for R&D. Round panels remain aesthetically attractive for production but are harder to source quickly and should not block testing.

### Current COTS prototype tracks

1. **PowerFeather V2 + LiFePO4 + solar panel + Adafruit IS31FL3741 13x9 matrix.** Primary design-aligned candidate.
2. **PowerFeather V2 + LiFePO4 + solar panel + M5Stack NeoHEX.** Alternative LED geometry test; not STEMMA-QT plug-and-play.
3. **FeatherS2 Neo + DFRobot DFR0559.** LiPo fallback: DFR0559 owns battery/solar, FeatherS2 Neo battery JST stays empty, Feather is powered over USB.
4. **M5Stack Atom Matrix + DFRobot DFR0559.** Ultra-simple LiPo fallback: small ESP32 + 5x5 LEDs powered by USB from the solar manager.

### Immediate tests once parts arrive

- Confirm whether Elecrow PowerFeather boards are V2 or V1 by chip markings and I2C scan.
- Verify LiFePO4 configuration and charging behavior on actual V2 hardware before trusting it.
- Measure sleep current with VSQT off and LED modules attached.
- Measure solar harvest and charge behavior for each 1-5 W panel under sun, shade, and heat.
- Compare IS31FL3741, NeoHEX, FeatherS2 Neo, and Atom Matrix for gobo projection, brightness, color fringing, PWM artifacts, current draw, and mechanical fit.
- RF-test each candidate inside a mock hat with panel, battery, screws, and wiring in realistic locations.
- Validate fail-safe behavior: LEDs stuck on, MCU hang, watchdog reset, low-battery cutoff, and recovery from depleted battery when solar input returns.

### Follow-up docs added

- `docs/research/COTS_SURVEY_2026-05-10.md`
- `docs/research/POWERFEATHER_V1_V2_SCHEMATIC_NOTES_2026-05-10.md`
- `docs/tests/COTS_BENCH_TEST_PLAN_2026-05-10.md`
- ADR 0015 -- PowerFeather V2 as leading COTS/reference architecture
- ADR 0016 -- Purchased COTS prototype shortlist
- ADR 0017 -- Battery cell format and sourcing
- ADR 0018 -- LED module/interface plan

## 2026-05-06 -- Ben + Claude (Cowork) -- Pre-share cleanup pass

Final cleanup before pushing the repo to GitHub and sharing with Steve and the wider team:

- **Bamboo "cone" -> "lantern" / "cylinder".** The bamboo piece is geometrically a cylinder with a steam-bent flared skirt at the bottom, not a cone. The only cone-shaped object in the project is the experimental projective-geometry filter / gobo. Scrubbed every "bamboo cone" reference across BACKGROUND, ROADMAP, README, AGENTS, glossary, ADR 0007, hardware/references, ops/bom, enclosure README. Gobo "cone" references preserved.
- **Agent-neutral voice.** Rewrote BACKGROUND.md from a Ben-addressed narrative into a third-person project-context document. Replaced "Ben (you)" with "Ben Eckart" throughout. Replaced "Dad" with "Steve Eckart" outside this LOG file.
- **Scrubbed historical / distracting context** from active docs. Removed "Critical dates" stale-deadline table from BACKGROUND. Removed crossed-out resolved items from TODO and ROADMAP. The narrative of "we initially thought X, then learned Y" now lives only in this LOG; active docs present the current state cleanly.
- **New ADR 0009 -- Minimize per-fixture operations at scale (O(1), not O(N)).** Captured Ben's strong constraint that anything done per-fixture is multiplied by 100. Specifies: no soldering on receipt; same firmware for every fixture; per-unit identity from MAC; investigate JLCPCB pre-flash service; design pogo-pin flashing jig as fallback. Reinforced in `README.md`, `hardware/README.md`, `TODO.md`. This is now the ninth and (so far) final ADR.

After this pass, the active docs (`README`, `AGENTS`, `BACKGROUND`, `TODO`, `ROADMAP`, `SYSTEM`, ADRs, glossary) read as a clean shared documentation set for Ben + Steve + future AI agents + the wider Resonance team. The journey from "what is this project" through "let's design solar lights" to "modular hat with LiFePO4 carrier board with O(1) ops" lives in this LOG.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Logistics flow confirmed: air-ship to TN, integrate at Grass Valley

Big risk-register item resolved: **Bamboo Pure is air-shipping a small batch of prototype bamboo lanterns to Steve in Tennessee.** Electronics workstream is fully decoupled from the May 10 Bali sea container. The end-to-end logistics flow:

1. Bali -> TN: prototype lanterns by air for early mechanical prototyping (Phase 2).
2. Bali -> Grass Valley, CA: tree structure + remaining bamboo by sea container.
3. Ben (CA): designs PCB, ships to Steve.
4. Steve (TN): finalizes hat enclosure with both bamboo and PCB in hand.
5. Steve -> Ben (TN -> CA): ships 100 hats.
6. Ben -> Grass Valley: drives hats + electronics to meet the bamboo container at the staging area.
7. Grass Valley: final integration. Truck to BRC.

**Updated docs:**

- `docs/ROADMAP.md` -- Phase 2 dependencies, Phase 6 rewritten as cross-country logistics + Grass Valley integration, risk register marked resolved, open dependencies list updated.
- `TODO.md` -- removed urgency on "catch Elliot before Bali," removed ship-path decision (resolved), added air-ship-timing confirmation.

**What this changes practically:**

- Phase 2 (mechanical prototyping) can start as soon as bamboo arrives in TN, not when Elliot returns from Bali.
- Phase 5 production fab no longer races a container deadline.
- Phase 6 is a cross-country logistics piece with TN -> CA -> Grass Valley flow rather than US -> BRC direct.
- Grass Valley pre-build staging area is now the canonical "integration site" terminology.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Roadmap, power-budget correction, prototyping strategy

Three additions:

**`docs/ROADMAP.md`** -- phases 0-10, working backward from BM 2026 (late August). Phase 1 (TTGO bench prototype) starts 2026-05-07 and runs ~3 weeks. Phase 3 (custom carrier board v1) lands ~2026-07-01. Phase 5 (production fab) ~2026-08-01. Risk register and open dependencies on team included.

**Prototyping strategy clarification.** The "validate the architecture before committing to LiFePO4 silicon" risk is fully mitigated by Phase 1 -- using the **TTGO T-Beam (with its built-in TP4056 LiPo charger)** as the LiPo prototype platform. No intermediate "LiPo carrier board" needed -- that would add a board spin without de-risking anything Phase 1 doesn't already cover. The CN3058 LiFePO4 charger circuit is the only chemistry-specific portion; we lift its reference circuit from datasheet, AI-review, and validate on Phase 3 v1 board with MCP73123 as designed-in fallback. (Captured in `docs/ROADMAP.md`, not yet a separate ADR -- promote to ADR if revisited.)

**Power budget correction.** Earlier estimate assumed "4 WS2812B all on at once" yielding ~10 mA LED average. Actual usage model is **1-9 LEDs per fixture, typically 1-3 lit at a time** (default ambient = 1 LED at 10%, showy = 3 LEDs at 30%, wand-burst = 9 LEDs full but rare and brief). Per-LED current scales linearly per WS2812B datasheet -- confirmed against 2018 Talisman v2 measurements on the 16-LED ring (500 mA / 16 = 31 mA per LED at full white, matching). Updated `docs/block-diagram/SYSTEM.md`:

- Per-LED reference table replaces "4-LED ring" table.
- Time-weighted nightly LED current ~5 mA (vs. 10 mA estimated earlier).
- Total daily drain ~120 mAh (vs. 170 mAh).
- Panel sizing recommendation now 1-2 W (vs. 2 W); 1 W is sufficient.
- Battery: 18650 still preferred for 12-night autonomy and 2-year life; 14430 (~3 nights) now reasonable if cell sourcing forces it.
- BOM updated for 1-9 LED count per fixture.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Handoff documents

Before switching to Claude Code for daily iteration, dumped context to handoff-friendly artifacts so future agents (Ben's Claude Code, Steve's Claude Code, Elliot's Co-Work, future Cowork sessions) can pick up cold:

- `AGENTS.md` at root -- explicit preamble for any agent picking up this repo. Read order, who's working, what's known vs assumed, what the repo does NOT cover, when to ask Ben.
- `docs/block-diagram/SYSTEM.md` -- the canonical system architecture. ASCII block diagram, voltage rails, current draw table grounded in 2018 Talisman v2 measurements + ESP32-C3 datasheet, single-fixture daily power budget (~170 mAh/night, well covered by 2 W panel + 1500 mAh 18650), back-of-envelope max-stress check for wand-interaction events. Cost-comparison sketch vs `INV_2026_00401`.
- `docs/decisions/` -- eight ADRs: ESP32-C3-MINI-1 (0001), LiFePO4 chemistry (0002), CN3058 charger (0003), ESP-NOW mesh (0004), FreeRTOS task architecture (0005), custom PCB not dev-board-on-carrier (0006), modular hat enclosure (0007), WS2812B from Vbat with no level shifter (0008).
- `firmware/ARCHITECTURE.md` -- RTOS task decomposition (`led_render_task`, `ca_tick_task`, `mesh_tx_task`, `mesh_rx callback`, `housekeeping_task`), inter-task communication via FreeRTOS queues + atomic shared state, sleep behavior, boot sequence, OTA strategy.
- `hardware/atopile/EXAMPLE.md` -- sample atopile module (`voltage_regulator.ato` for the AP2112K-3.3 LDO) so the schematic-as-code pattern is concrete. List of modules to build.
- `ops/bom.md` -- first-pass BOM grouped by carrier-board electronics, non-PCB electronics, and mechanical. Per-fixture target ~$23. 100-fixture total ~$2,310.
- `docs/glossary.md` -- proper nouns and acronyms for new agents dropping in cold.

These files are now the canonical project context outside this conversation. The earlier `BACKGROUND.md` remains the long-form narrative.

Switching to Claude Code from here. Cowork retains read access to this repo via GitHub (when pushed) for review and project management.

---

## 2026-05-06 -- Ben + Claude (Cowork) -- Repo bootstrap

Stood up this repo. Ported `BACKGROUND.md` from earlier Cowork session -- captures full project context, team, decisions to date, prior-art lessons from 2018 Talisman v2 build, code reusable from `beneckart/future-robotics`, and the design space for this year (electronics architecture, mandala filter program, mesh creative possibilities).

Decisions baked in so far (subject to team review):

- **MCU:** ESP32-C3-MINI-1 for production. Prototype on TTGO T-Beam and T-Ice modules already in Steve's workshop.
- **Battery chemistry:** LiFePO4. Chosen for thermal tolerance in desert deployment.
- **Charger IC:** CN3058 (LiFePO4-tuned, JLCPCB basic part, ~$0.30). Rejected TP4056, bq24074, CN3791 -- all LiPo-tuned, wrong charge profile.
- **3.3 V LDO:** AP2112K-3.3 (450 mV dropout, JLCPCB basic part, fits LiFePO4's 2.5-3.6 V range).
- **LEDs:** 1-4 WS2812B per fixture, powered direct from battery rail (3.3 V GPIO satisfies WS2812B's 0.7 x Vcc threshold per Talisman v2 verification).
- **Mesh:** ESP-NOW. No infrastructure required at BRC.
- **OTA:** required from day one. One USB-C flash per device, then over-the-air forever.
- **Enclosure:** sealed 3D-printed solar "hat" that sits partially inside / partially over the bamboo cone top. Set screws absorb bamboo dimensional variability.

Open team-side questions (see `BACKGROUND.md` and `TODO.md` for full list):

- Rope attachment point: hat, bamboo, or hybrid. Pending Vishnu / Ed / Elliot.
- Container vs separate ship for electronics. Bamboo ships from Bali 2026-05-10.
- Hat dimensions confirmation to Vishnu so he can finalize renders.
- INV_2026_00401 cost decomposition.

Next concrete steps for Ben + Steve:

1. System block diagram + power budget (highest-leverage upstream artifact).
2. atopile module library: `solar_input`, `lifepo4_charger`, `power_path`, `voltage_regulator`, `esp32_module`, `led_output`. Build each from reference schematics.
3. Bench validation on existing TTGO modules -- solar charging path first.

Switching to Claude Code for daily firmware/hardware iteration. Cowork retains read access to this repo via GitHub for project management and review.
