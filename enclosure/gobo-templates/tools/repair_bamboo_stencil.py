#!/usr/bin/env python3
"""Repair disconnected bamboo artwork and emit a production-size gobo SVG.

The input is expected to be dark artwork on a light background. Small scan
specks are removed, every meaningful artwork component is joined to its nearest
neighbor with short rounded bridges, and six radial ties connect the repaired
artwork to an exact 55 mm OD / 51 mm ID ring.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
from collections import deque
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import potrace


# Fusion 360 treats SVG user units as CSS pixels at 96 DPI even when width and
# height carry explicit millimeter units. Scaling coordinates avoids 55 ->
# 14.552 mm on import; negative coordinates put the disc center at the origin.
SVG_UNITS_PER_MM = 96.0 / 25.4


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="dark-on-light source image")
    parser.add_argument("output", type=Path, help="output SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument(
        "--source-erosion-pixels",
        type=int,
        default=1,
        help="inward cleanup passes applied to the thresholded source",
    )
    parser.add_argument("--minimum-component-pixels", type=int, default=20)
    parser.add_argument("--art-radius", type=float, default=25.0, help="artwork radius in mm")
    parser.add_argument("--internal-bridge-width", type=float, default=1.0, help="mm")
    parser.add_argument("--ring-bridge-width", type=float, default=1.4, help="mm")
    parser.add_argument("--ring-bridge-count", type=int, default=6)
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def label_components(mask: np.ndarray, minimum_size: int) -> list[np.ndarray]:
    height, width = mask.shape
    seen = np.zeros_like(mask, dtype=bool)
    components: list[np.ndarray] = []
    for y0, x0 in zip(*np.where(mask)):
        if seen[y0, x0]:
            continue
        queue = deque([(int(y0), int(x0))])
        seen[y0, x0] = True
        points: list[tuple[int, int]] = []
        while queue:
            y, x = queue.popleft()
            points.append((y, x))
            for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                if (
                    0 <= yy < height
                    and 0 <= xx < width
                    and mask[yy, xx]
                    and not seen[yy, xx]
                ):
                    seen[yy, xx] = True
                    queue.append((yy, xx))
        if len(points) >= minimum_size:
            components.append(np.asarray(points, dtype=np.int16))
    return components


def component_mask(shape: tuple[int, int], components: list[np.ndarray]) -> np.ndarray:
    mask = np.zeros(shape, dtype=bool)
    for component in components:
        mask[component[:, 0], component[:, 1]] = True
    return mask


def boundary_points(component: np.ndarray, labels: np.ndarray, label: int) -> np.ndarray:
    height, width = labels.shape
    result: list[tuple[int, int]] = []
    for y_raw, x_raw in component:
        y, x = int(y_raw), int(x_raw)
        for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if yy < 0 or yy >= height or xx < 0 or xx >= width or labels[yy, xx] != label:
                result.append((y, x))
                break
    return np.asarray(result, dtype=np.int16)


def minimum_spanning_bridges(
    components: list[np.ndarray], shape: tuple[int, int]
) -> list[tuple[float, tuple[int, int], tuple[int, int]]]:
    labels = np.zeros(shape, dtype=np.int16)
    for index, component in enumerate(components, start=1):
        labels[component[:, 0], component[:, 1]] = index
    boundaries = [
        boundary_points(component, labels, index)
        for index, component in enumerate(components, start=1)
    ]

    edges: list[tuple[int, int, int, tuple[int, int], tuple[int, int]]] = []
    for first in range(len(boundaries)):
        points_a = boundaries[first].astype(np.int32)
        for second in range(first + 1, len(boundaries)):
            points_b = boundaries[second].astype(np.int32)
            distances = ((points_a[:, None, :] - points_b[None, :, :]) ** 2).sum(axis=2)
            flat_index = int(distances.argmin())
            point_a_index, point_b_index = np.unravel_index(flat_index, distances.shape)
            edges.append(
                (
                    int(distances[point_a_index, point_b_index]),
                    first,
                    second,
                    tuple(map(int, points_a[point_a_index])),
                    tuple(map(int, points_b[point_b_index])),
                )
            )

    parent = list(range(len(components)))

    def find(index: int) -> int:
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    selected: list[tuple[float, tuple[int, int], tuple[int, int]]] = []
    for distance_sq, first, second, point_a, point_b in sorted(edges):
        root_a, root_b = find(first), find(second)
        if root_a == root_b:
            continue
        parent[root_a] = root_b
        selected.append((math.sqrt(distance_sq), point_a, point_b))
        if len(selected) == len(components) - 1:
            break
    return selected


def draw_rounded_line(
    drawing: ImageDraw.ImageDraw,
    start: tuple[float, float],
    end: tuple[float, float],
    width: int,
    round_start: bool = True,
    round_end: bool = True,
) -> None:
    drawing.line((*start, *end), fill=1, width=width)
    radius = width / 2.0
    rounded_points = []
    if round_start:
        rounded_points.append(start)
    if round_end:
        rounded_points.append(end)
    for x, y in rounded_points:
        drawing.ellipse((x - radius, y - radius, x + radius, y + radius), fill=1)


def connect_source_artwork(
    clean_mask: np.ndarray,
    bridges: list[tuple[float, tuple[int, int], tuple[int, int]]],
    width_px: int,
) -> np.ndarray:
    image = Image.fromarray(clean_mask)
    drawing = ImageDraw.Draw(image)
    for _distance, point_a, point_b in bridges:
        y1, x1 = point_a
        y2, x2 = point_b
        draw_rounded_line(drawing, (x1, y1), (x2, y2), width_px)
    return np.asarray(image, dtype=bool)


def source_geometry(mask: np.ndarray) -> tuple[float, float, float]:
    ys, xs = np.where(mask)
    cx = (float(xs.min()) + float(xs.max())) / 2.0
    cy = (float(ys.min()) + float(ys.max())) / 2.0
    radius = max(
        (float(xs.max()) - float(xs.min()) + 1.0) / 2.0,
        (float(ys.max()) - float(ys.min()) + 1.0) / 2.0,
    )
    return cx, cy, radius


def map_to_target(
    source: np.ndarray,
    cx: float,
    cy: float,
    source_radius: float,
    art_radius: float,
    od: float,
    pixels_per_mm: float,
) -> np.ndarray:
    side = int(round(od * pixels_per_mm))
    center = side / 2.0
    yy, xx = np.indices((side, side), dtype=float)
    x_mm = (xx + 0.5 - center) / pixels_per_mm
    y_mm = (yy + 0.5 - center) / pixels_per_mm
    scale = art_radius / source_radius
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
    return target


def add_ring_bridges(
    artwork: np.ndarray,
    count: int,
    width_mm: float,
    inner_radius: float,
    pixels_per_mm: float,
    desired_angles_deg: list[float] | None = None,
) -> tuple[np.ndarray, list[tuple[float, float, float]]]:
    side = artwork.shape[0]
    center = side / 2.0
    ys, xs = np.where(artwork)
    x_mm = (xs + 0.5 - center) / pixels_per_mm
    y_mm = (ys + 0.5 - center) / pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
    angles = np.arctan2(y_mm, x_mm)

    image = Image.fromarray(artwork)
    drawing = ImageDraw.Draw(image)
    width_px = int(round(width_mm * pixels_per_mm))
    tie_specs: list[tuple[float, float, float]] = []
    if desired_angles_deg is None:
        desired_angles = [
            math.radians(-90.0 + index * 360.0 / count) for index in range(count)
        ]
    else:
        desired_angles = [math.radians(value) for value in desired_angles_deg]
    for desired in desired_angles:
        delta = np.abs(np.angle(np.exp(1j * (angles - desired))))
        candidates = np.where(delta < math.radians(18.0))[0]
        if not len(candidates):
            raise ValueError(f"no artwork found near attachment angle {math.degrees(desired):.1f}")
        scores = radii[candidates] - 5.0 * delta[candidates]
        selected = candidates[int(scores.argmax())]
        actual_angle = float(angles[selected])
        # Begin slightly inside the selected leaf so the exact vector tie has a
        # dependable overlap with the traced artwork.
        start_radius = max(0.0, float(radii[selected]) - 0.60)
        start_x_mm = start_radius * math.cos(actual_angle)
        start_y_mm = start_radius * math.sin(actual_angle)
        start_x = center + start_x_mm * pixels_per_mm
        start_y = center + start_y_mm * pixels_per_mm
        end_radius = inner_radius
        end_x = center + end_radius * pixels_per_mm * math.cos(actual_angle)
        end_y = center + end_radius * pixels_per_mm * math.sin(actual_angle)
        draw_rounded_line(
            drawing,
            (start_x, start_y),
            (end_x, end_y),
            width_px,
            round_start=True,
            round_end=False,
        )
        tie_specs.append((start_x_mm, start_y_mm, actual_angle))

    # Make the outside end of every tie coincide with the 51 mm ID. This removes
    # the capsule-end bumps from the ring opening while retaining a shared edge
    # between the artwork and the exact circular ring.
    artwork_with_ties = np.asarray(image, dtype=bool)
    yy, xx = np.indices(artwork_with_ties.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / pixels_per_mm
    y_mm = (yy + 0.5 - center) / pixels_per_mm
    return artwork_with_ties & (np.hypot(x_mm, y_mm) <= inner_radius), tie_specs


def trace_printed_solid(
    blocked: np.ndarray,
    working_pixels_per_mm: float,
    trace_pixels_per_mm: float,
    od_mm: float,
    tolerance: float,
) -> tuple[str, int, int]:
    """Trace a boolean-unioned solid, replacing its outside with an exact circle."""
    if trace_pixels_per_mm > working_pixels_per_mm:
        raise ValueError("trace resolution cannot exceed working resolution")
    trace_side = int(round(blocked.shape[0] * trace_pixels_per_mm / working_pixels_per_mm))
    trace_image = Image.fromarray(blocked.astype(np.uint8) * 255).resize(
        (trace_side, trace_side), Image.Resampling.LANCZOS
    )
    trace_mask = np.asarray(trace_image) >= 128
    traced = potrace.Bitmap(~trace_mask).trace(
        turdsize=4,
        alphamax=1.3,
        opticurve=True,
        opttolerance=tolerance,
    )
    if not traced:
        raise RuntimeError("unified solid trace produced no contours")

    def point(value) -> str:
        x_mm = float(value.x) / trace_pixels_per_mm - od_mm / 2.0
        y_mm = float(value.y) / trace_pixels_per_mm - od_mm / 2.0
        return (
            f"{x_mm * SVG_UNITS_PER_MM:.4f},"
            f"{y_mm * SVG_UNITS_PER_MM:.4f}"
        )

    def curve_bbox_area(curve) -> float:
        points = [curve.start_point]
        for segment in curve:
            if segment.is_corner:
                points.extend((segment.c, segment.end_point))
            else:
                points.extend((segment.c1, segment.c2, segment.end_point))
        xs = [float(value.x) for value in points]
        ys = [float(value.y) for value in points]
        return (max(xs) - min(xs)) * (max(ys) - min(ys))

    # Potrace includes the rasterized OD as one contour. Replace it with a true
    # SVG circle so the Fusion import retains an exact 55 mm outside diameter.
    outer_index = max(range(len(traced)), key=lambda index: curve_bbox_area(traced[index]))
    outer_radius = od_mm / 2.0 * SVG_UNITS_PER_MM
    commands: list[str] = [
        f"M 0,{-outer_radius:.4f}",
        f"A {outer_radius:.4f},{outer_radius:.4f} 0 1 1 0,{outer_radius:.4f}",
        f"A {outer_radius:.4f},{outer_radius:.4f} 0 1 1 0,{-outer_radius:.4f}",
        "Z",
    ]
    segment_count = 0
    for index, curve in enumerate(traced):
        if index == outer_index:
            continue
        commands.append(f"M {point(curve.start_point)}")
        for segment in curve:
            segment_count += 1
            if segment.is_corner:
                commands.append(f"L {point(segment.c)}")
                commands.append(f"L {point(segment.end_point)}")
            else:
                commands.append(
                    f"C {point(segment.c1)} {point(segment.c2)} {point(segment.end_point)}"
                )
        commands.append("Z")
    return " ".join(commands), len(traced) - 1, segment_count


def write_preview(path: Path, blocked: np.ndarray, pixels_per_mm: float) -> None:
    margin = int(round(2.5 * pixels_per_mm))
    padded = np.pad(blocked, margin, mode="constant")
    gray = np.where(padded, 194, 255).astype(np.uint8)
    eroded = np.asarray(Image.fromarray(padded).filter(ImageFilter.MinFilter(3)))
    gray[padded & ~eroded] = 35
    path.parent.mkdir(parents=True, exist_ok=True)
    image = Image.fromarray(gray, mode="L")
    image.resize((image.width // 2, image.height // 2), Image.Resampling.LANCZOS).save(path)


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
        raise ValueError("ID must be positive and smaller than OD")
    if args.ring_bridge_count < 5:
        raise ValueError("at least five ring bridges are required")

    gray = np.asarray(Image.open(args.source).convert("L"))
    raw_mask = gray < args.threshold
    for _ in range(args.source_erosion_pixels):
        raw_mask = np.asarray(
            Image.fromarray(raw_mask).filter(ImageFilter.MinFilter(3)), dtype=bool
        )
    components = label_components(raw_mask, args.minimum_component_pixels)
    if not components:
        raise ValueError("no meaningful artwork components found")
    clean = component_mask(raw_mask.shape, components)
    cx, cy, source_radius = source_geometry(clean)
    scale = args.art_radius / source_radius

    mst_bridges = minimum_spanning_bridges(components, raw_mask.shape)
    source_bridge_width_px = max(3, int(round(args.internal_bridge_width / scale)))
    connected_source = connect_source_artwork(clean, mst_bridges, source_bridge_width_px)
    repaired_components = label_components(connected_source, args.minimum_component_pixels)
    if len(repaired_components) != 1:
        raise RuntimeError(f"artwork repair left {len(repaired_components)} components")

    artwork = map_to_target(
        connected_source,
        cx,
        cy,
        source_radius,
        args.art_radius,
        args.od,
        args.pixels_per_mm,
    )
    inner_radius = args.id / 2.0
    outer_radius = args.od / 2.0
    # Keep natural leaf tips and all generated ties at or inside the exact ring
    # ID before tracing. The six ties themselves are emitted again below as exact
    # vector paths with an outer edge matching the circular 25.5 mm radius.
    side = artwork.shape[0]
    center = side / 2.0
    yy, xx = np.indices(artwork.shape, dtype=float)
    x_mm = (xx + 0.5 - center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
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
    blocked_percent = 100.0 * np.count_nonzero(blocked & disc) / np.count_nonzero(disc)

    solid_components = label_components(blocked, 1)
    if len(solid_components) != 1:
        raise RuntimeError(
            f"ring/artwork boolean union left {len(solid_components)} printed components"
        )

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
  <title>Bamboo stalks and leaves gobo - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. Ring, ties, stalks, and leaves contain no buried intersection edges.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    widths = [args.ring_bridge_width] * len(tie_specs)
    longest_gap_mm = max(distance for distance, _a, _b in mst_bridges) * scale
    print(f"source artwork components: {len(components)}")
    print(f"internal bridges: {len(mst_bridges)} at {args.internal_bridge_width:.2f} mm")
    print(f"longest repaired source gap: {longest_gap_mm:.2f} mm")
    print(
        f"ring attachments: {len(widths)} "
        f"({', '.join(f'{width:.2f} mm' for width in widths)})"
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
