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

## Astro lower-height control

Astro `9E5B44`, reported by Ben as roughly 12 ft high, provided the lower-hung
control using the identical raw 1-5 m predicate. The only recipe change from
Gible was the compile-time target lock:

- Revision: `fx-260829-066846f-t`
- Source commit: `d9cb77528e646bfb06bf5882202181f8790bca64`
- Target lock: compile-time short MAC `9E5B44`
- Binary: 1,221,136 bytes
- Binary SHA-256:
  `b9dba40884967fd182093d1e9ab370a7d40fd89b74f5ccc3ec6588d967636877`
- Recipe SHA-256:
  `066846f139e79f02aaa1ef6709e8618563d544e28bebada4e7db3ecf2cfeba09`
- Promotion state: target-test-only, never fleetable

Exact OTA job `861C9C57` updated only Astro and verified the diagnostic through
pending verify at 31,899 ms uptime. Dawn then put the field fleet into its
scheduled dark posture. Astro accepted a nonpersistent exact-target commission
profile, but a pre-existing RAM-only fleet dark lease still suppressed its
rail. Releasing that lease left field/day fixtures dark and made Astro alone
render the intended full-red baseline. No persistent profile mutation was
made.

Ben's visual result was unambiguous despite increasing dawn light: ordinary
walking beneath Astro produced at most a split-second flicker; raising a hand
above his head made the response substantially denser and appear to stick.
Astro was estimated near 12 ft and Ben is 6 ft 3 in, leaving about 5 ft 9 in
(1.75 m) from sensor to head before a raised-hand gesture.

The rolling drain retained 913 samples spanning 304.3 seconds with zero
sequence gaps. It recorded 35 white samples across 18 sampled runs and 28 raw
rising indications. Individual sampled runs were 0.3-1.62 seconds; grouping
runs separated by at most two seconds produced two principal eight-second
interaction clusters plus shorter clusters. This diagnostic intentionally had
no debounce or hold, so the denser raised-hand response should not be described
as a continuously held production latch.

Every qualifying zone return was 1,003-1,561 mm at confidence 20-41, distributed
across zones 1, 2, 3, and 5. There were no sampled returns from 1.562 m through
5 m. The trace was red for 49.676 seconds before the first white sample and
212.752 seconds after the last, with no sampled 1-5 m false positive in either
empty interval. Fixed self-splay remained visible separately at roughly
171-315 mm with high confidence and was excluded by the diagnostic lower bound.

Dawn is a real negative confound, not a reason to discard the control. It made
the red/white output harder to judge and may have reduced optical ToF range and
confidence. The accepted returns were only confidence 20-41 against a threshold
of 20, so a modest ambient-light penalty could plausibly turn a marginal
roughly-1.75 m head return into intermittent flicker while preserving the closer
raised-hand response. Do not treat 1.561 m as a hard nighttime range limit.
Conversely, the run provides no basis for expecting darkness to turn this into
reliable 3-5 m ranging. Repeat after dark before locking height, and require
margin for shorter people, clothing, dust, sway, and unit variation.

This lower-height control resolves the central Gible ambiguity. A real nearby
target produces physically plausible 1.0-1.56 m ranges on Astro, whereas
Gible reported a roughly 2 m raised bamboo target as 3.24-5.00 m. Astro also
shows that a 12 ft mounting remains marginal for passive walk-under detection:
Ben's roughly 1.75 m head gap was outside the captured reliable band, while a
raised hand moved into it. If ordinary adult walk-under interaction is a goal,
test a roughly 10 ft mounting or equivalent re-aim next; do not infer fleet
acceptance from the raised-hand response alone.

## Verdict

**FAIL -- current Gible sensor geometry; MARGINAL -- Astro at roughly 12 ft;
PASS -- parser and end-to-end software path.** OTA safety, exact artifact
identity, field posture, sensor health,
full-range report acceptance, production debounce/latch, raw predicate, and
output arbitration all passed. The raw retest shows that empty-scene false
positives are not the immediate problem. Gible fails to detect an ordinary
person even without software discrimination, and it reports a nearby raised
bamboo target at physically implausible 3.24-5.00 m distances. Do not
fleet-promote or threshold-tune around this installation. Lower or re-aim it,
clear/inspect the TMF field of view, then repeat the named-person gate. Astro's
plausible 1.0-1.56 m returns prove that lowering helps, but its walk-under
flicker/raised-hand requirement shows that 12 ft is not yet a passive-person
acceptance height.

## Cleanup

Final exact restore job `105DD74F` replaced the raw distant-range trace image
with the normal `fx-260829-7906e6f-p` binary and verified it through the
pending-verify gate at 26,599 ms uptime. A fresh dashboard heartbeat showed field
profile, NIGHT_SHOW, FULL tier, GH CA, downlight class, sensor bits `9`, no class
mismatch, recovery state 0, ordinary mesh mode, and no active maintenance
campaign. No temporary visible lease or trace image remains on Gible.

Astro final restore job `AFF7F2E7` replaced the target-only trace image with
its exact pre-test `fx-260828-658b7d2-p` binary, SHA-256
`95de59286831bcbb9d8f610f84b09e3ac761be558f106b10b9aee8dfb01bd8cc`.
It passed fresh exact-revision rejoin and pending verify at 32,039 ms uptime with
downlight class, field profile, and recovery state 0. The nonpersistent
commission posture cleared on reboot; Astro did not retain a test profile or
trace image.

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
- `20260829-astro-9E5B44-distant-range-height-trace-ota-job.jsonl`
- `20260829-astro-9E5B44-lower-height-walk-flicker-raised-hand-stick.jsonl`
- `20260829-astro-9E5B44-lower-height-final-restore-job.jsonl`
