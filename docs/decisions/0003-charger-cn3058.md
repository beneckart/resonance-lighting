# 0003 — CN3058 LiFePO4 charger IC

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

Need a charger IC compatible with single-cell LiFePO4 (3.6 V max charge), input from a 1–5 W solar panel, output ~500 mA peak charge current. Must be SMT-assemblable in the JLCPCB Basic library.

## Options considered

- **CN3058 (Consonance):** linear LiFePO4 charger, ~$0.30, JLCPCB Basic part typically. Status output, programmable charge current via Rprog. No MPPT.
- **MCP73123 (Microchip):** linear LiFePO4 charger, ~$1.50, JLCPCB Extended. Better datasheet quality, US-side sourcing reputation. No MPPT.
- **TP4056:** rejected — LiPo only, 4.2 V max charge — overcharges LiFePO4.
- **bq24074:** rejected — LiPo only.
- **CN3791:** rejected — Li-ion only, 4.2 V max.
- **TI BQ25895 / BQ25890 with I2C config to LiFePO4 profile:** programmable, MPPT-capable, but more complex and more expensive (~$4). Overkill for the application.

## Decision

**CN3058.** Cheapest, JLCPCB Basic stock, well-tested in Chinese open-source LiFePO4 designs. Linear (non-MPPT) is fine — for a 1–3 W panel into a single LiFePO4 cell, MPPT efficiency gain is small (~10–20%) and not worth the IC complexity for an art project.

## Consequences

- **Compensate for non-MPPT** by sizing the panel with margin (target 2 W; design hat to accept up to 3 W). See power budget.
- Linear charger dissipates excess panel voltage as heat. With Vpanel ~5–6 V open-circuit and Vbatt ~3.4 V, voltage drop is ~2 V at up to 500 mA = 1 W of heat in the IC. Plan a thermal pad / copper pour on the PCB. Not a thermal crisis but worth being deliberate about.
- CN3058 doesn't have built-in power-path (load sharing). Add a P-MOSFET ideal-diode on the battery side so the panel-direct-to-load case is clean (when sun is on, regulator pulls from panel-via-charger; when sun is off, regulator pulls from battery). Alternative: simpler topology where load is always on battery and charger only fills battery — works fine for our load profile, simpler BOM. **Lean toward the simpler topology** unless bench testing reveals battery cycling that wears out the cell faster than expected.
- Status output (open-drain charging indicator) → drive a small status LED on the hat for visual feedback during deployment ("did this fixture's panel actually start charging this morning?"). Cheap, valuable diagnostic.

## Open subdecisions

- Final charge current setting (Rprog value). Match to LiFePO4 cell capacity / 2 (typical C/2 charge rate). For 1500 mAh 18650 → 750 mA Iset. Verify cell datasheet.
- Whether to add a panel-side input voltage clamp (TVS or zener) to protect CN3058 from open-circuit panel voltage spikes. Probably yes, cheap insurance.
