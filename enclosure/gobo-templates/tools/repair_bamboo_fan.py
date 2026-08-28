#!/usr/bin/env python3
"""Convert the wide bamboo-2 stencil artwork into a circular printable gobo.

The source is fitted by height so its wide left/right leaf tips extend past the
51 mm opening and are deliberately cropped by the circle. Disconnected leaves
and stem fragments are joined with a minimum-length network before the artwork,
ring ties, and outer ring are boolean-unioned into one Fusion-ready SVG path.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image

from repair_bamboo_stencil import (
    SVG_UNITS_PER_MM,
    add_ring_bridges,
    component_mask,
    connect_source_artwork,
    label_components,
    minimum_spanning_bridges,
    trace_printed_solid,
    write_preview,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="dark-on-light bamboo fan image")
    parser.add_argument("output", type=Path, help="output SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument("--minimum-component-pixels", type=int, default=20)
    parser.add_argument(
        "--art-height",
        type=float,
        default=48.0,
        help="uncropped black-artwork height in mm",
    )
    parser.add_argument("--internal-bridge-width", type=float, default=1.0, help="mm")
    parser.add_argument("--ring-bridge-width", type=float, default=1.4, help="mm")
    parser.add_argument("--ring-bridge-count", type=int, default=6)
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def source_geometry(mask: np.ndarray) -> tuple[float, float, int, int]:
    ys, xs = np.where(mask)
    cx = (float(xs.min()) + float(xs.max())) / 2.0
    cy = (float(ys.min()) + float(ys.max())) / 2.0
    width = int(xs.max() - xs.min() + 1)
    height = int(ys.max() - ys.min() + 1)
    return cx, cy, width, height


def map_by_height(
    source: np.ndarray,
    cx: float,
    cy: float,
    source_height: int,
    art_height_mm: float,
    od_mm: float,
    pixels_per_mm: float,
) -> tuple[np.ndarray, float]:
    scale = art_height_mm / source_height
    side = int(round(od_mm * pixels_per_mm))
    center = side / 2.0
    yy, xx = np.indices((side, side), dtype=float)
    x_mm = (xx + 0.5 - center) / pixels_per_mm
    y_mm = (yy + 0.5 - center) / pixels_per_mm
    source_x = np.rint(cx + x_mm / scale).astype(int)
    source_y = np.rint(cy + y_mm / scale).astype(int)
    valid = (
        (source_x >= 0)
        & (source_x < source.shape[1])
        & (source_y >= 0)
        & (source_y < source.shape[0])
    )
    target = np.zeros((side, side), dtype=bool)
    target[valid] = source[source_y[valid], source_x[valid]]
    return target, scale


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
        raise ValueError("ID must be positive and smaller than OD")
    if args.ring_bridge_count < 5:
        raise ValueError("at least five ring bridges are required")

    gray = np.asarray(Image.open(args.source).convert("L"))
    raw_mask = gray < args.threshold
    components = label_components(raw_mask, args.minimum_component_pixels)
    if not components:
        raise ValueError("no meaningful artwork components found")
    clean = component_mask(raw_mask.shape, components)
    cx, cy, source_width, source_height = source_geometry(clean)
    scale = args.art_height / source_height

    mst_bridges = minimum_spanning_bridges(components, raw_mask.shape)
    source_bridge_width_px = max(3, int(round(args.internal_bridge_width / scale)))
    connected_source = connect_source_artwork(clean, mst_bridges, source_bridge_width_px)
    repaired_components = label_components(connected_source, args.minimum_component_pixels)
    if len(repaired_components) != 1:
        raise RuntimeError(f"artwork repair left {len(repaired_components)} components")

    artwork, scale = map_by_height(
        connected_source,
        cx,
        cy,
        source_height,
        args.art_height,
        args.od,
        args.pixels_per_mm,
    )
    inner_radius = args.id / 2.0
    outer_radius = args.od / 2.0
    side = artwork.shape[0]
    center = side / 2.0
    yy, xx = np.indices(artwork.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)

    # The width is intentionally oversized. Circular clipping trims the left and
    # right leaf tips instead of distorting the leaf proportions to fit a square.
    artwork &= radii <= inner_radius
    artwork_with_ties, tie_specs = add_ring_bridges(
        artwork,
        args.ring_bridge_count,
        args.ring_bridge_width,
        inner_radius,
        args.pixels_per_mm,
    )
    ring = (radii <= outer_radius) & (radii >= inner_radius)
    disc = radii <= outer_radius
    blocked = ring | artwork_with_ties
    solid_components = label_components(blocked, 1)
    if len(solid_components) != 1:
        raise RuntimeError(
            f"ring/artwork boolean union left {len(solid_components)} printed components"
        )
    blocked_percent = 100.0 * np.count_nonzero(blocked & disc) / np.count_nonzero(disc)

    solid_d, openings, segments = trace_printed_solid(
        blocked,
        args.pixels_per_mm,
        args.trace_pixels_per_mm,
        args.od,
        args.trace_tolerance,
    )
    view_size = args.od * SVG_UNITS_PER_MM
    view_min = -view_size / 2.0
    uncropped_width_mm = source_width * scale
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>Bamboo fan gobo - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. The wide source is fitted to {args.art_height:g} mm high and its side leaf tips are circularly cropped.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    longest_gap_mm = max(distance for distance, _a, _b in mst_bridges) * scale
    print(f"source artwork components: {len(components)}")
    print(f"internal bridges: {len(mst_bridges)} at {args.internal_bridge_width:.2f} mm")
    print(f"longest repaired source gap: {longest_gap_mm:.2f} mm")
    print(
        f"uncropped artwork: {uncropped_width_mm:.2f} mm wide x "
        f"{args.art_height:.2f} mm high; side tips circularly cropped"
    )
    print(
        f"ring attachments: {len(tie_specs)} "
        f"({', '.join(f'{args.ring_bridge_width:.2f} mm' for _ in tie_specs)})"
    )
    print(f"blocked area: {blocked_percent:.2f}% of the {args.od:g} mm disc")
    print(f"unified solid: 1 exact outer contour, {openings} opening contours")
    print(f"trace complexity: {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
