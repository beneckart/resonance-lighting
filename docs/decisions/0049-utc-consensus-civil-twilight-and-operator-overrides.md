# 0049 -- UTC consensus, civil twilight, and operator overrides

**Date:** 2026-08-24

**Status:** Accepted in source; native-tested, hardware validation pending

**Owner:** Ben

**Extends:** ADR 0031, ADR 0039, ADR 0047, ADR 0048

## Context

The field lifecycle still inferred dusk and dawn from panel current. That is a
useful failsafe but not a deterministic show clock: charging conditions,
shadows, USB, and battery state can all look like time of day. GPS and RTC
anchor boards are on hand, and the T-Deck Plus GPS has already produced NMEA at
38400 baud.

Build week also needs two properties that can pull in opposite directions:

- daylight should be electrically dark by default to avoid needless draw;
- an operator should still be able to test lights during day or suppress the
  show at night without reflashing or defeating local battery safety.

Ordinary peer traffic was also updating the lifecycle's ten-minute RX hold.
In a live fleet, heartbeats and choreography traffic can therefore prevent
daytime sleep forever even when no operator is using the bridge.

## Decision

1. Activate the existing append-only `NB_TIME_QUALITY` type 20 as the sole UTC
   wire contract. The T-Deck publishes checksum-valid, active RMC date/time as
   a direct GPS source every two seconds. A fixture with a DS3231 reads it as a
   read-only UTC holdover source every ten seconds, but only when OSF is clear
   and all BCD/date fields are plausible. Firmware does not set an RTC.
2. Each fixture keeps a bounded eight-source selector. Direct GPS, bridge, or
   RTC time may stand alone; peer-relayed time requires two agreeing reports.
   GPS outranks bridge, bridge outranks RTC, and RTC outranks peer when vote
   counts tie. Reports must agree within three seconds. Accepted wall time never
   moves backward, a source more than five minutes from accepted time is
   rejected, and authority expires 30 minutes after the last accepted source
   observation.
3. Fixtures calculate solar elevation locally from UTC for Black Rock City
   (`40.7864 N, 119.2065 W`). Civil twilight, solar elevation `-6 deg`, is the
   default dusk/dawn boundary. No local timezone or daylight-saving rule enters
   the firmware.
4. Trustworthy schedule time feeds the existing field lifecycle's day/night
   seam. If time is absent or expires, the existing panel-current confirmation
   and bounded-night behavior regain authority. Local LED tiers, boot guard,
   transport dark, and solenoid safety remain higher authority than schedule.
5. Control precedence is:

   ```text
   safety/power veto
     -> transport or dark lease
       -> active direct/program lease
         -> RAM-only force day/night
           -> UTC civil-twilight baseline
             -> panel-current fallback
   ```

   Thus LED Studio or a program lease can deliberately light fixtures during
   scheduled day, and a dark lease can suppress scheduled night. Commission
   profile retains ADR 0039's always-awake listener behavior; automatic
   day-sleep and autonomous night show remain field-profile behavior.
6. Add a T-Deck **Time / Schedule** app with Auto, Day Dark, and Night Show.
   These are fleet-wide `NB_FORCE_LIFECYCLE` controls, remain RAM-only, and
   clear on fixture reboot. The bridge repeats the selected baseline every two
   seconds for six minutes so a 300-second field sleeper gets a full wake-cycle
   opportunity to hear it.
7. Only a validated, addressed operator command updates the ten-minute awake
   hold. Heartbeats, choreography, time quality, and other peer traffic do not.
   Continuous LED Studio already spans sleep windows. Knock remains one-shot
   and safety-gated; it is not a guaranteed wake mechanism.
8. Keep the production daytime cadence at 300 seconds asleep / 15 seconds
   listening. A 60/8 build-week cadence is a useful future selectable posture,
   but this change does not persist a new cadence or write NVS.

## Power consequence

Measured dark-but-radio-awake current is 126-144 mA. The following duty-cycle
figures are estimates from that measured awake endpoint plus the measured
sub-mA rails-off sleep endpoint; boot overhead still needs an external-meter
run:

| Posture | Estimated average | 12-hour use | Worst command wait |
|---|---:|---:|---:|
| Dark, radio continuously awake | 126-144 mA | 1.51-1.73 Ah | immediate |
| 60 s sleep / 8 s listen | about 15-18 mA | about 0.18-0.22 Ah | 60 s |
| 300 s sleep / 15 s listen | about 6-8 mA | about 0.07-0.10 Ah | 300 s |

The 60/8 estimate is roughly 7-9 times lower than continuous RX while giving
an average command wait near 34 seconds. The 300/15 estimate is roughly 18-24
times lower than continuous RX, with an average wait near 2.5 minutes.

## Consequences and validation

- Time is deterministic and inspectable, while loss of all anchors fails back
  to the already-deployed heuristic rather than inventing wall time.
- The T-Deck GPS is the immediate absolute source. Fixture SAM-M8Q I2C time
  acquisition and peer relay are still later work; the existing packet and
  selector already accept those sources without a second protocol.
- A wrong RTC with OSF clear is bounded by cross-source ranking, agreement, and
  the five-minute accepted-time jump limit. RTC commissioning remains an
  operator responsibility.
- Native tests cover NMEA checksum/status/date, packet layout, source trust,
  monotonic time, peer agreement, and civil-twilight day/night samples.
- Before fleet deployment, validate T-Deck GPS publication on channel 11,
  DS3231 OSF refusal and valid UTC, at least two fixtures changing field state
  across compressed dusk/dawn, explicit Auto/Day/Night override capture across
  a sleep cycle, and external-INA averages for both proposed cadences.

