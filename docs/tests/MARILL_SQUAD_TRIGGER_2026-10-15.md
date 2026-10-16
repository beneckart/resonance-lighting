# Marill squad trigger canary -- 2026-10-15

Owner/operator: Ben + Codex, sole USB writer. Target: Marill F40348
(68:EE:8F:F4:03:48), direct USB, battery attached. Squad receivers are nearby,
charged, initially asleep; no receiver changes are part of this canary.

## Artifact

- Source: 3638d01ba14b9d1b97a00e71baf9fdf8a63b2a0d
- Revision: fx-261015-86ca5b1-b
- Application SHA-256:
  9bd27f67f58887fb261082e17e9366a9279fcec74e53fbcf622fc39c8eff2a25
- Application bytes: 1,233,776
- Build: field, channel 11, basic listener, 300 mA precharge, 120 s sleep,
  12,000 ms listen, exact-target PROTECT recovery and squad controller F40348.
- No RES_DECOMPRESSION receiver flag. Configuration and other app slot kept.
- Retained under firmware/fixture/build/fx-261015-86ca5b1-b; manifest copied to
  ops/bench/data/Oakland/fx-261015-86ca5b1-b-manifest.json.

## Software checks

All fixture native tests passed, including the 391-check squad cycle/presence/
framing/legacy-event compatibility test and 343-check PROTECT recovery test.
Build-wrapper contract checks passed. Cold Arduino build completed with
1,233,441 program bytes (36%) and 70,124 global-data bytes (21%). Deployed
flags in build.options.json and manifest matched the requested exact target.

The local live viewer was checked in the browser: layout, both distance scales,
live telemetry, stale/wake gaps, and zero reported browser errors. Original
firmware preview captured returns around 29..41 cm as well as the fixed close
housing return; the page explicitly labels the old image Preview only.

## USB procedure and evidence

Fresh preflight: previous fx-261015-d953f80-b, app1, FULL, battery 3.332 V.
The initial temporary-flasher backup failed after about 20 KiB; a lower-speed
retry failed after about 32 KiB. No application write had occurred. Only the
paused live viewer process was present; no competing serial helper was found.
This does not establish an extension/cable diagnosis.

A no-stub ROM read then successfully preserved all 64 KiB at 0x000000 in
34.8 seconds. Partition bytes matched the artifact, app1 was confirmed at
0x340000 with length 0x330000, and ROM again confirmed the exact full MAC.
The write/verify procedure uses the same no-stub ROM path and touches app1 only.

Evidence under ops/bench/data/Oakland:

- 20261015T225620Z-F40348-trigger-flash.jsonl: fresh preflight and failed first read.
- 20261015T225901Z-F40348-trigger-rom-flash.jsonl: ROM write/verify continuation.
- 20261015T224813Z-F40348-live-presence.jsonl: closed original-image preview.
- 20261015T225640Z-F40348-live-presence.jsonl: resumed live observer.

ROM prefix backup stays private at
ops/bench/private-backups/20261015T225620Z-F40348-pre-trigger/prefix-64KiB-rom.bin.

## Hardware acceptance

ROM application and prefix verification both passed at 23:00:06 UTC. The reset
request returned the expected port-loss error; fresh application telemetry then
confirmed the new revision, app1, FULL, guard stage 1, healthy VL53/MSA sensors,
and retained recovery support. At one-hour session admission, VBAT was 3.355 V.
The exact-target session command was acknowledged and reported 3,597 seconds
remaining. Evidence: 20261015T230009Z-F40348-trigger-command.jsonl.

The first session did NOT pass the ten-step test. All nine expected revisions
were heard. Presence admitted step 1 at 23:01:29 UTC (Ness, queued mask 001);
TX errors appeared at 23:01:33. Step 2 at 23:01:38 advanced but queued mask 000.
All READY receipts expired by 23:01:45. At 23:02:18, heap was 209,424 bytes,
VBAT 3.359 V, FULL, sensors healthy, ESP-NOW reported up, but 132 sends had
failed. The exact immediate error was not instrumented in this image.
Ended the session using !TF40348:0. No receiver firmware/settings changed.
Evidence: 20261015T230217Z-F40348-canary.jsonl and the live observer trace.

Completion pacing and bounded radio recovery require a replacement canary
and retest. A queued legacy strike is not an acoustic receipt; precision
synchronization remains unmeasured.

## Completion-paced replacement

Built clean source 0214c7143c34f774577d50d9dbcce859f4b27d28 as
fx-261015-3216a18-b with the same exact-target build flags. Application SHA-256:
3a666b06e97e06526acc8386db13e5c4e08e3fdfb75b62aaad8e9eeb742ac97b;
1,235,488 bytes. Cold build passed (1,235,149 program bytes, 70,156 global
data bytes). The expanded native squad suite passed 399 checks.

No-stub ROM backup, app1-only write, and prefix/application verification passed.
Fresh boot confirmed the expected revision, FULL, 3.360 V, healthy VL53/MSA,
and retained PROTECT recovery. The one-hour session was acknowledged.
Radio/cycle qualification follows; this boot check alone is not acceptance.

Evidence: 20261015T231153Z-F40348-trigger-r2-rom-flash.jsonl,
20261015T231341Z-F40348-trigger-command.jsonl,
20261015T231133Z-F40348-live-presence.jsonl, and
fx-261015-3216a18-b-manifest.json under ops/bench/data/Oakland.

### Replacement result: command cycle and listening test passed

All nine reported the expected fx-261014-65eeb39-b revision and fresh READY
receipts by about 132 seconds of controller uptime. The first USB command at
23:15:55 UTC targeted Ness. All nine stayed ready after it, with zero immediate
send errors, failed send callbacks, or radio restarts.

Completed steps 2..9 with a mixture of presence and USB commands. The host
runner stopped twice when a concurrent presence event changed its expected
step; it did not blindly send an extra command. These were test coordination
stops, not radio failures. At 23:20:06 UTC, step 10 queued mask 1ff for requested
mask 1ff, using one event/deadline across the nine exact unicast destinations.
Ten seconds later the cycle was back at Ness, all nine were READY, and send
errors/radio restarts were still zero. See the 23:17:24, 23:19:19, and 23:20:05
cycle-check JSONL files and the 23:15:55 command trace.

Ben confirmed: "Heard the individual bells and a synchronized group strike."
This passes the first listening test; no acoustic skew measurement was made.
Ben was away for part of the test and identified the laptop in the sensor's
field of view as the likely 35..40 cm return and source of unwanted detections.
Do not treat these as confirmed hand waves. Aim into clear space and confirm
the graph says Armed before testing deliberate approaches. Presence and USB
paths ran on hardware; the USER routing is implemented but a separate physical
USER press was not identified in this trace.

Left the one-hour RAM session active and the read-only graph running. The
closed qualification graph trace ends at 23:20:26 UTC; the continuing viewer
uses 20261015T232026Z-F40348-live-presence.jsonl. No receiver was reflashed or
had its persistent configuration changed. The original first-canary TX root
cause remains unproven; the paced replacement passed this bounded retest.

## Close-range tuning revision

Ben requested a 5 cm minimum, potentially less. Clean source
b1de946cac7a4a019a5a42068f05c43aff2acacc built as fx-261015-9ba0941-b.
Application SHA-256:
3fb1399b24807e7af8da208752b6ea7a00d60bc393f591bb6ef35dc9cfa31992;
1,235,984 bytes. Exact-target flags were unchanged. Cold compile passed with
1,235,657 program bytes and 70,156 global-data bytes. Native squad tests passed
502 checks, serial framing passed 44, and the viewer Python/JavaScript parsed.

ROM app1-only write and prefix/application verification passed. Fresh boot
reported a 50 mm minimum, 700 mm trigger ceiling, and 800 mm clear boundary.
With the session off, !LF40348:30 then !LF40348:50 changed the reported cutoff
and left the event count at zero and detector unarmed. The session was restored
for 36 minutes, rounded down from its original remaining time. All nine were
READY again, the empty-space detector armed, and no TX errors/restarts appeared.
The live browser showed the 5..70 cm band and the same firmware cutoff.

Evidence under ops/bench/data/Oakland:

- fx-261015-9ba0941-b-manifest.json
- 20261015T233538Z-F40348-range-rom-flash.jsonl
- 20261015T233724Z-F40348-range-verify.jsonl
- 20261015T232844Z-F40348-live-presence.jsonl

This verifies the default and live controls, not a physical 3 cm accuracy claim.
Requested a deliberate 5..10 cm hand test; that observation remains separate
from the software/USB checks. The existing sensor filter still drops distances
below 30 mm. No receiver image, power policy, or persistent settings changed.

The first minute of an empty view remained armed at 50 mm with zero accepted
events, all nine READY, and zero radio errors/restarts. Browser error log was
empty. The qualification trace closed at 23:38:53 UTC; continuing live readings
use 20261015T233853Z-F40348-live-presence.jsonl.

## 5..40 cm band, 60 cm rearm

Ben confirmed the 5 cm minimum works and requested a 40 cm maximum and
60 cm rearm boundary. Clean source 1a53a2398d4e53507a5799069bce1b18d199f6e7
built as fx-261016-220d90a-b (UTC commit date, still October 15 locally).
Application SHA-256:
00c25f9912cd3119810727084fde0607dac72720e07abbb92e376c337945e7e1;
1,236,336 bytes. Compile passed: 1,236,001 program bytes; 70,156 global bytes.
Manifest and build.options.json agree on the unchanged F40348-only recovery
and controller flags. No receiver image was flashed by this operation.

Serial parser tests passed 61 checks, including new maximum/clear commands,
exact targeting, malformed values and bounds. Windows Application Control
blocked execution of the compiled expanded squad test executable. Instead,
C++17 compile-time assertions validated the actual presence state machine:
400/401 mm trigger edge, 600/601 mm clear edge, 800 ms clear dwell, invalid
ordering, no-op writes, and rearm invalidation on changed bounds. Do not read
this as a successful runtime execution of the full expanded squad suite.

ROM verified the full MAC, saved the 64 KiB prefix, wrote app1 only, and
verified both application and unchanged prefix. Fresh boot reported defaults
50/400/600 mm. With session off, USB U450, V650, U400, V600 commands each
changed the expected live boundary and left count zero and detector unarmed.
Restored 44 minutes, rounded down from the previous session's remaining time.
Fresh telemetry passed battery, charger-fault and sensor checks; existing
PROTECT recovery history was retained. The live graph showed 5..40 cm,
60 cm rearm and Armed in the empty view.

Evidence under ops/bench/data/Oakland:

- fx-261016-220d90a-b-manifest.json
- 20261016T025601Z-F40348-band-rom-flash.jsonl
- 20261016T025750Z-F40348-band-verify.jsonl
- 20261016T025840Z-F40348-band-ready.json
- continuing viewer: 20261016T024148Z-F40348-live-presence.jsonl

At 02:58:40 UTC all nine were freshly READY with zero TX errors or radio
restarts. The empty-space detector was armed. Some received peer revisions
had changed to fx-261016-d572656-b during this session; those receiver updates
were not performed by this Marill-only operation.

Physical feel at the new 40/60 cm edges remains a user observation; the
software and USB checks do not measure sensor distance accuracy.
