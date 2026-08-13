# plan.md — Bamboo-Leaf Gobo Generator

**Audience:** an AI coding agent picking this up cold, working with a
human who wants to keep iterating on this asset set. This document
explains what the system does, why it's built the way it is, every
tunable parameter and its current value, the invariants that must never
break, and how to run/extend it safely.

**Companion files you should have alongside this doc:**
- `generate_gobos.py` — the geometry engine (single-gobo generation)
- `run.py` — batch orchestration (75 gobos, manifest, previews, tuning table)
- `requirements.txt` — `shapely`, `numpy` (+ optional `cairosvg`/`Pillow` for PNG previews)

Read those two source files alongside this doc — this document explains
*why* they're structured the way they are and *what invariants their
code enforces*, but the files themselves are the ground truth for exact
current behavior.

---

## 1. What this produces

75 circular gobo designs (stage-lighting style cutout patterns): a solid
ring frame with a procedurally-generated bamboo branch-and-leaf silhouette
growing inward from the ring. Output is SVG, in real millimetre units,
intended to be imported into Blender, converted to mesh, and extruded
~3mm thick for 3D printing.

Each gobo is entirely algorithmic — there is no hand-drawn art anywhere
in the pipeline. Every design is `build_gobo(seed, spanning=bool)` called
with a different seed (1000, 1001, 1002, … for gobo_01…gobo_75).
Re-running with the same seed reproduces the exact same design
(deterministic — `random.Random(seed)` is seeded explicitly, no global
RNG state is touched).

## 2. Why procedural + boolean union (not hand-drawn, not simple layered shapes)

Three physical/print constraints drove this:
1. **No floating elements.** A gobo (whether laser-cut, etched, or
   3D-printed) is only physically valid if every black region is
   connected to the outer ring — an unconnected "island" literally falls
   out of a real cut gobo, and in a 3D-printed version it's a separate
   disconnected mesh shell floating inside the model.
2. **Manifold mesh readiness.** Blender needs a single, non-self-
   intersecting 2D boundary to cleanly extrude into a solid. Overlapping-
   but-not-unioned shapes (e.g. a leaf drawn on top of a stem with two
   separate outlines) create duplicate/crossing edges that produce
   non-manifold geometry after extrusion.
3. **Provable, not eyeballed.** "Looks connected" isn't good enough —
   the pipeline actually computes the boolean union (via Shapely) of the
   ring + every stem + every leaf into one polygon, and asserts that the
   result is a single `Polygon` (not a `MultiPolygon`) before writing the
   SVG. This is checked per gobo, every run, not just visually reviewed.

## 3. Coordinate system and physical scale

Geometry is authored in an abstract "unit space" (a 220×220 canvas, ring
outer radius 100, ring inner radius 88), then scaled to millimetres only
at SVG-export time via one constant:

```python
SCALE_MM_PER_UNIT = 0.25   # 1 unit = 0.25 mm
```

This gives:
- Canvas: 220 units → **55mm**
- Ring outer diameter: 200 units → **50mm** (exact)
- Ring band thickness: 12 units → **3mm** (exact)

All physical minimums (e.g. the 1mm line-width floor) are converted to
unit-space by dividing by this same constant (`MIN_LINE_UNITS = 1.0 /
0.25 = 4.0 units`) so the *authoring* math never has to think in mm — only
`SCALE_MM_PER_UNIT`, the two `CANVAS_MM`/`RING_*_MM` derived constants,
and the final SVG-writing step do.

**If you need a different physical size:** change `SCALE_MM_PER_UNIT` (or
`R_OUT`/`R_IN` directly) — nothing else in the geometry logic needs to
change, since it's all unit-space until export.

SVGs are written with explicit millimetre units
(`width="55.000mm" height="55.000mm"`, matching `viewBox`), so a
standards-compliant importer (Blender included) should read the correct
real-world size without manual rescaling on import. **Always verify** in
Blender's N-panel after import, since importer behavior can vary by version.

## 4. Pipeline, step by step

`build_gobo(seed, spanning=False)` in `generate_gobos.py` is the entry
point for one gobo. Steps, in order:

### 4.1 Layout generation — `build_layout(rng, spanning)`
Pure geometry, **independent of final stroke widths** (this separation
matters — see §4.5). Produces:
- `branches`: list of `{points: [(x,y),...], width_scale: float}`
- `leaves`: list of `{poly: Polygon}` (already placed/rotated/sized)
- `grain_angle`: the gobo's shared direction bias (see §4.3)

Steps inside:
1. Pick `n_branches` (currently `choice([2,2,3,3,4])`) and one random
   `grain_angle` (0–360°) for the whole gobo.
2. **If `spanning=True`:** add one extra branch whose *both* endpoints
   sit on the ring's inner edge (§4.4), before the regular branches.
3. For each regular branch: pick a ring-contact angle `a0` (rejection-
   sampled against `used_angles` to keep branches at least 16° apart at
   their ring contact point), compute a heading biased toward the grain
   direction (§4.3), walk a gently-curving polyline inward
   (`smooth_branch`), optionally spawn 0–2 sub-branches off it.
4. For every branch/sub-branch, call `_add_leaf_stations` to place leaves
   along it at intervals (§4.2).

### 4.2 Leaf placement — `_add_leaf_stations`
Walks along a branch's centerline at `n_stations` evenly-spaced points
(3–5 for regular branches, 7–11 for spanning branches, since those are
longer). At each station, places 1–2 leaves (mostly 1), each:
- Rotated to the branch's local tangent + a random spread angle (35–80°,
  alternating sides)
- Positioned with a small perpendicular jitter (±3 units) so leaves at
  neighboring stations don't all radiate from mechanically identical points
- Nudged **back** along the branch by 1.8 units so the leaf's base
  polygon physically overlaps into the stem's buffered polygon (this
  overlap — not just touching — is what guarantees the union merges them
  into one connected piece; see §4.6)
- Built via `leaf_polygon(...)` — see §4.2.1 for the shape itself

#### 4.2.1 Leaf shape — `leaf_polygon()`
Current shape: **canoe/finger** — pointed at both base and tip, broad
fairly-uniform-width midsection (not a sharp mid-blade bulge, not a
one-directional icicle taper). Width envelope:

```python
w(t) = max_width * sin(π · t_skewed)^flatness
```
where `t_skewed` is `t` warped by a `skew` exponent (see code), `t∈[0,1]`
along the leaf's length.

- `flatness` (0.35–0.55, randomized per leaf): lower = broader/flatter
  plateau = more finger-like, less bulge. This is the primary "less
  bulge, more uniform" control.
- `skew` (0.7–1.4, randomized per leaf): shifts where the widest point
  sits along the length, so leaves don't all read as identical
  symmetric canoes viewed dead-on.
- `symmetry` (constant, `LEAF_SYMMETRY ≈ 0.715`): ratio of bottom-edge
  width to top-edge width — <1.0 gives a slightly lopsided/sickle blade
  rather than a perfectly mirrored one.
- `bend`: a gentle sinusoidal y-offset along the leaf for a natural curve.
- Width is floored at `width_floor` (= `MIN_LINE_UNITS`, 1mm) **everywhere
  along the blade including both tapered ends** — this is a hard
  printability constraint, not just at the base.

This shape has changed twice based on human feedback (pea-pod →
icicle → canoe); see §8 changelog for why each didn't work and what
replaced it. **If asked to change leaf shape again, edit this function's
width-envelope formula and re-tune `flatness`/`skew` ranges — don't touch
anything else in the pipeline, this function is fully self-contained.**

### 4.3 Parallel/perpendicular branch bias
Two tunable constants control how "spoke-like" (radiating from ring
toward center) vs. "row-like" (parallel canes, like real bamboo)
branches read:

```python
BRANCH_PARALLEL_BIAS = 0.78    # regular branches
SPANNING_PARALLEL_BIAS = 0.92  # spanning (ring-to-ring) branches
```

**Regular branches:** `grain_aligned_heading(radial_heading, grain_angle,
bias)` blends (via unit-vector interpolation, `_angle_lerp`) between the
purely-radial heading (`bias=0`) and the shared grain axis (`bias=1`),
picking whichever end of the grain axis (`grain_angle` or
`grain_angle+180`) is closer to radial so branches still generally grow
inward rather than flip to point outward.

**Spanning branches:** `parallel_chord_points(grain_angle, lane_offset,
R_IN, jitter_deg)` computes the two ring-contact points as an exact
parallel chord along the grain direction, offset perpendicular by a
random `lane_offset`. At `bias=1.0` this is mathematically exact — two
parallel chords on the same circle cannot cross. `jitter_deg` (derived
from `1 - SPANNING_PARALLEL_BIAS`) allows small random deviation from
perfectly parallel.

Branch curvature (`bend_strength`, 8–32° currently) was also kept modest
— a wildly curving branch undercuts the parallel-rows look even with a
well-aligned starting heading.

**Verification metric:** `stem_crossing_count` in the per-gobo metrics —
counts proper geometric crossings (`LineString.crosses()`, which excludes
shared-endpoint touches) between all branch centerline pairs. Currently
median 0, max 2 across the 75-gobo batch.

### 4.4 Spanning branches (ring-to-ring)
`SPANNING_BRANCH_FRACTION = 0.25` — exactly this fraction of gobos (via
`run.py`'s `spanning_flags`, every 4th gobo, evenly spread through the
batch rather than randomly clustered) get one extra branch whose BOTH
endpoints are fixed on the ring's inner edge, rather than growing inward
from one point. Built as a quadratic Bezier (`spanning_branch_points`)
through a control point offset perpendicular to the p0→p1 chord, so it
can bow gently without needing to pass through dead center. The
`has_spanning_branch` flag is recorded per gobo in the manifest, and
these gobos are marked with `*` in the labeled preview.

### 4.5 Leaf density: thinning + overlap capping
Two independent reduction steps, applied to the leaf list *after* layout
generation but *before* the layout's branches are given a final width
(this ordering matters — see §4.6):

1. **Random thinning:** `LEAF_KEEP_FRACTION = 0.90 * 0.75 = 0.675` — keep
   67.5% of generated leaves, chosen via `rng.sample` (deterministic per
   seed). Represents two compounded density-reduction requests (10%,
   then a further 25%) — see §8 for history. Removing any subset of
   leaves is always safe: every leaf independently overlaps its own
   stem, so no leaf's removal can disconnect anything else.
2. **Overlap capping:** `reduce_leaf_overlap(polys, max_ratio=1.5)` —
   greedily removes the single most-overlapping leaf (by intersection
   area with the union of all others), recomputes
   `sum(individual areas)/union(area)`, and repeats until that ratio is
   ≤`MAX_LEAF_OVERLAP_RATIO` (1.5) or only 3 leaves remain. This is
   **actively enforced per gobo**, not just measured after the fact —
   every gobo in the current batch achieves ≤1.4993.

### 4.6 Stem width: tuning, sturdier multiplier, floor
This is the trickiest part of the pipeline and worth understanding fully
before changing it.

```python
target_ratio = rng.uniform(6.0, 12.0)   # per-gobo target, drawn once
```

1. **Baseline tuning pass:** iteratively adjust a single scalar
   `stem_width` (starting at 2.6 units) so that
   `leaf_area / stem_area` lands in `[6.0, 12.0]`, using the *already-
   fixed* `leaf_area` from §4.5 (this is why leaf generation is fully
   decoupled from stem width — earlier versions of this pipeline re-rolled
   leaf layout on every width-tuning iteration, which meant the "ratio"
   being tuned toward was a moving target; decoupling fixed that).
   Stem width scales roughly linearly with area for a fixed branch
   layout, so this converges in a handful of passes (capped at 8).
2. **Sturdier multiplier:** `final_stem_width = baseline_stem_width *
   STURDIER_MULT` where `STURDIER_MULT = 1.25`.
3. **Absolute floor:** `final_stem_width = max(final_stem_width,
   MIN_LINE_UNITS)` (4 units = 1mm). **In the current parameter regime,
   this floor dominates** — the ratio-tuned baseline typically comes out
   around 0.6–0.8mm before flooring, so nearly every stem in the batch
   ends up sitting at exactly the 1mm floor, not at
   `baseline * 1.25`. This is a known, reported characteristic of the
   current tuning — see §8. If you're asked to make stems "even
   sturdier" again, know that nudging `STURDIER_MULT` further will do
   **nothing** visible unless the floor itself also moves, or the ring/
   leaf scale changes enough that the tuned baseline exceeds the floor
   on its own.
4. Sanity cap at 16 units (~4mm) so nothing can runaway to
   absurd thickness.

The **leaf:stem area ratio** reported in the manifest (`leaf_to_stem_
area_ratio`) is computed from the *final* (post-multiplier, post-floor)
stem width, so it reflects reality — it is not the same as
`target_ratio`, which is only the aim of the baseline-tuning step before
the multiplier/floor are applied on top.

### 4.7 Final assembly and connectivity check
```python
final = unary_union([RING, stem_shape, leaf_shape])
final = final.buffer(0)                 # clean up any topology noise
final = final.intersection(DISK)        # clip anything poking past the outer ring
final = final.buffer(0)
is_single = final.geom_type == "Polygon"   # THE connectivity assertion
```
If `is_single` is ever `False` (should not happen given the overlap-
by-construction guarantees in §4.2 and §4.4, but defensively handled),
the code falls back to keeping only the piece(s) touching the ring and
re-unioning them, and still reports `is_single` honestly in the output
so a failure would be visible in the manifest, not silently masked.

### 4.8 Metrics computation
A block of derived stats — leaf/stem/whitespace coverage, leaf overlap
ratio & fraction, average nearest-neighbor leaf spacing, average leaf
area, and `stem_crossing_count` — computed per gobo and aggregated by
`run.py` into `tuning_metrics.md` (min/max/avg/median across all 75).
These exist specifically so future tuning requests can be phrased and
verified as numbers ("get whitespace ratio above 0.6") instead of
eyeballing renders. **If you add a new visual parameter, add a
corresponding metric here** — this has been the pattern for every pass
so far (leaf overlap ratio was added when overlap became a concern; stem
crossing count was added when branch-crossing became a concern).

### 4.9 SVG output
`polygon_to_path_d()` scales the final unit-space polygon to millimetres
and emits a single `<path>` per gobo using `fill-rule="evenodd"` — this
is what lets one path element correctly represent the ring + all
branches + all leaves + the interior "holes" (white space) without
needing separate overlapping shapes. `gobo_svg()` wraps that into a
minimal standalone SVG with explicit mm-unit `width`/`height`.

## 5. Invariants — must hold for every gobo, every run

| Invariant | Where enforced | How verified |
|---|---|---|
| Every gobo is one connected black region (no floating islands) | §4.7, via boolean union of ring+stems+leaves | `is_single` check; `single_connected_black_region` in manifest |
| No self-intersecting/invalid geometry | `.buffer(0)` cleanup at multiple stages; `clean()`/`robust_union()` helpers | `geometry_valid_no_self_intersections` (`geom.is_valid`) in manifest |
| Ring outer diameter exactly 50mm, band exactly 3mm | Fixed `R_OUT`/`R_IN`/`SCALE_MM_PER_UNIT`, never randomized | `RING_OUTER_DIAM_MM`/`RING_THICKNESS_MM` constants, printed in manifest spec block |
| No line narrower than 1mm anywhere (stems or leaf blades) | `MIN_LINE_UNITS` floor applied to both `render_stems` and `leaf_polygon`'s width envelope | `stem_width_mm` in manifest (currently always exactly 1.0); leaf floor is structural, not separately logged per-leaf |
| Leaf overlap ratio ≤ 1.5 | `reduce_leaf_overlap()` active pruning | `metrics.leaf_overlap_ratio` in manifest, max checked in `run.py`'s printed summary |
| Exactly ~1/4 of gobos have a spanning branch | `run.py`'s `spanning_flags` (deterministic, every 4th index) | `has_spanning_branch` in manifest; count printed in `tuning_metrics.md` header |

**If you're asked to change something, check this table first** — most
requests will touch exactly one row. Don't loosen an invariant to hit an
aesthetic request without flagging the trade-off explicitly (this has
come up repeatedly — e.g. reducing leaf density pulled the leaf:stem
ratio down as a direct, expected consequence, and that was reported
rather than silently absorbed).

## 6. Current parameter reference

All at the top of `generate_gobos.py` unless noted:

| Constant | Value | Meaning |
|---|---|---|
| `SCALE_MM_PER_UNIT` | 0.25 | unit-space → mm conversion |
| `MIN_LINE_MM` / `MIN_LINE_UNITS` | 1.0mm / 4.0 units | absolute minimum line width, stems and leaves |
| `STURDIER_MULT` | 1.25 | stem width multiplier over tuned baseline (currently dominated by the floor — see §4.6) |
| `LEAF_KEEP_FRACTION` | 0.675 (0.90×0.75) | fraction of generated leaves kept after random thinning |
| `LEAF_LENGTH_MULT` | 1.30 | leaf length multiplier |
| `LEAF_SYMMETRY_BASE` / `LEAF_SYMMETRY` | 0.55 / 0.715 | bottom-edge-width : top-edge-width ratio |
| `MAX_LEAF_OVERLAP_RATIO` | 1.5 | hard cap, actively enforced |
| `SPANNING_BRANCH_FRACTION` | 0.25 | target fraction of gobos with a ring-to-ring branch |
| `BRANCH_PARALLEL_BIAS` | 0.78 | 0=radial/spoke-like, 1=fully parallel to shared grain (regular branches) |
| `SPANNING_PARALLEL_BIAS` | 0.92 | same, for spanning branches |
| `R_OUT` / `R_IN` | 100 / 88 units | ring outer/inner radius (→ 50mm / 44mm diameter) |
| `flatness` range | 0.35–0.55 (randomized per leaf, in `_add_leaf_stations`) | leaf plateau broadness |
| `skew` range | 0.7–1.4 (randomized per leaf) | leaf widest-point position |
| `leaf_len` range | `uniform(30,46) * size_scale * LEAF_LENGTH_MULT` | per-leaf length |
| `leaf_w` range | `leaf_len * uniform(0.30, 0.40)` | per-leaf max width |
| `n_branches` | `choice([2,2,3,3,4])` | regular branches per gobo |
| `n_stations` | 3–5 (regular), 7–11 (spanning) | leaf attachment points per branch |

## 7. Batch orchestration (`run.py`)

- Generates all 75 gobos (`seed = 1000 + i`), writes each as its own
  file (`individual_svgs/gobo_NN.svg`) — this **is** the "sliced" form;
  no post-processing needed to get individual files.
- Assembles `master_sheet_75_gobos.svg`: a 10×8 grid, each gobo in its
  own `<g id="gobo_NN" transform="translate(...)">`, unlabeled (kept
  clean for re-slicing/re-use, since the labeled version below adds
  `<text>` elements you wouldn't want in a cut/print file).
- Assembles `preview_labeled.svg`/`.png` (via `cairosvg`, if installed):
  same grid, each thumbnail with its `gobo_NN` id (and `*` if
  `has_spanning_branch`) printed underneath, generated from the *same*
  cached `all_results` list (not regenerated) so it's guaranteed to
  match the manifest exactly.
- Writes `manifest.json`: a `spec` block (all the tunable constants'
  current values) + a `gobos` list (per-gobo stats and the full
  `metrics` dict).
- Writes `tuning_metrics.md`: aggregate min/max/avg/median across all 75
  for every metric, plus written definitions.
- Prints a validation summary to stdout (disconnected count, invalid-
  geometry list, ratio min/max/avg, stem width min/max, overlap ratio
  min/max, spanning count) — **check this output after every run**,
  it's the fast way to catch a regression before generating previews.

## 8. Changelog / tuning history (context for why values are what they are)

Useful if asked "why is X this value" — short version of each pass:

1. **v1 (aesthetic-only, no physical units):** established the core
   union-based connectivity approach and the leaf/stem area ratio concept.
2. **v2 (50mm/3mm/1mm physical spec):** discovered the ratio-tuned stem
   baseline was ~0.15–0.2mm at real scale — the 1mm floor became the
   dominant factor, not the "+25% sturdier" multiplier. Reported honestly
   rather than hidden.
3. **Leaf spacing fix:** original leaf clusters (2–4 leaves per station)
   caused ~40% overlap fraction; reduced to mostly 1 leaf/station with
   positional jitter → ~22% overlap, at the cost of leaf:stem ratio
   (clumped leaves had been inflating it).
4. **Leaf length/symmetry +30%, overlap cap 1.5, spanning branches,
   labeled preview:** added the ring-to-ring spanning branch type and the
   active overlap-pruning algorithm (previously overlap was only
   measured, not enforced). Ratio recovered to ~6.9 avg as a side effect
   of bigger leaves.
5. **Icicle taper attempt:** tried a monotonic base-thick/tip-thin taper
   per a literal reading of "taper toward the point" — lost the aesthetic
   (too spiky, not enough "leaf" left). Reverted.
6. **Canoe/finger shape (current):** pointed at both ends, broad
   low-bulge midsection (`flatness`/`skew` params), which is what's in
   place now. Ratio landed back near ~5.6–5.8 avg without deliberately
   targeting that — a side effect of the wider canoe body.
7. **Parallel/perpendicular branch bias (current):** branches were
   radiating from the ring toward center (spoke-like), guaranteeing
   crossings near the middle. Added `BRANCH_PARALLEL_BIAS`/
   `SPANNING_PARALLEL_BIAS` and the `grain_angle` mechanism; added
   `stem_crossing_count` metric to make the improvement measurable
   (median 0, max 2 across the batch, down from a structurally-
   guaranteed-several-per-gobo baseline).

**Pattern to notice:** almost every pass traded one metric for another
(density vs. ratio, spacing vs. overlap, taper sharpness vs. aesthetic).
None of these trade-offs were resolved by silently picking a side — each
was reported with before/after numbers so the human could decide whether
to push back. **Keep doing this.** If a request will predictably move a
metric in the wrong direction, say so up front with a number, don't just
ship it.

## 9. How to run

```bash
pip install shapely numpy          # cairosvg + Pillow optional, for PNG previews
python3 run.py                     # regenerates everything into ./out/
```
Outputs land in `out/`: `individual_svgs/gobo_01.svg`...`gobo_75.svg`,
`master_sheet_75_gobos.svg`, `preview_labeled.svg`, `manifest.json`,
`tuning_metrics.md`. To also get PNG previews:
```python
import cairosvg
cairosvg.svg2png(url='out/master_sheet_75_gobos.svg', write_to='out/preview.png', output_width=2380)
cairosvg.svg2png(url='out/preview_labeled.svg', write_to='out/preview_labeled.png', output_width=2600)
```

**To regenerate a single gobo** (e.g. to inspect or hand-tune one
design): `from generate_gobos import build_gobo, gobo_svg; res =
build_gobo(1005, spanning=True); print(gobo_svg(res["path_geom"], "test"))`
— seed `1000+i` where `i` is the 0-indexed position (`gobo_06` → seed 1005).

## 10. How to extend safely

- **New leaf shape:** edit only `leaf_polygon()`'s width-envelope
  formula. Keep the `width_floor` clamp. Re-check `tuning_metrics.md`
  after regenerating — leaf shape changes almost always move
  `avg_leaf_area_mm2` and therefore the leaf:stem ratio; report the
  before/after like previous passes did.
- **New branch behavior:** edit `build_layout()`. If you add a new
  branch *type* (like spanning branches were added), give it its own
  boolean flag and fraction constant (mirroring
  `SPANNING_BRANCH_FRACTION`), and make sure its leaf attachment still
  goes through `_add_leaf_stations` so density/overlap rules stay uniform.
- **Different physical size:** change `SCALE_MM_PER_UNIT` and/or
  `R_OUT`/`R_IN`. Re-run and check the `MIN_LINE_UNITS`-derived stem
  width in the manifest — at a large enough size the floor stops
  dominating and `STURDIER_MULT` starts actually mattering again (see §8.2).
- **New tunable parameter:** add it near the top of `generate_gobos.py`
  with the same comment style as the existing ones (state both extremes
  of the range and what they mean), thread it into the manifest `spec`
  block in `run.py`, and if it affects something measurable, add a
  metric for it (§4.8's pattern).
- **Don't:** bypass the union step to draw leaves/stems as separate
  overlapping shapes for convenience — this is the one thing that must
  never regress, since it's the whole basis for the connectivity and
  manifold-mesh guarantees this pipeline exists to provide.

## 11. Glossary

- **Gobo:** here, a circular cutout/silhouette pattern (ring + branch/leaf
  design), ultimately extruded to a 3D-printable disk.
- **Unit space:** the abstract 220×220 coordinate system geometry is
  authored in; scaled to mm only at export (§3).
- **Grain angle:** a per-gobo random direction that branches bias toward,
  producing a parallel-rows look (§4.3).
- **Spanning branch:** a branch touching the ring at two separate points
  rather than growing inward from one (§4.4).
- **Leaf overlap ratio:** `sum(individual leaf areas) / union(leaf area)`
  — 1.0 = no overlap, higher = more leaves stacked on each other.
- **Leaf:stem ratio:** `leaf_area / stem_area` in the final geometry —
  the main "how leafy vs. how twiggy" knob, currently landing ~5.6–5.8
  average as an emergent result of several other parameters rather than
  being directly set.
