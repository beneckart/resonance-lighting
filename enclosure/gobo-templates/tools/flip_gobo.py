#!/usr/bin/env python3
"""Invert a finished gobo's internal pattern while retaining its printed ring.

The input is a square raster render of a finished positive gobo. Printed gray or
black pixels inside the ring become openings; former white openings become the
new printed interior. Isolated negative-space regions are joined through the
narrowest original stalk, joint, and leaf contacts before the result is traced
as one Fusion-ready boolean-unioned path.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image

from repair_bamboo_stencil import (
    SVG_UNITS_PER_MM,
    component_mask,
    connect_source_artwork,
    label_components,
    minimum_spanning_bridges,
    trace_printed_solid,
    write_preview,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="square render of the positive gobo")
    parser.add_argument("output", type=Path, help="output negative-pattern SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument(
        "--printed-threshold",
        type=int,
        default=230,
        help="pixels darker than this are printed material in the source render",
    )
    parser.add_argument("--minimum-component-pixels", type=int, default=4)
    parser.add_argument("--bridge-width", type=float, default=1.4, help="mm")
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
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

    positive_artwork = (gray < args.printed_threshold) & inner_disc
    flipped_interior = inner_disc & ~positive_artwork
    initial = ring | flipped_interior
    components = label_components(initial, args.minimum_component_pixels)
    if not components:
        raise ValueError("flipped gobo produced no printed components")
    clean = component_mask(initial.shape, components)
    bridges = minimum_spanning_bridges(components, initial.shape)
    bridge_width_px = max(3, int(round(args.bridge_width * pixels_per_mm)))
    blocked = connect_source_artwork(clean, bridges, bridge_width_px) & disc
    final_components = label_components(blocked, 1)
    if len(final_components) != 1:
        raise RuntimeError(
            f"flipped repair left {len(final_components)} printed components"
        )

    full_open_percent = (
        100.0 * np.count_nonzero((~blocked) & disc) / np.count_nonzero(disc)
    )
    aperture_open_percent = (
        100.0
        * np.count_nonzero((~blocked) & inner_disc)
        / np.count_nonzero(inner_disc)
    )
    blocked_percent = 100.0 - full_open_percent

    solid_d, openings, segments = trace_printed_solid(
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
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. The outer ring remains printed while the source gobo's internal printed and open regions are inverted. Short repair passages connect every printed negative-space region.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, pixels_per_mm)

    longest_gap_mm = (
        max(distance for distance, _a, _b in bridges) / pixels_per_mm
        if bridges
        else 0.0
    )
    print(f"initial flipped printed components: {len(components)}")
    print(f"negative-space passages: {len(bridges)} at {args.bridge_width:.2f} mm")
    print(f"longest opened source contact: {longest_gap_mm:.2f} mm")
    print(f"full-disc open area: {full_open_percent:.2f}%")
    print(f"51 mm aperture open area: {aperture_open_percent:.2f}%")
    print(f"full-disc blocked area: {blocked_percent:.2f}%")
    print(f"unified solid: 1 exact outer contour, {openings} opening contours")
    print(f"trace complexity: {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
