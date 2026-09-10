#!/usr/bin/env python3
"""Crop a square black bamboo pattern into a connected circular gobo.

The input is a high-resolution, black-on-white render of the source SVG. The
black shapes are lightly reduced to meet the blocked-area target, disconnected
stalk and leaf islands are joined with minimum-length bridges, and the natural
circle-crop contacts are used instead of generated ring ties.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

from repair_bamboo_stencil import (
    SVG_UNITS_PER_MM,
    boundary_points,
    component_mask,
    connect_source_artwork,
    draw_rounded_line,
    label_components,
    minimum_spanning_bridges,
    trace_printed_solid,
    write_preview,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="square black-on-white SVG render")
    parser.add_argument("output", type=Path, help="output SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument("--erosion-passes", type=int, default=2)
    parser.add_argument("--minimum-component-pixels", type=int, default=20)
    parser.add_argument("--internal-bridge-width", type=float, default=0.8, help="mm")
    parser.add_argument("--redundant-bridge-count", type=int, default=0)
    parser.add_argument("--redundant-bridge-width", type=float, default=1.4, help="mm")
    parser.add_argument("--redundant-bridge-max-gap", type=float, default=4.0, help="mm")
    parser.add_argument("--redundant-min-component-pixels", type=int, default=1200)
    parser.add_argument(
        "--target-bridge",
        action="append",
        default=[],
        metavar="X1,Y1,X2,Y2,WIDTH",
        help="add a targeted bridge in centered millimeter coordinates; repeatable",
    )
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def map_square_to_opening(
    source: np.ndarray,
    id_mm: float,
    od_mm: float,
    pixels_per_mm: float,
) -> np.ndarray:
    source_side = source.shape[0]
    source_center = source_side / 2.0
    source_pixels_per_mm = source_side / id_mm
    target_side = int(round(od_mm * pixels_per_mm))
    target_center = target_side / 2.0
    yy, xx = np.indices((target_side, target_side), dtype=float)
    x_mm = (xx + 0.5 - target_center) / pixels_per_mm
    y_mm = (yy + 0.5 - target_center) / pixels_per_mm
    source_x = np.rint(source_center + x_mm * source_pixels_per_mm).astype(int)
    source_y = np.rint(source_center + y_mm * source_pixels_per_mm).astype(int)
    valid = (
        (source_x >= 0)
        & (source_x < source_side)
        & (source_y >= 0)
        & (source_y < source_side)
    )
    target = np.zeros((target_side, target_side), dtype=bool)
    target[valid] = source[source_y[valid], source_x[valid]]
    target &= np.hypot(x_mm, y_mm) <= id_mm / 2.0
    return target


def count_contact_zones(
    artwork: np.ndarray,
    inner_radius: float,
    pixels_per_mm: float,
) -> tuple[int, list[float]]:
    side = artwork.shape[0]
    center = side / 2.0
    angles = np.linspace(-np.pi, np.pi, 7200, endpoint=False)
    probe_radius = inner_radius - 0.20
    xs = np.clip(
        np.rint(center + probe_radius * pixels_per_mm * np.cos(angles)).astype(int),
        0,
        side - 1,
    )
    ys = np.clip(
        np.rint(center + probe_radius * pixels_per_mm * np.sin(angles)).astype(int),
        0,
        side - 1,
    )
    hits = artwork[ys, xs]
    if hits[0] and hits[-1]:
        first_gap = int(np.where(~hits)[0][0])
        hits = np.roll(hits, -first_gap)
        angles = np.roll(angles, -first_gap)

    centers: list[float] = []
    index = 0
    while index < len(hits):
        if not hits[index]:
            index += 1
            continue
        end = index
        while end < len(hits) and hits[end]:
            end += 1
        if end - index >= 10:
            midpoint = (index + end - 1) // 2
            centers.append((float(np.degrees(angles[midpoint])) + 360.0) % 360.0)
        index = end
    return len(centers), centers


def add_target_bridges(
    artwork: np.ndarray,
    specifications: list[str],
    pixels_per_mm: float,
    inner_radius: float,
) -> tuple[np.ndarray, list[tuple[float, float, float, float, float]]]:
    if not specifications:
        return artwork, []
    image = Image.fromarray(artwork)
    drawing = ImageDraw.Draw(image)
    center = artwork.shape[0] / 2.0
    parsed: list[tuple[float, float, float, float, float]] = []
    for specification in specifications:
        values = [float(value.strip()) for value in specification.split(",")]
        if len(values) != 5:
            raise ValueError(
                f"target bridge must contain x1,y1,x2,y2,width: {specification}"
            )
        x1, y1, x2, y2, width_mm = values
        if width_mm <= 0:
            raise ValueError("target bridge width must be positive")
        draw_rounded_line(
            drawing,
            (center + x1 * pixels_per_mm, center + y1 * pixels_per_mm),
            (center + x2 * pixels_per_mm, center + y2 * pixels_per_mm),
            max(3, int(round(width_mm * pixels_per_mm))),
        )
        parsed.append((x1, y1, x2, y2, width_mm))
    result = np.asarray(image, dtype=bool).copy()
    yy, xx = np.indices(result.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / pixels_per_mm
    y_mm = (yy + 0.5 - center) / pixels_per_mm
    result &= np.hypot(x_mm, y_mm) <= inner_radius
    return result, parsed


def find_redundant_bridges(
    components: list[np.ndarray],
    shape: tuple[int, int],
    existing: list[tuple[float, tuple[int, int], tuple[int, int]]],
    count: int,
    maximum_gap_px: float,
    minimum_component_pixels: int,
) -> list[tuple[float, tuple[int, int], tuple[int, int]]]:
    """Choose distributed, short cross-links between substantial central shapes."""
    if count <= 0:
        return []
    labels = np.zeros(shape, dtype=np.int16)
    for index, component in enumerate(components, start=1):
        labels[component[:, 0], component[:, 1]] = index
    boundaries = [
        boundary_points(component, labels, index)
        for index, component in enumerate(components, start=1)
    ]
    existing_endpoints = {
        tuple(sorted((point_a, point_b))) for _distance, point_a, point_b in existing
    }
    center_y = shape[0] / 2.0
    center_x = shape[1] / 2.0
    central_limit = min(shape) * 0.41
    candidates: list[tuple[float, tuple[int, int], tuple[int, int], float, float]] = []
    for first in range(len(components)):
        if len(components[first]) < minimum_component_pixels:
            continue
        points_a = boundaries[first].astype(np.int32)
        for second in range(first + 1, len(components)):
            if len(components[second]) < minimum_component_pixels:
                continue
            points_b = boundaries[second].astype(np.int32)
            distances = ((points_a[:, None, :] - points_b[None, :, :]) ** 2).sum(axis=2)
            flat_index = int(distances.argmin())
            point_a_index, point_b_index = np.unravel_index(flat_index, distances.shape)
            distance = float(np.sqrt(distances[point_a_index, point_b_index]))
            if distance > maximum_gap_px:
                continue
            point_a = tuple(map(int, points_a[point_a_index]))
            point_b = tuple(map(int, points_b[point_b_index]))
            if tuple(sorted((point_a, point_b))) in existing_endpoints:
                continue
            dy = abs(point_a[0] - point_b[0])
            dx = abs(point_a[1] - point_b[1])
            if dx < 0.5 * dy:
                continue
            midpoint_y = (point_a[0] + point_b[0]) / 2.0
            midpoint_x = (point_a[1] + point_b[1]) / 2.0
            if np.hypot(midpoint_y - center_y, midpoint_x - center_x) > central_limit:
                continue
            candidates.append((distance, point_a, point_b, midpoint_y, midpoint_x))

    selected: list[tuple[float, tuple[int, int], tuple[int, int]]] = []
    selected_midpoints: list[tuple[float, float]] = []
    minimum_spacing = min(shape) * 0.07
    for distance, point_a, point_b, midpoint_y, midpoint_x in sorted(candidates):
        if any(
            np.hypot(midpoint_y - old_y, midpoint_x - old_x) < minimum_spacing
            for old_y, old_x in selected_midpoints
        ):
            continue
        selected.append((distance, point_a, point_b))
        selected_midpoints.append((midpoint_y, midpoint_x))
        if len(selected) == count:
            break
    if len(selected) < count:
        raise RuntimeError(
            f"found only {len(selected)} suitable redundant bridges; requested {count}"
        )
    return selected


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
        raise ValueError("ID must be positive and smaller than OD")

    gray = np.asarray(Image.open(args.source).convert("L"))
    if gray.shape[0] != gray.shape[1]:
        raise ValueError("source render must be square")
    raw = gray < args.threshold
    source_side = raw.shape[0]
    source_center = source_side / 2.0
    yy, xx = np.indices(raw.shape, dtype=float)
    source_circle = np.hypot(xx + 0.5 - source_center, yy + 0.5 - source_center) <= source_center
    for _ in range(args.erosion_passes):
        raw = np.asarray(
            Image.fromarray(raw).filter(ImageFilter.MinFilter(3)), dtype=bool
        ).copy()
    raw &= source_circle

    components = label_components(raw, args.minimum_component_pixels)
    if not components:
        raise ValueError("no meaningful black artwork components found")
    clean = component_mask(raw.shape, components)
    bridges = minimum_spanning_bridges(components, raw.shape)
    source_pixels_per_mm = source_side / args.id
    bridge_width_px = max(3, int(round(args.internal_bridge_width * source_pixels_per_mm)))
    connected = connect_source_artwork(clean, bridges, bridge_width_px)
    redundant_bridges = find_redundant_bridges(
        components,
        raw.shape,
        bridges,
        args.redundant_bridge_count,
        args.redundant_bridge_max_gap * source_pixels_per_mm,
        args.redundant_min_component_pixels,
    )
    if redundant_bridges:
        redundant_width_px = max(
            3, int(round(args.redundant_bridge_width * source_pixels_per_mm))
        )
        connected = connect_source_artwork(
            connected, redundant_bridges, redundant_width_px
        )
    if len(label_components(connected, 1)) != 1:
        raise RuntimeError("island repair did not produce one connected artwork component")

    artwork = map_square_to_opening(
        connected,
        args.id,
        args.od,
        args.pixels_per_mm,
    )
    artwork, target_bridges = add_target_bridges(
        artwork,
        args.target_bridge,
        args.pixels_per_mm,
        args.id / 2.0,
    )
    contact_count, contact_centers = count_contact_zones(
        artwork,
        args.id / 2.0,
        args.pixels_per_mm,
    )
    if contact_count < 5:
        raise RuntimeError(
            f"circle crop produced only {contact_count} natural contacts; ties are required"
        )

    side = artwork.shape[0]
    center = side / 2.0
    yy, xx = np.indices(artwork.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
    ring = (radii <= args.od / 2.0) & (radii >= args.id / 2.0)
    disc = radii <= args.od / 2.0
    blocked = ring | artwork
    if len(label_components(blocked, 1)) != 1:
        raise RuntimeError("ring/artwork union is not one printed component")
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
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>Square bamboo grove gobo - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. The square vector pattern is circle-cropped and uses its natural perimeter contacts without generated ring ties.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    longest_gap_mm = (
        max(distance for distance, _a, _b in bridges) / source_pixels_per_mm
        if bridges
        else 0.0
    )
    print(f"cropped artwork components: {len(components)}")
    print(f"internal bridges: {len(bridges)} at {args.internal_bridge_width:.2f} mm")
    print(f"longest repaired gap: {longest_gap_mm:.2f} mm")
    if redundant_bridges:
        redundant_longest_mm = max(
            distance for distance, _a, _b in redundant_bridges
        ) / source_pixels_per_mm
        print(
            f"redundant central bridges: {len(redundant_bridges)} at "
            f"{args.redundant_bridge_width:.2f} mm; longest {redundant_longest_mm:.2f} mm"
        )
    if target_bridges:
        print(
            "targeted joint bridges: "
            + "; ".join(
                f"({x1:.2f},{y1:.2f})->({x2:.2f},{y2:.2f}) at {width:.2f} mm"
                for x1, y1, x2, y2, width in target_bridges
            )
        )
    print(f"natural ring contacts: {contact_count}; no generated ties")
    print(
        "contact centers (0=right, 90=bottom): "
        + ", ".join(f"{value:.0f}" for value in contact_centers)
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
