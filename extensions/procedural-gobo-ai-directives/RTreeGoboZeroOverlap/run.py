import json, os, math, statistics
from generate_gobos import (
    build_gobo, gobo_svg, polygon_to_path_d, CANVAS_MM,
    RING_OUTER_DIAM_MM, RING_THICKNESS_MM, MIN_LINE_MM,
    SPANNING_BRANCH_FRACTION, MAX_LEAF_OVERLAP_RATIO,
    BRANCH_PARALLEL_BIAS, SPANNING_PARALLEL_BIAS,
    flatten_bezier_d_to_polygon, count_path_commands, SCALE_MM_PER_UNIT,
    robust_curved_path_d, MIN_LEAVES_FLOOR,
)

OUT_DIR = "/home/claude/gobo_gen/out"
IND_DIR = os.path.join(OUT_DIR, "individual_svgs")
os.makedirs(IND_DIR, exist_ok=True)

N = 75
COLS = 10
GAP_MM = 6.0
cell = CANVAS_MM + GAP_MM

# exactly 1/4 of gobos (rounded) get a ring-to-ring spanning branch,
# evenly spread through the batch rather than clustered at the start
SPAN_EVERY = round(1 / SPANNING_BRANCH_FRACTION)  # = 4
spanning_flags = [(i % SPAN_EVERY == 0) for i in range(N)]

manifest = []
master_groups = []
all_results = []  # cache so the labeled preview doesn't recompute (overlap pruning isn't cheap)
all_d_strings = []  # cache the validated curved path 'd' per gobo too

fail_count = 0
ratio_out_of_band = 0

for i in range(N):
    seed = 1000 + i
    is_span = spanning_flags[i]
    res = build_gobo(seed, spanning=is_span)
    all_results.append(res)
    gid = f"gobo_{i+1:02d}"
    svg_str = gobo_svg(res["path_geom"], gobo_id=gid)
    with open(os.path.join(IND_DIR, f"{gid}.svg"), "w") as f:
        f.write(svg_str)

    d, curve_diag = robust_curved_path_d(res["path_geom"])
    d_straight_unsimplified = polygon_to_path_d(res["path_geom"], simplify_tol_units=None, use_bezier=False)
    n_commands_bezier = count_path_commands(d)
    n_commands_straight = count_path_commands(d_straight_unsimplified)

    # robust_curved_path_d already validates internally (flattens the fit
    # back to a polygon and confirms single/valid/close-in-area before
    # returning it, backing off to gentler curves or straight lines if
    # not) - curve_diag reports which attempt succeeded for this gobo.
    bezier_export_valid = curve_diag["valid"]
    bezier_export_single = curve_diag["single_region"]
    bezier_area_diff_pct = curve_diag["area_diff_pct"]
    curve_fit_method = curve_diag["method"]
    all_d_strings.append(d)

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
    in_band = 6.0 <= res["ratio"] <= 12.0
    if not in_band:
        ratio_out_of_band += 1

    geom = res["path_geom"]
    is_valid_geom = geom.is_valid
    n_holes = len(geom.interiors) if geom.geom_type == "Polygon" else -1

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
        "baseline_stem_width_mm_before_sturdier_mult": round(res["baseline_stem_width_mm"], 3),
        "leaf_to_stem_area_ratio": round(res["ratio"], 2),
        "aesthetic_target_ratio_baseline": round(res["target_ratio_aesthetic_baseline"], 2),
        "single_connected_black_region": ok,
        "geometry_valid_no_self_intersections": is_valid_geom,
        "n_interior_holes": n_holes,
        "file": f"individual_svgs/{gid}.svg",
        "metrics": {k: round(v, 4) for k, v in m.items()},
        "svg_path_commands_bezier": n_commands_bezier,
        "svg_path_commands_straight_unsimplified": n_commands_straight,
        "svg_path_command_reduction_pct": round(100 * (1 - n_commands_bezier / n_commands_straight), 1) if n_commands_straight else 0,
        "bezier_export_valid": bezier_export_valid,
        "bezier_export_single_region": bezier_export_single,
        "bezier_area_diff_pct_vs_source": round(bezier_area_diff_pct, 3),
        "curve_fit_method": curve_fit_method,
        "content_floor_met": res["content_floor_met"],
        "content_floor_attempts": res["content_floor_attempts"],
        "density_boost_used": res["density_boost_used"],
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
with open(os.path.join(OUT_DIR, "master_sheet_75_gobos.svg"), "w") as f:
    f.write(master_svg)

manifest_wrapper = {
    "spec": {
        "ring_outer_diameter_mm": RING_OUTER_DIAM_MM,
        "ring_thickness_mm": RING_THICKNESS_MM,
        "minimum_line_width_mm": MIN_LINE_MM,
        "stem_sturdier_multiplier_applied": 1.25,
        "leaf_density_kept_fraction": 0.90,
        "leaf_length_multiplier_applied": 1.30,
        "leaf_symmetry_multiplier_applied": 1.30,
        "max_leaf_overlap_ratio_cap": MAX_LEAF_OVERLAP_RATIO,
        "spanning_branch_fraction_target": SPANNING_BRANCH_FRACTION,
        "spanning_branch_count_actual": sum(spanning_flags),
        "branch_parallel_bias": BRANCH_PARALLEL_BIAS,
        "spanning_parallel_bias": SPANNING_PARALLEL_BIAS,
        "min_leaves_content_floor": MIN_LEAVES_FLOOR,
    },
    "gobos": manifest,
}
with open(os.path.join(OUT_DIR, "manifest.json"), "w") as f:
    json.dump(manifest_wrapper, f, indent=2)

print("done")
print("fail_count (disconnected):", fail_count)
print("ratio_out_of_band:", ratio_out_of_band)
invalid = [m["id"] for m in manifest if not m["geometry_valid_no_self_intersections"]]
print("invalid geometry (self-intersecting):", invalid)
ratios = [m["leaf_to_stem_area_ratio"] for m in manifest]
print("ratio min/max/avg:", min(ratios), max(ratios), sum(ratios)/len(ratios))
widths = [m["stem_width_mm"] for m in manifest]
print("stem width mm min/max:", min(widths), max(widths))
print("ring outer diam mm:", RING_OUTER_DIAM_MM, "ring thickness mm:", RING_THICKNESS_MM)
overlaps = [m["metrics"]["leaf_overlap_ratio"] for m in manifest]
print("leaf overlap ratio min/max:", min(overlaps), max(overlaps), "(cap =", MAX_LEAF_OVERLAP_RATIO, ")")
print("spanning-branch gobos:", sum(spanning_flags), "/", N)

bezier_valid_count = sum(1 for m in manifest if m["bezier_export_valid"] and m["bezier_export_single_region"])
cmd_reductions = [m["svg_path_command_reduction_pct"] for m in manifest]
area_diffs = [m["bezier_area_diff_pct_vs_source"] for m in manifest]
print("bezier export valid+single:", bezier_valid_count, "/", N)
print("path command reduction % min/max/avg:", min(cmd_reductions), max(cmd_reductions), sum(cmd_reductions)/len(cmd_reductions))
print("bezier area diff % vs source min/max/avg:", min(area_diffs), max(area_diffs), sum(area_diffs)/len(area_diffs))
method_counts = {}
for m in manifest:
    method_counts[m["curve_fit_method"]] = method_counts.get(m["curve_fit_method"], 0) + 1
print("curve-fit method breakdown:", method_counts)

floor_met_count = sum(1 for m in manifest if m["content_floor_met"])
retried_count = sum(1 for m in manifest if m["content_floor_attempts"] > 1)
n_leaves_after = [m["n_leaves_kept"] for m in manifest]
print(f"content floor (>= {MIN_LEAVES_FLOOR} leaves) met:", floor_met_count, "/", N)
print("gobos that needed a retry:", retried_count, "/", N)
print("leaves per gobo min/max/avg after floor pass:", min(n_leaves_after), max(n_leaves_after), sum(n_leaves_after)/len(n_leaves_after))

# ---------------------------------------------------------------- tuning table

def stats(key, fmt="{:.3f}", scale=1.0, unit=""):
    vals = [m["metrics"][key] * scale for m in manifest]
    return {
        "min": fmt.format(min(vals)) + unit,
        "max": fmt.format(max(vals)) + unit,
        "avg": fmt.format(sum(vals)/len(vals)) + unit,
        "median": fmt.format(statistics.median(vals)) + unit,
    }

rows_def = [
    (f"Leaf overlap ratio (sum/union, 1.0=no overlap, capped at {MAX_LEAF_OVERLAP_RATIO})", "leaf_overlap_ratio", "{:.2f}", ""),
    ("Leaf overlap fraction (% of leaf area lost to overlap)", "leaf_overlap_fraction_pct", "{:.1f}", "%"),
    ("Stem crossing count (proper intersections between branches)", "stem_crossing_count", "{:.0f}", ""),
    ("Whitespace ratio (white / full disk)", "whitespace_ratio", "{:.3f}", ""),
    ("Black fill ratio (black / full disk)", "black_fill_ratio", "{:.3f}", ""),
    ("Leaf coverage ratio (leaf union / disk)", "leaf_coverage_ratio", "{:.3f}", ""),
    ("Stem coverage ratio (stem union / disk)", "stem_coverage_ratio", "{:.3f}", ""),
    ("Ring coverage of disk", "ring_coverage_of_disk_pct", "{:.1f}", "%"),
    ("Avg nearest-neighbor leaf spacing", "avg_nearest_neighbor_leaf_spacing_mm", "{:.2f}", " mm"),
    ("Avg individual leaf area", "avg_leaf_area_mm2", "{:.2f}", " mm2"),
]

lines = []
lines.append("| Metric | Min | Max | Avg | Median |")
lines.append("|---|---|---|---|---|")

# leaf:stem ratio row (top-level field, not nested in metrics)
lts = [m["leaf_to_stem_area_ratio"] for m in manifest]
lines.append(f"| Leaf : stem area ratio | {min(lts):.2f} | {max(lts):.2f} | {sum(lts)/len(lts):.2f} | {statistics.median(lts):.2f} |")

for label, key, fmt, unit in rows_def:
    s = stats(key, fmt, 1.0, unit)
    lines.append(f"| {label} | {s['min']} | {s['max']} | {s['avg']} | {s['median']} |")

n_branches_list = [m["n_branches"] for m in manifest]
n_leaves_list = [m["n_leaves_kept"] for m in manifest]
lines.append(f"| Branches per gobo (count) | {min(n_branches_list)} | {max(n_branches_list)} | {sum(n_branches_list)/len(n_branches_list):.1f} | {statistics.median(n_branches_list)} |")
lines.append(f"| Leaves per gobo (count, post-thinning + overlap prune) | {min(n_leaves_list)} | {max(n_leaves_list)} | {sum(n_leaves_list)/len(n_leaves_list):.1f} | {statistics.median(n_leaves_list)} |")

sw_list = [m["stem_width_mm"] for m in manifest]
lines.append(f"| Stem width | {min(sw_list):.2f} mm | {max(sw_list):.2f} mm | {sum(sw_list)/len(sw_list):.2f} mm | {statistics.median(sw_list):.2f} mm |")
vc_list = [m["svg_path_commands_bezier"] for m in manifest]
lines.append(f"| SVG path anchor/vertex count | {min(vc_list)} | {max(vc_list)} | {sum(vc_list)/len(vc_list):.1f} | {statistics.median(vc_list)} |")

table_md = "\n".join(lines)
with open(os.path.join(OUT_DIR, "tuning_metrics.md"), "w") as f:
    f.write("# Tuning metrics - current batch (75 gobos)\n\n")
    f.write("Aggregate stats across all 75 designs. Use this to steer future ")
    f.write("passes (e.g. \"push whitespace ratio up\", \"reduce leaf overlap ")
    f.write("fraction\").\n\n")
    f.write(f"Spanning-branch gobos (touch the ring at two points): "
            f"{sum(spanning_flags)}/{N} ({100*sum(spanning_flags)/N:.1f}%).\n\n")
    f.write(table_md)
    f.write("\n\n## Definitions\n\n")
    f.write("- **Leaf overlap ratio**: sum of each individual leaf's own area, "
            "divided by the area of all leaves unioned together. 1.0 means no "
            "two leaves overlap at all; higher means leaves are stacking on "
            "top of each other (reduces individual leaf visibility). This pass "
            f"actively prunes the most-overlapping leaves until this hits {MAX_LEAF_OVERLAP_RATIO} "
            "or below, rather than just reporting whatever came out.\n")
    f.write("- **Leaf overlap fraction**: same idea, expressed as % of raw leaf "
            "area that disappears into overlap once unioned.\n")
    f.write("- **Whitespace ratio**: fraction of the full gobo disk that ends "
            "up white (open) in the final cut/print, vs black.\n")
    f.write("- **Leaf coverage ratio / stem coverage ratio**: fraction of the "
            "full disk area occupied by leaves alone / stems alone (pre-union "
            "with the ring), useful for tuning density independent of the "
            "overlap metric.\n")
    f.write("- **Avg nearest-neighbor leaf spacing**: average distance (mm) "
            "from each leaf's centroid to its closest neighboring leaf's "
            "centroid - a rough proxy for how visually separated individual "
            "leaves are. Larger = more spread out.\n")
print("wrote tuning_metrics.md")

# ---------------------------------------------------------------- labeled preview

LABEL_H_MM = 8.0
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
    d = all_d_strings[i]
    span_marker = " *" if spanning_flags[i] else ""
    vcount = manifest[i]["svg_path_commands_bezier"]
    labeled_groups.append(
        f'<g transform="translate({tx:.3f},{ty:.3f})">'
        f'<path d="{d}" fill="#000000" fill-rule="evenodd"/>'
        f'<text x="{CANVAS_MM/2:.2f}" y="{CANVAS_MM + 3.6:.2f}" '
        f'font-family="monospace" font-size="3.1" text-anchor="middle" '
        f'fill="#000000">{gid}{span_marker}</text>'
        f'<text x="{CANVAS_MM/2:.2f}" y="{CANVAS_MM + 7.2:.2f}" '
        f'font-family="monospace" font-size="2.6" text-anchor="middle" '
        f'fill="#666666">{vcount} pts</text>'
        f'</g>'
    )

labeled_svg = (
    f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {Wl:.3f} {Hl:.3f}" '
    f'width="{Wl:.3f}mm" height="{Hl:.3f}mm">\n'
    f'<rect x="0" y="0" width="{Wl:.3f}" height="{Hl:.3f}" fill="#ffffff"/>\n'
    + "\n".join(labeled_groups) +
    '\n<text x="4" y="' + f"{Hl - 2:.2f}" +
    '" font-family="monospace" font-size="3.2" fill="#666666">'
    '* = has a ring-to-ring spanning branch | pts = SVG path anchor/vertex count</text>\n'
    "</svg>\n"
)
with open(os.path.join(OUT_DIR, "preview_labeled.svg"), "w") as f:
    f.write(labeled_svg)
print("wrote preview_labeled.svg")

