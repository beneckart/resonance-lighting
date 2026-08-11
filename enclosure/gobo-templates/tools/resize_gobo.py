#!/usr/bin/env python3
"""Refit a finished gobo to a different exact OD/ID ring interface.

The input is a square raster render of a finished gobo. Printed material inside
the source aperture is scaled by the aperture ratio, not the outside-diameter
ratio. The source ring is discarded and replaced with an exact target ring.
This preserves the original composition and ring contacts while allowing, for
example, a 55/51 mm design to become an exact 50/46 mm design.

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
    label_components,
    trace_printed_solid,
    write_preview,
)
from negative_art_gobo import connect_background_passages, wavefront_bridges


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="square raster of finished source gobo")
    parser.add_argument("output", type=Path, help="output Fusion-ready SVG")
    parser.add_argument("--preview", type=Path, help="optional working-mask preview")
    parser.add_argument("--source-od", type=float, default=55.0, help="source OD in mm")
    parser.add_argument("--source-id", type=float, default=51.0, help="source ID in mm")
    parser.add_argument("--target-od", type=float, default=50.0, help="target OD in mm")
    parser.add_argument("--target-id", type=float, default=46.0, help="target ID in mm")
    parser.add_argument("--printed-threshold", type=int, default=230)
    parser.add_argument("--minimum-component-pixels", type=int, default=4)
    parser.add_argument(
        "--repair-width",
        type=float,
        default=1.4,
        help="width in mm for reconnecting mapped sub-pixel contact gaps",
    )
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=12.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.1)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    for label, outer, inner in (
        ("source", args.source_od, args.source_id),
        ("target", args.target_od, args.target_id),
    ):
        if inner <= 0 or outer <= inner:
            raise ValueError(f"{label} ID must be positive and smaller than OD")

    gray = np.asarray(Image.open(args.source).convert("L"))
    if gray.shape[0] != gray.shape[1]:
        raise ValueError("source render must be square")
    source_side = gray.shape[0]
    source_ppm = source_side / args.source_od
    source_center = source_side / 2.0
    source_printed = gray < args.printed_threshold

    target_side = int(round(args.target_od * args.pixels_per_mm))
    target_center = target_side / 2.0
    yy, xx = np.indices((target_side, target_side), dtype=float)
    x_mm = (xx + 0.5 - target_center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - target_center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
    disc = radii <= args.target_od / 2.0
    inner_disc = radii < args.target_id / 2.0
    ring = disc & ~inner_disc

    aperture_scale = args.source_id / args.target_id
    source_x = np.rint(source_center + x_mm * aperture_scale * source_ppm).astype(int)
    source_y = np.rint(source_center + y_mm * aperture_scale * source_ppm).astype(int)
    valid = (
        inner_disc
        & (source_x >= 0)
        & (source_x < source_side)
        & (source_y >= 0)
        & (source_y < source_side)
    )
    artwork = np.zeros((target_side, target_side), dtype=bool)
    artwork[valid] = source_printed[source_y[valid], source_x[valid]]
    initial = ring | artwork

    components = label_components(initial, args.minimum_component_pixels)
    if not components:
        raise RuntimeError("resized ring/artwork produced no printed components")
    clean = component_mask(initial.shape, components)
    bridges = wavefront_bridges(components, initial.shape) if len(components) > 1 else []
    repair_width_px = max(3, int(round(args.repair_width * args.pixels_per_mm)))
    blocked = connect_background_passages(clean, bridges, repair_width_px) & disc
    final_components = label_components(blocked, 1)
    if len(final_components) != 1:
        raise RuntimeError(
            f"resized repair left {len(final_components)} printed components"
        )

    aperture_open = (
        100.0 * np.count_nonzero((~blocked) & inner_disc) / np.count_nonzero(inner_disc)
    )
    full_open = 100.0 * np.count_nonzero((~blocked) & disc) / np.count_nonzero(disc)
    blocked_percent = 100.0 - full_open

    solid_d, openings, segments = trace_printed_solid(
        blocked,
        args.pixels_per_mm,
        args.trace_pixels_per_mm,
        args.target_od,
        args.trace_tolerance,
    )
    view_size = args.target_od * SVG_UNITS_PER_MM
    view_min = -view_size / 2.0
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.target_od:g}mm" height="{args.target_od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>{args.output.stem} - {args.target_od:g} mm OD / {args.target_id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. The source interior was scaled by the aperture ratio and joined to an exact target ring.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    print(
        f"interface: {args.source_od:g}/{args.source_id:g} mm -> "
        f"{args.target_od:g}/{args.target_id:g} mm"
    )
    print(f"interior linear scale: {args.target_id / args.source_id:.6f}")
    print(
        f"mapped component repairs: {len(bridges)} at {args.repair_width:.2f} mm"
    )
    print(
        "longest mapped contact gap: "
        f"{max((bridge[0] for bridge in bridges), default=0.0) / args.pixels_per_mm:.2f} mm"
    )
    print(f"{args.target_id:g} mm aperture open area: {aperture_open:.2f}%")
    print(f"full-disc open area: {full_open:.2f}%")
    print(f"full-disc blocked area: {blocked_percent:.2f}%")
    print(f"unified solid: 1 exact outer contour, {openings} internal contours")
    print(f"trace complexity: {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
