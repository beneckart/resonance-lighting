# 0033 -- Two-amp battery charge-current ceiling

**Date:** 2026-08-06
**Status:** Accepted. Resolves the open production charge-current profile in ADR
0025 and supersedes lower generic defaults in active firmware. Historical test
artifacts retain the current they actually used.
**Owners:** Ben + Codex

## Context

Active sketches accumulated 500, 1,000, and 1,500 mA charge-current defaults.
The 500 mA value came from gentle handling of early small/unknown bench cells and
from an unqualified multi-port USB-hub commissioning plan. It was never selected
as the production-cell optimum, but it leaked into LED Studio, demos, the fleet
bringup tool, and the unified fixture NVS default.

There are two independent limits that have also been described as a "500 mA
cap":

- ICHG is the battery-side current ceiling programmed by
  `setBatteryChargingMaxCurrent()`.
- USB source detection/IINDPM can independently restrict input current to about
  500 mA. USB input current is measured at the input voltage and is not the same
  quantity as battery charge current.

PowerFeather V2 exposes a BQ25628E charger with a 2,000 mA maximum charge-current
setting. PowerFeather documents VUSB and VDC inputs at 2 A maximum. The selected
fullbattery 32700 6 Ah cell's vendor specification lists a 6 A standard charge,
so the board's 2 A ceiling is 0.33 C and is the limiting term. The Gotion 33140
15 Ah cell is still under qualification; 2 A is 0.13 C, but its exact temperature
and charging limits must still be confirmed from the procured-cell specification
and qualification runs.

## Decision

Use **2,000 mA as the default battery-side charge-current ceiling in every active
PowerFeather firmware and fleet tool** for the known production LFP cells.

This is a ceiling, not a commanded result. Safe delivered charge current remains
bounded by all of the following:

```text
I_charge <= min(cell limit at SOC and temperature,
                2 A PowerFeather/BQ25628E limit,
                battery wiring/connector/thermal limit,
                eta * (V_source * I_source_limit - P_system) / V_battery)
```

The source term is a power constraint because source and battery voltages differ.
The BQ25628E's VINDPM/IINDPM and dynamic power management reduce battery charge
current as the input voltage/current limit, system load, CV taper, or charger-die
thermal limit is reached.

The 2 A ICHG policy does **not** authorize 2 A from an arbitrary USB port. Do not
override source detection or IINDPM above the source's advertised/measured
capability, and never exceed the PowerFeather input/connector 2 A rating. Solar
images retain panel-specific VINDPM settings so weak panels settle at their
available operating point rather than being collapsed by the 2 A ceiling.

Keep explicit lower-current controls (`--charge-ma`, `G<ma>`) for a smaller,
unknown, damaged, hot/cold, or otherwise limited cell and for a protocol that
requires a lower rate. Chemistry, capacity, and a verified cell specification
remain prerequisites; "2 A everywhere" means the default ceiling for this
production hardware, not permission to ignore a different cell's data sheet.

Battery temperature is part of the cell limit. Charger-die thermal regulation
does not measure cell temperature. Hardware JEITA protection is enabled only when
a physical Semitec 103AT NTC is attached to the cell; the production thermistor
path remains a hardware qualification gate.

Application reflashing does not erase Preferences/NVS. Charge-policy version 1
in `fixture` and `net_bench` therefore replaces legacy persisted 500/1,000/1,500
mA defaults with 2,000 mA once. A nonstandard pre-existing value is preserved as
a possible deliberate cell limit, and a lower `G<ma>` set after the migration
continues to persist.

## Consequences

- `fixture`, `net_bench`, `power_bench`, LED Studio, the standalone demos, field
  OTA defaults, and fleet commissioning default to a 2,000 mA ICHG ceiling.
- A firmware reflash reliably applies the new policy even to previously
  commissioned boards with old NVS values.
- Historical logs, qualification reports, and named artifacts continue to state
  their actual 500/1,000/1,500 mA settings.
- Actual current may remain around 500 mA on USB because of source detection, or
  below 2 A on solar because of available panel power. That is expected dynamic
  regulation, not a failure of this policy.
- Production validation must exercise battery temperature protection, input and
  cable temperatures, charger faults, and VINDPM/IINDPM telemetry at the 2 A
  ceiling before unattended sealed-hat deployment.

## References

- PowerFeather specifications: https://docs.powerfeather.dev/
- BQ25628E data sheet: https://www.ti.com/lit/ds/symlink/bq25628e.pdf
- FullBattery 32700 specification:
  https://cdn.shopify.com/s/files/1/1307/6829/files/32700-6.0Ah_Specification.pdf?v=1688422956
- `firmware/POWERFEATHER_NOTES.md`
- ADR 0025 (production battery selection and Gotion qualification status)
