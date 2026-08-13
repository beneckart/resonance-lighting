# Bamboo-Leaf Gobo Set — 75 Designs (v10)

## What changed this pass

### 1. Minimum-content floor (gobo_27 as the reference)
`gobo_27` from the last batch had 8 leaves surviving zero-overlap
pruning — flagged as a good example of the least content a gobo should
have. That's now an enforced floor (`MIN_LEAVES_FLOOR = 8`):

- Every gobo still generates normally first. If it already ends up with
  ≥8 leaves after thinning + zero-overlap pruning, **nothing changes** —
  same seed, same layout, same output as the last batch.
- If a gobo would land under 8 leaves, it retries with progressively
  denser raw generation (more branches, more leaf-attachment stations
  per branch) feeding the *same* zero-overlap pruning step, up to 5 extra
  attempts, keeping whichever attempt produced the most leaves. This adds
  more non-overlapping candidates rather than relaxing the zero-overlap
  rule — the floor and the zero-overlap constraint both hold at once.
- **Result:** minimum leaf count across the batch went from 3 → **8**
  (exactly at the floor, not above it). 29/75 gobos needed at least one
  retry; all 75 now meet the floor. Logged per gobo in the manifest:
  `content_floor_met`, `content_floor_attempts`, `density_boost_used`.

### 2. Vertex/segment count added to the labeled preview
Each thumbnail in `preview_labeled.svg`/`.png` now shows a second line
under the gobo id: **`NNN pts`** — the SVG path anchor/vertex count for
that gobo (same number as `svg_path_commands_bezier` in the manifest,
i.e. total M/L/C command count across the whole path). Useful for
eyeballing which designs are cheapest/most complex for the mesh pipeline
without opening the manifest.

## Verified

| | |
|---|---|
| Minimum-content floor met | **75/75** (min leaf count now 8, was 3) |
| Leaf overlap ratio | exactly 1.0 for all 75 (zero-overlap constraint held through the retries) |
| Single connected region | 75/75 |
| Geometry valid | 75/75 |
| Bezier export valid | 75/75 (70 curved, 5 straight-line fallback) |
| Ring | 47.0mm / 2.0mm, unchanged |
| Spanning branches | 19/75 (25.3%), unchanged |

## Side effect on density metrics
Bumping the sparse tail up naturally raised the batch averages a little:

| Metric | Previous batch | This batch |
|---|---|---|
| Leaves per gobo | avg 8.0 (range 3–14) | avg 10.4 (range **8**–15) |
| Leaf : stem area ratio | avg 3.60 | avg 3.67 |

Nothing else moved — the retry mechanism only adds leaves for gobos that
needed it, it doesn't touch overlap, ring size, spanning-branch rate, or
any other parameter.

## What's in this package

- `individual_svgs/gobo_01.svg` … `gobo_75.svg`
- `master_sheet_75_gobos.svg` — clean grid, for re-slicing
- `preview_labeled.svg` / `preview_labeled.png` — **now with vertex
  counts** under each id
- `manifest.json` — includes the new `content_floor_met`,
  `content_floor_attempts`, `density_boost_used` fields, plus the
  `min_leaves_content_floor: 8` spec entry
- `tuning_metrics.md` — now includes an "SVG path anchor/vertex count"
  row in the aggregate table
- `preview.png` — unlabeled flattened raster
- `generate_gobos.py` / `run.py` — updated source (`MIN_LEAVES_FLOOR = 8`
  and the retry loop live in `build_gobo`; `build_layout` gained a
  `density_boost` parameter)

## Physical spec (unchanged from last pass)

| | |
|---|---|
| Ring outer diameter | 47 mm (exact) |
| Ring band thickness | 2 mm (exact) |
| Minimum line width | 1 mm (hard floor) |
| Max leaf overlap ratio | 1.0 (zero overlap, default) |
| Minimum leaf count | **8** (new) |
| Spanning branches | 19/75 (25.3%) |

## Design rules every gobo still satisfies
1. Solid outer ring, 47mm / 2mm, exact.
2. No floating elements — verified pre- and post-curve-fit.
3. 1mm minimum line width everywhere.
4. Leaf overlap = exactly 1.0 (zero).
5. **≥8 leaves per gobo (new).**
6. `fill-rule="evenodd"`, geometry validity checked for all 75.

---

## Regenerating or adjusting the set
Same as always — seed-based, `generate_gobos.py` + `run.py`. If 8 feels
like the wrong floor after seeing this batch, it's one constant
(`MIN_LEAVES_FLOOR`) to change and rerun.
