# ADR 0042: BQ-qualified low-VBAT fleet recovery

- Status: Accepted
- Date: 2026-08-16
- Decider: Ben Eckart
- Extends: ADR 0023, ADR 0033, ADR 0041 (universal defaults)

## Context

The normal bare-board-safe charger guard treated any MAX17260 battery reading
at or below 2.5 V as an implausible or absent cell. That prevented a missing
battery node from being charged, but it also stranded real installed LFP cells
that had fallen into the 2.2-2.5 V range. An exact-MAC, 100 mA recovery image
proved that one supervised cell could recover safely on strong USB power, but a
target-specific bypass is too slow for field service and cannot be fleetable.

Lowering the voltage threshold alone is not acceptable. With an adapter
present, the charger's BAT-node capacitance can resemble a low cell. The
BQ25628E provides a documented battery-removal test specifically for this case.

## Decision

The common fixture image may recover a low installed LFP only as follows:

1. The ordinary path remains unchanged above 2.5 V. Below 2.2 V, automatic
   recovery remains refused.
2. In the 2.2-2.5 V window, firmware requires a valid external source at or
   above 4.6 V and 50 mA, near-zero pre-enable battery current, no BQ fault, and
   a verified 300 mA precharge register setting.
3. Charging is disabled. Firmware applies the BQ25628E `FORCE_IBATDIS` roughly
   30 mA BAT discharge for 5 ms, releases it, takes a one-shot VBAT ADC sample,
   and restores the prior ADC configuration. Only an ADC result at or above the
   nominal 2.2 V `VBAT_UVLO` threshold and below 4.4 V proves an attached cell.
4. A proven low cell charges with a 100 mA absolute software ceiling while LED
   and sensor rails are parked. Charger trickle/precharge selection, input DPM,
   TS/thermal protection, timers, and fault handling remain hardware-owned.
5. Any charger fault stops recovery. After VBAT remains at or above 2.55 V for
   60 seconds, firmware restores the unit's normal persisted charge ceiling.
6. Heartbeat telemetry reports the recovery state and the BQ presence-test ADC
   result. A missing battery remains a safe no-charge state.

The exact-target deep-recovery build remains a test-class artifact and does not
graduate above its immutable 100 mA ceiling.

## Consequences

- One fleet image can rescue USB-powered fixtures in the measured low-voltage
  window without confusing an empty BAT node with a cell.
- Recovery can wait for a qualified source rather than requiring a reboot if
  USB is connected shortly after boot.
- A cell below 2.2 V, a failed presence test, weak input, unexpected current,
  or charger fault still requires bench diagnosis rather than automatic charge.
- The installation has no independent cell thermistor telemetry. Initial fleet
  rollout therefore remains physically supervised even though the charger keeps
  its own TS and thermal protections.

## Verification

- Native tests pin every preflight boundary and the BQ ADC presence threshold.
- Packet-layout tests pin the append-only recovery fields.
- An immutable fleet artifact must first pass a healthy canary and one low-VBAT
  USB canary, including the 20-second OTA pending-verify window, before expansion.
