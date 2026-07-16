# System Architecture + Power Budget

**Status:** Current working architecture, 2026-07-08. This supersedes the old
ESP32-C3/CN3058/AP2112K/direct-Vbat first pass. Historical decisions remain in earlier
ADRs; for the live path read this with ADRs 0021-0029. **The Fleet Plan table below is
the canonical living count** -- other docs reference it instead of repeating numbers.

## System Goal

Build ~150 autonomous bamboo lighting fixtures for Burning Man 2026/2027. Each fixture is
fungible: no per-unit pairing, no fixed wiring topology, no infrastructure dependency, and
no skilled repetitive assembly operation at 150-unit scale.

## Fleet Plan (living counts -- canonical)

Counts are TENTATIVE until installation (decision record: ADR 0024). The design is
fungible and fully wireless, so placement is free and the split can shift on-site.
`ops/bom.md` mirrors these counts; update here first.

| Class | Count | LED | Power | Sensors (tentative) |
|---|---|---|---|---|
| Hanging downlight (7-10 ft) | 72 (<=110 by large-enclosure pool) | 4 W RGBW + gobo | Voltaic P105-class 5 W panel | MSA311 + TMF8820-mini (downward) |
| Perimeter (5 ft shepherd hooks) | 38-40 | SK6812 HEX | Voltaic P126-class 2 W panel | VL53L5CX (outward); MSA311 likely |
| Uplight (simple cylinder, no gobo) | 24 (perimeter + uplights <=60 by small-enclosure pool) | 4 W RGBW | hinged solar "wing" on the boot (likely P105 5 W) + 6 Ah; low-brightness budget, tuned at the NC prebuild (RESOLVED 07-15; 20 Ah cancelled on sourcing) | none |
| Chandelier (16 central shafts) | 16 | HEX + RGBW mix (TBD) | likely 6 Ah + USB-C top-ups, carpenter-built box housing | none |

Total 150-152. All classes share PowerFeather V2 internals, firmware, and day-sleep
behavior. Every fixture gets a gasketed panel-mount USB-C rescue/charge port wired
to the PowerFeather's USB-C (150 extension cables bought 2026-07-10) -- USB recovery
without opening the hat; the solar-free classes also charge through it.
Chandelier scope/ownership is still loose; its 16 shafts are the only locked positions
and some may stay unpopulated. Board spares are healthy since the 90-board order
(158 production + ~8 bench); enclosure pools cap the allocation (large <=110
downlights, small <=60 perimeter + boots) -- see `ops/bom.md` spares math.

## Current Block Diagram

```
                     Solar panel
          Voltaic P105/P126 class, or measured alternate
                              |
                              v
                   PowerFeather V2 VDC input
          BQ25628E charger / power path / VINDPM
          - set VBUS_OVP=1 for 6 V-class panels
          - HIZ requalification guard (shipped: solar_guard.h)
                              |
                              v
                      LiFePO4 cell
          production target: one large cell, likely 32700
          measured: 5726 / 5752 mAh (n=2, 2026-07-06)
                              |
                              v
         PowerFeather V2 power-management + telemetry stack
          MAX17260 gauge/current sense
          TPS631013 3.3 V buck-boost
          ESP32-S3-WROOM controller
          switchable 3V3 header rail (GPIO4)
          switchable VSQT/STEMMA-QT rail
                              |
                +-------------+--------------+
                |                            |
                v                            v
        Direct-GPIO LED role          Sensors by class (ADR 0027)
        GPIO10/A0 in bench rigs       MSA311 accel (STEMMA, 100 kHz bus)
        - HEX SK6812 on 3V3 rail      TMF8820-mini downward (downlights)
        - 4 W RGBW on 3V3 rail        VL53L5CX outward (perimeter)
          (DECIDED by A/B, 0029)      bench-only: thermal/radar/INA
        - LED rail switchable/default-off

        Noisemaker (OPEN): STEMMA speaker synth vs solenoid
        bamboo-strike -- candidates benched, none selected (LOG 2026-07-07)
```

The 2026 production path is **COTS PowerFeather V2** (ADR 0024); the
PowerFeather-derived custom assembly is the 2027 option. The reference architecture is
the same either way: ESP32-S3 WROOM-class RF, BQ25628E-class charger/power path,
MAX17260-class gauge, buck-boost 3.3 V rail, direct-GPIO LEDs, keyed/serviceable
connectors, and boring USB/pogo recovery.

## Validated On Hardware

- **PowerFeather V2 feasibility:** ADR 0021 is the go decision. ESP-NOW at the projected
  100-node scale, battery-only no-touch OTA with A/B rollback, watchdog recovery, and the
  solar charge path are all validated.
- **Network/radio:** 5-node bench showed roughly 98-99% projected PDR at 100 nodes for a
  1-2 Hz heartbeat. Range held through a house, yard, and oak; the bamboo lantern is
  RF-transparent enough, while the solar panel is the major attenuator.
- **OTA/recovery:** battery-only OTA recovered repeatedly with no button press; a
  self-test-failing image auto-reverted. Production firmware still needs the same
  rollback/health pattern, including delayed mark-valid for late crashes.
- **Sleep:** always-on receive is too expensive, but deep sleep with both switchable 3.3 V
  rails cut is sub-mA by external INA ground truth.
- **Battery:** the 2000 mAh LFP bench cell measured about 2077 mAh. The 32700 6 Ah
  production cell is qualified at n=2 (5,726 / 5,752 mAh clean to 2.5 V); the Amazon
  "7.2 Ah" alternative measured 5,643 mAh with 2.3x IR and was rejected (ADR 0025,
  LOG 2026-07-06/07). For autonomy math use usable-above-floor, not the lab number:
  5,139 mAh above 3.0 V (ADR 0023 has the full voltage-to-remaining map and the
  dim/off/sleep thresholds derived from it).
- **Bus integrity:** the June/July battery-only reboot epidemics were one mechanism --
  degraded signal integrity on the shared charger/gauge I2C bus upsetting the
  BQ25628E's power path (ADR 0028). Convicted by a controlled 400-vs-100 kHz A/B and
  sealed by a 46.2 h continuous battery soak that ended in honest cell exhaustion.
- **LED electrical drive:** measured per role (ADR 0029) -- the full rail/VBAT/boost
  matrix exists (VBAT-direct buys +33 % fringed white, 1,746 lux no wall; clean
  W-only unchanged; TPS63802 boost = 2.3x clean-white ceiling at ~25-30 % efficacy
  tax, shelved). Both feeds DECIDED on the 3V3 rail: the 2026-07-11 instrumented
  A/B through production-realistic cabling inverted the fat-wire result -- rail
  wins +2.5 % mean, 22/25 comparisons (ADR 0029 amendment).
- **Low-battery lifecycle:** net_bench field-cycle (charge -> wait-dark -> draw ->
  protect) has run multi-day outdoor solar cycles; low-VBAT OTA proven to ~3.10 V
  loaded battery-only, 2.901 V solar-assisted, 2.496 V USB-assisted.

## LED Architecture

ADR 0022 records the current LED decision: use a **mixed fleet by optical role**.

- **HEX SK6812 array:** close-range animation, split-color effects, ambient glow, and
  intimate fixtures. Ben's current visual preference is usually 1 pixel white or 3 pixels
  single-channel/trail rather than all-37 full white.
- **4 W RGBW point source:** long-throw crisp gobo projection with useful color-fringe
  overlap effects.
- **IS31FL3741 13x9 matrix:** ruled out for the PowerFeather V2 battery build. It browns
  out the board on the shared charger/gauge I2C bus under WiFi; use direct-GPIO LEDs.

Electrical feed per role (ADR 0029 + 2026-07-11 amendment): BOTH roles on the
regulated switchable 3V3 rail (V+/GND/A0, right-angle JST-XH) -- one harness, one
pinout, and the rail is the hard LED kill. The VBAT-direct option was measured
better only through fat bench wire; through production-realistic cabling the rail
wins on every color. The 4.2 V boost stays shelved with complete numbers.

Firmware implications:

- send an explicit all-off frame before sleep or rail shutdown;
- ramp gently and cap `brightness x lit_count`, especially for boosted HEX builds;
- keep the LED rail switchable/default-off so a hung MCU cannot drain the pack;
- keep role-specific patterns/config while sharing one direct-GPIO LED abstraction.

## Solar Architecture

The charger path works, but sizing is still being closed from measured harvest and measured
show loads.

Measured/known:

- Seeed 3 W hot-panel sweep: best point around 4.6-4.7 V; panel-side INA measured about
  1.91 W while BQ-side telemetry reported about 1.73 W. The default 5.5 V setpoint left a
  large amount of harvest on the table in heat.
- BQ25628E default input OVP is too low for many "6 V" panels at connect-time Voc. The
  fix is firmware: `firmware/powerfeather_solar_guard.h` (baseline in every charging
  sketch since 2026-06-29) forces wide VBUS_OVP and kicks a HIZ requalification when
  supply is present-but-not-good. Bright-sun hardware validation still pending.
- **Panels selected and bought** (ADR 0026; 110x P105 + 50x P126 + 160 pigtails,
  2026-06-24), measured outdoors 2026-06-29 into a hungry LFP:
  - P105 5 W -> downlight/RGBW role: ~3.8-3.9 W panel-side at the ~m46-m48 optimum
    (charger input ~3.47 W). Possibly still acceptance-limited; hungrier re-run queued.
  - P126 2 W -> perimeter/HEX role: ~1.89 W panel-side at ~m58 -- at rating in real
    heat.
  - Uplights/chandelier may go solar-free instead (20 Ah or budgeted 6 Ah cell +
    USB-C charging) -- open decision shared by ADRs 0025/0026.

Bench/sizing rules:

- use panel-side INA power as the panel-capability truth source when available;
- treat MAX17260 LFP SOC as advisory until learned; use voltage/current/charge acceptance
  as guardrails;
- run MPP sweeps on a hungry battery to avoid demand-limited false flats;
- record panel temperature method, because IR front readings, back-side SHT31 readings, and
  different backing thicknesses are not interchangeable.

## Power Budget Status

There is **no final production nightly budget yet**. The old ~120 mAh/night number is
**retired** (2026-07-02): it was pre-hardware napkin math (low-current ESP32-C3, very dim
1-3 pixel assumptions) and the gobo bench work shows crisp projection needs far more LED
power than it assumed. Do not use it as a floor or an anchor; the budget comes bottom-up
from measured LED draw x a realistic show duty cycle.

Use this accounting shape until the real duty cycle is chosen:

```
daily_energy_deficit_Wh =
    night_LED_show_Wh
  + night_controller_and_radio_Wh
  + daytime_sleep_overhead_Wh
  - measured_harvest_at_MPP_Wh
```

Known measured anchors:

| Item | Measured / current read |
|---|---|
| Always-on ESP-NOW peer | about 168 mA / 0.55 W, unsustainable |
| Rails-off duty-cycled sleep | sub-mA by external INA |
| HEX 1 px full | about 41.8 mA LED-rail draw |
| HEX 3 px | about 105 mA LED-rail draw |
| HEX actual preferred looks | rough 0.4-0.6 W battery-side with overhead |
| HEX all-37 full class | rough 2 W+ battery-side, not a normal show state |
| 4 W RGBW RGB-full class | rough 1.1 W battery-side |
| 4 W RGBW white-only class | rough 0.45 W battery-side |
| 32700 production cell (n=2) | 5,726/5,752 mAh to 2.5 V; 5,139 mAh usable above the 3.0 V floor (ADR 0023) |

The next sizing output should be role-specific: one budget for HEX fixtures and one for
point-source RGBW fixtures, then panel size by role.

## Production Design Rules

- Do not use IS31FL3741 on the PowerFeather V2 shared power-management I2C bus.
- **Bus integrity (ADR 0028):** any I2C bus shared with the charger/gauge runs at
  100 kHz -- never raise the clock; a custom PCBA gets a dedicated power-management
  bus; no power-management I2C from core-0-pinned tasks while WiFi is active; treat
  battery-only `poweron` resets as possible power-path register upsets.
- **LED wiring (ADR 0029):** if any LED branch is ever VBAT-fed, tap downstream of
  the gauge's current-sense shunt (or coulomb telemetry goes blind to the dominant
  load), use fat conductors, and provide a default-off kill per ADR 0013 -- the
  3V3-rail shutoff no longer covers it.
- Do not trust LFP percentage SOC alone for solar qualification or low-battery decisions.
- Do not connect/boot high-Voc panels in bright sun without the BQ25628E OVP/HIZ firmware
  guard, or at least shade the panel during connection.
- Keep the antenna out from under the panel, battery, screws, wiring, and metal.
- Keep USB/pogo flashing as the guaranteed recovery path even if OTA is strong.
- Avoid per-unit hand soldering/crimping/configuration. Use keyed connectors, fixtures,
  factory assembly, and one firmware image with runtime/NVS config where possible.

## Open Gates

- Bottom-up nightly energy budget by LED role and show duty cycle.
- (RESOLVED 2026-07-15) Uplight/chandelier power -> hinged solar wing + 6 Ah for
  uplights; 6 Ah + USB-C for chandelier. Remaining wing items are in the gate above.
- Sensor allocation confirmation per class + presence choreography firmware
  (ADR 0027 open items).
- Uplight wing: mechanical design (hinge + panel mount on the boot), panel choice
  (likely P105 5 W), and the low-brightness budget (NC prebuild experiments).
- Noisemaker verdict: solenoid bamboo-strike vs STEMMA speaker synth vs relay
  clicks (all still live, even simple beeps; wider crowd input at the 2026-07-09
  camp meeting).
- MPPT policy: fixed setpoint, temperature-compensated setpoint, or software P&O.
- Mock-hat RF with real panel/battery placement.
- Sealed-hat thermal test, especially LFP charge-temperature behavior.
- ADR 0023 low-battery state machine into production firmware (current bench floors
  strand capacity).
- Re-check the ESP-NOW scale extrapolation at 150 nodes (computed at 100).
