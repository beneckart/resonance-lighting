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

## Cleanup

Release any temporary visible lease if one was used, confirm no maintenance
campaign remains active, and record exact artifact revision/SHA, job identity,
rejoin evidence, interaction result, and any rollback. Leave Gible on the
canary only if the image passes OTA safety and ordinary field behavior.
