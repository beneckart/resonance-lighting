# 0038 -- Bridge-authoritative commissioning profile

**Date:** 2026-08-15

**Status:** Accepted and built; one-fixture hardware canary required before fleet OTA

**Owners:** Ben + Codex

## Context

The production fixture firmware combined several individually reasonable field
behaviors: solar-cycle inference, autonomous choreography, long energy-saving
sleeps, low-battery PROTECT, and sparse OTA windows. During pre-build and build
week those behaviors make basic bring-up unnecessarily ambiguous. A fixture may
go dark because of a commanded frame, autonomous fallback, daylight inference,
a lifecycle transition, an energy tier, or a sleep interval. A firmware change
may then wait up to a full sleep cadence before OTA can begin.

The immediate field symptom was a fleet with only four fixtures visible at the
bridge after deployment. Some fixtures may simply be in their normal five-minute
day sleep. A powered fixture already in PROTECT should still timer-wake and emit
setup heartbeats every roughly 15 minutes; PROTECT can prevent release but should
not make a powered board permanently radio-silent. A peer still absent after a
full roughly 16-minute listen is therefore more likely unpowered, in battery-BMS
cutoff, or out of radio range than merely waiting in PROTECT.

There was also a real PROTECT release deadlock. The policy required 60 seconds of
qualified recovery, while PROTECT's ordinary awake grace was shorter, so a peer
could go back to sleep before satisfying its own release timer.

## Decision

1. Keep one fixture image with two runtime profiles. The existing wire/NVS values
   remain stable: value 0 (`PROFILE_DEV`) is presented to operators as
   **commission**, and value 1 (`PROFILE_PROD`) is presented as **field**.
2. Commission profile is bridge-authoritative. It does not infer dusk/dawn, start
   an autonomous program, or transition to Game of Life / CA behavior. With no
   current bridge lease it is dark, and the LED rail is off.
3. Commission profile keeps the ESP-NOW control plane awake during ordinary
   operation. It does not take lifecycle day sleeps. Field profile retains the
   autonomous and energy-managed behavior needed after deployment.
4. Local safety is not bypassed. Low-voltage, critical-battery, thermal, charger,
   and actuator interlocks retain final authority in both profiles.
5. PROTECT is an output-safety state, not a communications policy. While the
   compound voltage/current/supply release condition is accumulating its
   60-second qualification, the peer stays awake. In commission profile a
   verified good external supply also keeps it awake. A truly critical,
   battery-only commission peer may sleep, but retries every 60 seconds instead
   of every 15 minutes.
6. Direct bridge frames hard-cut into the requested program when their no-fade
   flag is set. When direct frames or their lease go stale, commission profile
   hard-cuts to dark rather than crossfading into an autonomous show.
7. Profile changes are live and may be persisted over ESP-NOW. The normal CoreS3
   bridge exposes `F0` / `F1` for all reachable fixtures and
   `F<fixture-id>:0|1` for a targeted fixture. Field promotion is deliberate and
   must be verified by census; it is not inferred from command silence.
8. USB remains the last-resort rescue/data path, not the expected iteration path.
   Shared-WiFi OTA through the portable router remains the fleet OTA mechanism.

## Consequences

- Bring-up has one legible rule: no bridge command means dark, not a surprise
  autonomous behavior.
- A bridge can keep fixtures observable while firmware and mappings change
  rapidly. One persisted profile command later promotes the same binary to its
  field posture.
- Commission mode spends more energy because the radio remains awake. It is for
  powered bench/build-week work and short field service windows, not indefinite
  unattended solar operation.
- A fixture with no usable battery power still cannot be made reachable by
  software. A full-cadence census distinguishes delayed sleepers from units that
  need power-path inspection or rescue USB.
- The current boot-stage guard can still conservatively escalate after certain
  resets. Separately qualifying a durable "load was armed" marker remains a
  hardening item; commission-mode liveness makes that fault recoverable but does
  not prove it impossible.

## Validation required

1. OTA the exact named commission artifact to one battery-backed fixture.
2. Verify continuous heartbeats for at least 20 minutes with no lifecycle sleep.
3. Command a visible pattern, stop direct frames, and verify an immediate dark
   hard-cut plus LED-rail shutdown.
4. Exercise low-voltage/PROTECT entry and qualified recovery. Confirm safety loads
   remain off while the radio stays reachable and that the 60-second release can
   actually complete.
5. Reboot through the OTA pending-verify window with the production LFP installed;
   confirm mark-valid or safe A/B rollback.
6. Expand to four/five fixtures, then the 24 downlights. Verify every expected ID
   before persisting commission or field profile fleet-wide.
7. Before final deployment, persist field profile and prove autonomous schedule,
   energy tiers, sleep/wake, maintenance OTA, and command-loss behavior again.

## Implementation

- Fixture version: `fixture-2026-08-15.4`
- Commission fallback: `PROG_COMMISSION_DARK`
- CoreS3 bridge version: `cores3-bridge-2026-08-15.1`
- Native fixture coverage: 368 checks
- Named fixture artifact:
  `firmware/fixture/build/commission-rmt-railfix-20260815-r1/fixture.ino.bin`
- Fixture artifact SHA-256:
  `e4b0efaff0dcd93b3c36ab6e12dd5a1c21b45be1ad4e5269c381eb600c78de2a`
- Canary status: USB-flashed to `9E5A94`, then returned to the older A/B slot
  before acceptance. Diagnose pending-verify/rollback and complete the explicit
  two-rail-cycle RGBW regression before any fleet expansion.
