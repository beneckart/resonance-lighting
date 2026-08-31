# 0074 -- Static inspection light and night radio duty

**Date:** 2026-08-31

**Status:** Accepted for the emergency art-inspection image; native suite,
ESP32-S3 development build, and immutable artifact pass. Rollout pending.

**Owner:** Ben

**Supersedes:** ADR 0072 autonomous night rotation and ADR 0073 field-render
behavior for this inspection posture. Extends ADR 0049's schedule and radio
cadence without weakening ADR 0023 power authority.

## Context

Art Support judged the installed piece too dim and unsafe. Passing inspection
now has higher priority than preserving autonomous choreography. The quickest
legible posture is static maximum RGB-white from every point source, with the
already-adopted one-pixel perimeter gobo. The field image must retain local
battery protection because many fixtures are in or near PROTECT.

The existing night show kept an unassociated ESP-NOW receiver continuously
awake. Measured full `R=G=B=255,W=0` system draw is about 417.6 mA, including
the active controller/radio. Separate measurements put that awake branch at
about 116-168 mA and identify it as radio-RX dominated. Radio cost is therefore
large enough to address while the art no longer needs continuous peer state.

## Decision

1. Production field output during the scheduled light interval is static
   linear `R=255,G=255,B=255,W=0` for downlight/canopy, uplight, chandelier,
   and unknown classes. The physical one-pixel LED profile remains the safe
   default for unknown hardware.
2. Perimeters render the same RGB value on only the physical HEX center pixel.
   The other 36 pixels and W remain zero.
3. This static field frame supersedes autonomous programs, direct/show frames,
   local presence effects, Identify, and Smoke while the scheduled inspection
   light is active. Program/direct packets may still release a deliberate
   transport-dark latch but cannot change the inspection color.
4. Autonomous CA/Color Virus/Epidemic scheduling, presence-wave origination,
   choreography keepalives, ordinary wake chimes, and the hourly daytime
   ritual are disabled in this posture. Their implementations remain in source
   for deliberate restoration after inspection.
5. Battery and recovery authority is unchanged. FULL renders 255, DIM scales
   through the existing brightness cap, and OFF/PROTECT cut the LED rail.
   Startup ramp/sag checks, boot guard, maintenance loads-off, OTA pending
   verify, and transport darkness remain higher authority.
6. The evening light boundary moves from civil dusk to exactly one hour before
   civil dusk. The calculation looks one hour ahead only on the evening side;
   civil dawn remains the off boundary. Loss of trustworthy UTC retains the
   panel-current fallback.
7. During a trustworthy scheduled inspection interval, the CPU and LED rail
   remain continuously awake while ESP-NOW/WiFi uses the same deployed field
   shape as daytime: 12 seconds listening followed by 120 seconds radio-off.
   This is 9.1 percent radio duty and a worst-case command wait of 120 seconds.
   Every listen-window start sends a full heartbeat.
8. Radio duty does not run during OTA pending verify, before the power wake
   sample is complete, under transport dark, or without trustworthy scheduled
   time. Loss of eligibility immediately restores continuous radio. Failed
   ESP-NOW restart falls back to the existing one-second recovery loop.

## Energy consequence

The exact radio-off-but-CPU-awake floor has not been recovered from hardware,
so no false precision is claimed. The known bounds still justify the change:

- 12/(120+12) = 9.1 percent radio-on duty;
- if the measured 116-168 mA awake branch were fully avoidable, the upper-bound
  saving would be about 105-153 mA average, or 25-37 percent of the measured
  417.6 mA full-RGB system load;
- even if only half of the low measured branch is actually radio-avoidable,
  the saving is about 53 mA average, roughly 13 percent of total full-RGB draw;
- starting one hour earlier adds about 0.418 Ah at the measured continuous
  full-RGB load, so radio duty materially offsets the requested longer night.

Actual deployed draw should be measured later, but it is not a gate for this
emergency visibility image. Deep sleep is explicitly not used at night because
it would cut the switchable 3V3 LED rail and visibly blink the safety light.

## Consequences

- The piece becomes deliberately noninteractive and visually uniform until
  inspection passes and modes are reintroduced one at a time.
- Normal fleet control may take up to two minutes to land, and a handheld may
  miss a fixture if its command burst is shorter than one complete off phase.
  Existing sustained OTA maintenance campaigns already span this interval.
- Inspection output no longer helps identify fixtures by blinking. Operators
  can force day/commission posture for supervised service if identification is
  required.
- Full RGB is an aggressive nightly load. DIM/OFF/PROTECT behavior is expected
  and must not be described as a firmware failure or bypassed for inspection.

## Validation

1. Native role tests pin 255/255/255/0 for canopy, uplight, chandelier, and
   unknown, plus one physical perimeter center pixel.
2. Native schedule tests pin pre-dusk start without early dawn shutoff.
3. Native radio tests pin the first listen window, 12-second pause, 120-second
   resume, and fail-open continuous-radio restoration.
4. The complete fixture native suite passes.
5. ESP32-S3 development build passes at 36 percent flash and 20 percent static
   RAM using field/channel-11, 300 mA precharge, 120-second off, and 12-second
   listen compile defaults.
6. Clean source commit `4efad9320fb3f0134e730ca5ad7b586627d492b0`
   produced immutable field artifact `fx-260831-32dcb76-b`, 1,216,864 bytes,
   SHA-256
   `e82376535270057297f28198c94f9d51d65b2886fde16d63d1de386f10dac981`.
   Its manifest, recipe, build options, SHA file, and exact application binary
   are retained for the camp-laptop handoff. OTA completion still requires a
   fresh exact-revision heartbeat beyond pending verify.

## References

- `firmware/fixture/src/core/inspection_posture.*`
- `firmware/fixture/src/core/field_role_policy.*`
- `firmware/fixture/src/core/show_schedule.*`
- `firmware/fixture/src/esp32/behavior_glue.cpp`
- `firmware/fixture/src/esp32/maintenance.cpp`
- `firmware/fixture/fixture.ino`
