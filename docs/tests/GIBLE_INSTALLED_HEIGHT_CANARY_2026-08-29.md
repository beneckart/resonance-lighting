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

The two accepted 1,024-sample windows were:

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

## Verdict

**FAIL -- installed-height interaction gate.** OTA safety, exact artifact
identity, field posture, sensor health, ordinary CA behavior, and rolling-trace
alignment all passed. Gible did not see a standing or walking person at useful
range and never asserted the ADR 0070 presence response. Do not fleet-promote
this canopy interaction on the basis of Gible. The next action is mechanical
range/aim diagnosis at the TMF window and installed sensor angle; do not widen
the 5 m parser limit or weaken confidence/debounce thresholds without new raw
range evidence.

## Cleanup

Final exact restore job `32F8BF78` replaced the temporary trace image with the
normal `fx-260829-7906e6f-p` binary and verified it through the pending-verify
gate at 28,353 ms uptime. A later fresh dashboard heartbeat showed field
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
