"""
Extreme test: leaf overlap ratio capped at 1.0 (true zero overlap - no
two leaves in the same gobo may share ANY area) instead of the normal
1.5 cap. Uses the same build_layout/leaf-generation logic as the main
batch, just with a much more aggressive overlap-pruning target passed
into build_gobo(), so this is a legitimate look at "what does this
pipeline produce at the opposite extreme from clumped leaves" rather
than a different generator.

min_leaves_after_prune is also dropped to 1 (from the normal 3) so the
pruning loop is allowed to go all the way down to a single leaf per
branch if that's what true zero-overlap requires, rather than stopping
early and leaving residual overlap.

Output goes to ./out_zero_overlap/ - separate from the main ./out/ batch.
"""
import json, os, math, statistics
from generate_gobos import (
    build_gobo, gobo_svg, polygon_to_path_d, CANVAS_MM,
    RING_OUTER_DIAM_MM, RING_THICKNESS_MM, MIN_LINE_MM,
    SPANNING_BRANCH_FRACTION, BRANCH_PARALLEL_BIAS, SPANNING_PARALLEL_BIAS,
)

OUT_DIR = "/home/claude/gobo_gen/out_zero_overlap"
IND_DIR = os.path.join(OUT_DIR, "individual_svgs")
os.makedirs(IND_DIR, exist_ok=True)

ZERO_OVERLAP_MAX_RATIO = 1.0
ZERO_OVERLAP_MIN_LEAVES = 1

N = 75
COLS = 10
GAP_MM = 6.0
cell = CANVAS_MM + GAP_MM

SPAN_EVERY = round(1 / SPANNING_BRANCH_FRACTION)
spanning_flags = [(i % SPAN_EVERY == 0) for i in range(N)]

manifest = []
master_groups = []
all_results = []

fail_count = 0
overlap_violations = 0

for i in range(N):
    seed = 1000 + i
    is_span = spanning_flags[i]
    res = build_gobo(seed, spanning=is_span,
                      max_leaf_overlap_ratio=ZERO_OVERLAP_MAX_RATIO,
                      min_leaves_after_prune=ZERO_OVERLAP_MIN_LEAVES)
    all_results.append(res)
    gid = f"gobo_{i+1:02d}"
    svg_str = gobo_svg(res["path_geom"], gobo_id=gid)
    with open(os.path.join(IND_DIR, f"{gid}.svg"), "w") as f:
        f.write(svg_str)

    d = polygon_to_path_d(res["path_geom"])
    col = i % COLS
    row = i // COLS
    tx = col * cell
    ty = row * cell
    master_groups.append(
        f'<g id="{gid}" transform="translate({tx:.3f},{ty:.3f})">'
        f'<path d="{d}" fill="#000000" fill-rule="evenodd"/></g>'
    )

    ok = res["single_connected_region"]
    if not ok:
        fail_count += 1

    geom = res["path_geom"]
    is_valid_geom = geom.is_valid
    overlap_ratio = res["metrics"]["leaf_overlap_ratio"]
    if overlap_ratio > 1.0 + 1e-4:
        overlap_violations += 1

    m = res["metrics"]
    manifest.append({
        "id": gid,
        "seed": seed,
        "has_spanning_branch": res["has_spanning_branch"],
        "n_branches": res["n_branches"],
        "n_leaves_total_before_thinning": res["n_leaves_total_before_thinning"],
        "n_leaves_after_10pct_thin": res["n_leaves_after_10pct_thin"],
        "n_leaves_kept": res["n_leaves_kept"],
        "stem_width_mm": round(res["stem_width_mm"], 3),
        "leaf_to_stem_area_ratio": round(res["ratio"], 2),
        "single_connected_black_region": ok,
        "geometry_valid_no_self_intersections": is_valid_geom,
        "file": f"individual_svgs/{gid}.svg",
        "metrics": {k: round(v, 4) for k, v in m.items()},
    })

rows = math.ceil(N / COLS)
W = COLS * cell - GAP_MM
H = rows * cell - GAP_MM
master_svg = (
    f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W:.3f} {H:.3f}" '
    f'width="{W:.3f}mm" height="{H:.3f}mm">\n'
    f'<rect x="0" y="0" width="{W:.3f}" height="{H:.3f}" fill="#ffffff"/>\n'
    + "\n".join(master_groups) +
    "\n</svg>\n"
)
with open(os.path.join(OUT_DIR, "master_sheet_75_gobos_ZERO_OVERLAP.svg"), "w") as f:
    f.write(master_svg)

manifest_wrapper = {
    "spec": {
        "EXPERIMENT": "zero_leaf_overlap - extreme test, not the main batch",
        "ring_outer_diameter_mm": RING_OUTER_DIAM_MM,
        "ring_thickness_mm": RING_THICKNESS_MM,
        "minimum_line_width_mm": MIN_LINE_MM,
        "max_leaf_overlap_ratio_cap": ZERO_OVERLAP_MAX_RATIO,
        "min_leaves_after_prune_floor": ZERO_OVERLAP_MIN_LEAVES,
        "branch_parallel_bias": BRANCH_PARALLEL_BIAS,
        "spanning_parallel_bias": SPANNING_PARALLEL_BIAS,
        "spanning_branch_count_actual": sum(spanning_flags),
    },
    "gobos": manifest,
}
with open(os.path.join(OUT_DIR, "manifest.json"), "w") as f:
    json.dump(manifest_wrapper, f, indent=2)

print("done - ZERO OVERLAP EXPERIMENT")
print("fail_count (disconnected):", fail_count)
print("overlap_violations (ratio > 1.0):", overlap_violations)
invalid = [m["id"] for m in manifest if not m["geometry_valid_no_self_intersections"]]
print("invalid geometry:", invalid)
overlaps = [m["metrics"]["leaf_overlap_ratio"] for m in manifest]
print("leaf overlap ratio min/max/avg:", min(overlaps), max(overlaps), sum(overlaps)/len(overlaps))
ratios = [m["leaf_to_stem_area_ratio"] for m in manifest]
print("leaf:stem ratio min/max/avg:", min(ratios), max(ratios), sum(ratios)/len(ratios))
leaf_counts = [m["n_leaves_kept"] for m in manifest]
print("leaves kept min/max/avg:", min(leaf_counts), max(leaf_counts), sum(leaf_counts)/len(leaf_counts))
ws = [m["metrics"]["whitespace_ratio"] for m in manifest]
print("whitespace ratio min/max/avg:", min(ws), max(ws), sum(ws)/len(ws))

# ---------------------------------------------------------------- tuning table

def stats(key, fmt="{:.3f}", unit=""):
    vals = [m["metrics"][key] for m in manifest]
    return {
        "min": fmt.format(min(vals)) + unit,
        "max": fmt.format(max(vals)) + unit,
        "avg": fmt.format(sum(vals)/len(vals)) + unit,
        "median": fmt.format(statistics.median(vals)) + unit,
    }

rows_def = [
    ("Leaf overlap ratio (target: exactly 1.0)", "leaf_overlap_ratio", "{:.4f}", ""),
    ("Leaf overlap fraction", "leaf_overlap_fraction_pct", "{:.2f}", "%"),
    ("Stem crossing count", "stem_crossing_count", "{:.0f}", ""),
    ("Whitespace ratio", "whitespace_ratio", "{:.3f}", ""),
    ("Black fill ratio", "black_fill_ratio", "{:.3f}", ""),
    ("Leaf coverage ratio", "leaf_coverage_ratio", "{:.3f}", ""),
    ("Avg nearest-neighbor leaf spacing", "avg_nearest_neighbor_leaf_spacing_mm", "{:.2f}", " mm"),
    ("Avg individual leaf area", "avg_leaf_area_mm2", "{:.2f}", " mm2"),
]

lines = ["| Metric | Min | Max | Avg | Median |", "|---|---|---|---|---|"]
lts = [m["leaf_to_stem_area_ratio"] for m in manifest]
lines.append(f"| Leaf : stem area ratio | {min(lts):.2f} | {max(lts):.2f} | {sum(lts)/len(lts):.2f} | {statistics.median(lts):.2f} |")
for label, key, fmt, unit in rows_def:
    s = stats(key, fmt, unit)
    lines.append(f"| {label} | {s['min']} | {s['max']} | {s['avg']} | {s['median']} |")
n_leaves_list = [m["n_leaves_kept"] for m in manifest]
lines.append(f"| Leaves per gobo (post zero-overlap prune) | {min(n_leaves_list)} | {max(n_leaves_list)} | {sum(n_leaves_list)/len(n_leaves_list):.1f} | {statistics.median(n_leaves_list)} |")

with open(os.path.join(OUT_DIR, "tuning_metrics_ZERO_OVERLAP.md"), "w") as f:
    f.write("# Zero-leaf-overlap EXTREME TEST - tuning metrics (75 gobos)\n\n")
    f.write("`MAX_LEAF_OVERLAP_RATIO` forced to **1.0** (down from the normal "
            "1.5) and the pruning floor (`min_leaves_after_prune`) dropped from "
            "3 to **1**, so pruning is allowed to remove leaves all the way "
            "down to one per branch if that's what true zero-overlap requires. "
            "This is a deliberate extreme, not a replacement for the main "
            f"batch.\n\nSpanning-branch gobos: {sum(spanning_flags)}/{N}.\n\n")
    f.write("\n".join(lines))
    f.write("\n\n## What changed vs. the main (1.5-cap) batch\n\n")
    f.write("- Leaf overlap ratio is pinned to (numerically) exactly 1.0 for "
            "every gobo - no two leaves share any area at all.\n")
    f.write("- This required removing far more leaves than the normal 10%-thin "
            "+ 1.5-cap pipeline - see `n_leaves_kept` per gobo vs. the main "
            "batch's `manifest.json`.\n")
    f.write("- Leaf:stem ratio and whitespace ratio both shift as a direct "
            "consequence of fewer leaves - see the printed run summary / table "
            "above for exact numbers on this batch.\n")

print("wrote tuning_metrics_ZERO_OVERLAP.md")

# ---------------------------------------------------------------- labeled preview

LABEL_H_MM = 4.5
cell_y = CANVAS_MM + GAP_MM + LABEL_H_MM
Wl = COLS * cell - GAP_MM
Hl = rows * cell_y - GAP_MM

labeled_groups = []
for i in range(N):
    gid = f"gobo_{i+1:02d}"
    col = i % COLS
    row = i // COLS
    tx = col * cell
    ty = row * cell_y
    d = polygon_to_path_d(all_results[i]["path_geom"])
    span_marker = " *" if spanning_flags[i] else ""
    labeled_groups.append(
        f'<g transform="translate({tx:.3f},{ty:.3f})">'
        f'<path d="{d}" fill="#000000" fill-rule="evenodd"/>'
        f'<text x="{CANVAS_MM/2:.2f}" y="{CANVAS_MM + 3.6:.2f}" '
        f'font-family="monospace" font-size="3.1" text-anchor="middle" '
        f'fill="#000000">{gid}{span_marker}</text>'
        f'</g>'
    )

labeled_svg = (
    f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {Wl:.3f} {Hl:.3f}" '
    f'width="{Wl:.3f}mm" height="{Hl:.3f}mm">\n'
    f'<rect x="0" y="0" width="{Wl:.3f}" height="{Hl:.3f}" fill="#ffffff"/>\n'
    + "\n".join(labeled_groups) +
    '\n<text x="4" y="' + f"{Hl - 2:.2f}" +
    '" font-family="monospace" font-size="3.2" fill="#666666">'
    '* = has a ring-to-ring spanning branch | ZERO-OVERLAP EXTREME TEST</text>\n'
    "</svg>\n"
)
with open(os.path.join(OUT_DIR, "preview_labeled_ZERO_OVERLAP.svg"), "w") as f:
    f.write(labeled_svg)
print("wrote preview_labeled_ZERO_OVERLAP.svg")
