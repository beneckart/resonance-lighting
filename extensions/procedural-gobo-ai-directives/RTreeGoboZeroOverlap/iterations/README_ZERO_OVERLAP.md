# Zero Leaf Overlap — Extreme Test Batch

This is a **deliberate extreme**, not a replacement for the main 75-gobo
set. Same generator, same seeds, same everything else — the only change
is `MAX_LEAF_OVERLAP_RATIO` forced from **1.5 down to 1.0** (true zero
overlap — no two leaves in a gobo may share any area at all), with the
pruning floor also dropped from 3 leaves to 1, so the algorithm is
allowed to remove leaves all the way down to a single leaf per branch if
that's what's needed to hit exactly zero.

## Result

**Every one of the 75 gobos hits exactly 1.0** (verified — see
`manifest.json`, `metrics.leaf_overlap_ratio`). All 75 are still single
connected regions and all pass geometry validity — the overlap change
doesn't touch the union/connectivity logic, only how many leaves survive
pruning before that union happens.

| Metric | Main batch (cap 1.5) | This extreme (cap 1.0) |
|---|---|---|
| Leaf overlap ratio | avg 1.29 | **exactly 1.00, all 75** |
| Leaves per gobo | avg 16.7 | avg 8.2 (range 3–14) |
| Leaf : stem area ratio | avg 5.81 | avg 3.58 (range 1.89–5.52) |
| Whitespace ratio | avg 0.582 | avg 0.645 |

As expected, forcing zero overlap costs roughly half the leaves per
gobo (fewer leaves means fewer chances to overlap), which pulls the
leaf:stem ratio and whitespace ratio in the same direction the last few
density-reduction passes did — just far more aggressively, since this
is a hard constraint rather than a percentage-based thin.

## What's in this folder

- `individual_svgs_zero_overlap.zip` — 75 individual SVGs
- `master_sheet_ZERO_OVERLAP.svg` / `preview_ZERO_OVERLAP.png` — unlabeled grid
- `preview_labeled_ZERO_OVERLAP.svg` / `.png` — labeled grid (gobo id + spanning marker)
- `manifest.json` — per-gobo stats for this batch specifically
- `tuning_metrics_ZERO_OVERLAP.md` — aggregate table for this batch

## How this was made
`run_zero_overlap.py` (new script, doesn't modify `generate_gobos.py`'s
defaults) calls `build_gobo(seed, spanning=..., max_leaf_overlap_ratio=1.0,
min_leaves_after_prune=1)` — both now optional overrides on `build_gobo`,
so the main pipeline's default behavior (1.5 cap, main `run.py`) is
completely unaffected by this experiment. If you want anything between
these two extremes (e.g. 1.2), the same override mechanism works — just
change the two numbers passed in.
