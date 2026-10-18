#!/usr/bin/env python3
"""Turn black-on-white artwork into a Fusion-ready negative-pattern gobo.

Black source artwork becomes open space. The white source background and an
exact OD/ID ring become one printed solid. If the open artwork divides the
background into islands, short printed passages reconnect those islands.

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

from repair_bamboo_stencil import (
    SVG_UNITS_PER_MM,
    component_mask,
    label_components,
    trace_printed_solid,
    write_preview,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="black-on-white raster artwork")
    parser.add_argument("output", type=Path, help="output Fusion-ready SVG")
    parser.add_argument("--preview", type=Path, help="optional gray/white preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument(
        "--art-width",
        type=float,
        default=51.0,
        help="width in mm assigned to the full source canvas before circular crop",
    )
    parser.add_argument("--art-shift-x", type=float, default=0.0, help="mm")
    parser.add_argument("--art-shift-y", type=float, default=0.0, help="mm")
    parser.add_argument(
        "--open-expansion",
        type=float,
        default=0.0,
        help="outward expansion of the source openings in mm",
    )
    parser.add_argument(
        "--leaf",
        action="append",
        default=[],
        metavar="X,Y,LENGTH,WIDTH,ANGLE[,CURVE]",
        help="add a pointed open leaf; coordinates and dimensions are mm",
    )
    parser.add_argument("--minimum-component-pixels", type=int, default=4)
    parser.add_argument(
        "--passage-width",
        type=float,
        default=1.4,
        help="printed passage width in mm used to reunite background islands",
    )
    parser.add_argument("--pixels-per-mm", type=float, default=24.0)
    parser.add_argument("--trace-pixels-per-mm", type=float, default=8.0)
    parser.add_argument("--trace-tolerance", type=float, default=1.5)
    return parser.parse_args()


def map_source(
    source: np.ndarray,
    side: int,
    pixels_per_mm: float,
    art_width_mm: float,
    shift_x_mm: float,
    shift_y_mm: float,
) -> np.ndarray:
    target_width = max(1, int(round(art_width_mm * pixels_per_mm)))
    target_height = max(
        1, int(round(target_width * source.shape[0] / source.shape[1]))
    )
    resized = np.asarray(
        Image.fromarray(source).resize(
            (target_width, target_height), Image.Resampling.LANCZOS
        )
    ) >= 128
    x0 = int(round((side - target_width) / 2.0 + shift_x_mm * pixels_per_mm))
    y0 = int(round((side - target_height) / 2.0 + shift_y_mm * pixels_per_mm))
    source_x0 = max(0, -x0)
    source_y0 = max(0, -y0)
    target_x0 = max(0, x0)
    target_y0 = max(0, y0)
    width = min(target_width - source_x0, side - target_x0)
    height = min(target_height - source_y0, side - target_y0)
    result = np.zeros((side, side), dtype=bool)
    if width > 0 and height > 0:
        result[target_y0 : target_y0 + height, target_x0 : target_x0 + width] = (
            resized[source_y0 : source_y0 + height, source_x0 : source_x0 + width]
        )
    return result


def parse_leaf(value: str) -> tuple[float, float, float, float, float, float]:
    fields = [float(field.strip()) for field in value.split(",")]
    if len(fields) not in (5, 6):
        raise argparse.ArgumentTypeError(
            "leaf must be X,Y,LENGTH,WIDTH,ANGLE[,CURVE]"
        )
    if len(fields) == 5:
        fields.append(0.0)
    if fields[2] <= 0 or fields[3] <= 0:
        raise argparse.ArgumentTypeError("leaf length and width must be positive")
    return tuple(fields)  # type: ignore[return-value]


def add_pointed_leaf(
    mask: np.ndarray,
    spec: tuple[float, float, float, float, float, float],
    pixels_per_mm: float,
) -> None:
    """Add a gently curved, pointed leaf beginning at a stem attachment point."""
    base_x, base_y, length, width, angle_deg, curve = spec
    angle = math.radians(angle_deg)
    tangent = np.array((math.cos(angle), math.sin(angle)))
    normal = np.array((-math.sin(angle), math.cos(angle)))
    left: list[tuple[float, float]] = []
    right: list[tuple[float, float]] = []
    for index in range(25):
        t = index / 24.0
        center = (
            np.array((base_x, base_y))
            + tangent * (length * t)
            + normal * (curve * math.sin(math.pi * t))
        )
        half_width = 0.5 * width * math.sin(math.pi * t) ** 0.72
        left.append(tuple(center + normal * half_width))
        right.append(tuple(center - normal * half_width))
    side = mask.shape[0]
    center_px = side / 2.0

    def to_pixel(point: tuple[float, float]) -> tuple[float, float]:
        return (
            center_px + point[0] * pixels_per_mm,
            center_px + point[1] * pixels_per_mm,
        )

    image = Image.fromarray(mask)
    drawing = ImageDraw.Draw(image)
    drawing.polygon([to_pixel(point) for point in left + right[::-1]], fill=1)
    mask[:] = np.asarray(image, dtype=bool)


def wavefront_bridges(
    components: list[np.ndarray], shape: tuple[int, int]
) -> list[tuple[float, tuple[int, int], tuple[int, int]]]:
    """Find a short Manhattan-distance spanning tree without dense pair matrices."""
    labels = np.zeros(shape, dtype=np.int16)
    for index, component in enumerate(components, start=1):
        labels[component[:, 0], component[:, 1]] = index
    occupied = labels > 0
    boundary = occupied.copy()
    boundary[1:-1, 1:-1] &= (
        (labels[1:-1, 1:-1] != labels[:-2, 1:-1])
        | (labels[1:-1, 1:-1] != labels[2:, 1:-1])
        | (labels[1:-1, 1:-1] != labels[1:-1, :-2])
        | (labels[1:-1, 1:-1] != labels[1:-1, 2:])
    )
    owner = np.zeros(shape, dtype=np.int16)
    distance = np.full(shape, -1, dtype=np.int32)
    origin_y = np.full(shape, -1, dtype=np.int16)
    origin_x = np.full(shape, -1, dtype=np.int16)
    ys, xs = np.where(boundary)
    owner[ys, xs] = labels[ys, xs]
    distance[ys, xs] = 0
    origin_y[ys, xs] = ys
    origin_x[ys, xs] = xs
    queue = deque(zip(map(int, ys), map(int, xs)))
    candidates: dict[
        tuple[int, int], tuple[int, tuple[int, int], tuple[int, int]]
    ] = {}
    height, width = shape
    while queue:
        y, x = queue.popleft()
        own = int(owner[y, x])
        for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if yy < 0 or yy >= height or xx < 0 or xx >= width:
                continue
            neighbor_owner = int(owner[yy, xx])
            if neighbor_owner == 0:
                owner[yy, xx] = own
                distance[yy, xx] = distance[y, x] + 1
                origin_y[yy, xx] = origin_y[y, x]
                origin_x[yy, xx] = origin_x[y, x]
                queue.append((yy, xx))
            elif neighbor_owner != own:
                pair = tuple(sorted((own, neighbor_owner)))
                total = int(distance[y, x] + distance[yy, xx] + 1)
                point_a = (int(origin_y[y, x]), int(origin_x[y, x]))
                point_b = (int(origin_y[yy, xx]), int(origin_x[yy, xx]))
                if own != pair[0]:
                    point_a, point_b = point_b, point_a
                current = candidates.get(pair)
                if current is None or total < current[0]:
                    candidates[pair] = (total, point_a, point_b)

    parent = list(range(len(components) + 1))

    def find(index: int) -> int:
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    selected: list[tuple[float, tuple[int, int], tuple[int, int]]] = []
    for _pair, (gap, point_a, point_b) in sorted(
        candidates.items(), key=lambda item: item[1][0]
    ):
        first, second = _pair
        root_a, root_b = find(first), find(second)
        if root_a == root_b:
            continue
        parent[root_a] = root_b
        selected.append((float(gap), point_a, point_b))
        if len(selected) == len(components) - 1:
            break
    if len(selected) != len(components) - 1:
        raise RuntimeError("could not connect every printed background component")
    return selected


def connect_background_passages(
    printed: np.ndarray,
    bridges: list[tuple[float, tuple[int, int], tuple[int, int]]],
    width_px: int,
) -> np.ndarray:
    """Draw butt-ended passages with overlap inside both printed components."""
    image = Image.fromarray(printed)
    drawing = ImageDraw.Draw(image)
    overlap = width_px * 0.55
    for _distance, point_a, point_b in bridges:
        y1, x1 = point_a
        y2, x2 = point_b
        dx = float(x2 - x1)
        dy = float(y2 - y1)
        length = math.hypot(dx, dy)
        if length:
            ux, uy = dx / length, dy / length
            start = (x1 - ux * overlap, y1 - uy * overlap)
            end = (x2 + ux * overlap, y2 + uy * overlap)
        else:
            start = end = (float(x1), float(y1))
        drawing.line((*start, *end), fill=1, width=width_px)
    return np.asarray(image, dtype=bool)


def main() -> None:
    args = parse_args()
    if args.id <= 0 or args.id >= args.od:
        raise ValueError("ID must be positive and smaller than OD")
    if args.art_width <= 0:
        raise ValueError("art width must be positive")

    gray = np.asarray(Image.open(args.source).convert("L"))
    source_open = (gray < args.threshold).astype(np.uint8) * 255
    side = int(round(args.od * args.pixels_per_mm))
    open_art = map_source(
        source_open,
        side,
        args.pixels_per_mm,
        args.art_width,
        args.art_shift_x,
        args.art_shift_y,
    )
    leaf_specs = [parse_leaf(value) for value in args.leaf]
    for spec in leaf_specs:
        add_pointed_leaf(open_art, spec, args.pixels_per_mm)
    if args.open_expansion < 0:
        raise ValueError("open expansion cannot be negative")
    expansion_px = int(round(args.open_expansion * args.pixels_per_mm))
    if expansion_px:
        open_art = np.asarray(
            Image.fromarray(open_art).filter(
                ImageFilter.MaxFilter(2 * expansion_px + 1)
            ),
            dtype=bool,
        ).copy()

    center = side / 2.0
    yy, xx = np.indices((side, side), dtype=float)
    x_mm = (xx + 0.5 - center) / args.pixels_per_mm
    y_mm = (yy + 0.5 - center) / args.pixels_per_mm
    radii = np.hypot(x_mm, y_mm)
    disc = radii <= args.od / 2.0
    inner_disc = radii < args.id / 2.0
    ring = disc & ~inner_disc
    open_art &= inner_disc
    initial = ring | (inner_disc & ~open_art)
    components = label_components(initial, args.minimum_component_pixels)
    if not components:
        raise ValueError("negative conversion produced no printed material")
    clean = component_mask(initial.shape, components)
    bridges = wavefront_bridges(components, initial.shape)
    passage_width_px = max(3, int(round(args.passage_width * args.pixels_per_mm)))
    blocked = connect_background_passages(clean, bridges, passage_width_px) & disc
    final_components = label_components(blocked, 1)
    if len(final_components) != 1:
        raise RuntimeError(
            f"negative repair left {len(final_components)} printed components"
        )

    initial_aperture_open = (
        100.0 * np.count_nonzero(open_art) / np.count_nonzero(inner_disc)
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
        args.od,
        args.trace_tolerance,
    )
    view_size = args.od * SVG_UNITS_PER_MM
    view_min = -view_size / 2.0
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>{args.output.stem} - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is one boolean-unioned printed solid; white is open. The exact ring and source white background print, while the bamboo artwork is open. Short printed passages reconnect every background island.</desc>
  <path id="printed-solid" fill="#c2c2c0" stroke="none" fill-rule="evenodd" d="{solid_d}"/>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")
    if args.preview:
        write_preview(args.preview, blocked, args.pixels_per_mm)

    longest_gap = max((bridge[0] for bridge in bridges), default=0.0)
    print(f"source placement: {args.art_width:.2f} mm wide")
    print(f"supplemental open leaves: {len(leaf_specs)}")
    print(f"source opening expansion: {args.open_expansion:.2f} mm")
    print(f"initial printed components: {len(components)}")
    print(f"background passages: {len(bridges)} at {args.passage_width:.2f} mm")
    print(f"longest opened-art crossing: {longest_gap / args.pixels_per_mm:.2f} mm")
    print(f"aperture open area before passages: {initial_aperture_open:.2f}%")
    print(f"{args.id:g} mm aperture open area: {aperture_open:.2f}%")
    print(f"full-disc open area: {full_open:.2f}%")
    print(f"full-disc blocked area: {blocked_percent:.2f}%")
    print(f"unified solid: 1 exact outer contour, {openings} opening contours")
    print(f"trace complexity: {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
