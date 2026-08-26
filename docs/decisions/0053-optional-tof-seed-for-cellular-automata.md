# ADR 0053: Optional ToF seed for cellular automata

- Status: Accepted; source-built, hardware validation pending
- Date: 2026-08-25
- Decider: Ben Eckart
- Extends: ADR 0027, ADR 0044, ADR 0052

## Context

Bridge OS can lease the Greenberg-Hastings cellular automaton with either light
or daytime knock output. Until now, only spontaneous sparks and fresh excited
neighbors could ignite it. Ben wants presence sensing to be an optional CA
input.

The August 16 hanging-rig color-wipe test is important evidence, not a clean
sensitivity reference. Its detector was intentionally hardened after a fixed
distance trigger produced overlapping origins in the close rig. The current
gate learns the closest background independently for nine TMF channels over 90
reports, requires one channel to move at least 300 mm closer for three
consecutive confident reports, and requires four clear reports to re-arm. Ben
reported that the result was somewhat difficult to trigger and sometimes
appeared to trigger without an intentional broom pass.

Earlier bench interactions were more immediate. The presence-comparison
dashboard evaluated a changed TMF zone on each fresh frame, and LED Studio
continuously mapped an adaptive visitor-depth anomaly to color with only its
near-threshold hysteresis. The hanging-rig path therefore had materially more
debounce and a longer learned-background phase. Close-to-ground geometry and
optical interference among adjacent ranging modules are plausible additional
contributors, but the retained run has no per-zone trace that separates those
effects from code sensitivity.

## Decision

1. GH-CA program parameter byte 6 selects an optional local ToF seed. Zero is
   the default; one enables it for either light or knock output.
2. Use one shared ADR 0044 `TmfPresenceGate`. Do not add a second threshold or
   let the color-wipe and CA paths observe the same report independently.
3. A fresh, debounced rising edge is presented to the active CA as a one-shot.
   If the local node is refractory, hold that edge until it next becomes
   quiescent. A stale distance or a continuously present target is not a
   repeated seed.
4. Only sensor-verified canopy/downlight fixtures can originate a ToF seed.
   Fixtures without TMF still receive and propagate ordinary neighbor CA state.
5. During any CA lease, the separate presence color-wipe gossip remains
   suppressed. With ToF seed enabled, a visitor excites the CA itself rather
   than launching a second propagation mechanism underneath it.
6. ToF sensing grants no light, knock, or power authority. Existing lifecycle,
   battery, rail, solenoid arm/rest, maintenance, timer, and failsafe gates keep
   final authority.
7. Params byte 1 now honors zero as zero spontaneous sparks. Internal autonomous
   CA resets request defaults explicitly, preserving the existing default
   `2/256` spark rate while allowing a presence/neighbor-only leased run.
8. Do not tune the shared 300 mm / three-report detector from the broom test
   alone. First capture per-zone distance and confidence on an isolated hanging
   canary and on the close multi-emitter rig.

## Consequences

- Bridge OS presents one off-by-default **ToF seed** control for both CA outputs.
- A single detected visitor can originate the existing distributed wildfire,
  while disabling the option preserves CA operation where sunlight or geometry
  makes ToF unreliable.
- The option inherits the hardened detector's approximately 25-30 second
  learning interval after sensor startup and its deliberate debounce latency.
- Presence-only demonstrations can set spontaneous sparks to zero instead of
  relying on a nominal zero that silently reverted to the default rate.
- Perimeter, trunk, and chandelier fixtures do not originate optical events;
  they remain full CA relays and outputs.

## Validation required

1. On one named downlight with `spark /256 = 0`, prove ToF off stays quiescent
   and ToF on produces exactly one local excitation per enter/clear/re-enter.
2. Repeat for light and knock outputs. For knock, prove that CA state propagates
   even when local actuator safety refuses the physical strike.
3. Confirm the separate presence color wipe does not appear during the lease and
   returns after release to the listener fallback.
4. Compare an isolated downlight against two or more adjacent active TMF modules
   at the actual hanging height, logging all zone distances and confidence.
   Include the low-ground broom geometry only as a stress case, not as the sole
   human-presence acceptance test.
5. Use those traces to decide whether delta, consecutive-hit count, sensor
   timing, or optical crosstalk mitigation should change.
