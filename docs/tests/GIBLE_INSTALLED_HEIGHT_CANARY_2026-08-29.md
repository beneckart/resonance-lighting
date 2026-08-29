# Gible installed-height canopy canary -- 2026-08-29

## Purpose

Close ADR 0070's remaining named installed-height range gate on exact hanging
downlight Gible `9E5B34`. Sakura proved the new sensor-to-latch-to-white path
while physically in a bin; this test must prove useful person range on a real
hanging canopy fixture.

## Ownership and scope

- Owner: Ben/Codex laptop bench.
- Bridge: exact T-Deck `8EB508`, channel 11.
- OTA target: Gible `9E5B34` only.
- Artifact class: normal fleetable `p`, immutable ADR 0040 identity.
- Source behavior: ADR 0069 plus ADR 0070 through `ab71b89`, with this test
  contract as the unique clean artifact commit.
- No profile, channel, capacity, class override, NVS, or unrelated fixture
  mutation is authorized.

## Pre-update evidence

At 2026-08-29 03:31 PDT, the fresh dashboard identified Gible as a field-profile
downlight on `fx-260828-658b7d2-p`, automatic class 1, TMF sensor bit `1`, no
class mismatch, NIGHT_SHOW, FULL tier, GH CA, 3.302 V battery, and normal
low-white output. Ben used Fleet Identify and confirmed Gible is hanging in the
installed canopy rather than sitting in a bin.

## OTA acceptance

1. Use one declared OTA writer and exact target `9E5B34`.
2. Require a fresh identity-matching maintenance endpoint and fresh safe power
   evidence before upload.
3. Upload the exact retained binary once; never retry an acknowledged or
   ambiguous upload automatically.
4. Require a heartbeat newer than the job start, the exact expected revision,
   survival through the pending-verify window, field profile, downlight class,
   healthy TMF, recovery state 0, and later fresh presence.

## Installed-height interaction acceptance

With ordinary autonomous CA visible and no direct bridge lease:

1. Record a quiet empty scene first. It must not hold full white continuously.
2. Have one person walk and then stand directly beneath Gible.
3. Require a confident ordinary-height TMF return or debounced presence evidence
   and a visible full dedicated-white response.
4. After the person leaves, require clean release back to ordinary CA.
5. Watch a second empty interval for spontaneous full-white triggers while
   retaining healthy TMF read/error/recovery telemetry.

Wind-driven scanning may cause intermittent or multi-fixture discovery and is
artistically acceptable. The gate is useful named-person range plus clean
release, not continuous lock on a stationary target. If Gible cannot see a
person, do not widen; capture raw range/aim evidence before changing thresholds.

## Artifact and OTA result

The normal canary is immutable revision `fx-260829-7906e6f-p`, 1,212,736
bytes, binary SHA-256
`95df1a6b18f21c0f0643949e70474a4d2af019e85efbb86f21877af350dadb7d`.
Its clean artifact source/test commit is `fe6619f`. Exact one-target OTA job
`A04CDF37` found Gible at `192.168.1.107`, accepted fresh battery evidence,
uploaded once, observed a fresh exact-revision software-reset heartbeat, and
verified the image past the pending-verify gate at 25,122 ms uptime. Gible
reported field profile, downlight class, healthy TMF/MSA sensor bits, no class
mismatch, and recovery state 0.

The first empty and nominal person TMF maintenance censuses both recorded zero
valid depths. Their operator choreography was not controlled tightly enough to
accept or reject the interaction gate, so they are retained as diagnostic
evidence only. Likewise, the first timed four-phase motion trace and the later
condensed trace are not acceptance evidence: the internal-RAM rolling buffer
overwrote part of the timed run, and the condensed choreography forced the
person to move faster than was comfortable.

## Exact-target rolling trace

To remove the stopwatch from the physical experiment, an exact-target-only
trace variant continuously recorded Gible before a human-triggered drain:

- Revision: `fx-260829-1170f20-t`
- Target lock: compile-time short MAC `9E5B34`
- Binary: 1,222,176 bytes
- Binary SHA-256:
  `944abc2ca63ef85e486f6afc12e9672e38a561db265b3ac465903f8c1e5920db`
- Recipe SHA-256:
  `1170f2092de77b59ba93b83a97132e485bbb361045b327e06f177d31666c90ee`

The board has physical PSRAM, but this firmware reported `psram_bytes=0`.
The recorder therefore used its explicit 1,024-sample internal-RAM fallback.
Nominal capacity is about 41 seconds at 25 Hz; cooperative sensor scheduling
made both accepted captures span about 70 seconds of fixture uptime. OTA jobs
`0D400326` and `DA9A38BD` each proved the exact trace revision through the
pending-verify gate. The trace was never widened beyond Gible.

The accepted procedure was deliberately untimed from the person's perspective:
stand or walk naturally, stop when comfortable, then say `capture`. Only then
did the operator start exact-target maintenance and drain the already-recorded
rolling window. This preserves the preceding interaction instead of asking the
person to race a capture deadline.

## Triggered results

The two diagnostic 1,024-sample windows were:

| Condition | Retained span | Positive depth frames | Depth range | Person-range frames | Presence active/rising | Full dedicated W=255 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Standing beneath Gible | 70.045 s | 48 | 168-363 mm | 0 | 0 / 0 | 0 |
| Natural walk, then stop and capture | 69.366 s | 23 | 166-362 mm | 0 | 0 / 0 | 0 |

Every positive return remained a fixed near-field obstruction. The standing
capture used zones 4 and 7: 12 returns at 357-363 mm and 36 at 168-172 mm. The
walking capture used the same zones: seven returns at 358-362 mm and 16 at
166-172 mm. Neither capture contained a return near the expected roughly
2,700-3,500 mm person range, a debounced presence edge, a held presence latch,
or the required full dedicated-white response.

Ben visually observed Gible turn warm white repeatedly during the natural
walk. The trace agrees: it contains seven separate `W=25` intervals before the
capture trigger. This is not a missed person response. GH CA intentionally
renders a one-pixel point source's quiescent state as dim dedicated warm white
at 25/255; the ADR 0070 interaction clears the frame and drives dedicated white
at 255/255 while the presence latch is active. The trace therefore provides an
important alignment control: the ordinary warm-white states Ben saw are in the
record, while the required full-white state and its owning presence latch are
absent.

## Foolproof sentinel retest

Ben did not accept the earlier visual distinction or trigger timing as
foolproof, so the final retest used an exact-target diagnostic that left the
production presence detector unchanged and made its state visually binary:

- Revision: `fx-260829-e98247b-t`
- Source commit: `ce785c571cc6adc2ddd03145ded11444cd933297`
- Target lock: compile-time short MAC `9E5B34`
- Binary: 1,222,416 bytes
- Binary SHA-256:
  `f19c8e2f597e0baa3145c0c2c450becf07b83a398685da4a21140a6e118d2bdc`
- Recipe SHA-256:
  `e98247b9ddff406e8e16769073792e839b14a9751200aa55853cfa166c15bbb5`
- Promotion state: target-test-only, never fleetable
- No latch: `R=255,G=0,B=0,W=0`
- Presence latch: `R=255,G=255,B=255,W=0`

The sentinel applies only after ordinary program arbitration and only while the
frame is already visible; identify, smoke, blackout, battery, and rail authority
remain unchanged. Native tests passed, including the complete 558-check
presence suite. The 300 ms trace interval gives the internal 1,024-sample
fallback just over five minutes of retained history. Job `C2DFDDB2` updated
only Gible and verified the exact trace revision through pending verify at
23,972 ms uptime. Ben physically confirmed the red baseline.

The T-Deck's exact Gible Identify action supplied two approximately 10-second
green markers. The first occupied sequences 1031-1060 at 369.415-379.119 s
fixture uptime; the second occupied sequences 1501-1530 at 526.360-536.044 s.
The complete retained window was frozen only after the second marker, so the
person was not racing a host-side capture timer. Exact maintenance campaign
`29CC104A` selected only Gible, froze before the drain, and made no NVS or
profile mutation.

Ben reported that he had to trigger with a split held high. The raw record
between the markers contained 46 positive depth frames above 1 m. It asserted
five separate presence edges:

| Rising sequence | Fixture uptime | Trigger depth | Confidence |
| ---: | ---: | ---: | ---: |
| 1395 | 490.974 s | 4,497 mm | 30 |
| 1415 | 497.614 s | 2,508 mm | 47 |
| 1442 | 506.664 s | 2,860 mm | 38 |
| 1459 | 512.330 s | 3,829 mm | 38 |
| 1472 | 516.689 s | 2,504 mm | 59 |

The maximum retained post-parser individual zone was 4,982 mm at confidence 32
(sequence 1471). The maximum closest-target/frame-summary depth was 4,781 mm at
confidence 32 (sequence 1440). Those values prove that the old 2,500 mm parser
cap is absent and that far TMF reports reach the presence gate. They do not, by
themselves, prove accurate or useful physical ranging at 5 m; the raw retest
below rejects that stronger interpretation.
They did not initiate presence because `PRESENCE_MAX_MM` intentionally limits a
previously empty zone to 4,500 mm, separate from the 5,000 mm sensor/parser
limit. A 4,763 mm sample occurred while an earlier latch was still held; it was
not a rising-edge trigger. Most new assertions began at 2,504-2,860 mm, matching
Ben's visual estimate, while the 3,829 and 4,497 mm edges independently prove
operation beyond the old cap.

The five active runs covered sequences 1395-1404, 1415-1430, 1442-1451,
1459-1465, and 1472-1477: 49 samples total. All 49 active samples rendered exact
full RGB white, no inactive sample rendered white, and no active sample remained
red. After the first green marker ended, the recorder retained 111.855 seconds
of red baseline before the first presence edge. After the last white sample it
returned to red for 8.030 seconds before the second green marker.

This is conclusive end-to-end evidence for the installed Gible sensor, full-
range parser, unchanged production debounce/latch, and visible output
arbitration. It also confirms that the remaining failure is not host capture
timing. It is not a useful-person-range pass: ordinary standing/walking did not
trigger, and the successful interaction required the high-held gesture.

## Raw distant-range height/aim retest

Ben pointed out that Gible is less than 5 m high, yet ordinary people did not
trigger it and the bamboo split had to be held much higher, roughly 2 m from the
sensor. To distinguish a software threshold failure from installed physical
range/aim, a final exact-target diagnostic removed every production
discriminator except validity:

- Revision: `fx-260829-4192016-t`
- Source commit: `d9cb77528e646bfb06bf5882202181f8790bca64`
- Target lock: compile-time short MAC `9E5B34`
- Binary: 1,221,136 bytes
- Binary SHA-256:
  `27b9d412d4fe51c44fc37d4adc75a7c2b0481ab2750c13b08f020482773b7fc2`
- Recipe SHA-256:
  `419201645ff0321b58030837faac989ebb8648e108e5429131e0ad696907f113`
- Promotion state: target-test-only, never fleetable
- Active predicate: any confidence >=20 zone with `1,000 <= mm < 5,000`
- Background learning, hit debounce, and hold hysteresis: disabled
- Mesh presence-wave origin/relay: disabled
- No active range: `R=255,G=0,B=0,W=0`
- Active range: `R=255,G=255,B=255,W=0`

The 1 m lower bound deliberately excludes Gible's already-known 166-363 mm
bamboo/sensor self-splay. All native tests passed, including 565 presence
checks and the build-wrapper isolation contract. The fresh embedded build used
1,220,833 bytes program and 68,788 bytes globals. Exact job `AB8F3FA6` updated
only Gible and verified the image through pending verify at 32,289 ms uptime.

Exact campaign `29D15A90` froze the rolling trace after Ben's test. The drain
retained 860 samples spanning 300.500 seconds with zero overwrite and zero
sequence gap. The scene was red for 123.895 seconds before and 142.250 seconds
after the sole 34.355-second interaction window. Neither empty interval
contained a sampled 1-5 m return or white output. The interaction window
contained 45 white samples across 23 raw rising edges. Every sampled 1-5 m
return was white and every sample without one was not white.

The range distribution contradicts the physical target distance:

| Zone-return band | Returns | Confidence range |
| --- | ---: | ---: |
| Known self-splay, 166-358 mm | 3 | 255 |
| 1-<3 m | 0 | -- |
| 3-<4 m | 8 | 22-79 |
| 4-<5 m | 65 | 20-75 |
| Exactly 5,000 mm, raw but excluded | 1 | 49 |

The distant reports covered zones 0-4. Maximum closest-target/frame summary was
4,988 mm; maximum individual zone was exactly 5,000 mm. Yet Ben estimates that
the bamboo split was roughly 2 m from the sensor when it finally triggered.
These far returns are not independent empty-scene noise: they occur only while
the nearby raised target changes the scene. Plausible causes are sensor aim,
partial aperture/splay occlusion, edge/background ranging, or multipath. The
trace cannot distinguish those optical causes, but it does show that the 4-5 m
numbers are not calibrated evidence of a target physically 4-5 m away.

Most importantly, an ordinary person did not trigger even this raw predicate.
The current installation therefore fails before background learning, delta,
debounce, or hold policy matter. Lower or re-aim Gible and inspect/clear the TMF
aperture and field of view before another software threshold change.

## Verdict

**FAIL -- current Gible sensor geometry; PASS -- parser and end-to-end software
path.** OTA safety, exact artifact identity, field posture, sensor health,
full-range report acceptance, production debounce/latch, raw predicate, and
output arbitration all passed. The raw retest shows that empty-scene false
positives are not the immediate problem. Gible fails to detect an ordinary
person even without software discrimination, and it reports a nearby raised
bamboo target at physically implausible 3.24-5.00 m distances. Do not
fleet-promote or threshold-tune around this installation. Lower or re-aim it,
clear/inspect the TMF field of view, then repeat the named-person gate.

## Cleanup

Final exact restore job `105DD74F` replaced the raw distant-range trace image
with the normal `fx-260829-7906e6f-p` binary and verified it through the
pending-verify gate at 26,599 ms uptime. A fresh dashboard heartbeat showed field
profile, NIGHT_SHOW, FULL tier, GH CA, downlight class, sensor bits `9`, no class
mismatch, recovery state 0, ordinary mesh mode, and no active maintenance
campaign. No temporary visible lease or trace image remains on Gible.

Retained evidence is under `ops/bench/data/ca/`:

- `20260829-gible-9E5B34-adr0070-canary-job.jsonl`
- `20260829-gible-9E5B34-empty-tmf-census.jsonl`
- `20260829-gible-9E5B34-person-tmf-census.jsonl`
- `20260829-gible-9E5B34-triggered-trace-ota-job.jsonl`
- `20260829-gible-9E5B34-trigger-under-motion.jsonl`
- `20260829-gible-9E5B34-trigger-walk-motion.jsonl`
- `20260829-gible-9E5B34-triggered-trace-final-restore-job.jsonl`
- `20260829-gible-9E5B34-presence-sentinel-trace-ota-job.jsonl`
- `20260829-121148-C2DFDDB2-fleet-ota-results.jsonl`
- `20260829-gible-9E5B34-presence-sentinel-bracketed-split-high.jsonl`
- `20260829-gible-9E5B34-presence-sentinel-final-restore-job.jsonl`
- `20260829-122316-F02313D7-fleet-ota-results.jsonl`
- `20260829-gible-9E5B34-distant-range-height-trace-ota-job.jsonl`
- `20260829-124637-AB8F3FA6-fleet-ota-results.jsonl`
- `20260829-gible-9E5B34-distant-range-bamboo-split-height.jsonl`
- `20260829-gible-9E5B34-distant-range-final-restore-job.jsonl`
- `20260829-125308-105DD74F-fleet-ota-results.jsonl`
