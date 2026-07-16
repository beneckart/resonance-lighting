# 0025 -- Production battery: fullbattery.com 32700 6 Ah, qualified n=2; Palowextra rejected

**Date:** 2026-07-08 (records the 2026-06-11 first qualification, the 2026-07-06/07
shootout verdict, and the resulting purchases)
**Status:** Accepted. Supersedes the 18650-default format guidance of ADR 0017 (its
one-larger-cell-per-fixture principle stands) and the sizing specifics of ADR 0002
(its LiFePO4 chemistry decision stands). ~~One sub-decision remains OPEN: the 20 Ah
solar-free option for uplights/chandelier (shared with ADR 0026).~~
Annotation 2026-07-15: the 20 Ah sub-decision is CLOSED -- the sample verified
honest (19,412 mAh, 97.1 % of label, 2026-07-12) but batteryspace could not supply
quantity in time and the Alibaba counterpart (~$4.50/cell bulk) needs ocean freight
that misses 2026. Uplights get a hinged solar "wing" on the boot + the standard
6 Ah cell (low-brightness budget); the 20 Ah + Alibaba route is a 2027 idea.
**Owners:** Ben + Claude

## Context

ADR 0002 chose LiFePO4; ADR 0017 chose one larger cell per fixture, defaulting to
18650 (1500-2000 mAh) -- both before any production-class cell was measured, and
before measured LED show loads retired the ~120-170 mAh/night napkin budget. Bench
work through June showed real show loads of hundreds of mA and made a larger cell
attractive; the 32700 format fits the hat envelope.

Two candidate 32700 cells were measured with full-discharge coulomb runs (HEX37
white val224, ~0.86 A battery-side, 79.9 deg F, INA219 ground truth, Nitecore-only
charging; protocol in `docs/tests/BATTERY_32700_SHOOTOUT_PLAN_2026-07.md`):

- **fullbattery.com 6 Ah** -- 5,726 mAh (2026-06-11, first sample, clean to 2.473 V,
  zero resets) and 5,752 mAh (2026-07-06, second sample, +0.5 % vs June). 96 % of
  label. IR ~60 mOhm. $5.10/cell class, ~$0.89 per delivered Ah.
- **Palowextra "7.2 Ah"** (Amazon) -- 5,643 mAh to 2.5 V = 78 % of label, with 2.3x
  the IR (136 vs 60 mOhm), which pulls its knee through the 3.0 V product floor
  1.4 h earlier: 4,342 vs 5,139 mAh usable above 3.0 V (-15.5 %). Weight parity
  (136 vs 138 g) says same-class cell in a bigger-number wrapper; corroborated by
  Off-Grid Garage's 5,450 mAh on a cycler.

## Options considered

- **fullbattery 6 Ah:** honest label, low IR, reproducible n=2. Chosen.
- **Palowextra "7.2 Ah":** rejected -- label busted, and the IR (not the label)
  costs 15.5 % of usable capacity where it counts. The planned cycle-2 rig-swap was
  deliberately cancelled: the ~8 % rig confound cannot close a 15.5 % gap.
- **18650 1500-2000 mAh (ADR 0017 default):** superseded -- measured show loads and
  the retirement of the old nightly budget make the small cell the wrong size.

## Decision

Production cell: **fullbattery.com 32700 LiFePO4 6 Ah, qualified at n=2**
(5,726 / 5,752 mAh delivered; plan around **5,139 mAh usable above the 3.0 V floor**
per ADR 0023, not the lab number).

Purchases: **175 cells bought** -- 75 on 2026-06-11, the same day the first sample
qualified, and 100 more on 2026-07-07 after the shootout sealed the vendor
($441.70 + $565.20; see `ops/PROCUREMENT.md`).

OPEN sub-decision: **20 Ah LFP cylindrical single cell** (batteryspace.com product
#6832; 2 samples on hand) as the solar-free power source for uplights/chandelier.
Gated on an uplight energy-budget bench test on the samples; alternatives are an
off-light 5 W panel or an aggressive power budget on the plentiful 6 Ah cells. A
bulk buy (~40 cells) happens only if the bench test wins the argument.

## Consequences

- ADR 0023's dim/off/sleep thresholds were derived on this cell and inherit its
  qualification; re-derive per the ADR 0023 recipe if the cell, temperature range,
  or load class changes (the 20 Ah cell would need its own map).
- Charging discipline for bench cells stays Nitecore-only until production charge
  profiles are locked; the MAX17260 gauge current bias (/1.08) and cold-POR gotcha
  are chip traits documented in `firmware/POWERFEATHER_NOTES.md`.
- Spares math: 175 cells vs 150-152 fixtures leaves ~23-25 spares (healthy), before
  any 20 Ah decision shifts uplight/chandelier off the 6 Ah pool -- which would only
  increase the margin.
- Battery-vendor risk is closed in the ROADMAP risk register; the remaining battery
  risk is the open 20 Ah decision and its batteryspace lead time.

## References

- `docs/tests/BATTERY_32700_SHOOTOUT_PLAN_2026-07.md` and
  `BATTERY_32700_SHOOTOUT_REPORT_2026-07-06.html`
- ADR 0023 (thresholds derived from the F-cell curve)
- LOG 2026-06-11 (cont. 3) -- first qualification; LOG 2026-07-06/07 -- shootout
- ADR 0002 (chemistry, stands), ADR 0017 (one-larger-cell principle, stands;
  format superseded)
