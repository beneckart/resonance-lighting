# System Architecture + Power Budget

**Status:** Current working architecture, 2026-06-17. This supersedes the old
ESP32-C3/CN3058/AP2112K/direct-Vbat first pass. Historical decisions remain in earlier
ADRs; for the live path read this with ADR 0021 and ADR 0022.

## System Goal

Build 100 autonomous bamboo downlight fixtures for Burning Man 2026/2027. Each fixture is
fungible: no per-unit pairing, no fixed wiring topology, no infrastructure dependency, and
no skilled repetitive assembly operation at 100-unit scale.

## Current Block Diagram

```
                     Solar panel
          Voltaic P105/P126 class, or measured alternate
                              |
                              v
                   PowerFeather V2 VDC input
          BQ25628E charger / power path / VINDPM
          - set VBUS_OVP=1 for 6 V-class panels
          - HIZ-toggle requalification guard needed
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
        Direct-GPIO LED role          Optional sensors
        GPIO10/A0 in bench rigs       ToF / IMU / env / INA
        - HEX SK6812 array            via I2C/UART as tested
        - 4 W RGBW point source
        - LED rail switchable/default-off
```

The production path may be COTS PowerFeather V2, a PowerFeather-derived custom assembly,
or a hybrid with a custom LED/power adapter. The reference architecture is the same either
way: ESP32-S3 WROOM-class RF, BQ25628E-class charger/power path, MAX17260-class gauge,
buck-boost 3.3 V rail, direct-GPIO LEDs, keyed/serviceable connectors, and boring USB/pogo
recovery.

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
  "7.2 Ah" alternative measured 5,643 mAh with 2.3x IR and was rejected (LOG
  2026-07-06/07). For autonomy math use usable-above-floor, not the lab number:
  5,139 mAh above 3.0 V (ADR 0023 has the full voltage-to-remaining map and the
  dim/off/sleep thresholds derived from it).

## LED Architecture

ADR 0022 records the current LED decision: use a **mixed fleet by optical role**.

- **HEX SK6812 array:** close-range animation, split-color effects, ambient glow, and
  intimate fixtures. Ben's current visual preference is usually 1 pixel white or 3 pixels
  single-channel/trail rather than all-37 full white.
- **4 W RGBW point source:** long-throw crisp gobo projection with useful color-fringe
  overlap effects.
- **IS31FL3741 13x9 matrix:** ruled out for the PowerFeather V2 battery build. It browns
  out the board on the shared charger/gauge I2C bus under WiFi; use direct-GPIO LEDs.

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
- BQ25628E default input OVP is too low for many "6 V" panels at connect-time Voc. Set
  VBUS_OVP=1 and add a supply-present-but-not-good HIZ requalification kick before any
  panel buy with Voc above about 6 V.
- Voltaic ETFE candidates:
  - P105 5 W: larger, heavier, mounting holes, better energy/storm/dust margin.
  - P126 2 W: much nicer hat footprint, likely role-specific if the HEX budget closes.

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
- Do not trust LFP percentage SOC alone for solar qualification or low-battery decisions.
- Do not connect/boot high-Voc panels in bright sun without the BQ25628E OVP/HIZ firmware
  guard, or at least shade the panel during connection.
- Keep the antenna out from under the panel, battery, screws, wiring, and metal.
- Keep USB/pogo flashing as the guaranteed recovery path even if OTA is strong.
- Avoid per-unit hand soldering/crimping/configuration. Use keyed connectors, fixtures,
  factory assembly, and one firmware image with runtime/NVS config where possible.

## Open Gates

- Bottom-up nightly energy budget by LED role and show duty cycle.
- Voltaic P105/P126 outdoor harvest tests after BQ25628E OVP/HIZ guard.
- MPPT policy: fixed setpoint, temperature-compensated setpoint, or software P&O.
- HEX 4.2 V boost bench result and boosted-build current cap.
- Mock-hat RF with real panel/battery placement.
- Sealed-hat thermal test, especially LFP charge-temperature behavior.
- Production path: COTS PowerFeather V2, custom PowerFeather-derived assembly, or hybrid.
