# 0058 -- CoreS3 Audio releases the prior program lease on start

**Date:** 2026-08-25

**Status:** Accepted; takeover and spoken response hardware-validated, fallback pending

**Owner:** Ben

## Context

CoreS3 Audio publishes `NB_DIRECT_FRAME` traffic with the existing direct
micro-lease flag. Fixture arbitration correctly gives an explicit bounded
program lease precedence over those frames. During the first standalone test,
the CoreS3 showed Audio publishing while all awake fixtures remained in CA: an
earlier CA Studio lease was still active, so the direct stream could not become
the rendered program.

The same test exposed a second independent selection bug. Audio's full-heartbeat
filter recognized only an obsolete `fixture-*` revision prefix. Production
firmware has used ADR 0040 immutable `fx-*` revisions since August 16, so the
screen could count a fixture as live while Audio omitted it from every direct
frame after learning its firmware identity.

Requiring a laptop or a separate T-Deck Release action makes the standalone
Audio app depend on hidden prior bridge state. The operator has already made an
unambiguous ownership choice by starting Audio.

## Decision

1. Starting or resuming CoreS3 Audio sends one fleet-wide `NB_PROGRAM_SET`
   release (`program_id=0`, `lease_s=0`) before its first direct frame.
2. The release is RAM-only. It does not write NVS, change lifecycle, change the
   commission/autonomous default, reboot a fixture, or require fixture OTA.
3. The existing fixture arbitration contract is unchanged. The next flagged
   direct frame acquires the normal direct micro-lease.
4. Pausing or leaving Audio still sends a zero direct frame and stops the
   publisher. Direct-frame staleness then returns each fixture to its configured
   autonomous program in about three seconds.
5. Audio does not continuously fight a competing explicit program publisher.
   If another bridge intentionally sends a new program lease after takeover,
   that explicit lease wins and operators must stop the other publisher.
6. The CoreS3 lowercase `b` serial command is documented and labelled as a
   general program-lease release, matching its existing wire semantics.
7. Once a full heartbeat supplies firmware identity, Audio recognizes current
   `fx-*`, older `fixture-*`, and fixture-cache `dev-local` revisions as fixture
   targets. It excludes identified legacy net-bench and bridge firmware that
   cannot consume type-25 direct frames. Before the infrequent full identity
   heartbeat arrives, a fresh peer remains optimistically eligible as before.

## Consequences

- CoreS3 Audio is independent of a laptop and of hidden prior CA, Contagion, or
  Dark lease state.
- Current immutable production artifacts remain Audio targets after their full
  firmware-identity heartbeat arrives.
- Entering Audio is a fleet-wide artistic-control action and should still follow
  the one-publisher-at-a-time field rule.
- No fixture firmware or wire-format change is required.

## Validation required

1. Apply a bounded CA lease from T-Deck to an explicitly observed awake cohort.
2. Confirm the CoreS3 Audio census includes current ADR 0040 `fx-*` fixture
   revisions while excluding identified legacy net-bench/bridge peers.
3. Start CoreS3 Audio with Ambient Mic and verify the next fresh heartbeats
   report Direct rather than CA.
4. Speak after the two-second calibration and verify visible response across the
   awake cohort.
5. Pause Audio and verify the roughly three-second autonomous fallback.
6. Confirm the release changes no saved profile, lifecycle, or autonomous
   default.

## References

- `firmware/cores3_bridge/cores3_bridge.ino`
- `firmware/fixture/src/core/choreo/runtime.cpp`
- `firmware/fixture/src/core/packet.h`
- `docs/decisions/0054-cores3-wireless-two-app-bridge-os.md`
