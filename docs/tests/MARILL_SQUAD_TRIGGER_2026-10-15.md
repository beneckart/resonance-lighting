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
