# Ops

Logistics, vendors, BOM management, procurement. Non-engineering operational work.

## Contents

- `bom.md` -- fleet bill of materials: shared core + per-class tables, fleet totals
  vs bought, spares math, open BOM inputs. Counts mirror the canonical fleet table
  in `docs/block-diagram/SYSTEM.md`.
- `PROCUREMENT.md` -- orders ledger (dates, costs, statuses), small/sample orders,
  to-buy queue, lead-time risks backward from Aug 20, procurement timeline, vendor
  directory. Absorbs the previously planned `vendors.md` / `shipping.md` /
  `timeline.md`.
- `bench/` -- bench tooling (Python loggers, sweep/analysis scripts, dashboards) and
  measurement data under `bench/data/` (site-partitioned JSONL; see
  `bench/data/README.md`).

Container/bamboo shipping (Bali -> US) is Elliot's track with Michelle Satkin /
Mainfreight -- coordinate through Ben, not from this repo.
