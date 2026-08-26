# 0054 -- CoreS3 wireless two-app Bridge OS

**Date:** 2026-08-25

**Status:** Accepted; Audio hardware-validated, Listener field matrix pending

**Owner:** Ben

## Context

The CoreS3 already had every hardware capability needed to work without a
laptop: its own battery, touchscreen, ESP32-S3 radio, built-in microphones, and
the validated Module Audio/RODE path. The software split those capabilities into
separate normal and audio artifacts. The normal image exposed most useful detail
only through a USB host dashboard, while switching to audio required a reflash.

T-Deck Bridge OS established the better operator model: mesh functions remain
local and useful with no WAN or laptop, and each role is an explicit app. The
CoreS3 does not need the T-Deck's full LVGL, keyboard, Claude, or command surface,
but it benefits from the same launcher/app boundary.

## Decision

1. The ordinary non-Cambium CoreS3 image is one touch-first Bridge OS with two
   switchable apps: **Listener** and **Audio**.
2. Listener is read-only. It adapts the bench dashboard to the 320x240 display as
   a stable short-ID-sorted, 24-fixture-per-page grid. Fixture class controls the
   glyph shape; raw reported VBAT controls the health color; freshness controls
   the off-air state; reported RGBW output controls the top color bar. Tapping a
   fixture opens exact radio, power, class/program, sensor/recovery, LED-output,
   and firmware detail.
3. Audio retains the validated 10 Hz `NB_DIRECT_FRAME` contract, two-second
   calibration, four visual looks, five-second live-fixture selection, and
   three-second fixture stale fallback. Start/pause and look selection are local
   touch controls. Entering Audio starts a fresh calibration. Pausing or leaving
   Audio sends a zero frame and stops publishing.
4. Built-in microphone versus Module Audio is a hardware build variant, not a
   separate operator image. The default ordinary build uses the CoreS3
   microphones. `--audio-module` includes the external module driver and retains
   built-in fallback if the module is not detected. The old `--audio` argument
   remains a compatibility alias for the ordinary image.
5. Wireless here means battery-powered ESP-NOW operation with no laptop. The
   CoreS3 remains intentionally unassociated from infrastructure WiFi and pinned
   to channel 11. It does not duplicate the T-Deck internet/Claude client.
6. USB `nb-*` telemetry and the existing serial command grammar remain available
   for the complete host dashboard, logging, and compatibility. They are not a
   dependency of either app.
7. Cambium remains a separate binary COBS/CRC artifact. It does not show the app
   launcher and must never emit text into Cambium's serial stream.
8. No packet type or layout changes. `firmware/fixture/src/core/packet.h` remains
   the only fleet wire contract.

## Consequences

- One inspected ordinary image replaces normal-vs-audio reflashing in the field.
- The CoreS3 can serve as a compact backup listener and the already-proven audio
  publisher even when no laptop or camp network is available.
- Leaving Audio has an explicit ownership handoff, reducing the chance that a
  hidden publisher fights T-Deck LED Studio, Patterns, or a future PUCA stream.
- Listener deliberately does not reproduce the host dashboard's mutating bench
  controls. Persistent configuration, OTA, and other high-risk work still use
  the established operator tools and artifact handoff rules.
- The CoreS3 and T-Deck have similar operator concepts but remain separate code
  bases sized to their hardware and roles.

## Validation required

1. USB-flash one exact CoreS3 Bridge OS Module Audio artifact and confirm the
   launcher, local battery operation, channel-11 receive, and USB `nb-*` output.
2. Exercise Listener paging and fixture detail with more than 24 observed peers;
   verify class shapes, raw-VBAT bands, stale/off-air state, and rendered-color
   bars against the host dashboard for named fixtures.
3. Confirm touch targets and text remain legible in field light and with the
   actual CoreS3 enclosure/module stack.
4. With an explicit mixed HEX/RGBW canary cohort, confirm Audio entry calibration,
   all four looks, pause/start, app-exit zero frame, and three-second fallback.
5. Run an unplugged runtime measurement with Listener active and one with Module
   Audio publishing. Record screen brightness and CoreS3 battery state.
6. Rebuild and smoke-test Cambium after the shared-source change; it must retain
   binary-only serial behavior and its original status identity.

## References

- `firmware/cores3_bridge/README.md`
- `docs/howto/CORES3_AUDIO_REACTIVE.md`
- `docs/howto/BRIDGE_OS_FIELD_MANUAL.md`
- `docs/decisions/0035-puca-performance-audio-bridge.md`
- `docs/decisions/0047-bridge-os-tdeck-app-platform.md`
- `firmware/fixture/src/core/packet.h`
