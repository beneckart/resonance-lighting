# 0069 -- Count-aware HEX power and local interaction layer

**Date:** 2026-08-28

**Status:** Accepted in source; native and embedded compile validation passed;
installed-fixture canaries pending

**Owner:** Ben

**Extends:** ADR 0018, ADR 0023, ADR 0029, ADR 0031, ADR 0039

## Context

Build-week field observation changed the artistic priority. The installation is
beautiful and responsive, but generally dim. The 37-pixel perimeter HEX is the
electrical exception: a dense full-output frame is not safe, while a sparse
single-pixel or split-channel gobo should use the maximum output allowed by the
battery policy. Treating brightness as one global artistic control makes those
two cases fight each other.

The autonomous show currently runs only Greenberg-Hastings CA and its default
downlight hue reads as continuously blue. The fleet already has shared UTC for
the deterministic day/night schedule, so synchronized palette changes do not
need more time hardware or a new coordinator.

The existing ToF path reduces presence to a debounced edge for Color Virus, CA
seeding, and the listener presence wave. That binary event works well for
cross-fixture propagation but loses the earlier bench interaction in which
distance continuously moved around a color wheel and close approach exposed a
crisp gobo. Direct bridge frames render the HEX as a uniform 37-pixel wash, so
implementing the effect separately in every bridge pattern would duplicate
sensor and safety logic.

MSA311 sway and BMP581 pressure are currently telemetry only. Field wind and
human-motion distributions have not yet been separated, so enabling an
untuned accelerometer threshold fleet-wide could create a tree-wide false
trigger.

## Decision

1. **Budget the physical HEX frame, not an abstract pattern brightness.** Before
   the LED driver transmits a 37-pixel RGB frame, sum its linear R, G, and B
   channel units after composing the existing battery-tier cap. Permit at most
   765 channel units, equivalent to one full RGB-white pixel or three full
   saturated single-channel pixels. A frame at or below the limit is unchanged.
   A dense frame is uniformly scaled and rounded down so 8-bit quantization
   cannot exceed the limit. One-pixel point sources and their RGBW white channel
   retain existing behavior.

2. **Use trusted UTC for an initial synchronized artistic cadence.** Autonomous
   GH CA advances its configured hue by one 43-count color-wheel step every
   1,200 seconds. Six steps form a deterministic two-hour loop. Fixtures change
   on the same absolute UTC boundary; their configured hue remains the phase
   offset. Invalid UTC preserves the configured hue. A bridge lease keeps its
   requested palette unchanged. This is a synchronized CA palette cadence, not
   yet an autonomous rotation between program IDs.

3. **Apply local interaction after visible program rendering.** A pure core
   interaction layer may transform the frame produced by any program, including
   a direct bridge pattern, before the physical current budget is applied. It
   never changes choreography state or the mesh packet contract and never turns
   an all-zero frame on.

   - A valid ToF target from 150 through 1,800 mm continuously selects color.
   - A perimeter HEX begins as all 37 pixels, then peels to 19, 7, and finally
     the center pixel as a person approaches. The current budget makes the
     dense state safe while allowing the sparse gobo to become progressively
     brighter.
   - A downlight uses the same distance color response, then switches to its
     dedicated full white point source at 380 mm or closer for the crisp gobo
     pop.
   - A target outside the interaction window leaves the program frame alone.
     Program-suppressed light, rail-off policy, battery caps, and blackouts keep
     their existing authority.

4. **Land the MSA seam but leave it disabled by default.** The same post-render
   layer accepts healthy MSA311 sway in milli-g. A compile-time canary switch can
   boost an existing color to full scale above 120 milli-g, but the normal build
   leaves that switch off until an installed fixture records wind, quiet, touch,
   swing, and climbing traces. The MSA path also cannot awaken a dark frame.

5. **Do not infer barometric or multi-fixture gestures yet.** BMP581 modulation
   and ring/all-perimeter simultaneous-presence jackpots remain separate work.
   They need slow-signal artistic mapping and a bounded aggregate presence
   contract respectively; neither is smuggled into this local frame change.

## Consequences

- Sparse HEX gobos can be authored at 255 without requiring every program to
  know the safe LED count. Dense washes degrade gracefully instead of relying
  on artist-selected dim values.
- Current draw is bounded in channel units, not calibrated milliamps. The
  765-unit choice is intentionally conservative and still requires a measured
  perimeter canary before fleet promotion.
- Continuous ToF interaction becomes available across existing modes without a
  wire-format change. It deliberately overrides the visible color and spatial
  mask while a target is in range; leaving range restores the underlying
  program immediately.
- The ToF thresholds are first-pass geometry. Installed height, outward sensor
  aim, sunlight, and crowd behavior may justify tuning, but the pure mapping and
  tests make that a bounded change.
- Autonomous fixtures gain visible time evolution now, while full program
  rotation and distributed group gestures remain explicit later steps.

## Validation evidence

- The complete native fixture suite passed, including new frame-budget,
  synchronized-palette, interaction-range, ring-count, blackout, and gated-MSA
  tests.
- A guarded PowerFeather ESP32-S3 commission/listener development build passed:
  1,201,937 bytes program, 68,708 bytes globals, 1,202,240-byte binary, SHA-256
  `9f9cd933e3c1a51929824d625e49d916be1fab298cf1aa9aea554d2d8e4a060f`.
  This `dev-local` binary is compile evidence only and must not be flashed or
  promoted.

## Required hardware canaries

1. On one named, battery-installed perimeter fixture, record battery/rail draw
   for the 37-, 19-, 7-, and 1-pixel states at representative hues and both FULL
   and DIM battery tiers. Confirm no brownout, reset, rail collapse, or visible
   discontinuity at each threshold.
2. At the installed sensor angle, confirm a person enters below 1,800 mm while
   empty-scene ground/structure remains outside the active window. Tune the five
   distance constants only from that evidence.
3. On one named downlight, confirm distance color and the <=380 mm dedicated-W
   gobo pop without interfering with Color Virus/presence-wave edges.
4. Record an installed MSA311 trace for quiet, wind, touch, swing, and climb.
   Choose threshold, hysteresis, and hold time from that trace before enabling
   `RES_MSA_LOCAL_INTERACTION` in any fleet artifact.
5. Only after those checks, create one clean immutable ADR 0040 field artifact,
   name exact canary MACs, and require fresh revision plus pending-verify survival
   before widening.

## References

- `firmware/fixture/src/core/frame_budget.*`
- `firmware/fixture/src/core/show_palette.*`
- `firmware/fixture/src/core/interaction_modulator.*`
- `firmware/fixture/src/core/choreo/prog_gh_ca.cpp`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/esp32/led_driver.cpp`
