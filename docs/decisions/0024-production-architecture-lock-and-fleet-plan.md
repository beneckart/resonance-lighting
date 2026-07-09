# 0024 -- Production architecture lock: COTS PowerFeather V2 fleet, four fixture classes

**Date:** 2026-07-08 (records decisions made 2026-06-11 through 2026-07-08)
**Status:** Accepted. Resolves the COTS-vs-custom production decision left open by
ADR 0012. Fleet counts are tentative until installation; the living counts table is
`docs/block-diagram/SYSTEM.md` (this ADR records the decision snapshot).
**Owners:** Ben

## Context

ADR 0012 set up a dual-track production plan (COTS fallback + custom PCBA) with a hard
go/no-go date driven by lead times. ADR 0021 then validated PowerFeather V2 end-to-end
on hardware: ESP-NOW at fleet scale, battery-only no-touch OTA with A/B rollback, and
the solar charge path. Since then the remaining production gates closed in practice:

- battery vendor and format qualified (ADR 0025);
- solar panels selected and outdoor-measured (ADR 0026);
- LED electrical drive measured per role (ADR 0029);
- the June/July reboot epidemic root-caused to a bus-integrity rule, not a board
  defect (ADR 0028) -- removing the last open reliability question against the COTS
  board.

Procurement then happened on the COTS path without a custom board ever being designed:

- 2026-06-11: 68 PowerFeather V2 boards ordered from Elecrow (~$25/board, $2,219.75).
- Second batch of 82 boards (~$30/board) arranged directly with Elecrow; invoice
  expected 2026-07-10, shipping same day per their rep. Total 150 production boards.

Meanwhile the fleet definition grew past the original "100 downlights". The 2026 CAD
and installation plan now spans four fixture classes (recorded from Ben, 2026-07-08).

## Options considered

- **COTS production (PowerFeather V2 + role-specific LED/panel/harness):** all
  feasibility gates measured green; supply proven at 150-board scale; zero custom
  hardware risk. Chosen.
- **Hybrid (COTS controller + custom adapter PCBA):** remains available for the LED
  adapter / harness layer if connectorization needs it (NeoHEX adapter Rev A packet
  exists), but is not required to ship 2026.
- **Custom PowerFeather-derived PCBA:** no longer schedulable for 2026 (fab + assembly
  + validation lead time vs the ~Aug 20 fixtures-in-hand deadline). Deferred, not
  killed -- it is the 2027 optimization path.

## Decision

2026 production is **COTS PowerFeather V2 at a fleet of 150-152 fixtures** in four
classes. Counts are tentative until installation -- the design is fungible and fully
wireless, so placement is free and the split can shift on-site:

| Class | Count | LED | Power | Sensors (tentative) |
|---|---|---|---|---|
| Hanging downlight | 72 | 4 W RGBW + gobo | P105-class 5 W panel | MSA311 + TMF8820-mini (downward) |
| Perimeter (5 ft shepherd hooks) | 38-40 | SK6812 HEX | P126-class 2 W panel | VL53L5CX (outward); MSA311 likely |
| Uplight (no gobo, simple cylinder) | 24 | 4 W RGBW | OPEN: off-light 5 W panel vs solar-free 20 Ah LFP vs 6 Ah budget | none |
| Chandelier (central, 16 bamboo shafts) | 16 | HEX + RGBW mix (TBD) | likely solar-free, USB-charged | none |

- All classes share the same PowerFeather V2 internals, firmware, and day-sleep
  behavior; uplight/chandelier variants add a gasketed panel-mount USB-C charge/flash
  port if the solar-free option wins (ADRs 0025/0026 carry that open decision).
- Chandelier scope/ownership is still loose (16 hats similar to the uplight "boots";
  structure itself is built and in the shipping container). Treat the 16 shafts as the
  only locked positions; some may even stay unpopulated.
- Spares: 150 production boards + ~5 bench boards (Ben) + ~3 (Steve). Thin at a 152
  target; a further Elecrow top-up order is likely if they allow. Flagged in the
  ROADMAP risk register.

## Consequences

- `ops/PROCUREMENT.md` (orders ledger) and `ops/bom.md` (per-class BOM) become the
  procurement records; `docs/block-diagram/SYSTEM.md` holds the living fleet table
  that every other doc references instead of repeating counts.
- The ESP-NOW scale projection (ADR 0021) was computed at 100 nodes; re-check the
  extrapolation at 150 (TODO). Radio physics gives margin, but say so honestly.
- ADR 0009's no-per-unit-skilled-ops constraint now applies at 150-unit scale --
  connectorization (pre-crimped JST-XH/STEMMA) matters proportionally more.
- The custom-PCBA track (ADR 0012 Track B) restarts, if ever, from the
  PowerFeather-derived reference in `hardware/README.md`, carrying ADR 0028's
  dedicated power-management-bus rule.
- The four-class fleet obsoletes "100 downlights" phrasing across README, BACKGROUND,
  SYSTEM.md, ROADMAP, and the glossary (swept 2026-07-08).

## References

- ADR 0012 (dual track -- resolved by this ADR), 0015/0021 (PowerFeather adoption and
  validation), 0022 (mixed LED fleet), 0025-0029 (component locks).
- LOG 2026-07-07 (deployment geometry first recorded), 2026-07-08 (fleet interview).
- `ops/PROCUREMENT.md` for order dates, costs, and statuses.
