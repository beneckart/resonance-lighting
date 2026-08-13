# Tuning metrics - current batch (75 gobos)

Aggregate stats across all 75 designs. Use this to steer future passes (e.g. "push whitespace ratio up", "reduce leaf overlap fraction").

Spanning-branch gobos (touch the ring at two points): 19/75 (25.3%).

| Metric | Min | Max | Avg | Median |
|---|---|---|---|---|
| Leaf : stem area ratio | 2.08 | 5.23 | 3.67 | 3.69 |
| Leaf overlap ratio (sum/union, 1.0=no overlap, capped at 1.0) | 1.00 | 1.00 | 1.00 | 1.00 |
| Leaf overlap fraction (% of leaf area lost to overlap) | 0.0% | 0.0% | 0.0% | -0.0% |
| Stem crossing count (proper intersections between branches) | 0 | 4 | 1 | 0 |
| Whitespace ratio (white / full disk) | 0.557 | 0.739 | 0.659 | 0.654 |
| Black fill ratio (black / full disk) | 0.261 | 0.443 | 0.341 | 0.346 |
| Leaf coverage ratio (leaf union / disk) | 0.120 | 0.282 | 0.185 | 0.180 |
| Stem coverage ratio (stem union / disk) | 0.027 | 0.084 | 0.052 | 0.050 |
| Ring coverage of disk | 16.3% | 16.3% | 16.3% | 16.3% |
| Avg nearest-neighbor leaf spacing | 5.34 mm | 10.28 mm | 7.25 mm | 6.91 mm |
| Avg individual leaf area | 23.96 mm2 | 39.03 mm2 | 30.91 mm2 | 30.72 mm2 |
| Branches per gobo (count) | 3 | 10 | 5.9 | 6 |
| Leaves per gobo (count, post-thinning + overlap prune) | 8 | 15 | 10.4 | 10 |
| Stem width | 1.00 mm | 1.00 mm | 1.00 mm | 1.00 mm |
| SVG path anchor/vertex count | 149 | 1035 | 320.3 | 318 |

## Definitions

- **Leaf overlap ratio**: sum of each individual leaf's own area, divided by the area of all leaves unioned together. 1.0 means no two leaves overlap at all; higher means leaves are stacking on top of each other (reduces individual leaf visibility). This pass actively prunes the most-overlapping leaves until this hits 1.0 or below, rather than just reporting whatever came out.
- **Leaf overlap fraction**: same idea, expressed as % of raw leaf area that disappears into overlap once unioned.
- **Whitespace ratio**: fraction of the full gobo disk that ends up white (open) in the final cut/print, vs black.
- **Leaf coverage ratio / stem coverage ratio**: fraction of the full disk area occupied by leaves alone / stems alone (pre-union with the ring), useful for tuning density independent of the overlap metric.
- **Avg nearest-neighbor leaf spacing**: average distance (mm) from each leaf's centroid to its closest neighboring leaf's centroid - a rough proxy for how visually separated individual leaves are. Larger = more spread out.
