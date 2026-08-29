# 0070 -- Canopy presence uses the full TMF8820 range

**Date:** 2026-08-28

**Status:** Accepted in source; native and embedded compile validation passed;
installed-height canary pending

**Owner:** Ben

**Supersedes:** ADR 0069 downlight absolute-distance interaction only

**Extends:** ADR 0039, ADR 0041, ADR 0069

## Context

During final build-week installation, canopy fixtures were being hung roughly
15 ft above the ground. An exact-target field census gathered 61 live
TMF-bearing downlights, discovered 46 shared-WiFi endpoints, and recorded six
raw samples from each. Forty-two produced stable close returns from 145 through
582 mm, consistent with bin walls/lids. Panther `9E5A84`, Gible `9E5B34`,
Sakura `F2BE0C`, and Leia `F40384` produced six consecutive zero-depth reports
with a healthy TMF device and no read errors, consistent with a clear/high
installed view. Fifteen targets did not present an endpoint during the bounded
window and remain unsampled, not failed. All discovered fixtures returned to
mesh and later sent fresh heartbeats.

The field evidence exposed two incorrect software assumptions:

1. The sensor parser discarded every otherwise confident return above 2,500 mm,
   even though the TMF8820 supports targets through 5,000 mm. At 15 ft from
   sensor to ground, a standing person's head is normally about 2,700-3,200 mm
   from the sensor and was therefore invisible to firmware.
2. ADR 0069 used one absolute distance mapping for both fixture roles. The
   perimeter sensor is deliberately aimed at chest height and benefits from
   close continuous distance/color/ring behavior. A canopy sensor instead needs
   to distinguish a nearer person from its per-zone ground/structure background.

The existing per-zone TMF presence gate already provided confidence checking,
warmup, delta comparison, debounce, and a held latch. However, its 2,500 mm
validation cap and 2,200 mm empty-scene threshold repeated the range error. Its
baseline also retained the closest historical return forever, so a powered
fixture warmed in a close shipping bin could not re-baseline after being moved
into the tree without a reboot.

## Decision

1. Accept confident TMF8820 scene returns from 80 through 5,000 mm. The lower
   bound still rejects the known enclosure/window return. Confidence below 20
   remains invalid.
2. Canopy presence remains per-zone and background-relative. A return must be at
   least 300 mm closer than that zone's learned background for three consecutive
   reports. With no background return, a persistent confident target through
   4,500 mm is presence; the margin below the sensor limit avoids treating
   marginal 5 m noise as a person.
3. Let a non-presence background move farther with a bounded one-eighth EMA.
   This re-baselines a fixture moved from a 200 mm bin wall to a multi-metre
   installed scene in seconds. Twelve consecutive empty reports clear a stale
   per-zone baseline, covering an installed ground that is beyond range or too
   weak to return.
4. Downlight local interaction consumes the debounced presence latch, not an
   absolute distance/color mapping. While presence is held, a visible one-pixel
   RGBW downlight becomes full dedicated white for a crisp gobo response. It
   still cannot awaken a dark/suppressed program or bypass battery and rail
   policy.
5. Perimeter interaction is unchanged: 150-1,800 mm selects hue and peels the
   HEX 37 -> 19 -> 7 -> 1 pixels as a person approaches.

## Consequences

- A head roughly 3 m below a 15 ft fixture is no longer discarded before the
  detector sees it.
- A valid ground return near 4.5 m becomes background rather than permanent
  presence. If ground produces no valid return, a later head return can still
  create presence.
- Nominal 5 m capability does not guarantee a useful return in full sun, on
  dark clothing, or at poor aim. One updated installed fixture plus a person
  walking underneath is the release gate; mounting height/aim remains a concern
  until that canary passes.
- Continuous distance/color behavior is now explicitly a perimeter affordance.
  Canopy output is a stable presence gesture rather than a distance wheel.

## Validation

- Complete native fixture suite passed, including 558 presence-wave checks and
  new close-bin -> 4.6 m background -> 3.0 m person, empty-high-scene -> 3.1 m
  person, and downlight-latch tests.
- PowerFeather ESP32-S3 commission/listener development compile passed at
  1,201,065 bytes program and 68,716 bytes globals. The 1,201,360-byte
  `dev-local` binary SHA-256 is
  `c19b95df3e65aaade0a43625716defe0bf16911c6d1c307a1e9aaf32762775a2`.
  It is compile evidence only and is not a fleet artifact.
- Census evidence is retained in
  `ops/bench/data/Black Rock City/2026-08-29-tmf-installed-height-census-0218UTC.jsonl`.

## Required canary

Build one clean immutable ADR 0040 artifact, update one named healthy installed
zero-return candidate, prove fresh exact revision and pending-verify survival,
then have a person walk beneath it while a visible show lease is active. Pass
requires a debounced full-white response from ordinary standing/walking height,
clean release after departure, no spontaneous empty-scene triggering, and
healthy TMF read/error/recovery telemetry.

## References

- ams OSRAM TMF8820 product specification (10-5,000 mm):
  https://ams-osram.com/products/sensor-solutions/direct-time-of-flight-sensors-dtof/ams-tmf8820-3x3-multi-zone-time-of-flight-sensor
- `firmware/fixture/src/esp32/sensors/sensors.cpp`
- `firmware/fixture/src/core/presence_wave.*`
- `firmware/fixture/src/core/interaction_modulator.*`
- `docs/tests/BUILD_WEEK_TMF_HEIGHT_CENSUS_2026-08-29.md`
