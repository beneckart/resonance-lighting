# 0026 -- Solar panel selection: Voltaic ETFE P105 / P126, assigned by fixture role

**Date:** 2026-07-08 (records the 2026-06-24 purchase and the 2026-06-29 outdoor
measurements)
**Status:** Accepted for the solar-equipped classes. One sub-decision remains OPEN:
whether uplights/chandelier go solar-free (shared with ADR 0025).
**Owners:** Ben

## Context

The panel axis started as "square 1-5 W panels for R&D, round panels aesthetically
preferred for production" (2026-05 surveys). Bench work then produced hard
constraints and a qualifier harness:

- **Charger window (BQ25628E, buck-only):** panel hot-loaded Vmp must be >= 4.6 V
  (the SDK's VINDPM floor -- lower setpoints are silently rejected), and Voc must
  qualify at the input. Bright sun can latch input qualification off until firmware
  kicks it (`firmware/powerfeather_solar_guard.h` forces wide VBUS_OVP and toggles
  EN_HIZ; ADR 0021 era finding, guard shipped 2026-06-29).
- **The 10-minute MPP sweep harness is the panel qualifier.** It rejected the "4 W"
  ring-camera economy candidate with numbers (~1.0-1.1 W real in full sun = ~4x
  overrated, bezel self-shading, diode tax) and showed Voltaic P139-class
  (Voc 2.76 V) is boost-ecosystem, unusable on this buck charger.

Voltaic ETFE panels were bought 2026-06-24 (110x P105-class 5 W + 50x P126-class
2 W, $3,521.99, plus 160x 3.5x11 mm DC pigtails, $364.79) and measured outdoors
2026-06-29 into a hungry LFP with panel-side INA truth:

- **P105 5 W:** best region ~m46-m48 VINDPM; ~5.1-5.3 V at 0.73 A = **~3.8-3.9 W
  panel-side** (charger input ~3.47 W, battery-side ~3.1-3.2 W). Possibly still
  battery-acceptance-limited; a hungrier-cell re-run is queued.
- **P126 2 W:** best region ~m58; ~6.1 V at 0.31 A = **~1.89 W panel-side**
  (charger input ~1.66-1.68 W) -- proportionally at rating in real conditions.
- MPP setpoint matters materially for both (~1-2 Wh/day over a 5-sun-hour
  heuristic); a simple software MPPT/hill-climber is worth measuring (policy still
  open, `--field-mppt` bench firmware exists).

## Options considered

- **Voltaic ETFE P105/P126:** measured near rating, mounting holes, ETFE for dust/UV,
  vendor supplies matching pigtails. Chosen.
- **Round panels:** aesthetically appealing, sourcing too slow for 2026. Dropped.
- **Economy "4 W" camera panels:** rejected by measurement (above).
- **No panel (solar-free + big cell):** OPEN for uplights/chandelier only -- see
  ADR 0025's 20 Ah question.

## Decision

Production panels are **Voltaic ETFE: P105-class 5 W and P126-class 2 W**, assigned
by fixture role (tentative until installation, like all fleet counts -- ADR 0024):

- **P105 5 W -> hanging downlights** (4 W RGBW role; storm-recovery margin).
- **P126 2 W -> perimeter HEX fixtures** (lower show power; mechanically elegant).
- **Uplights/chandelier:** OPEN -- off-light 5 W panel on the tree vs solar-free
  (20 Ah or budgeted 6 Ah). The 110/50 split bought keeps all options alive.
- Panel connection is the pre-made 3.5x11 mm pigtail into the PowerFeather VDC
  input, with strain relief -- no bare panel-wire soldering at fleet scale.

Purchased quantities (160 panels) vs the solar-equipped fleet (~110-112 fixtures if
uplights/chandelier go solar-free) leave healthy spares; if uplights take panels the
margin thins to ~8. See `ops/bom.md` spares math.

## Consequences

- Any panel change re-runs the MPP sweep qualifier; the ADR 0021 bright-sun latch
  guard must be active for any panel with Voc near or above ~6 V.
- MPPT policy (fixed 4.6 V vs periodic hill-climb) is still an open firmware
  decision; the P105's measured optimum (~m46-m48) says the default setpoint is
  close but not free.
- Hat/panel mechanical integration (pocket, backup retention, pigtail strain
  relief) is on Steve's enclosure track (ROADMAP Phase 2).
- A hungry-cell P105 re-run and a dawn-to-dusk harvest log (effective solar hours
  for in-tree shading) remain the open measurement items feeding the per-role
  energy budget.

## References

- `docs/tests/VOLTAIC_ETFE_PANEL_TEST_PREP_2026-06-15.md` (+ 2026-06-29 follow-up)
- LOG 2026-06-29 (Voltaic outdoor MPP), 2026-06-12 (panel-shopping spec),
  2026-06-11 (camera-panel rejection, first wireless MPP sweep)
- `firmware/powerfeather_solar_guard.h`; ADR 0021, 0024, 0025
- `ops/PROCUREMENT.md` (order line), `ops/bom.md` (per-class allocation)
