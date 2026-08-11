#!/usr/bin/env python3
"""Add pointed openings to a finished gobo while preserving its exact ring.

The input is a square raster render of a finished gobo. Existing white openings
are retained, requested leaf-shaped openings are subtracted from the printed
interior, and any newly separated printed background regions are reconnected
with butt-ended passages before tracing one Fusion-ready path.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image

from negative_art_gobo import (
    add_pointed_leaf,
    connect_background_passages,
    parse_leaf,
    wavefront_bridges,
)
from repair_bamboo_stencil import (
    SVG_UNITS_PER_MM,
    component_mask,
    label_components,
    trace_printed_solid,
    write_preview,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="square raster of finished gobo")
    parser.add_argument("output", type=Path, help="output Fusion-ready SVG")
    parser.add_argument("--preview", type=Path, help="optional working-mask preview")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument("--printed-threshold", type=int, default=230)
    parser.add_argument(
        "--leaf",
        action="append",
        default=[],
        metavar="X,Y,LENGTH,WIDTH,ANGLE[,CURVE]",
        help="add a pointed open leaf; coordinates and dimensions are mm",
    )
    parser.add_argument("--minimum-component-pixels", type=int, default=4)
    parser.add_argument("--passage-width", type=float, default=1.4, help="mm")
    parser.add_argument("--trace-pixels-per-mm", type=float, default=12.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.1)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.id <= 0 or args.id >= args.od:
        raise ValueError("ID must be positive and smaller than OD")
    gray = np.asarray(Image.open(args.source).convert("L"))
    if gray.shape[0] != gray.shape[1]:
        raise ValueError("source render must be square")
    side = gray.shape[0]
    pixels_per_mm = side / args.od
    center = side / 2.0
    yy, xx = np.indices(gray.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / pixels_per_mm
    y_mm = (yy + 0.5 - center) / pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
    disc = radii <= args.od / 2.0
    inner_disc = radii < args.id / 2.0
    ring = disc & ~inner_disc

    source_printed = (gray < args.printed_threshold) & disc
    source_aperture_open = (
        100.0
        * np.count_nonzero((~source_printed) & inner_disc)
        / np.count_nonzero(inner_disc)
    )
    openings = (~source_printed) & inner_disc
    leaf_specs = [parse_leaf(value) for value in args.leaf]
    for spec in leaf_specs:
        add_pointed_leaf(openings, spec, pixels_per_mm)
    openings &= inner_disc
    initial = ring | (inner_disc & ~openings)

    components = label_components(initial, args.minimum_component_pixels)
    if not components:
        raise RuntimeError("opening edit produced no printed components")
    clean = component_mask(initial.shape, components)
    bridges = wavefront_bridges(components, initial.shape) if len(components) > 1 else []
    passage_width_px = max(3, int(round(args.passage_width * pixels_per_mm)))
    blocked = connect_background_passages(clean, bridges, passage_width_px) & disc
    final_components = label_components(blocked, 1)
    if len(final_components) != 1:
        raise RuntimeError(
            f"opening repair left {len(final_components)} printed components"
        )

    aperture_open = (
        100.0 * np.count_nonzero((~blocked) & inner_disc) / np.count_nonzero(inner_disc)
    )
    full_open = 100.0 * np.count_nonzero((~blocked) & disc) / np.count_nonzero(disc)
    blocked_percent = 100.0 - full_open
    solid_d, internal_contours, segments = trace_printed_solid(
        blocked,
        pixels_per_mm,
        args.trace_pixels_per_mm,
        args.od,
        args.trace_tolerance,
    )
    view_size = args.od * SVG_UNITS_PER_MM
    view_min = -view_size / 2.0
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>{args.output.stem} - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. Supplemental pointed openings increase light transmission while the exact ring remains printed.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, pixels_per_mm)

    print(f"supplemental open leaves: {len(leaf_specs)}")
    print(f"source aperture open area: {source_aperture_open:.2f}%")
    print(f"initial printed components after leaves: {len(components)}")
    print(f"background passages: {len(bridges)} at {args.passage_width:.2f} mm")
    print(
        "longest opened-art crossing: "
        f"{max((bridge[0] for bridge in bridges), default=0.0) / pixels_per_mm:.2f} mm"
    )
    print(f"{args.id:g} mm aperture open area: {aperture_open:.2f}%")
    print(f"aperture open gain: {aperture_open - source_aperture_open:+.2f} points")
    print(f"full-disc open area: {full_open:.2f}%")
    print(f"full-disc blocked area: {blocked_percent:.2f}%")
    print(f"unified solid: 1 exact outer contour, {internal_contours} internal contours")
    print(f"trace complexity: {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
