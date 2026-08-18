# ADR 0046: Charge-knee power thresholds

- Status: Accepted (VBAT_LOWV bench verification pending)
- Date: 2026-08-18
- Decider: Ben Eckart
- Amends: ADR 0023 (threshold values only; the ladder structure, hysteresis,
  load compensation, and coulomb-primary intent all stand)
- Relates: ADR 0033 (2 A ICHG ceiling), ADR 0042 (low-VBAT recovery lane)

## Context

The August bring-up showed that the expensive failure regime is not the cell
damage floor -- it is the BQ25628E's precharge regime. Below the charger's
VBAT_LOWV precharge-to-fast-charge boundary the charger sources at most
IPRECHG (310 mA hardware maximum; 30 mA at power-on reset before our register
write), regardless of available panel power. A P105 offers roughly 3.5 W of
charger input; precharge uses under 1 W of it.

Measured cost of a below-the-knee morning: the 2.95-3.00 V band holds about
234 mAh on the qualified 6 Ah 32700 (5,373 vs 5,139 mAh usable), roughly
585 mAh scaled to the 15 Ah 33140. Climbing that band takes ~19 hours at the
30 mA POR default (a full solar day lost -- the observed death spiral), ~2
hours at our 300 mA setting, and ~30 minutes in fast charge. The stranded
energy is ~4% of the pack; the time to re-earn it in precharge is the whole
morning.

The ADR 0023 ladder (dim 3.00 / off 2.95 / protect 2.90 V load-compensated)
parks a fixture with a resting voltage that straddles the knee, so a fixture
that rides the ladder down overnight can start the morning in precharge.

VBAT_LOWV is charger-owned (deliberately never written by firmware) and is
believed to sit very close to 3.00 V in LFP configuration, from dashboard
observation of charge-current steps. It has not been bench-measured.

## Decision

1. Raise the default ladder one notch above the charger knee, keeping the
   50 mV spacing and all confirm/hysteresis semantics:
   dim 3.15 V / LEDs-off 3.10 V / protect 3.05 V (load-compensated), PROTECT
   release floor 3.25 V. LED turn-on ramp guards move in lockstep
   (park 3.05 V, dim-clamp 3.10 V).
2. The goal invariant: a fixture that parks on the ladder must rest above
   VBAT_LOWV at dawn, so the morning always starts in fast charge. The 300 mA
   precharge (ADR 0042 tooling) becomes a safety net, not the daily path.
3. Approximate SoC at the new trips (LFP OCV): dim ~10-12%, off ~8%, protect
   ~5% -- materially closer to ADR 0023's intended coulomb tiers (15/7/5%)
   than the old voltage ladder was.
4. Per-unit NVS overrides (`dim_mv`, `off_mv`, `slp_mv`) remain the tuning
   path: known-shaded positions may trade runtime margin either direction
   without a reflash.
5. REVISIT once VBAT_LOWV is bench-measured (see TODO): if the true knee is
   materially below 3.00 V, the ladder may be relaxed downward toward it to
   reclaim runtime; if above, protect must rise to stay clear of it.

## Consequences

- Roughly 4-7% of usable pack energy is intentionally left in reserve below
  the protect floor. The reference show budget (~4.1 Wh against ~9 Wh median
  usable) absorbs this without a visible change.
- A parked fixture rests comfortably above the 2.5 V family of failures
  (charge refusal, gauge cold-POR muteness) for on the order of two sunless
  weeks at the measured sub-mA sleep floor.
- The dashboard battery bands mirror the new ladder (healthy 3.25+ / watch
  3.15+ / low 3.10+ / critical below).
- Native tests shift all sample voltages +150 mV; every relative assertion
  (confirm windows, IR-sag anti-oscillation, compensation crossings, compound
  release) is preserved unchanged.
