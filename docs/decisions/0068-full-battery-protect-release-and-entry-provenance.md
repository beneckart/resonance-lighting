# 0068 -- Full-battery PROTECT release and entry provenance

**Date:** 2026-08-28

**Status:** Accepted in source; exact Rikku canary and fleet promotion pending

**Owner:** Ben

**Extends:** ADR 0023, ADR 0046, ADR 0051, ADR 0059, ADR 0064

## Context

Outer downlight Rikku `9F26B0` was dark in durable PROTECT despite an installed
15 Ah LFP near 3.6 V. Its retained entry audit recorded 3.596 V in field profile
at 2.809 seconds uptime. The normal ladder cannot enter PROTECT from that
voltage: its standard dim/off/protect points are 3.15/3.10/3.05 V.

Fleet history proved the latch existed before the latest OTA. Rikku was already
using the 900-second PROTECT cadence on `fx-260826-51d1fe1-p`; the later
`fx-260827-1254f04-p` OTA at 3.582 V correctly preserved NVS and passed its
pending-verify window. OTA already calls `allLoadsOff("OTA")`, which parks the
rails and clears the durable `load_arm` marker before writing the image. It did
not create or perpetuate a live load marker, and an OTA reset must not blindly
erase a real low-battery PROTECT latch.

A high open-circuit VBAT also does not disprove a legitimate reset-loop guard.
A poor crimp, resistive harness, cold cell, load transient, or BATFET/power-path
event can collapse VSYS only under load and then rebound to a healthy voltage
after the guard parks the rail. ADR 0023 therefore deliberately forbids voltage
rebound alone from reapplying that load. The pre-init guard cannot safely read
the battery before making its rail-off decision.

The actual deadlock was the release rule. With good USB input, valid BQ status,
no fault, and VBAT around 3.56-3.58 V, Rikku accepted only about 0-2 mA because
the charger was in constant-voltage regulation near the top of charge. The old
release required at least +20 mA continuously for 60 seconds. A healthy full
battery could therefore remain protected forever precisely because it was too
full to accept the proof current.

The retained PROTECT record also lacked the predecessor stage, reset reason,
prior `load_arm`, and unexpected-reset streak. Its voltage proved that the
runtime low-VBAT branch did not initiate this entry, but it could not distinguish
load-armed escalation from the disarmed three-reset loop breaker.

## Decision

1. **Keep fail-safe reset entry and OTA preservation.** An unexpected reset from
   an armed FULL stage still consumes the bounded DIM retry; an unexpected reset
   from armed DIM/LEDS_OFF, or the existing three-reset loads-off loop breaker,
   still parks and persists PROTECT before any load can turn on. OTA still parks
   and disarms loads but does not directly clear `fc_stage`. This preserves the
   cause-independent collapse-loop guarantee.

2. **Require valid charger state on every automatic release.** Both release
   proofs require a valid external supply judged good, readable BQ25628E
   `CHG_CTRL0`/`CHG_STAT`/`FAULT_STAT0`, charging enabled in both firmware state
   and `CHG_EN`, and no charger fault. Missing/unknown charger data is not a
   no-fault result. This closes a gap in the previous current proof, whose code
   checked the fault byte but did not require that the charger snapshot itself
   was valid or enabled.

3. **Retain the ordinary recovered-current proof.** Corrected battery current
   `>= +20 mA` and load-compensated VBAT `>= 3.25 V`, with all charger/input
   gates above, must hold continuously for 60 seconds.

4. **Add a full/tapered-battery proof.** A second proof may release PROTECT when
   all of the following hold continuously for 60 seconds:

   - a valid and independently corroborated real battery;
   - load-compensated VBAT `>= 3.45 V`;
   - the same valid/good input, enabled charger, and no-fault gates;
   - BQ `CHG_STAT` is constant-voltage, top-off, or not-charging/done.

   Constant-current state with less than +20 mA is not full-battery proof. The
   MAX17260 SOC estimate remains forbidden as a gate. A proof change or any
   missing battery sample resets the 60-second clock; time from two different
   proofs is never combined. While high VBAT is plausible but the battery is not
   yet corroborated, the fixture requests ADR 0051's rate-limited 30 mA BQ
   presence test under verified external power.

5. **Release only to LEDS_OFF and reboot cleanly.** Either proof first persists
   stage 3, keeps all loads off, and performs the existing software reboot. The
   normal hysteretic ladder must earn DIM and FULL later. The release proof is
   printed as `charge-current` or `full-battery` before reboot. The source audit
   also exposed a pre-existing persistence hole: failure to write a new
   PROTECT stage could still enter timer sleep and wake under the older brighter
   stage. A failed PROTECT write now remains parked awake and retries NVS each
   tick; a failed release-stage write returns to PROTECT and never reboots.

6. **Persist PROTECT entry provenance without changing the NVS record size.** A
   version-1 `SleepAuditRecord` for local PROTECT never used its remote-source
   fields. Flag bit 1 now marks those fields as:

   - origin: low VBAT, load-armed reset, reset streak, NVS fail-safe,
     stage-persist failure, or legacy/unknown;
   - predecessor boot stage;
   - numeric ESP reset reason;
   - prior durable `load_arm` value;
   - consecutive unexpected-reset streak.

   The record remains exactly 32 bytes with the same magic/version/checksum
   rules. A valid older record is annotated once as legacy/unknown while
   retaining its original entry voltage, profile, lifecycle, and uptime.

7. **Expose the provenance fleet-wide.** Append heartbeat tail 18 carries the
   compact origin/predecessor/reset/marker/streak context. T-Deck census and
   `nb-peer`, the logger, and the fleet dashboard length-gate and display it.
   Older fixtures and bridges remain compatible under the canonical append-only
   packet contract.

## Consequences

- A full installed LFP such as Rikku can recover automatically on USB or a
  proven solar input even when charge acceptance is below +20 mA.
- A rebound-depleted battery cannot release at night: it lacks a verified good
  external source and enabled/no-fault charger proof. High voltage alone still
  does nothing.
- An OTA performed under good input gives the ordinary policy enough awake time
  to complete a full-battery proof, but OTA itself does not erase safety state.
- The 3.45 V floor is intentionally conservative. Fully charged project LFPs
  have been observed around 3.55-3.60 V while charging and about 3.40-3.45 V
  rested. A terminated cell that has already fallen below 3.45 V must either
  resume charge evidence or wait; lower this floor only with battery-backed
  fault-injection evidence.
- A new entry can now identify whether `load_arm` or the loads-off reset streak
  caused escalation. Existing records remain honest about missing evidence by
  reporting legacy/unknown rather than inventing a cause.

## Validation required

1. From a clean commit, build one immutable ADR 0040 production artifact and
   OTA exact Rikku `9F26B0` with its installed 15 Ah LFP and good USB input.
2. Require fresh telemetry proving old stage 4, field profile, correct capacity,
   VBAT above 3.45 V, valid/enabled/no-fault BQ, a corroborated cell, and CV,
   top-off, or not-charging/done state while IBAT remains below +20 mA.
3. Observe a continuous 60-second `full-battery` proof, stage 3 persistence,
   one clean software reboot, survival beyond pending verify, correct class and
   sensors, and no load energization before the reboot.
4. Fault-inject each negative gate on a supervised battery-backed fixture:
   missing charger status, charging disabled, BQ fault, bad/removed input,
   uncorroborated BAT, VBAT below 3.45 V, CC with low current, proof change, and
   a missing battery sample. Every case must remain PROTECT and restart the
   evidence window.
5. Re-run the ordinary low-battery +20 mA recovery to prove that path still
   releases only after 60 seconds and still reboots into LEDS_OFF.
6. Trigger one supervised load-armed reset escalation and one loads-off reset
   streak. Confirm the durable record and fleet dashboard distinguish them with
   the exact predecessor, reset reason, marker, and streak.
7. Audit the fleet for stage-4 fixtures with entry VBAT above 3.45 V and triage
   their new origin fields before broad promotion.

## Source validation

- Complete fixture native suite passed, including 195 power-policy checks, 26
  sleep-audit checks, and 70 packet-layout checks.
- All directly compiled T-Deck native test binaries passed; its wrapper's
  separate generated-registry freshness gate is currently blocked by an
  unrelated dirty Donkey registry edit.
- Fleet dashboard/parser tests passed (18 tests).
- Guarded ESP32-S3 PowerFeather field-profile development build passed:
  1,210,353 bytes program, 68,740 bytes globals, 1,210,656-byte binary,
  SHA-256 `ca910470953cd3ebff9c952d04becf15b6e220931d74ddc19ebb9043c01648ee`.
  This dirty-worktree `dev-local` binary is compile evidence only and must not
  be flashed or promoted.

## References

- `firmware/fixture/src/core/power_policy.*`
- `firmware/fixture/src/core/sleep_audit.*`
- `firmware/fixture/src/core/packet.h`
- `firmware/fixture/src/esp32/boot_guard_io.*`
- `firmware/fixture/src/esp32/power_glue.cpp`
- `docs/decisions/0023-lfp-power-policy-thresholds.md`
- `docs/decisions/0051-corroborated-protect-and-load-armed-escalation.md`
- TI BQ25628E data sheet: `https://www.ti.com/lit/ds/symlink/bq25628e.pdf`
