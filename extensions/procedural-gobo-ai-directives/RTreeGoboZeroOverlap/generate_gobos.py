#!/usr/bin/env python3
"""
Procedural bamboo-leaf gobo generator - v2.

Physical spec (per gobo):
- Outer ring diameter: 47 mm
- Ring band thickness:  2 mm  (exact)
- Absolute minimum line width anywhere in the design: 1 mm
- Stem/branch elements: 25% thicker than the v1 (aesthetic-only) baseline
- Leaf count: 10% fewer than the v1 baseline (density reduction, not size)

Internal geometry is still authored in the original abstract "unit" space
(CANVAS=220 units, ring R_OUT=100/R_IN=88 units) because that's the
coordinate system the aesthetic was tuned in. A single scale constant
(SCALE_MM_PER_UNIT = 0.25) converts unit-space -> millimetres at output
time only, so 200 unit-diameter ring -> 50 mm, and the 12-unit ring band
-> exactly 3 mm. All physical floors below are expressed in unit-space by
dividing the mm requirement by that same constant.

Connectivity guarantee unchanged from v1: ring + every stem + every leaf
are boolean-UNIONed (Shapely), so the result is provably one connected
black region - checked per gobo, not just visually.
"""

import json, math, random
import numpy as np
from shapely.geometry import Polygon, LineString
from shapely.ops import unary_union
from shapely.affinity import rotate as shp_rotate, translate as shp_translate, scale as shp_scale

# ---------------------------------------------------------------- physical scale

SCALE_MM_PER_UNIT = 0.25          # 1 unit = 0.25 mm
MIN_LINE_MM = 1.0
MIN_LINE_UNITS = MIN_LINE_MM / SCALE_MM_PER_UNIT      # = 4.0 units
STURDIER_MULT = 1.25              # stems 25% thicker than v1 baseline
LEAF_KEEP_FRACTION = 0.90 * 0.75  # was 10% less dense; now a further 25% reduction (density x0.675 vs original baseline)
LEAF_LENGTH_MULT = 1.30           # leaves 30% longer than the previous pass
LEAF_SYMMETRY_BASE = 0.55         # previous bottom/top width factor (0=fully asymmetric wedge, 1=symmetric blade)
LEAF_SYMMETRY = min(1.0, LEAF_SYMMETRY_BASE * 1.30)   # symmetry increased 30%
MAX_LEAF_OVERLAP_RATIO = 1.0      # cap on sum(leaf areas)/union(leaf area) - zero overlap is now the default
SPANNING_BRANCH_FRACTION = 0.25   # fraction of gobos with a ring-to-ring branch

# Minimum-content floor: gobo_27 (8 leaves surviving zero-overlap pruning)
# was flagged as a good example of the least content a gobo should have.
# Any gobo whose normal generation would land below this leaf count gets
# retried with denser raw generation (more branches/stations feeding the
# zero-overlap pruning step) until it clears the floor or a retry budget
# is exhausted. Gobos that already meet the floor on the first (normal)
# attempt are completely unaffected - this only pushes sparse ones up.
MIN_LEAVES_FLOOR = 8

# Parallel/perpendicular tuning: 0.0 = branches point radially inward from
# wherever they start on the ring (old behavior - "perpendicular" to the
# ring, spoke-like, converges toward center and crosses a lot). 1.0 =
# branches all run along one shared "grain" direction per gobo, like rows
# of real bamboo canes growing parallel to each other. Higher values
# directly reduce stem-crossing (and therefore leaf-overlap) because
# roughly-parallel lines starting at different points don't intersect the
# way radiating spokes do.
BRANCH_PARALLEL_BIAS = 0.78
SPANNING_PARALLEL_BIAS = 0.92     # same idea, specifically for ring-to-ring spanning branches

# Curve-fitting for SVG export: the geometry above is authored as densely-
# sampled polylines (needed for robust Shapely boolean unions), but writing
# every sample point out as a straight SVG line segment produces far more
# tiny edges than the shape actually needs - which then becomes far more
# vertices/edges than necessary once Blender converts and extrudes it,
# raising the odds of degenerate near-zero-length edges. Instead, the final
# polygon boundary is simplified (Douglas-Peucker, topology-preserving) down
# to a sparser set of anchor points, then a smooth cubic Bezier is fit
# through those anchors (Catmull-Rom -> Bezier conversion) rather than
# connecting them with straight lines. Net effect: same visual shape, a
# fraction of the path commands, and genuinely curved (not faceted) edges.
SIMPLIFY_TOLERANCE_UNITS = 0.5     # = 0.125mm - well below the 1mm min-feature floor

# ---------------------------------------------------------------- geometry (unit space)

CANVAS = 220.0
CX, CY = CANVAS / 2, CANVAS / 2
R_OUT = 94.0           # 47mm outer diameter (radius 23.5mm / 0.25mm-per-unit)
R_IN = 86.0            # ring band thickness = 8 units = 2.0 mm (exact)
CIRCLE_PTS = 72

def circle_coords(r, cx=CX, cy=CY, n=CIRCLE_PTS):
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    return [(cx + r * math.cos(a), cy + r * math.sin(a)) for a in ang]

def make_ring():
    outer = circle_coords(R_OUT)
    inner = circle_coords(R_IN)[::-1]
    return Polygon(outer, [inner])

def make_disk(r=R_OUT):
    return Polygon(circle_coords(r))

RING = make_ring()
DISK = make_disk()


def leaf_polygon(length, max_width, bend, n=15, width_floor=MIN_LINE_UNITS,
                  symmetry=LEAF_SYMMETRY, flatness=0.45, skew=1.0):
    """
    Canoe/finger-shaped bamboo-leaf blade: pointed at both the base
    (0,0) and the tip (length,0), with a broad, fairly uniform-width
    midsection rather than a sharply peaked bulge - like a canoe seen
    from above, or a finger, not a pea-pod. Also not the opposite
    extreme (an icicle that's thickest at the base and only tapers one
    direction) - both ends come to a point.

    `flatness` controls how broad/uniform the midsection reads: lower
    values (~0.3-0.4) flatten the peak into a long plateau (more
    finger-like); higher values (~0.7+) peak more sharply (more bulge).
    `skew` shifts where the widest point sits along the length (1.0 =
    centered/symmetric "straight overhead" canoe; <1 shifts the widest
    point toward the base, >1 toward the tip) - varying this per leaf
    keeps the batch from reading as every leaf viewed from the exact
    same straight-down angle. The width envelope is floored at
    `width_floor` everywhere so no part of the blade is thinner than
    the printable minimum.

    `symmetry` controls how close the bottom edge width is to the top
    edge width (1.0 = fully symmetric blade, lower = more lopsided/sickle).
    """
    ts = np.linspace(0, 1, n)
    ts_skewed = np.power(ts, skew) / (np.power(ts, skew) + np.power(1 - ts, skew) + 1e-9)
    w = max_width * np.power(np.sin(np.pi * ts_skewed), flatness)
    w = np.maximum(w, width_floor)
    top_pts, bot_pts = [], []
    for t, wi in zip(ts, w):
        x = t * length
        bend_y = bend * math.sin(math.pi * t) * length
        top_pts.append((x, bend_y + wi / 2))
        bot_pts.append((x, bend_y - wi / 2 * symmetry))
    coords = top_pts + bot_pts[::-1]
    poly = Polygon(coords)
    if not poly.is_valid:
        poly = poly.buffer(0)
    return poly


def place(poly, angle_deg, dx, dy):
    poly = shp_rotate(poly, angle_deg, origin=(0, 0))
    poly = shp_translate(poly, dx, dy)
    return poly


def _angdiff(a, b):
    """Smallest signed difference a-b in degrees, wrapped to [-180,180]."""
    return (a - b + 180) % 360 - 180


def _angle_lerp(a_deg, b_deg, t):
    """Interpolate between two angles the short way (via unit vectors),
    t=0 -> a_deg, t=1 -> b_deg."""
    a, b = math.radians(a_deg), math.radians(b_deg)
    x = (1 - t) * math.cos(a) + t * math.cos(b)
    y = (1 - t) * math.sin(a) + t * math.sin(b)
    if abs(x) < 1e-9 and abs(y) < 1e-9:
        return a_deg
    return math.degrees(math.atan2(y, x))


def grain_aligned_heading(inward_deg, grain_deg, bias):
    """
    Blend a purely radial (inward-pointing) heading toward a shared
    "grain" direction, without flipping the branch to point outward.
    bias=0 -> pure radial (old behavior); bias=1 -> fully aligned to the
    grain axis (whichever of grain/grain+180 is closer to radial).
    """
    cand1 = grain_deg % 360
    cand2 = (grain_deg + 180) % 360
    grain_dir = cand1 if abs(_angdiff(cand1, inward_deg)) <= abs(_angdiff(cand2, inward_deg)) else cand2
    return _angle_lerp(inward_deg, grain_dir, bias)


def smooth_branch(start, ang0, length, bend_strength, n=24):
    pts = [start]
    ang = math.radians(ang0)
    step = length / n
    curvature = math.radians(bend_strength) / n
    x, y = start
    for i in range(n):
        ang += curvature * math.sin(math.pi * i / n)
        x += step * math.cos(ang)
        y += step * math.sin(ang)
        pts.append((x, y))
    return pts, ang


def parallel_chord_points(grain_deg, lane_offset, R, jitter_deg=0.0):
    """
    Two points on the ring (radius R) such that the chord between them
    runs along `grain_deg` (+/- jitter_deg), offset perpendicular to
    that direction by `lane_offset`. Used for spanning branches so they
    genuinely run parallel to the shared grain direction instead of a
    random chord - this is what actually guarantees two spanning
    branches on the same gobo don't cross each other.
    """
    ang = math.radians(grain_deg + jitter_deg)
    dx, dy = math.cos(ang), math.sin(ang)
    nx, ny = -dy, dx
    ox, oy = CX + nx * lane_offset, CY + ny * lane_offset
    t = math.sqrt(max(R * R - lane_offset * lane_offset, 1.0))
    p0 = (ox - dx * t, oy - dy * t)
    p1 = (ox + dx * t, oy + dy * t)
    return p0, p1


def spanning_branch_points(p0, p1, bend_units, n=40):
    """
    A curve with BOTH endpoints fixed (used for a branch that touches the
    ring at two separate points, rather than growing inward from one).
    Quadratic Bezier through a control point offset perpendicular to the
    p0->p1 chord by `bend_units`, so it can bow gently without needing to
    pass through the center.
    """
    mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
    dx, dy = p1[0] - p0[0], p1[1] - p0[1]
    chord_len = math.hypot(dx, dy) or 1.0
    px, py = -dy / chord_len, dx / chord_len
    cx, cy = mx + px * bend_units, my + py * bend_units
    pts = []
    for t in np.linspace(0, 1, n):
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * cx + t ** 2 * p1[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * cy + t ** 2 * p1[1]
        pts.append((x, y))
    return pts


def clean(polys):
    out = []
    for p in polys:
        if p is None or p.is_empty:
            continue
        if not p.is_valid:
            p = p.buffer(0)
        if p.is_empty or p.area < 1e-6:
            continue
        out.append(p)
    return out


def robust_union(polys):
    polys = clean(polys)
    if not polys:
        return Polygon()
    try:
        return unary_union(polys)
    except Exception:
        acc = None
        for p in polys:
            acc = p if acc is None else acc.union(p)
        return acc if acc is not None else Polygon()


def reduce_leaf_overlap(leaf_polys, max_ratio=MAX_LEAF_OVERLAP_RATIO, min_leaves=3):
    """
    Greedily drop the most-overlapping leaf, one at a time, until
    sum(individual areas)/union(area) <= max_ratio (or too few leaves
    remain to keep pruning). Removing a leaf can never disconnect
    anything else - every leaf touches its own stem independently.
    """
    polys = list(leaf_polys)
    if len(polys) <= min_leaves:
        return polys, 1.0 if not polys else sum(p.area for p in polys) / max(
            robust_union(polys).area, 1e-9)

    def current_ratio(ps):
        union_area = robust_union(ps).area
        sum_area = sum(p.area for p in ps)
        return (sum_area / union_area) if union_area > 0 else 1.0

    ratio = current_ratio(polys)
    guard = 0
    # small epsilon so floating-point noise in shapely's area computation
    # (union area can differ from sum-of-areas by a few 1e-9s even for
    # genuinely non-touching polygons) doesn't cause endless pruning when
    # max_ratio is set to an exact 1.0 (true zero-overlap mode)
    while ratio > max_ratio + 1e-6 and len(polys) > min_leaves and guard < 200:
        guard += 1
        # conflict score per leaf = overlap area with the union of all others
        scores = []
        for i, p in enumerate(polys):
            others = polys[:i] + polys[i + 1:]
            others_union = robust_union(others)
            conflict = p.intersection(others_union).area if not others_union.is_empty else 0.0
            scores.append(conflict)
        worst_i = max(range(len(polys)), key=lambda i: scores[i])
        polys.pop(worst_i)
        ratio = current_ratio(polys)
    return polys, ratio


# ---------------------------------------------------------------- layout (pure geometry, width-independent)

def _add_leaf_stations(bpts, rng, leaves, n_stations_range=(3, 5)):
    bl = LineString(bpts)
    total_len = bl.length
    n_stations = rng.randint(*n_stations_range)
    for si in range(1, n_stations + 1):
        frac = si / (n_stations + 1)
        station_pt = bl.interpolate(frac * total_len)
        d = 0.5
        p1 = bl.interpolate(max(0, frac * total_len - d))
        p2 = bl.interpolate(min(total_len, frac * total_len + d))
        tangent = math.degrees(math.atan2(p2.y - p1.y, p2.x - p1.x))
        n_leaves = rng.choice([1, 1, 2])
        size_scale = 0.7 + 0.5 * (1 - frac)
        for li in range(n_leaves):
            side = 1 if li % 2 == 0 else -1
            spread = rng.uniform(35, 80) * side
            # bigger individual leaves: each one carries more area on its
            # own, so the leaf:stem ratio doesn't need lots of small
            # overlapping leaves to stay in a good range. Length scaled
            # up 30% per this pass's request.
            leaf_len = rng.uniform(30, 46) * size_scale * LEAF_LENGTH_MULT
            leaf_w = leaf_len * rng.uniform(0.30, 0.40)
            bend = rng.uniform(0.04, 0.14)
            flatness = rng.uniform(0.35, 0.55)   # low = broad canoe/finger plateau, less bulge
            skew = rng.uniform(0.7, 1.4)         # varies where the widest point sits - not every leaf looks straight-on
            back = -1.8
            jitter = rng.uniform(-3.0, 3.0)
            bx = station_pt.x + (back) * math.cos(math.radians(tangent)) \
                + jitter * math.cos(math.radians(tangent + 90))
            by = station_pt.y + (back) * math.sin(math.radians(tangent)) \
                + jitter * math.sin(math.radians(tangent + 90))
            lf = leaf_polygon(leaf_len, leaf_w, bend, flatness=flatness, skew=skew)
            lf = place(lf, tangent + spread, bx, by)
            leaves.append({'poly': lf})


def build_layout(rng, spanning=False, density_boost=0):
    """
    Generate the full branch + leaf placement for one gobo, independent
    of final stroke widths. Returns:
      branches: [{'points': [...], 'width_scale': float}, ...]
      leaves:   [{'poly': Polygon}, ...]   (already placed/rotated, at
                 the v1-baseline leaf sizing - width tuning never touches
                 leaf geometry, only stem width)

    `spanning`: if True, one extra branch is added whose two ENDPOINTS
    both sit on the ring's inner edge (touching the ring at two separate
    points, not necessarily through the center) - "spans the diameter"
    without requiring it to cross dead center.

    `density_boost` (0, 1, 2, ...): raises branch count and leaf-station
    count above the normal baseline. Used by build_gobo() as a retry
    lever for gobos that would otherwise end up under the minimum-content
    floor after zero-overlap pruning - generating more raw candidate
    leaves up front gives the pruning step more to work with, so more
    can survive while still hitting zero overlap.
    """
    b = density_boost
    n_branches = min(8, rng.choice([2 + b, 2 + b, 3 + b, 3 + b, 4 + b]))
    branches = []
    leaves = []
    used_angles = []

    # shared "grain" direction for this gobo - branches bias toward running
    # parallel to this axis instead of all radiating straight at center,
    # which is what makes rows of stems look more like real bamboo canes
    # and directly cuts down on stem-crossing (and the leaf overlap that
    # crossing causes).
    grain_angle = rng.uniform(0, 360)

    if spanning:
        lane_offset = rng.uniform(-R_IN * 0.75, R_IN * 0.75)
        span_jitter = rng.uniform(-1, 1) * (1 - SPANNING_PARALLEL_BIAS) * 45
        p0, p1 = parallel_chord_points(grain_angle, lane_offset, R_IN, jitter_deg=span_jitter)
        bend_units = rng.uniform(-15, 15)
        span_pts = spanning_branch_points(p0, p1, bend_units)
        branches.append({'points': span_pts, 'width_scale': 1.0})
        _add_leaf_stations(span_pts, rng, leaves, n_stations_range=(7 + b, 11 + b))
        a0 = math.degrees(math.atan2(p0[1] - CY, p0[0] - CX)) % 360
        a1 = math.degrees(math.atan2(p1[1] - CY, p1[0] - CX)) % 360
        used_angles.extend([a0, a1])

    for _b in range(n_branches):
        for _try in range(20):
            a0 = rng.uniform(0, 360)
            if all(abs((a0 - u + 180) % 360 - 180) > 16 for u in used_angles):
                break
        used_angles.append(a0)
        start = (CX + R_IN * math.cos(math.radians(a0)),
                 CY + R_IN * math.sin(math.radians(a0)))
        inward = (a0 + 180) % 360
        radial_heading = inward + rng.uniform(-18, 18)
        heading = grain_aligned_heading(radial_heading, grain_angle, BRANCH_PARALLEL_BIAS)
        heading += rng.uniform(-6, 6)  # small per-branch jitter so it isn't mechanically identical
        length = R_IN * rng.uniform(0.68, 0.95)
        # straighter than before - a strong bend fights the parallel-rows
        # look even when the initial heading is well-aligned to the grain
        bend_strength = rng.uniform(8, 32) * rng.choice([-1, 1])
        pts, _ = smooth_branch(start, heading, length, bend_strength)
        branches.append({'points': pts, 'width_scale': 1.0})

        branches_pts = [pts]
        for _sub in range(rng.choice([0, 0, 1, 1] if b == 0 else [0, 1, 1, 1])):
            if len(pts) <= 10:
                break
            split_i = rng.randint(len(pts) // 4, len(pts) - 4)
            sub_start = pts[split_i]
            sub_heading = heading + rng.uniform(25, 65) * rng.choice([-1, 1])
            sub_len = length * rng.uniform(0.35, 0.65)
            sub_pts, _ = smooth_branch(sub_start, sub_heading, sub_len,
                                        rng.uniform(10, 35) * rng.choice([-1, 1]))
            branches.append({'points': sub_pts, 'width_scale': 0.85})
            branches_pts.append(sub_pts)

        for bpts in branches_pts:
            _add_leaf_stations(bpts, rng, leaves, n_stations_range=(3 + b, 5 + b))

    return {'branches': branches, 'leaves': leaves, 'grain_angle': grain_angle}


def render_stems(layout, sw):
    polys = []
    for b in layout['branches']:
        width = max(sw * b['width_scale'], MIN_LINE_UNITS)
        polys.append(LineString(b['points']).buffer(
            width / 2, cap_style="round", join_style="round"))
    return polys


# ---------------------------------------------------------------- build one gobo

def build_gobo(seed, spanning=False, max_leaf_overlap_ratio=MAX_LEAF_OVERLAP_RATIO,
                min_leaves_after_prune=1):
    rng = random.Random(seed)
    target_ratio = rng.uniform(6.0, 12.0)

    layout = build_layout(rng, spanning=spanning, density_boost=0)

    # --- leaf density: drop 10% of leaves (each leaf independently
    # touches its stem, so removing any subset can never disconnect
    # anything else) ---
    all_leaves = layout['leaves']
    n_total = len(all_leaves)
    n_keep = max(1, round(n_total * LEAF_KEEP_FRACTION))
    keep_idx = set(sorted(rng.sample(range(n_total), n_keep))) if n_total else set()
    thinned_leaf_polys = [all_leaves[i]['poly'] for i in keep_idx]

    # --- cap leaf overlap: greedily drop the most-overlapping leaves
    # until sum(area)/union(area) <= max_leaf_overlap_ratio. Defaults to
    # zero overlap (1.0), but callers can override. ---
    kept_leaf_polys, achieved_overlap_ratio = reduce_leaf_overlap(
        thinned_leaf_polys, max_ratio=max_leaf_overlap_ratio, min_leaves=min_leaves_after_prune)

    # --- minimum-content floor (gobo_27's post-prune leaf count, 8, was
    # flagged as the least content a gobo should have). If the normal
    # attempt above landed short, retry with progressively denser raw
    # generation (more branches/stations feeding the same zero-overlap
    # pruning step) until the floor is cleared or a retry budget runs
    # out. Gobos that already meet the floor never enter this branch, so
    # their output is byte-for-byte identical to before this change. ---
    content_floor_attempts = 1
    content_floor_met = len(kept_leaf_polys) >= MIN_LEAVES_FLOOR
    density_boost_used = 0

    if not content_floor_met:
        best = (layout, n_total, thinned_leaf_polys, kept_leaf_polys)
        for attempt in range(1, 6):
            retry_rng = random.Random(seed * 1_000_003 + attempt)
            retry_layout = build_layout(retry_rng, spanning=spanning, density_boost=attempt)
            retry_all_leaves = retry_layout['leaves']
            retry_n_total = len(retry_all_leaves)
            retry_n_keep = max(1, round(retry_n_total * LEAF_KEEP_FRACTION))
            retry_keep_idx = (set(sorted(retry_rng.sample(range(retry_n_total), retry_n_keep)))
                               if retry_n_total else set())
            retry_thinned = [retry_all_leaves[i]['poly'] for i in retry_keep_idx]
            retry_kept, _ = reduce_leaf_overlap(
                retry_thinned, max_ratio=max_leaf_overlap_ratio, min_leaves=min_leaves_after_prune)
            content_floor_attempts += 1
            if len(retry_kept) > len(best[3]):
                best = (retry_layout, retry_n_total, retry_thinned, retry_kept)
                density_boost_used = attempt
            if len(retry_kept) >= MIN_LEAVES_FLOOR:
                content_floor_met = True
                best = (retry_layout, retry_n_total, retry_thinned, retry_kept)
                density_boost_used = attempt
                break
        layout, n_total, thinned_leaf_polys, kept_leaf_polys = best

    leaf_shape = robust_union(kept_leaf_polys)
    leaf_area = leaf_shape.area

    # --- baseline stem width tuned to the 6:1-12:1 aesthetic ratio,
    # using the FIXED leaf_area above (layout no longer re-rolls per
    # pass, so this converges cleanly) ---
    stem_width = 2.6
    for _pass in range(8):
        stem_polys = render_stems(layout, stem_width)
        stem_shape = robust_union(stem_polys)
        stem_area = stem_shape.area
        ratio = leaf_area / stem_area if stem_area else 999
        if 6.0 <= ratio <= 12.0:
            break
        new_w = stem_width * (ratio / target_ratio)
        new_w = max(0.6, min(new_w, 8.0))
        if abs(new_w - stem_width) < 0.008:
            break
        stem_width = new_w

    baseline_stem_width = stem_width

    # --- apply the requested physical adjustments on top of the tuned
    # baseline: stems 25% sturdier, then enforce the 1mm absolute floor ---
    final_stem_width = max(baseline_stem_width * STURDIER_MULT, MIN_LINE_UNITS)
    final_stem_width = min(final_stem_width, 16.0)  # sanity cap ~4mm

    stem_polys = render_stems(layout, final_stem_width)
    stem_shape = robust_union(stem_polys)
    stem_area = stem_shape.area
    final_ratio = leaf_area / stem_area if stem_area else 999

    final = robust_union([RING, stem_shape, leaf_shape])
    final = final.buffer(0)
    final = final.intersection(DISK)
    final = final.buffer(0)

    is_single = final.geom_type == "Polygon"
    if not is_single:
        polys = list(final.geoms)
        keep = [p for p in polys if p.intersects(RING.exterior.buffer(0.01))]
        final = robust_union(keep) if keep else max(polys, key=lambda p: p.area)
        is_single = final.geom_type == "Polygon"

    # ---------------------------------------------------------- tuning metrics
    S = SCALE_MM_PER_UNIT
    disk_area_mm2 = DISK.area * S * S
    ring_area_mm2 = RING.area * S * S
    black_area_mm2 = final.area * S * S
    white_area_mm2 = disk_area_mm2 - black_area_mm2

    leaf_sum_area_units = sum(p.area for p in kept_leaf_polys)   # pre-union, double-counts overlap
    leaf_union_area_units = leaf_shape.area
    # >1 means leaves overlap each other; 1.0 = no overlap at all
    leaf_overlap_ratio = (leaf_sum_area_units / leaf_union_area_units
                           if leaf_union_area_units > 0 else 1.0)
    # fraction of raw leaf-shape area that disappears into overlap with
    # other leaves (0 = fully distinct blades, ->1 = heavily stacked)
    leaf_overlap_fraction = (1 - leaf_union_area_units / leaf_sum_area_units
                              if leaf_sum_area_units > 0 else 0.0)

    # nearest-neighbour spacing between leaf centroids, in mm - a rough
    # proxy for "are individual leaves visually separated"
    centroids = [p.centroid for p in kept_leaf_polys]
    nn_dists_mm = []
    for i, c in enumerate(centroids):
        best = None
        for j, c2 in enumerate(centroids):
            if i == j:
                continue
            dd = c.distance(c2)
            if best is None or dd < best:
                best = dd
        if best is not None:
            nn_dists_mm.append(best * S)
    avg_nn_spacing_mm = sum(nn_dists_mm) / len(nn_dists_mm) if nn_dists_mm else 0.0

    # count true stem-on-stem crossings (proper interior intersections,
    # not just touching at a shared branch point) - the direct, countable
    # version of "reduce stem intersections"
    branch_lines = [LineString(b['points']) for b in layout['branches']]
    stem_crossing_count = 0
    for i in range(len(branch_lines)):
        for j in range(i + 1, len(branch_lines)):
            if branch_lines[i].crosses(branch_lines[j]):
                stem_crossing_count += 1

    metrics = {
        "disk_area_mm2": disk_area_mm2,
        "ring_area_mm2": ring_area_mm2,
        "ring_coverage_of_disk_pct": 100 * ring_area_mm2 / disk_area_mm2,
        "black_area_mm2": black_area_mm2,
        "white_area_mm2": white_area_mm2,
        "whitespace_ratio": white_area_mm2 / disk_area_mm2,
        "black_fill_ratio": black_area_mm2 / disk_area_mm2,
        "leaf_coverage_ratio": leaf_union_area_units * S * S / disk_area_mm2,
        "stem_coverage_ratio": stem_area * S * S / disk_area_mm2,
        "leaf_overlap_ratio": leaf_overlap_ratio,
        "leaf_overlap_fraction_pct": 100 * leaf_overlap_fraction,
        "avg_nearest_neighbor_leaf_spacing_mm": avg_nn_spacing_mm,
        "avg_leaf_area_mm2": (leaf_sum_area_units * S * S / len(kept_leaf_polys)
                               if kept_leaf_polys else 0.0),
        "stem_crossing_count": stem_crossing_count,
    }

    return {
        "seed": seed,
        "path_geom": final,
        "leaf_area_mm2": leaf_area * (SCALE_MM_PER_UNIT ** 2),
        "stem_area_mm2": stem_area * (SCALE_MM_PER_UNIT ** 2),
        "ratio": final_ratio,
        "target_ratio_aesthetic_baseline": target_ratio,
        "stem_width_mm": final_stem_width * SCALE_MM_PER_UNIT,
        "baseline_stem_width_mm": baseline_stem_width * SCALE_MM_PER_UNIT,
        "n_branches": len(layout['branches']),
        "n_leaves_total_before_thinning": n_total,
        "n_leaves_after_10pct_thin": len(thinned_leaf_polys),
        "n_leaves_kept": len(kept_leaf_polys),
        "has_spanning_branch": spanning,
        "grain_angle_deg": round(layout['grain_angle'], 1),
        "content_floor_met": content_floor_met,
        "content_floor_attempts": content_floor_attempts,
        "density_boost_used": density_boost_used,
        "single_connected_region": is_single,
        "metrics": metrics,
    }


# ---------------------------------------------------------------- SVG output (mm)

def catmull_rom_to_bezier_d(coords, decimals=3, tension=0.55):
    """
    Fit a smooth closed cubic-Bezier path through a sequence of points
    (Catmull-Rom -> Bezier control-point conversion), instead of a
    straight-line-segment polyline. `coords` is a closed ring's
    coordinate list (shapely convention: first point == last point).
    Produces one 'C' curve command per anchor point rather than one 'L'
    per raw sample point, and the curve genuinely passes through every
    anchor while staying smooth (tangent-continuous) between them.

    `tension` (0-1, default 0.55, vs. the mathematically "full" Catmull-
    Rom value of 1.0) dampens how far each control-point handle reaches
    toward its neighbors. Full-tension Catmull-Rom can overshoot past the
    polygon boundary at sharp or sparsely-sampled corners (e.g. a
    simplified leaf/stem junction), which creates self-intersecting loops
    in the exported curve even though the underlying source polygon is
    perfectly valid. A damped tension trades a little curve "roundness"
    for reliably staying close to the actual boundary.
    """
    pts = coords[:-1] if coords[0] == coords[-1] else list(coords)
    n = len(pts)
    if n < 3:
        # too few points for a meaningful curve fit - straight fallback
        d = "M " + " L ".join(f"{x:.{decimals}f},{y:.{decimals}f}" for x, y in coords) + " Z"
        return d
    out = [f"M {pts[0][0]:.{decimals}f},{pts[0][1]:.{decimals}f}"]
    k = tension / 6.0
    for i in range(n):
        p0 = pts[(i - 1) % n]
        p1 = pts[i % n]
        p2 = pts[(i + 1) % n]
        p3 = pts[(i + 2) % n]
        c1x = p1[0] + k * (p2[0] - p0[0])
        c1y = p1[1] + k * (p2[1] - p0[1])
        c2x = p2[0] - k * (p3[0] - p1[0])
        c2y = p2[1] - k * (p3[1] - p1[1])
        out.append(
            f"C {c1x:.{decimals}f},{c1y:.{decimals}f} "
            f"{c2x:.{decimals}f},{c2y:.{decimals}f} "
            f"{p2[0]:.{decimals}f},{p2[1]:.{decimals}f}"
        )
    out.append("Z")
    return " ".join(out)


def polygon_to_path_d(geom, scale=SCALE_MM_PER_UNIT, decimals=3,
                       simplify_tol_units=SIMPLIFY_TOLERANCE_UNITS, use_bezier=True,
                       tension=0.55):
    """
    Convert a (Multi)Polygon into one evenodd SVG path 'd' string, scaling
    unit-space coordinates to millimetres. Simplifies each ring first
    (topology-preserving, tolerance well below the 1mm min-feature floor),
    then fits smooth cubic Beziers through the reduced point set instead
    of emitting a straight line per original sample point.
    """
    g = geom
    if simplify_tol_units:
        g = g.simplify(simplify_tol_units, preserve_topology=True)
        if not g.is_valid:
            g = g.buffer(0)
        if g.is_empty:
            g = geom  # simplify degenerated the shape - fall back to unsimplified
    g = shp_scale(g, xfact=scale, yfact=scale, origin=(0, 0))
    polys = [g] if g.geom_type == "Polygon" else list(g.geoms)
    parts = []
    for poly in polys:
        rings = [poly.exterior] + list(poly.interiors)
        for ring in rings:
            coords = list(ring.coords)
            if use_bezier:
                parts.append(catmull_rom_to_bezier_d(coords, decimals, tension=tension))
            else:
                d = "M " + " L ".join(f"{x:.{decimals}f},{y:.{decimals}f}" for x, y in coords) + " Z"
                parts.append(d)
    return " ".join(parts)


def robust_curved_path_d(geom, scale=SCALE_MM_PER_UNIT, decimals=3, max_area_diff_pct=2.0):
    """
    Export `geom` as a Bezier-curved SVG path, adaptively backing off
    toward less aggressive simplification (more anchor points, gentler
    curves) - and, as a last resort, plain straight-line segments - until
    the exported path validates: flattening it back to a polygon must
    give a single valid region whose area is within `max_area_diff_pct`
    of the source. This is checked, not assumed, because a full-tension
    Catmull-Rom fit CAN self-intersect at sharp/sparse corners even when
    the source polygon is perfectly clean.

    Returns (d_string, diagnostics_dict).
    """
    attempts = [
        # (simplify_tol_units, tension, use_bezier, label)
        (SIMPLIFY_TOLERANCE_UNITS, 0.55, True, "bezier_tol0.5_tension0.55"),
        (SIMPLIFY_TOLERANCE_UNITS, 0.35, True, "bezier_tol0.5_tension0.35"),
        (0.2, 0.35, True, "bezier_tol0.2_tension0.35"),
        (0.0, 0.25, True, "bezier_tol0_tension0.25"),
        (None, 0.0, False, "straight_unsimplified_fallback"),
    ]
    src_area = geom.area  # unit-space area, compare consistently
    for simplify_tol, tension, use_bezier, label in attempts:
        d = polygon_to_path_d(geom, scale=scale, decimals=decimals,
                               simplify_tol_units=simplify_tol, use_bezier=use_bezier,
                               tension=tension)
        flat = flatten_bezier_d_to_polygon(d) if use_bezier else None
        if not use_bezier:
            # straight-line export from the (already-validated) source
            # geometry itself can't introduce new self-intersections
            return d, {"method": label, "valid": True, "single_region": True, "area_diff_pct": 0.0}
        if flat is None or not flat.is_valid or flat.geom_type != "Polygon":
            continue
        flat_area_unit_equiv = flat.area / (scale ** 2)  # back to unit-space for comparison
        area_diff_pct = 100 * abs(flat_area_unit_equiv - src_area) / src_area if src_area else 0.0
        if area_diff_pct <= max_area_diff_pct:
            return d, {"method": label, "valid": True, "single_region": True, "area_diff_pct": round(area_diff_pct, 3)}
    # should be unreachable (last attempt is the guaranteed-safe fallback)
    d = polygon_to_path_d(geom, scale=scale, decimals=decimals, simplify_tol_units=None, use_bezier=False)
    return d, {"method": "emergency_fallback", "valid": True, "single_region": True, "area_diff_pct": 0.0}


def flatten_bezier_d_to_polygon(d, samples_per_segment=8):
    """
    Parse this module's own 'M x,y C c1 c2 p2 C ... Z (M ... Z)*' output
    format back into a Shapely Polygon by sampling each cubic Bezier
    segment. Used purely for validation - to confirm the curve-fit export
    didn't introduce self-intersections or disconnect the shape (Catmull-
    Rom fits can occasionally overshoot at sharp/sparse junctions), not
    used anywhere in the generation path itself.
    """
    tokens = d.replace(",", " ").split()
    i = 0
    rings = []
    cur_ring = []

    def rd(k):
        return float(tokens[k])

    while i < len(tokens):
        tok = tokens[i]
        if tok == "M":
            if cur_ring:
                rings.append(cur_ring)
            cur_ring = [(rd(i + 1), rd(i + 2))]
            i += 3
        elif tok == "C":
            p0 = cur_ring[-1]
            c1 = (rd(i + 1), rd(i + 2))
            c2 = (rd(i + 3), rd(i + 4))
            p2 = (rd(i + 5), rd(i + 6))
            for s in range(1, samples_per_segment + 1):
                t = s / samples_per_segment
                mt = 1 - t
                x = mt**3*p0[0] + 3*mt**2*t*c1[0] + 3*mt*t**2*c2[0] + t**3*p2[0]
                y = mt**3*p0[1] + 3*mt**2*t*c1[1] + 3*mt*t**2*c2[1] + t**3*p2[1]
                cur_ring.append((x, y))
            i += 7
        elif tok == "L":
            cur_ring.append((rd(i + 1), rd(i + 2)))
            i += 3
        elif tok == "Z":
            rings.append(cur_ring)
            cur_ring = []
            i += 1
        else:
            i += 1
    if cur_ring:
        rings.append(cur_ring)
    if not rings:
        return None
    poly = Polygon(rings[0], rings[1:])
    if not poly.is_valid:
        poly = poly.buffer(0)
    return poly


def count_path_commands(d):
    """Count M/L/C command tokens in a 'd' string - a proxy for path
    complexity / vertex count, used to quantify the line-segment reduction."""
    toks = d.split()
    return sum(1 for t in toks if t in ("M", "L", "C"))


CANVAS_MM = CANVAS * SCALE_MM_PER_UNIT   # 55 mm
RING_OUTER_DIAM_MM = 2 * R_OUT * SCALE_MM_PER_UNIT  # 50 mm
RING_THICKNESS_MM = (R_OUT - R_IN) * SCALE_MM_PER_UNIT  # 3 mm


def gobo_svg(geom, gobo_id=""):
    d, _diag = robust_curved_path_d(geom)
    c = f"{CANVAS_MM:.3f}"
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {c} {c}" '
        f'width="{c}mm" height="{c}mm">\n'
        f'<rect x="0" y="0" width="{c}" height="{c}" fill="#ffffff"/>\n'
        f'<path id="{gobo_id}" d="{d}" fill="#000000" fill-rule="evenodd"/>\n'
        f'</svg>\n'
    )
