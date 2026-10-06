# 0079 -- Steve's Magic Wand gesture conductor

**Date:** 2026-09-03

**Status:** Accepted in source; native gesture/visual tests and a dedicated
PowerFeather build pass. Hardware gesture tuning and a named-canopy canary
remain open.

**Owners:** Steve + Ben

**Extends:** ADR 0037, ADR 0050, ADR 0065, ADR 0076, ADR 0077, and ADR 0078.

## Context

Steve's one-off Magic Wand is already a channel-11 fleet peer with identity
`68:EE:8F:F4:03:44` / `F40344`, one PowerFeather V2, a 15 Ah LFP, MSA311,
BMP581, and twenty daisy-chained NeoHex boards. Steve wants the wand to offer
the bounded Wake/gather and canopy-wave interaction now implemented on Ben's
Atom Conductor, but the wand has no button.

The external Pololu toggle switches only the 5.1 V LED supply. That supply
branches from the BatterySpace PCM in parallel with the PowerFeather battery
input, so LED current does not cross the PowerFeather MAX17260 sense resistor.
The firmware cannot reliably distinguish switch state from gauge current.
Inferring it from the small, cell-dependent battery-voltage sag would be unsafe.

The existing `net-bench-2026-08-19.1` wand image is known working. Its later
`.2` source and Steve's pattern-control notes were merged into `main` through
`codex/integrate-magic-wand`; that branch is now an ancestor, not a separate
development tip.

## Decision

### Dedicated image and identity

1. Add `firmware/magic_wand_conductor/` as a dedicated target. Preserve the
   working net-bench wand mode and commissioning sketch as recovery/history.
2. The binary checks the complete six-byte WiFi MAC at boot. On any identity
   other than `68:EE:8F:F4:03:44`, it refuses PowerFeather configuration,
   external LED frames, and all fleet transmission.
3. Reuse `firmware/fixture/src/core/packet.h` directly. Reuse the Atom session,
   fleet graph, time gate, canonical UTC selector, BRC solar schedule, MSA311
   filter, and HEX geometry rather than creating another wire contract or
   behavioral copy.
4. Keep the image ESP-NOW-only. It has no WiFi association, OTA endpoint,
   persistence, or serial command parser. ADR 0050 remains the only allowed
   path for a future deliberate wand OTA.
5. Keep this fleet-control image under Steve's or Ben's supervision while the
   command plane is unauthenticated. Gesture recognition limits accidents but
   is not authorization for an unattended participant-held controller.

### No-button gesture

1. One accepted gesture is: still for 1.6 seconds, rise at least 0.45 m within
   six seconds, then remain still at least 0.35 m above baseline for 0.8
   seconds.
2. The MSA311 gravity-removed envelope must cross 0.045 g during the lift; the
   BMP581 must independently retain the height change. Neither a knock, motion
   without height, nor pressure drift without motion can fire.
3. A shock at or above 0.90 g cancels the candidate. Missing/stale sensor data
   fails closed. Another gesture requires return within 0.20 m of baseline, or
   five uninterrupted still seconds to establish a new baseline.
4. These numerical thresholds are provisional until Steve's real handling is
   captured. They are testable source defaults, not a hardware-qualified UX.

### Session and effects

1. The first accepted gesture starts the six-minute Wake/neighbor gather from
   ADR 0078. Gestures during gather report incomplete progress. Once ready, an
   accepted gesture issues the time-selected wave.
2. Five-minute explicit Wake renewal, 15-minute gesture inactivity, one-hour
   absolute maximum, and ten-second repeated Auto cleanup match ADR 0078.
   Effects never send Wake themselves.
3. The wand cannot deep-sleep after cleanup: its LED toggle is not a controller
   input or wake source. Cleanup returns to an always-listening idle posture so
   another deliberate gesture can start a session.
4. Before true civil dusk, including the inspection light's one-hour overlap,
   the gesture sends the exact-target canopy chime wave. From civil dusk through
   civil dawn, it sends the next direct-frame color wave. Missing trusted UTC
   refuses both.
5. Night colors cycle red -> green -> blue -> dim RGB. The fourth selection is
   equal RGB, not the downlight's W die. It sends `21,21,21`, approximately the
   same requested channel sum as the value-64 single-channel colors.
6. Power tier is not a target filter. Fresh class-confirmed canopies are
   eligible; local fixture power and actuator protections remain authoritative.

### Local feedback

1. During gather, each of the twenty physical Hex boards fills center-out using
   the canonical 37-pixel spiral order. A bright head circles the active ring so
   progress stays visibly animated during the long gather.
2. At night, all 740 pixels show the next R/G/B/RGB selection at a total RGB
   sum of six, below Steve's validated all-pixel value-8 commissioning fill.
3. Center/ring cues expose gesture arm/lift/high-hold state. Amber means daytime
   chime readiness, and sparse purple means trusted time is unavailable.
4. Local frames are repeatedly refreshed so switching the independent LED rail
   on later restores the current display. The controller never claims to know
   whether that rail is actually powered.

## Consequences

- The fixture fleet does not need a flash or a new packet type.
- The deliberate two-sensor gesture greatly reduces false triggers but cannot
  be called field-safe until actual carries, handoffs, dances, and lifts are
  traced.
- Turning the LED toggle off does not disable the controller. If that coupling
  is desired, add a reviewed divider/buffer from the 5.1 V rail or Pololu enable
  node to a spare GPIO; never connect 5.1 V directly to the ESP32.
- The wand remains a protected, one-off OTA target. A build pass is not
  authorization to replace its known-good installed image.
- Unsupervised campmate/participant use remains blocked on authentication,
  labeling, lost-device revocation, and abuse/coexistence testing.
- The MAX17260 reports the PowerFeather branch, not total wand current, so wand
  energy accounting still needs an external meter or a sense element in the
  common battery path.

## Validation required

1. Confirm the exact MAC and use USB first. Record filtered pressure, MSA311
   envelope, recognizer states, false positives, and misses through realistic
   handling; tune thresholds and re-test natively.
2. Confirm every local Hex spiral and all four 740-pixel fills, including
   thermal and electrical behavior with the 5.1 V switch cycled.
3. Against one named canopy, prove gather, trusted/missing UTC, pre-dusk chime,
   post-dusk R/G/B/RGB, inactivity cleanup, absolute cleanup, and wand power
   loss recovery.
4. Expand to a small physical cluster and compare calculated graph layers with
   observed adjacency before attempting a fleet wave.

## References

- `firmware/magic_wand_conductor/README.md`
- `firmware/magic_wand_conductor/src/core/wand_gesture.cpp`
- `docs/projects/NeoHex-Magic-Wand/README.md`
- ADR 0050 and ADR 0078
