# 0023 - LFP power-policy thresholds (LED dim / LED off / sleep) from measured discharge

**Date:** 2026-07-07
**Status:** Proposed (standard tier recommended as the production default; adopt on
first production firmware that implements the low-battery state machine). Derived
floors supersede the ad-hoc bench floors in net_bench drawdown (3.18/3.05 V) and
field-cycle (3.10/3.00 V), which were set conservatively before capacity data existed.
**Owners:** Ben + Claude

## Context

Production lanterns need three low-battery behaviors: dim the LEDs, turn them off (while
keeping OTA reachability), and sleep (preserving enough charge to survive the night and
be rescued by solar at sunrise). Until now every voltage floor in the repo was a guess
made without a voltage-vs-remaining-capacity map for the production cell.

The 2026-07-06 32700 shootout (`docs/tests/BATTERY_32700_SHOOTOUT_PLAN_2026-07.md`,
report `BATTERY_32700_SHOOTOUT_REPORT_2026-07-06.html`) produced that map for the
production cell (fullbattery 6 Ah, now qualified at n=2: 5,752 / 5,726 mAh) under the
heaviest realistic lantern load — HEX37 white val224, ~860 mA battery-side, 79.9 °F.
Data: `ops/bench/data/ca/2026-07-06-discharge-F-cycle1.jsonl`.

## Measured facts (production cell, full HEX load, 79.9 °F)

1. **LED quality holds to the bitter end.** Full, flicker-free brightness (609–615 mA
   LED-side) down to 2.70 V under load. First instability (brownout reset + LED current
   collapse) at **2.69 V, with 99 % of usable capacity already delivered**.
2. **The 2.90 V firmware protect has ~200 mV / ~250 mAh of real margin** below it.
3. **The LFP plateau makes voltage thresholds load- and position-sensitive.** Remaining
   capacity when bv (under this load) first crosses:

   | bv under load | mAh remaining to 2.5 V | % of usable | minutes left at full load |
   |---|---|---|---|
   | 3.10 V | 3,490 | 61 % | 246 |
   | 3.05 V | 1,445 | 25 % | 102 |
   | 3.00 V | 765 | 13 % | 54 |
   | 2.95 V | 379 | 6.6 % | 27 |
   | 2.90 V | 308 | 5.4 % | 22 |
   | 2.70 V (first instability) | ~137 | 2.4 % | 10 |

4. **The overnight-rescue reserve is tiny.** Deep sleep with both 3V3 rails cut is
   sub-mA (<12 mAh per 12 h night); an hourly 60 s WiFi/OTA wake window adds ~34 mAh.
   Budget **~50 mAh/night with margin**. Solar recharge via the BQ25628E needs no
   firmware cooperation. The sleep threshold therefore exists to guard against *bugs*
   (a stuck-awake board — see the 46 h soak death), not to meet an energy budget.

## Decision — three threshold tiers

Voltages are **under full HEX load (~860 mA)**; see load compensation below.

| | Conservative | **Standard (default)** | Aggressive |
|---|---|---|---|
| Dim LEDs (~50 %) | 3.05 V (25 % left) | **3.00 V** (13 % left) | 2.95 V (6.6 %) |
| LEDs off, duty-cycled OTA windows | 3.00 V (765 mAh) | **2.95 V** (379 mAh) | 2.90 V (308 mAh) |
| Deep sleep, sparse wake only | 2.95 V | **2.90 V** | ride the 2.90 protect |
| Capacity unused vs aggressive | ~460 mAh (8 %) | **~70 mAh (1.2 %)** | — |
| Margin to first instability | ~310 mV | **~260 mV** | ~210 mV |

- **Standard** is the production default: gives up ~1 % of usable capacity, keeps ~7×
  the night reserve at LED-off, 260 mV of instability margin.
- **Conservative** for unattended winter deployments, first fleet bring-up, or aged
  cells. Note it is still *less* conservative than the old field-cycle floors.
- **Aggressive** buys ~70 mAh (≈1 h of dim light) for meaningfully less robustness to
  cold, cell variance (n=2), and aging. Use only with a measured reason.
- The 2.90 V hardware protect stays as-is in all tiers.

## Implementation requirements (these matter more than ±25 mV on the values)

1. **Hysteresis is mandatory.** Dimming sheds ~100+ mV of IR sag, so bv *recovers*
   after every transition and naive logic oscillates. Latch transitions, require a 60 s
   confirmation (field-cycle already does this), re-brighten only ≥ +150 mV or on a
   coulomb condition.
2. **Voltage thresholds are load-relative.** Effective source path ≈ 150–170 mΩ
   (cell IR + leads + polarization): compensate as `bv_comp = bv + 0.15 × I_load(A)`,
   or evaluate thresholds only at a known load state.
3. **Prefer coulomb-primary, voltage-backstop.** Capacity and gauge bias are now known:
   set gauge DesignCap ≈ 5,750 mAh and correct MAX17260 current by **/1.08** (universal
   chip trait, replicated ×8 across boards/cells/directions). Trigger dim/off/sleep at
   15 % / 7 % / 5 % coulomb-remaining with the voltage tiers as the backstop — on the
   LFP plateau coulombs are the honest signal; voltage only becomes informative right
   where these thresholds sit.
4. **LED-off must verifiably cut the load, and sleep must be watchdogged.** The 46 h
   soak board died precisely because it never slept. Gauge SOC % is advisory only
   (plateau-blind; swept 98→0 % in one run).
5. **MAX17260 gotcha:** it won't cold-POR from a deeply discharged cell (~2.8 V) —
   telemetry looks like "no cell attached" until charging lifts it. Self-recovers; see
   `firmware/POWERFEATHER_NOTES.md`.

## For future bench tests (how to re-derive these numbers)

When the cell, temperature range, or load class changes, reproduce the map with the
existing pipeline — one evening, unattended:

1. `afk_discharge.py` full run at the representative load (see the shootout plan doc
   for the two-rig / `ina_logger.py --ina-file` setup and `ina_mapcheck.py` pre-flight).
2. Extract the crossings table (bv threshold → mAh remaining, first reset, LED-dim
   onset) from the JSONL — the extraction one-liner is in LOG 2026-07-06/07.
3. Re-fit the tiers: keep dim ≥ ~250 mV above first instability, LED-off reserve ≥ 6×
   the measured night budget, and re-check the load-compensation slope.

Known gaps in the current numbers: single temperature (79.9 °F — cold raises IR and
pulls every threshold up; the conservative tier is the cold-weather hedge until a
cold-night run exists), n=2 cells, one rig's lead resistance. Boundaries are good to
±50 mV, not ±5 mV.

## Consequences

- net_bench drawdown / field-cycle firmware floors can be relaxed to the standard tier
  when next touched (they currently strand 25–60 % of capacity under heavy load).
- SYSTEM.md autonomy math should use **5,139 mAh usable above the 3.0 V floor**
  (conservative) or **5,373 mAh above 2.95 V** (standard), not the 5,752 mAh lab number.
- The Palowextra "7.2 Ah" comparison cell is rejected (ADR 0017 sourcing unchanged);
  its 2.3× IR pulling the knee 1.4 h earlier is the cautionary tale for why knee
  position — not label capacity — drives these thresholds.

## References

- `docs/tests/BATTERY_32700_SHOOTOUT_PLAN_2026-07.md` (protocol + results)
- `docs/tests/BATTERY_32700_SHOOTOUT_REPORT_2026-07-06.html` (report with charts)
- `ops/bench/data/ca/2026-07-06-discharge-{F,P}-cycle1.jsonl`, `2026-07-06-ina.log.gz`
- LOG 2026-06-11 (cont. 3) — first F sample, 5,726 mAh; LOG 2026-07-06/07 — shootout
- Commits: 3668554 (tooling), 3932434 (data), f0c92ca (report)
