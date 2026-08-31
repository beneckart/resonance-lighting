# 0075 -- Bounded inspection direct control and audio output gain

**Date:** 2026-08-31

**Status:** Accepted in source; native and embedded builds passed. Immutable
fixture artifact and field rollout pending.

**Owner:** Ben

**Supersedes:** ADR 0074 item 3 only, where direct frames could not alter the
scheduled inspection frame. All ADR 0074 battery, static fallback, schedule,
and radio-duty decisions remain in force.

## Context

ADR 0074 removed all show authority to produce the simplest bright inspection
image. That also removed useful low-risk handheld control. Ben wants the
T-Deck to retain a deliberate way to gather the duty-cycled fleet for LED
Studio and current CoreS3 audio-reactive modes, without allowing an abandoned
publisher to keep the radios awake or own the art indefinitely.

The T-Deck Wake Fleet command already broadcasts for six minutes, spanning the
inspection image's 120-second radio-off phase, and each accepted lifecycle
command already creates a ten-minute receive hold. CoreS3 Audio and T-Deck LED
Studio both publish the same per-fixture `NB_DIRECT_FRAME` contract. Fixture
firmware cannot distinguish their artistic source without a wire change.

CoreS3 Audio also normalized quiet material well but often looked too dim in
the installed optics. Its microphone and spectral AGC should remain stable;
an output gain after color generation is simpler and makes every mode respond
consistently.

## Decision

1. In production field firmware with static inspection enabled, Wake Fleet's
   lifecycle DAY command opens one RAM-only ten-minute direct-control window.
   It does not force the lifecycle to DAY; the fixture stays in AUTO so static
   inspection white remains the fallback.
2. Wake Fleet continues its six-minute repeated campaign. Each fixture's
   control deadline begins when that fixture receives a campaign packet, so a
   sleeping radio is caught on its next listen phase and then remains
   continuously reachable.
3. Only `NB_DIRECT_FRAME` is admitted during this window. Program leases,
   bridge show frames, autonomous programs, local interaction, and presence
   propagation remain disabled under ADR 0074.
4. Direct frames do not extend the inspection-control deadline or the ordinary
   receive hold. A continuing or forgotten 10 Hz LED/audio publisher therefore
   cannot create an unbounded awake lease. Re-running Wake Fleet is the
   explicit re-arm gesture. Auto or Night Show closes the control window.
5. The existing direct-frame three-second stale fallback is retained. When a
   stream pauses or disappears, static inspection white returns automatically
   during the scheduled interval. At window expiry, the static fallback takes
   over immediately and night radio duty resumes.
6. Explicit direct colors retain the physical role limit: point sources render
   their commanded one-pixel RGBW value, while perimeters render the commanded
   RGB value on only the physical center pixel. A direct stream can never turn
   the perimeter into a 37-pixel wash.
7. Battery and recovery policy stays authoritative. DIM scales direct output;
   OFF/PROTECT cut the rail. Boot guard, startup sag checks, transport dark,
   maintenance, and OTA verification retain their existing authority.
8. CoreS3 Audio adds RAM-only post-color output-gain steps of 1X, 1.5X, 2X,
   and 3X, with 2X as the field default. Each RGBW channel saturates at 255.
   This does not change microphone gain, calibration, FFT normalization,
   publish cadence, packet layout, or downstream fixture power caps.
9. The CoreS3 touch footer exposes Gain beside Start/Pause, Input, and Mode;
   USB `V` cycles the same setting. All seven current audio modes share it.
10. Standalone T-Deck-hosted fixture OTA is explicitly deferred. The proven
    laptop plus shared-WiFi path remains the next rollout mechanism.

## Consequences

- One deliberate T-Deck action gathers the radio-duty fleet for either LED
  Studio or a separate CoreS3 audio publisher.
- Manual control is intentionally time-bounded and can temporarily be darker
  than the inspection safety fallback. The static posture self-restores.
- Mixed firmware remains undesirable: older fixtures interpret Wake Fleet as
  forced dark DAY while ADR 0075 fixtures interpret it as the bounded control
  arm. Fleet rollout should target one exact artifact and verify revision
  convergence before relying on manual control.
- T-Deck itself still does not generate audio-reactive frames. It gathers and
  arms fixtures; CoreS3 plus its microphone or Module Audio is the publisher.
- Output gain can make normalized audio visibly stronger but cannot override
  a low-battery cap or create values above the existing 8-bit maximum.

## Validation

1. Native inspection tests pin arm, expiry, explicit close, and millis wrap.
2. Native role tests pin perimeter center-only direct color and point-source
   RGBW preservation.
3. The complete fixture native suite passes.
4. CoreS3 native tests pin every gain multiplier, step wrap, channel
   saturation, and existing audio analysis behavior.
5. Embedded fixture, T-Deck, and CoreS3 builds pass. The fixture development
   build uses field/channel 11, 300 mA precharge, 120-second radio-off, and a
   12-second listen window at 36 percent flash and 20 percent static RAM.
   T-Deck passes at 49/59 percent and CoreS3 at 36/29 percent flash/static RAM.
6. Immutable fixture artifact identity remains required before rollout.

## References

- `firmware/fixture/src/core/inspection_posture.*`
- `firmware/fixture/src/core/field_role_policy.*`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/tdeck_bridge/src/ui/app_schedule.cpp`
- `firmware/cores3_bridge/audio_reactive.h`
- `firmware/cores3_bridge/cores3_bridge.ino`
- `docs/howto/BRIDGE_OS_FIELD_MANUAL.md`
