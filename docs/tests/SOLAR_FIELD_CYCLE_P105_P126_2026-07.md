# P105/P126 outdoor solar field cycle and POR-loop findings -- July 2026

**Status:** Active bench record. P105 safety firmware was OTA-deployed July 12;
P126 may remain outside for several more harvest days before being disassembled.

## Purpose

This run combines two related questions:

1. What daily energy range can the production-cabled Voltaic P126 2 W panel deliver
   into a production 32700 LFP/PowerFeather fixture?
2. Why did the adjacent P105/HEX peer enter early nighttime power-on-reset (POR)
   loops, and what must firmware do so any POR source cannot recreate a full-load
   boot loop?

The two peers are useful A/B references but are not identical instruments. P105 has
external panel/battery INAs and a TSL2591. P126 deliberately uses production cabling,
no INAs, no Dupont leads, and no lux sensor.

## Devices and deployed images

| Peer | Panel/load | Instrumentation | Image/status |
|---|---|---|---|
| `9E5B0C` | P126 2 W, fixed 5.8 V; three full-bright single-channel R/G/B HEX pixels spiraling at 120 deg offsets | PowerFeather BQ/MAX17260 only; production cables | `net-bench-2026-07-10.1`; leave in place until teardown |
| `9F26F8` | P105 5 W, fixed 4.6 V; 18-pixel HEX load at brightness 128 | Panel/battery INAs plus onboard telemetry and TSL2591 | `net-bench-2026-07-12.1`; OTA-deployed July 12 |

P126 logger:
`ops/bench/data/ca/2026-07-10-ca-field-cycle-9E5B0C-p126-production-cabling.jsonl`.

P105/P126 share the master/logger, so the same file also captures the P105 peer after
the July 10 P126 deployment. Earlier P105 history is in the July 3-8 field-cycle files
under `ops/bench/data/ca/`.

## Executive read

- The P126 load is representative of the intended production HEX show. Do not raise
  it merely to force a full-to-empty cycle every night.
- The P126's instantaneous input-loss dusk rule created a 14 h 46 min LED window.
  That is a bench-policy artifact, not the intended playa show. Civil dusk to civil dawn during the Aug 30-
  Sep 7 event is about 9 h 53 min to 10 h 15 min, so use 9-10 h for HEX energy sizing.
- At the measured 157.7 mA average battery draw, a 9 h show costs about 1.42 Ah and a
  10 h show about 1.58 Ah. The good July 11 solar interval put about 1.74 Ah into the
  battery, provisionally leaving +0.32 Ah at 9 h or +0.16 Ah at 10 h.
- The observed P126 daily range so far is about 1.12-1.55 Ah at the 5.8 V BQ input,
  or about 1.11-1.74 Ah of positive battery charge. These are only two weather days,
  and the battery number is acceptance- and sampling-limited.
- P126 has avoided POR primarily because its roughly 158 mA show is about one-third
  of P105's 460-480 mA draw and its production harness is lower resistance. It has
  stayed around 3.24-3.33 V instead of exercising the marginal low-voltage region.
  This does not prove its older reset policy safe.
- P105's dense loop was a firmware-amplified loaded-collapse problem: a POR erased
  dim/protect state and confirmation timers, rebound voltage looked healthy on reboot,
  and firmware reapplied the full LED load until the next POR. A power-path I2C glitch
  can create the same reset signature, so the loop breaker must be independent of the
  initiating cause.

## P126 MPP and daily harvest

### Fixed MPP result

The July 10 onboard re-check found a broad best region around 5.8-6.0 V. The apparent
6.0 V charger-input gain was only about 3.8 percent and produced no corresponding
battery-current gain; 6.2 V rolled over. Two 5.8 V anchors agreed within 0.4 percent.
Keep the external-INA-qualified P126 setpoint at **5.8 V**.

### What "Ah/day" means on this peer

There is no panel-lead INA, so there is no direct panel-terminal Ah measurement.
Record all three quantities and do not substitute one for another:

- **BQ input Ah:** integral of positive onboard `supply_ma`. At fixed 5.8 V this is
  the best available day-to-day panel/charger-input comparison.
- **BQ input Wh:** integral of `supply_v * supply_ma`; preferred when comparing energy.
- **Positive battery Ah:** integral of positive corrected `battery_ma`. This is what
  reached the cell after conversion and the live system load, but it is also affected
  by battery charge acceptance and sparse sleep sampling.

`battery_ma` on the P126 image already includes the measured MAX17260 `/1.08`
correction. Never divide it again. BQ supply current does not receive that correction.

### Preliminary rolling ledger

One-second cached sample-and-hold integration, with host gaps over 5 seconds excluded:

| Local date | Coverage | BQ input | BQ input energy | Positive battery charge | Interpretation |
|---|---:|---:|---:|---:|---|
| Jul 10 | 0.72 h | 0.166 Ah | 0.95 Wh | 0.190 Ah | Partial deployment afternoon; not a daily result |
| Jul 11 | 16.56 h | 1.545 Ah | 9.02 Wh | 1.738 Ah | Best observed sunny day; host resumed before useful P126 input began |
| Jul 12 through about 18:00 | 17.93 h | 1.116 Ah | 6.51 Wh | 1.114 Ah | Sunny then overcast; provisional weaker-day point |

The input-Ah and battery-Ah columns may look inverted because the buck charger trades
the roughly 5.8 V input for higher current at roughly 3.3 V. Compare Wh across voltage
domains. Input-to-battery differences also include conversion loss and the awake system
load.

The purpose of any remaining P126 deployment is to grow this weather-conditioned range,
not to validate the current 14.8 h show duration. For each additional local day record:

1. BQ input Ah and Wh;
2. positive battery Ah and Wh;
3. sky condition and any shading/handling;
4. logger coverage and whether a number is complete or a lower bound;
5. minimum loaded VBAT and any reset/protect event.

Do not use MAX17260 SOC percent for this ledger. It has previously stuck at 1 percent
with roughly 35 percent of cell capacity remaining. Coulomb integration against the
measured roughly 5,500 mAh usable capacity is the relevant battery-state estimate.

## Production HEX show duration

The clean July 11-12 P126 session ran from 18:07:33 to 08:53:54 PDT:

- wall-clock LED window: 14.773 h;
- logger-integrated battery draw: 2.33 Ah;
- mean corrected draw: 157.7 mA;
- dawn loaded/resting region: about 3.27 V; no dim, protect, or POR.

The deployed July 10 peer has no light sensor and predates the July 12 dusk qualifier.
It declares dusk immediately when charger input stops meeting the useful-input test and
ends draw when useful input returns. It can chatter through new cycles as input crosses
that boundary and can start well before visual darkness or run well after dawn. The
30-minute no-sensor confirmation exists in `net-bench-2026-07-12.1`, but that image was
deployed only to P105 and is not running on P126.

Black Rock Desert tables for event week give about 19:59-19:46 civil-dusk end and
05:52-06:01 civil-dawn start. That is a roughly 9 h 53 min to 10 h 15 min dark-show
window. Provisional sizing:

| Show duration | Draw at 157.7 mA | Net after the Jul 11 1.74 Ah charge estimate |
|---:|---:|---:|
| 9 h | 1.42 Ah | +0.32 Ah |
| 10 h | 1.58 Ah | +0.16 Ah |
| 11 h | 1.73 Ah | about break-even |
| 14.77 h bench artifact | 2.33 Ah | -0.59 Ah |

This is encouraging, not yet a production margin. Playa heat, dust, panel angle,
fixture shading, smoke/clouds, battery taper, and incomplete telemetry can consume the
small 10 h surplus. The qualified cell provides multi-day buffering, but final sizing
needs more daily harvest points and the actual show profile.

Sources:

- https://www.timeanddate.com/sun/@5701651?month=8
- https://www.timeanddate.com/sun/@5701651?month=9
- https://burningman.org/black-rock-city/ticketing-information/

## P105 POR boot-loop reconstruction

### Observed symptom

On July 11, P105 began a dense series of `reset_reason=poweron` boots around loaded
VBAT 3.00-3.05 V after only about 0.68 Ah had been removed from a recently full cell.
This was far earlier than the qualified cell's hard knee. The board repeatedly booted,
reapplied the 18-pixel brightness-128 load, collapsed, and booted again. A final sample
at or below the immediate critical floor eventually allowed protect to latch.

### Firmware regression chain

1. The July 7/8 ADR23 image moved the loaded protection region down. Earlier firmware
   generally exited near 3.10 V; `net-bench-2026-07-08.1` dimmed only after 60 seconds
   at or below 3.00 V, protected after 60 seconds at or below 2.95 V, and used 2.90 V
   as the immediate critical floor.
2. The heavier P105 load could collapse the source before the dim decision was reached
   or confirmed. Added harness resistance can shift that boundary upward, although the
   unchanged harness does not explain why the firmware behavior changed.
3. Dim/protect latches and phase state lived in RTC memory. Timer deep sleep preserves
   them; a true POR does not.
4. The 10/60 second confirmation timers lived only in RAM/RTC state. Every POR reset
   them to zero.
5. On reboot, load release raised the measured battery voltage above the low threshold.
   With no remembered active session, firmware interpreted the rebound as permission
   to enter drawdown and reapplied the full load.
6. The reset could recur faster than either confirmation timer, making the intended
   low-voltage policy unreachable indefinitely.

There was a second, independent trigger problem: daylight required both supply voltage
and useful current. A full/tapered battery could stop accepting current, which firmware
misread as darkness. It briefly turned the LEDs on in the afternoon, then saw input
current return and declared sunrise. This created false cycles and could expose the
power path earlier than visual dusk.

### Initiating cause versus loop amplifier

The July 11 evidence best fits extra loaded droop plus the lower firmware thresholds;
the INA harness can plausibly move the knee by tens of millivolts. That is not the only
way to get a POR. A corrupted I2C transaction to the BQ25628E power-path registers can
open the battery switch at healthy voltage, as established by ADR 0028. Either event
looks like `reset_reason=poweron` because VSYS disappears.

Firmware must therefore treat an unexpected reset during an armed LED session as the
primary fact. It must not depend on distinguishing cell sag, connector resistance,
I2C upset, brownout, watchdog, or panic before making the next boot safe.

## `net-bench-2026-07-12.1` recovery policy

The July 12 P105 image was built with 18 pixels at brightness 128, 6,000 mAh LFP,
1,500 mA charge limit, fixed 4.6 V VINDPM, and these field thresholds:

- dim at or below 3.10 V for 10 seconds;
- LED off/protect at or below 2.95 V for 60 seconds;
- immediate critical protect at or below 2.90 V;
- TSL2591 dusk at or below 200 lux for five minutes;
- separate dawn threshold at or above 500 lux;
- bare-peer fallback: 30 minutes without useful input.

The exact artifact is:
`firmware/net_bench/build/field-cycle-peer-20260712-p105-dusk-dim-retry-r3/net_bench.ino.bin`.
Its full compiler flags are preserved beside it in `build.options.json`.

The reset guard is a cause-independent state machine:

1. Before LED rail-on, persist `idle`, `full`, `dim`, or `protect` in NVS. NVS write
   failure parks safely.
2. At the first instructions of boot, force pixel data and EN_3V3 low. `Board.init()`
   can reconfigure the RTC-held rail, so park it low again immediately after init.
3. An unexpected reset from persisted `full` atomically consumes the sole retry by
   storing `dim` before any rail can turn on. It may then retry once at dim brightness.
4. Any reset from `dim` or `protect`, or a failed NVS read, hard-parks with the LED rail
   off until verified positive battery charge. A second collapse cannot recreate the
   full-power loop.
5. A deliberate load start waits for power/network initialization, clears the pixel
   rail, ramps brightness in four steps, and samples VBAT during the ramp. A low sample
   parks; a dim-region sample reduces the target.
6. Only verified solar/USB charge recovery clears the persisted session stage.

Telemetry added for remote diagnosis:

- `field_session_stage` and `field_session_marker`;
- `field_interrupted_boot`, `field_interrupted_retry`, and
  `field_interrupted_park`;
- `field_dusk_s`;
- existing `field_phase=5`, `field_reason=8`, and `field_protect_latched=true` identify
  a hard-park in the ESP-NOW heartbeat without another protocol-tail change.

OTA completed without a button press. Direct `/telemetry` confirmed the new revision,
idle session stage, valid PowerFeather init, and normal charge state; `/resume` returned
the peer from maintenance to ESP-NOW. Natural low-voltage POR retry/park behavior still
needs an autonomous field observation or a controlled bench injection before it is
called fully validated.

## Reusable POR and telemetry gotchas

1. **A POR erases the safety decision that was about to happen.** Any timeout, debounce,
   coulomb budget, or dim latch stored only in RAM/RTC can be defeated forever by a
   faster reset loop. Persist the armed load tier before energizing it.
2. **Rebound voltage is not recovery.** Removing a load raises LFP voltage. Do not use
   that rebound alone to authorize the same load after reset; require a persisted retry
   budget or verified positive charge.
3. **Default-off must begin before SDK initialization.** PowerFeather cold init enables
   the switchable rail, GPIO4 is RTC-held, and raw `digitalWrite` is ineffective after
   SDK init. Drive/hold it low first, use the SDK correctly, and park it again after init.
4. **Consume retry state before rail-on.** Persisting `dim` after starting the retry
   leaves a power-fail window that recreates the loop.
5. **Ramp startup loads.** A rail enable plus stale pixel data plus WiFi startup can
   stack transients before steady-state low-voltage logic runs.
6. **Dusk needs qualification and hysteresis.** Charger current alone is not daylight;
   full-battery taper, shade, and clouds create brief low-current intervals.
7. **`poweron` has multiple physical causes.** Real loaded collapse and a BQ battery-
   switch opening both remove VSYS. Keep ADR 0028's 100 kHz/shared-bus rules even after
   adding the NVS loop breaker.
8. **Harness resistance changes loaded thresholds.** Apply load compensation or qualify
   each production power path; do not copy a voltage threshold from a different current
   and harness without margin.
9. **Onboard active-time integration currently under-reports.**
   `fieldCycleIntegrateActive()` truncates every `dt / 1000` and resets the millisecond
   origin, losing the remainder. The 14.773 h session reported 13.02 h / 2.08 Ah instead
   of the logger's 14.773 h / 2.33 Ah, about 12 percent low. Carry milliseconds across
   calls before trusting retained active mAh/Wh.
10. **Deep-sleep charging is sparsely sampled.** A five-minute held current estimate is
    useful but not ground truth during fast cloud edges. Always label coverage and
    uncertainty.
11. **Cached protect current is not continuous current.** The host repeats the last
    heartbeat while the peer sleeps. State-aware integrations must not charge that
    stale value across the protect interval.
12. **BQ input is not panel-side ground truth.** It is still the best available P126
    day-to-day proxy, but external panel-lead instrumentation remains the qualification
    reference.

## Current operating decision

Do **not** OTA P126 merely to shorten the current show window. Ben expects to disassemble
this peer for another bench experiment and may leave it outside only a few more days.
Changing firmware now would interrupt the small harvest series and add little value.

If it stays deployed, preserve `net-bench-2026-07-10.1` at fixed 5.8 V and use the added
days only to observe the weather-conditioned P126 BQ-input and battery-charge Ah/Wh
range. Treat its night discharge as an intentionally overlong load soak, not the
production daily-energy profile. The next purpose-built field cycle should implement a
9-10 h show cap and fix millisecond carry in the retained integrator.

As checked July 12 at 18:05 PDT, the logger process was alive and the file was still
growing. It was launched with `--duration 259200`, so it will stop around July 13 at
15:25 PDT. Restart it with a continuation file or longer duration if P126 remains
outside beyond then; do not silently treat later missing rows as zero harvest.

## Related records

- `firmware/net_bench/README.md` -- field-cycle firmware behavior and build flags.
- `firmware/POWERFEATHER_NOTES.md` -- reusable PowerFeather rail, POR, I2C, gauge,
  charge-acceptance, and solar-input gotchas.
- `docs/decisions/0023-lfp-power-policy-thresholds.md` -- measured LFP dim/off/sleep
  policy and load compensation.
- `docs/decisions/0028-power-management-bus-integrity.md` -- BQ power-path I2C failure
  class and 100 kHz rule.
- `docs/decisions/0026-solar-panel-selection-and-role-mix.md` -- P105/P126 role choice.
- `docs/tests/BATTERY_32700_SHOOTOUT_PLAN_2026-07.md` -- qualified capacity and gauge
  limitations.
