# 0040 -- Firmware artifact identity and shared-bench handoff

**Date:** 2026-08-15

**Status:** Accepted; build/OTA tooling implementation pending

**Owners:** Ben + Elliot + Justin

## Context

On 2026-08-15 Ben's strict commissioning/RMT-fix image and Elliot's later
listener/presence image both reported `fixture-2026-08-15.4`. They had different
source, flags, toolchains, and binary hashes. Telemetry therefore could not say
which `.4` was running. A color change initially interpreted as A/B rollback was
later proven to be Elliot's separate bridge OTA'ing Ben's attached fixture.

Cambium also selected the newest `fixture.ino.bin` by filesystem modification
time and considered any roster entry still marked `online` to be a post-flash
rejoin. A pre-flash heartbeat can keep that bit true for 30 seconds, so success
may be reported before a fresh packet or the fixture's 20-second A/B self-test.

The team intentionally does not require a fleet-wide artistic-control lease:
simultaneous live frames may flicker and prompt in-person coordination. Firmware
mutation needs a stronger boundary because its effects survive the controller.

## Decision

1. Retire manually incremented `fixture-YYYY-MM-DD.N` strings for new shared
   artifacts. Generate `fx-YYMMDD-<recipe7>-<variant>` as specified in
   `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`.
2. Build shared/fleet artifacts only from a clean committed source tree. Recipe
   identity includes source, flags, target board, toolchain, and library versions.
3. Every immutable artifact directory carries a manifest and exact binary
   SHA-256. One reported revision may map to only one accepted binary SHA-256.
4. Build once and distribute that artifact. OTA and USB tools do not rebuild it,
   overwrite it, or silently substitute a file named `latest`.
5. OTA selection is explicit by manifest/revision and target short MAC. Unknown
   targets fail before maintenance broadcast.
6. OTA success requires a fresh post-job heartbeat, expected revision, and
   survival through pending verification. Cached online state and HTTP upload
   acknowledgement are insufficient.
7. Live artistic control may remain leaseless. OTA, USB flash, profile/channel
   persistence, reboot, and NVS mutation are single-operator sessions across all
   active bridges/laptops, with an owner/source/artifact/targets callout.
8. A USB-only automatic boot salute is liveness, not artifact verification. Only
   a host-triggered salute after the full commissioning gate may mean ready to
   unplug and install.
9. The normal fleetable build remains one fixture image. Listener, strict
   diagnostic, and field behavior are runtime settings; the artifact suffix
   describes fleetable/bench/unsafe build class, not artistic posture.

## Consequences

- Human-readable telemetry can distinguish different source/configuration
  recipes, while SHA-256 remains the exact-byte authority.
- A branch name is no longer treated as an artifact identity; branches move.
- Mac and Windows benches can collaborate without independently rebuilding a
  supposedly identical fleet image.
- The OTA UI must show explicit target and artifact identity, and Cambium's OTA
  runner needs freshness/version tests before its status can be trusted.
- Existing historical firmware strings remain in logs. They are not retroactively
  renamed, but `.4` alone must never be used to infer which 2026-08-15 image ran.
