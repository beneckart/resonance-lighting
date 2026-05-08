# 0002 — LiFePO4 chemistry for battery

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

Battery for solar-powered outdoor fixture deployed at Burning Man (Black Rock Desert). Conditions:

- **Temperature:** 40 °C+ daytime, sub-10 °C overnight, repeated daily cycle for 1+ week deployment.
- **UV / dust:** continuous outdoor exposure inside a sealed enclosure, repeated transport.
- **Lifetime expectation:** 2-year reuse (2026 + 2027 builds), with months of inert storage between.
- **Capacity needed:** ~170 mAh / night drain (see `docs/block-diagram/SYSTEM.md`).
- **Weight:** under 1 kg per fixture total (structural budget).

## Options considered

- **LiPo / Li-ion (4.2 V max charge):** standard for most ESP32 dev boards. Higher energy density. **Concern:** thermal aging is real — repeated 40 °C heat cycles accelerate capacity loss. Long inert storage degrades. Bench experience was fine but desert conditions are stress-case.
- **LiFePO4 (3.6 V max charge):** ~half the energy density of LiPo per unit weight. Substantially better thermal tolerance, ~5× cycle life, much lower fire risk if punctured, low self-discharge for inert storage.
- **NiMH:** rugged, but charge management is more involved and energy density is poor.

## Decision

**LiFePO4.** Specifically targeting **18650 LiFePO4 cells (~1500 mAh nominal).** Trade-offs:

- Loses ~half the per-gram energy density vs LiPo. With 170 mAh / night drain and 1500 mAh capacity, this is irrelevant — we have multi-night autonomy regardless.
- Thermal tolerance fits the deployment.
- 5× cycle life means 2026 cells are still strong for 2027 reuse.
- Low self-discharge means storage between burns doesn't kill the cells.

## Consequences

- **Charger IC must be LiFePO4-tuned.** TP4056, bq24074, CN3791 are LiPo-only and overcharge LiFePO4. Use **CN3058** (JLCPCB Basic) or **MCP73123** (extended). See ADR 0003.
- 3.6 V max charge voltage is closer to the 3.3 V ESP32 rail than LiPo's 4.2 V. Designs around regulator dropout and the WS2812B level threshold need to account for this. AP2112K-3.3's 450 mV dropout works cleanly down to ~3.55 V battery; below that, 3.3 V rail droops with battery, but ESP32-C3 runs fine to ~3.0 V. **Practical battery cutoff: ~3.0 V** (matches LiFePO4 longevity recommendation).
- The "WS2812B direct from battery" trick still works: 3.3 V data ≥ 0.7 × 3.6 V = 2.52 V. Margin 780 mV. No level shifter needed. Verified in 2018 Talisman v2 build (same math, slightly different battery max).
- Per-cell weight is ~50 g for an 18650. Far inside the 1 kg / fixture structural budget.
