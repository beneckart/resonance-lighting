# 0031 -- Sparse GPS/RTC time anchors and scheduled dusk/dawn shows

**Date:** 2026-07-26
**Status:** Accepted direction. Four SAM-M8Q GPS modules and four Adafruit DS3231
STEMMA RTC modules with backup batteries are already bought as the initial experiment
set. Reception, acquisition energy, RTC drift/backup behavior, final anchor counts,
schedule offsets, and invalid-time fallback remain open and must be qualified before
production.
**Owners:** Ben + Codex

## Context

The July outdoor field cycle proved that panel/charger telemetry is a useful solar
measurement but a poor production show clock:

- charge taper can look like darkness while the panel is still in sun;
- shade, clouds, panel angle, and moving a fixture can start the show early;
- a no-lux peer waits for useful panel current to return and can run well after visual
  dawn;
- the P126 bench policy produced roughly 13-15 hour shows, including one clean
  14.773-hour capture, while the intended Black Rock Desert show is roughly 9-10 hours;
- the ESP32-S3 internal sleep clock measured roughly 0.6-1.0 percent fast outdoors,
  enough to accumulate approximately one to two hours of error over a week without
  correction.

Starlink is expected during build week but not during the event. The installation must
therefore establish and retain usable wall time without depending on continuing
internet, infrastructure WiFi, or a single special controller. GPS/GNSS provides
absolute UTC without enclosure penetration when an onboard antenna works through the
hat. A battery-backed external RTC provides inexpensive low-power holdover. Neither
needs to be fitted to every fixture if time can be distributed over the existing
ESP-NOW network.

The initial hardware is already on hand: four SparkFun SAM-M8Q Qwiic GPS modules and
four Adafruit DS3231 STEMMA RTC modules with backup batteries, ordered as bench
quantities rather than a fully qualified fleet allocation.

This decision concerns the daily show schedule. Tight choreography within an active
show may still use a separate monotonic/logical phase corrected by peer traffic; it
does not require GPS-level precision.

## Decision

1. **Production lightshows use an explicit site/date schedule for turn-on and turn-off.**
   The default intent is a Black Rock Desert dusk-to-dawn window with configurable
   offsets. Store and exchange schedule boundaries as UTC instants so timezone and DST
   interpretation is resolved before deployment.
2. **Only a subset of the fleet receives external time hardware.** Two complementary
   anchor capabilities are planned:
   - GPS/GNSS anchors acquire independent absolute UTC from satellites;
   - battery-backed external RTC anchors preserve wall time through deep sleep, network
     absence, and ordinary resets.
3. **An anchor is a capability, not a permanent coordinator or unique fleet identity.**
   Multiple anchors of each type provide redundancy. Any fixture must remain
   replaceable, and loss of one anchor must not stop the fleet.
4. **Anchors distribute time quality over ESP-NOW.** A time message must include at
   least source type, UTC estimate, observation age, uncertainty/quality, boot/session
   identity, and sequence information. Receivers reject stale or implausible time and
   slew or reschedule safely rather than moving time backward blindly.
5. **RTC anchors are commissioned from a trusted source and refreshed when possible.**
   GPS anchors are the post-Starlink absolute reference. Starlink/host time during
   build week may seed and verify the fleet but is not a runtime dependency.
6. **Fixtures without GPS or an external RTC learn wall time from time anchors and use
   their local clock only for bounded holdover.** They periodically reacquire from
   several peers. The measured ESP32 sleep-clock drift is not accurate enough for
   uncorrected event-week wall time.
7. **Panel current and optional lux are no longer the primary production dusk/dawn
   authority.** They remain valuable telemetry, sanity checks, and possible inputs to
   a deliberately specified degraded mode. The current field-cycle classifier remains
   a bench tool.
8. **Local power policy remains authoritative.** A scheduled show start is permission
   to request the load, not permission to bypass low-battery, charger, temperature,
   rail, or reset protections.
9. **Behavior with invalid or stale wall time must be explicit before deployment.**
   Whether a partitioned fixture runs no show, a conservative bounded show, or a
   panel-assisted fallback is still open. It must never recreate the unbounded
   13-15-hour bench artifact by accident.

## Consequences

- Nightly energy sizing can use a deterministic role-specific duration rather than an
  uncontrolled charger-current window. The July P126 week must be normalized to a
  9-10 hour schedule before drawing a production panel verdict.
- The fleet does not incur the cost, assembly, bus occupancy, backup cell, or enclosure
  burden of 150 RTCs or 150 GPS receivers.
- Sparse anchors add capability classes and provisioning records, but do not create a
  leader required for choreography or networking.
- GPS and RTC serve different failure cases: GPS supplies absolute UTC; RTC supplies
  immediate low-power holdover. A GPS receiver is not assumed to retain useful time
  through every indoor/no-fix interval, and an undisciplined RTC is not assumed to
  remain exact forever.
- Firmware needs a wall-time-quality model, time distribution messages, schedule
  storage/versioning, safe correction semantics, and telemetry exposing source, age,
  uncertainty, and next scheduled transition.
- Schedule updates must be possible during build week and through the validated OTA
  maintenance path without per-fixture manual configuration.

## Still open

- Qualification of the four purchased SAM-M8Q modules under the real hat, solar panel,
  battery, and installation geometry; decide whether they are the final GPS anchors
  after measuring acquisition time and energy.
- Qualification of the four purchased Adafruit DS3231 modules: drift across playa
  temperature, backup current/source behavior, oscillator aging, and I2C integration
  at the ADR 0028 100 kHz ceiling.
- Counts and physical distribution of GPS and RTC anchors by fleet class.
- Whether a single fixture may carry both GPS and RTC or the capabilities remain on
  separate fixtures.
- Time-quality selection and disagreement handling when anchors differ.
- Anchor wake/beacon cadence and the energy cost of acquisition and fleet
  resynchronization.
- Exact civil-twilight offsets, any pre-show/post-show scenes, and whether all roles
  share one window.
- Invalid-time, long-partition, and full-power-loss fallback behavior.

## Validation required

1. Acquire GPS time with an onboard antenna through the actual enclosure and nearby
   panel/battery placement; repeat at representative orientations.
2. Measure GPS acquisition/maintenance energy and choose an anchor duty cycle.
3. Measure the purchased DS3231 RTCs' drift and backup current across
   hot-day/cold-night temperatures and multi-day holdover.
4. Remove Starlink and the host, then prove that the fleet retains or reacquires the
   event schedule autonomously.
5. Exercise stale, conflicting, missing, and abruptly corrected time messages in native
   tests and a multi-node bench.
6. Test deep sleep, watchdog/software reset, true power-on reset, battery protection
   disconnect, network partition, and anchor replacement.
7. Run at least one compressed scheduled dusk/show/dawn cycle and one real overnight
   cycle before using the scheduler for production sizing.

## References

- `docs/tests/SOLAR_FIELD_CYCLE_P105_P126_2026-07.md`
- `docs/research/AUTONOMOUS_DISTRIBUTED_CHOREOGRAPHY_CONCEPT_2026-07-13.md`
- `docs/block-diagram/SYSTEM.md`
- ADR 0004 (ESP-NOW), ADR 0009 (minimize per-fixture operations), ADR 0023 (local
  power policy), ADR 0024 (fungible COTS fleet), ADR 0028 (I2C bus integrity)
