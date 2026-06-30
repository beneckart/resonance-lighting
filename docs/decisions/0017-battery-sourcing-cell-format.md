# 0017 -- Prefer one larger LiFePO4 cell over multi-14430 packs

**Date:** 2026-05-10
**Status:** Accepted
**Owners:** Ben

## Context

LiFePO4 remains the preferred production chemistry because of heat tolerance, safety, cycle life, and storage behavior. However, sourcing revealed a practical issue: low-cost LiFePO4 cells on Amazon are often 14430 format at roughly 400-450 mAh, while 18650 LiFePO4 cells around 1500-2000 mAh are less common and may require specialty suppliers such as BatterySpace.

One possible workaround is building a larger pack from several 14430 cells in parallel.

## Decision

Prefer **one larger LiFePO4 cell per fixture**, ideally 18650 in the 1500-2000 mAh range.

Do not build production packs from many 14430 cells in parallel unless mechanical constraints force it and the pack is designed/assembled/QA'd deliberately.

## Consequences

- 18650 LiFePO4 remains the default cell format for production if supply is reliable.
- 26650 LiFePO4 is a reasonable fallback if sourcing or autonomy demands it and the hat geometry allows the larger cell.
- 14430 cells are acceptable for small bench tests or emergency fallback, but not preferred for production packs.
- A multi-cell 14430 pack adds contacts, cell matching concerns, holder/wiring complexity, QA steps, and failure points. This conflicts with the project's no-grunt-work production constraint.
- The custom/COTS power system should be tested with the actual cell format before hat geometry is frozen.

## Test requirements

- Buy/sample actual 18650 LiFePO4 cells from the intended supplier.
- Confirm delivered cells match ordered chemistry and capacity.
- Run capacity spot checks on several cells.
- Measure charge behavior with PowerFeather V2 or selected charger.
- Measure thermal behavior inside mock hat.
- Confirm physical retention and serviceability.
