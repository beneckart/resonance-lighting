#!/usr/bin/env python3
"""Convert a green-on-white bamboo photograph into a printable circular gobo.

The green bamboo is isolated by color so pale stock-photo watermarks are not
traced. Disconnected branch clusters are joined, photographic stems are
thickened for FDM printing, and the oversized composition is circularly cropped
before it is boolean-unioned with six ring attachments and the production ring.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

from repair_bamboo_fan import map_by_height, source_geometry
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
    parser.add_argument("source", type=Path, help="green bamboo on a light background")
    parser.add_argument("output", type=Path, help="output SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument(
        "--green-excess",
        type=int,
        default=45,
        help="minimum green-channel lead over both red and blue",
    )
    parser.add_argument("--minimum-component-pixels", type=int, default=50)
    parser.add_argument(
        "--source-closing-pixels",
        type=int,
        default=0,
        help="close small highlight gaps in the color mask without widening its outline",
    )
    parser.add_argument(
        "--maximum-source-hole-pixels",
        type=int,
        default=0,
        help="fill enclosed highlight holes up to this source-image area",
    )
    parser.add_argument(
        "--art-height",
        type=float,
        default=44.5,
        help="uncropped bamboo height in mm",
    )
    parser.add_argument("--internal-bridge-width", type=float, default=0.9, help="mm")
    parser.add_argument(
        "--minimum-stem-width",
        type=float,
        default=1.4,
        help="minimum centerline band applied to every stem and connector, in mm",
    )
    parser.add_argument(
        "--centerline-end-prune",
        type=float,
        default=3.0,
        help="preserve tapered leaf tips by pruning this much centerline, in mm",
    )
    parser.add_argument(
        "--thicken-radius",
        type=float,
        default=0.10,
        help="outward thickening radius for fine photographic stems, in mm",
    )
    parser.add_argument("--ring-bridge-width", type=float, default=1.4, help="mm")
    parser.add_argument("--ring-bridge-count", type=int, default=6)
    parser.add_argument(
        "--ring-bridge-angles",
        help="optional comma-separated attachment angles in degrees; -90 is noon",
    )
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def green_mask(source: Path, green_excess: int) -> np.ndarray:
    rgb = np.asarray(Image.open(source).convert("RGB"), dtype=np.int16)
    red, green, blue = (rgb[:, :, channel] for channel in range(3))
    return (
        (green - red >= green_excess)
        & (green - blue >= green_excess)
        & (green >= 45)
    )


def fill_small_holes(mask: np.ndarray, maximum_pixels: int) -> np.ndarray:
    if maximum_pixels <= 0:
        return mask
    result = mask.copy()
    height, width = mask.shape
    for component in label_components(~mask, 1):
        if len(component) > maximum_pixels:
            continue
        ys = component[:, 0]
        xs = component[:, 1]
        if np.any(ys == 0) or np.any(ys == height - 1) or np.any(xs == 0) or np.any(xs == width - 1):
            continue
        result[ys, xs] = True
    return result


def thicken(mask: np.ndarray, radius_mm: float, pixels_per_mm: float) -> np.ndarray:
    radius_px = max(0, int(round(radius_mm * pixels_per_mm)))
    if radius_px == 0:
        return mask
    size = 2 * radius_px + 1
    return np.asarray(
        Image.fromarray(mask).filter(ImageFilter.MaxFilter(size)), dtype=bool
    ).copy()


def skeletonize(mask: np.ndarray) -> np.ndarray:
    """Return a one-pixel Zhang-Suen centerline without extra dependencies."""
    work = mask.copy()
    while True:
        changed = False
        for first_pass in (True, False):
            padded = np.pad(work, 1, mode="constant")
            p2 = padded[:-2, 1:-1]
            p3 = padded[:-2, 2:]
            p4 = padded[1:-1, 2:]
            p5 = padded[2:, 2:]
            p6 = padded[2:, 1:-1]
            p7 = padded[2:, :-2]
            p8 = padded[1:-1, :-2]
            p9 = padded[:-2, :-2]
            neighbors = (p2, p3, p4, p5, p6, p7, p8, p9)
            count = sum(neighbors)
            transitions = sum(
                (~neighbors[index] & neighbors[(index + 1) % 8])
                for index in range(8)
            )
            remove = work & (count >= 2) & (count <= 6) & (transitions == 1)
            if first_pass:
                remove &= ~(p2 & p4 & p6) & ~(p4 & p6 & p8)
            else:
                remove &= ~(p2 & p4 & p8) & ~(p2 & p6 & p8)
            if np.any(remove):
                work[remove] = False
                changed = True
        if not changed:
            return work


def prune_endpoints(skeleton: np.ndarray, iterations: int) -> np.ndarray:
    """Shorten terminal centerlines so reinforcement does not blunt leaf tips."""
    result = skeleton.copy()
    for _ in range(iterations):
        padded = np.pad(result, 1, mode="constant")
        neighbors = np.zeros_like(result, dtype=np.uint8)
        for row in range(3):
            for column in range(3):
                if row == 1 and column == 1:
                    continue
                neighbors += padded[row : row + result.shape[0], column : column + result.shape[1]]
        endpoints = result & (neighbors <= 1)
        if not np.any(endpoints):
            break
        result[endpoints] = False
    return result


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
        raise ValueError("ID must be positive and smaller than OD")
    selected_ring_angles = None
    if args.ring_bridge_angles:
        selected_ring_angles = [
            float(value.strip())
            for value in args.ring_bridge_angles.split(",")
            if value.strip()
        ]
        if not selected_ring_angles:
            raise ValueError("ring bridge angle list cannot be empty")
    elif args.ring_bridge_count < 5:
        raise ValueError("at least five ring bridges are required")

    raw_mask = green_mask(args.source, args.green_excess)
    if args.source_closing_pixels > 0:
        filter_size = 2 * args.source_closing_pixels + 1
        raw_mask = np.asarray(
            Image.fromarray(raw_mask)
            .filter(ImageFilter.MaxFilter(filter_size))
            .filter(ImageFilter.MinFilter(filter_size)),
            dtype=bool,
        ).copy()
    raw_mask = fill_small_holes(raw_mask, args.maximum_source_hole_pixels)
    components = label_components(raw_mask, args.minimum_component_pixels)
    if not components:
        raise ValueError("no meaningful green bamboo components found")
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
    artwork = thicken(artwork, args.thicken_radius, args.pixels_per_mm)
    stem_radius_px = 0
    if args.minimum_stem_width > 0:
        source_centerlines = skeletonize(connected_source)
        endpoint_prune_px = max(0, int(round(args.centerline_end_prune / scale)))
        source_centerlines = prune_endpoints(source_centerlines, endpoint_prune_px)
        stem_centerlines, _ = map_by_height(
            source_centerlines,
            cx,
            cy,
            source_height,
            args.art_height,
            args.od,
            args.pixels_per_mm,
        )
        # One extra working pixel per side protects the requested minimum width from
        # antialiasing and simplification when the mask is traced at lower resolution.
        stem_radius_px = max(
            1, int(np.ceil(args.minimum_stem_width * args.pixels_per_mm / 2.0)) + 1
        )
        stem_network = np.asarray(
            Image.fromarray(stem_centerlines).filter(
                ImageFilter.MaxFilter(2 * stem_radius_px + 1)
            ),
            dtype=bool,
        )
        artwork |= stem_network

    inner_radius = args.id / 2.0
    outer_radius = args.od / 2.0
    side = artwork.shape[0]
    center = side / 2.0
    yy, xx = np.indices(artwork.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)

    # Fit by height rather than shrinking the wide branch to a square. Natural
    # leaf and stem ends are clipped at the 51 mm opening and become organic
    # ring connections where they already reach the circle.
    artwork &= radii <= inner_radius
    artwork_with_ties, tie_specs = add_ring_bridges(
        artwork,
        args.ring_bridge_count,
        args.ring_bridge_width,
        inner_radius,
        args.pixels_per_mm,
        desired_angles_deg=selected_ring_angles,
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
    if args.minimum_stem_width > 0:
        stem_description = (
            f"Every photographic stem and connector is reinforced to at least "
            f"{args.minimum_stem_width:.1f} mm."
        )
    else:
        stem_description = "The natural photographic stalk and leaf widths are retained."
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>Bamboo branch gobo - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. {stem_description} The wide branch is circularly cropped.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    longest_gap_mm = (
        max(distance for distance, _a, _b in mst_bridges) * scale
        if mst_bridges
        else 0.0
    )
    print(f"source bamboo components: {len(components)}")
    print(f"internal bridges: {len(mst_bridges)} at {args.internal_bridge_width:.2f} mm")
    print(f"longest repaired source gap: {longest_gap_mm:.2f} mm")
    print(f"photographic stem thickening: {args.thicken_radius:.2f} mm per side")
    if stem_radius_px:
        print(
            f"minimum stem/connector band: "
            f"{(2 * stem_radius_px + 1) / args.pixels_per_mm:.2f} mm raster nominal"
        )
        print(f"tapered centerline end preservation: {args.centerline_end_prune:.2f} mm")
    else:
        print("minimum stem/connector band: disabled; natural source widths retained")
    print(
        f"uncropped artwork: {uncropped_width_mm:.2f} mm wide x "
        f"{args.art_height:.2f} mm high; perimeter ends circularly cropped"
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
