# Zero-leaf-overlap EXTREME TEST - tuning metrics (75 gobos)

`MAX_LEAF_OVERLAP_RATIO` forced to **1.0** (down from the normal 1.5) and the pruning floor (`min_leaves_after_prune`) dropped from 3 to **1**, so pruning is allowed to remove leaves all the way down to one per branch if that's what true zero-overlap requires. This is a deliberate extreme, not a replacement for the main batch.

Spanning-branch gobos: 19/75.

| Metric | Min | Max | Avg | Median |
|---|---|---|---|---|
| Leaf : stem area ratio | 1.89 | 5.52 | 3.58 | 3.49 |
| Leaf overlap ratio (target: exactly 1.0) | 1.0000 | 1.0000 | 1.0000 | 1.0000 |
| Leaf overlap fraction | 0.00% | 0.00% | 0.00% | 0.00% |
| Stem crossing count | 0 | 2 | 0 | 0 |
| Whitespace ratio | 0.526 | 0.751 | 0.645 | 0.645 |
| Black fill ratio | 0.249 | 0.474 | 0.355 | 0.355 |
| Leaf coverage ratio | 0.063 | 0.237 | 0.132 | 0.124 |
| Avg nearest-neighbor leaf spacing | 5.27 mm | 18.46 mm | 7.94 mm | 7.54 mm |
| Avg individual leaf area | 22.89 mm2 | 45.18 mm2 | 31.61 mm2 | 31.69 mm2 |
| Leaves per gobo (post zero-overlap prune) | 3 | 14 | 8.2 | 8 |

## What changed vs. the main (1.5-cap) batch

- Leaf overlap ratio is pinned to (numerically) exactly 1.0 for every gobo - no two leaves share any area at all.
- This required removing far more leaves than the normal 10%-thin + 1.5-cap pipeline - see `n_leaves_kept` per gobo vs. the main batch's `manifest.json`.
- Leaf:stem ratio and whitespace ratio both shift as a direct consequence of fewer leaves - see the printed run summary / table above for exact numbers on this batch.
