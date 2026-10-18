#!/usr/bin/env python3
"""Turn a gray-on-white circular gobo PNG into a dimensioned Fusion-ready SVG.

The source artwork is clipped just inside its original ring, scaled to overlap a
new exact-size ring, and traced as optimized cubic Bezier curves. The script also
writes a raster preview and reports the approximate blocked-area percentage and
the number of artwork-to-ring attachment zones.

Dependencies:
    python -m pip install numpy Pillow potracer
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter
import potrace


# Fusion 360 treats SVG user units as CSS pixels at 96 DPI even when width and
# height carry explicit millimeter units. Scaling coordinates avoids 55 ->
# 14.552 mm on import; negative coordinates put the disc center at the origin.
SVG_UNITS_PER_MM = 96.0 / 25.4


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="gray-on-white source PNG")
    parser.add_argument("output", type=Path, help="output SVG")
    parser.add_argument("--preview", type=Path, help="optional output preview PNG")
    parser.add_argument("--od", type=float, default=55.0, help="outer diameter in mm")
    parser.add_argument("--id", type=float, default=51.0, help="inner diameter in mm")
    parser.add_argument(
        "--threshold",
        type=int,
        default=245,
        help="pixels darker than this are treated as printed material",
    )
    parser.add_argument(
        "--art-radius-ratio",
        type=float,
        default=0.8588957,
        help="source radius retained as artwork; excludes the source ring",
    )
    parser.add_argument(
        "--overlap",
        type=float,
        default=0.05,
        help="artwork overlap into the new ring in mm",
    )
    parser.add_argument(
        "--trace-tolerance",
        type=float,
        default=0.35,
        help="Potrace curve optimization tolerance in source pixels",
    )
    return parser.parse_args()


def source_geometry(mask: np.ndarray) -> tuple[float, float, float]:
    ys, xs = np.where(mask)
    if not len(xs):
        raise ValueError("source contains no pixels below the selected threshold")
    left, right = float(xs.min()), float(xs.max())
    top, bottom = float(ys.min()), float(ys.max())
    cx = (left + right) / 2.0
    cy = (top + bottom) / 2.0
    radius = ((right - left + 1.0) + (bottom - top + 1.0)) / 4.0
    return cx, cy, radius


def point_svg(point, cx: float, cy: float, scale: float) -> tuple[float, float]:
    return (
        (float(point.x) - cx) * scale * SVG_UNITS_PER_MM,
        (float(point.y) - cy) * scale * SVG_UNITS_PER_MM,
    )


def fmt_point(point, cx: float, cy: float, scale: float) -> str:
    x, y = point_svg(point, cx, cy, scale)
    return f"{x:.4f},{y:.4f}"


def trace_path_data(
    artwork: np.ndarray,
    cx: float,
    cy: float,
    scale: float,
    tolerance: float,
) -> tuple[str, int, int]:
    # Potrace treats zero-valued pixels as the foreground to trace.
    traced = potrace.Bitmap(~artwork).trace(
        turdsize=4,
        alphamax=1.0,
        opticurve=True,
        opttolerance=tolerance,
    )
    commands: list[str] = []
    segment_count = 0
    for curve in traced:
        commands.append(f"M {fmt_point(curve.start_point, cx, cy, scale)}")
        for segment in curve:
            segment_count += 1
            if segment.is_corner:
                commands.append(f"L {fmt_point(segment.c, cx, cy, scale)}")
                commands.append(f"L {fmt_point(segment.end_point, cx, cy, scale)}")
            else:
                c1 = fmt_point(segment.c1, cx, cy, scale)
                c2 = fmt_point(segment.c2, cx, cy, scale)
                end = fmt_point(segment.end_point, cx, cy, scale)
                commands.append(f"C {c1} {c2} {end}")
        commands.append("Z")
    return " ".join(commands), len(traced), segment_count


def ring_path(outer_r_mm: float, inner_r_mm: float) -> str:
    outer_r = outer_r_mm * SVG_UNITS_PER_MM
    inner_r = inner_r_mm * SVG_UNITS_PER_MM
    return (
        f"M 0,{-outer_r:.4f} A {outer_r:.4f},{outer_r:.4f} 0 1 1 "
        f"0,{outer_r:.4f} A {outer_r:.4f},{outer_r:.4f} 0 1 1 "
        f"0,{-outer_r:.4f} Z "
        f"M 0,{-inner_r:.4f} A {inner_r:.4f},{inner_r:.4f} 0 1 0 "
        f"0,{inner_r:.4f} A {inner_r:.4f},{inner_r:.4f} 0 1 0 "
        f"0,{-inner_r:.4f} Z"
    )


def attachment_zones(mask: np.ndarray, cx: float, cy: float, radius: float) -> tuple[int, list[float]]:
    samples = 3600
    occupied: list[bool] = []
    for index in range(samples):
        angle = 2.0 * math.pi * index / samples
        x = int(round(cx + radius * math.cos(angle)))
        y = int(round(cy + radius * math.sin(angle)))
        occupied.append(bool(mask[y, x]))

    start = next((i for i, value in enumerate(occupied) if not value), 0)
    run = 0
    runs: list[int] = []
    for offset in range(1, samples + 1):
        value = occupied[(start + offset) % samples]
        if value:
            run += 1
        elif run:
            if run >= 2:
                runs.append(run)
            run = 0
    return len(runs), [run * 360.0 / samples for run in runs]


def target_mask(
    artwork: np.ndarray,
    cx: float,
    cy: float,
    scale: float,
    outer_r: float,
    inner_r: float,
    pixels_per_mm: float,
    margin_mm: float,
) -> tuple[np.ndarray, np.ndarray]:
    side_mm = 2.0 * (outer_r + margin_mm)
    side_px = int(round(side_mm * pixels_per_mm))
    yy, xx = np.indices((side_px, side_px), dtype=float)
    center_px = side_px / 2.0
    x_mm = (xx + 0.5 - center_px) / pixels_per_mm
    y_mm = (yy + 0.5 - center_px) / pixels_per_mm
    radius_mm = np.hypot(x_mm, y_mm)

    source_x = np.rint(cx + x_mm / scale).astype(int)
    source_y = np.rint(cy + y_mm / scale).astype(int)
    valid = (
        (source_x >= 0)
        & (source_x < artwork.shape[1])
        & (source_y >= 0)
        & (source_y < artwork.shape[0])
    )
    mapped_art = np.zeros_like(valid)
    mapped_art[valid] = artwork[source_y[valid], source_x[valid]]

    ring = (radius_mm <= outer_r) & (radius_mm >= inner_r)
    outer_disc = radius_mm <= outer_r
    return ring | mapped_art, outer_disc


def write_preview(path: Path, blocked: np.ndarray) -> None:
    high = Image.fromarray(np.where(blocked, 194, 255).astype(np.uint8), mode="L")
    edge = blocked & ~(np.asarray(Image.fromarray(blocked).filter(ImageFilter.MinFilter(3))))
    pixels = np.asarray(high).copy()
    pixels[edge] = 35
    image = Image.fromarray(pixels, mode="L")
    path.parent.mkdir(parents=True, exist_ok=True)
    image.resize((image.width // 2, image.height // 2), Image.Resampling.LANCZOS).save(path)


def main() -> None:
    args = parse_args()
    if args.id >= args.od or args.id <= 0:
        raise ValueError("ID must be positive and smaller than OD")

    gray = np.asarray(Image.open(args.source).convert("L"))
    full_mask = gray < args.threshold
    cx, cy, source_outer_r = source_geometry(full_mask)
    source_art_r = source_outer_r * args.art_radius_ratio
    yy, xx = np.indices(full_mask.shape, dtype=float)
    within_art = np.hypot(xx - cx, yy - cy) <= source_art_r
    artwork = full_mask & within_art

    outer_r = args.od / 2.0
    inner_r = args.id / 2.0
    target_art_r = inner_r + args.overlap
    scale = target_art_r / source_art_r
    artwork_d, contours, segments = trace_path_data(
        artwork, cx, cy, scale, args.trace_tolerance
    )

    view_size = args.od * SVG_UNITS_PER_MM
    view_min = -view_size / 2.0
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{args.od:g}mm" height="{args.od:g}mm"
     viewBox="{view_min:.6f} {view_min:.6f} {view_size:.6f} {view_size:.6f}" version="1.1">
  <title>Resonance Tree gobo - {args.od:g} mm OD / {args.id:g} mm ID</title>
  <desc>Fusion 360 coordinates use 96 SVG units per inch and are centered on 0,0. Gray is printed material; white is open. Artwork overlaps the ring by {args.overlap:g} mm.</desc>
  <g id="printed-material" fill="#c2c2c0" stroke="none">
    <path id="ring" fill-rule="evenodd" d="{ring_path(outer_r, inner_r)}"/>
    <path id="tree" fill-rule="evenodd" d="{artwork_d}"/>
  </g>
</svg>
'''
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8", newline="\n")

    blocked, outer_disc = target_mask(
        artwork,
        cx,
        cy,
        scale,
        outer_r,
        inner_r,
        pixels_per_mm=24.0,
        margin_mm=2.5,
    )
    blocked_percent = 100.0 * np.count_nonzero(blocked & outer_disc) / np.count_nonzero(outer_disc)
    zones, widths_deg = attachment_zones(full_mask, cx, cy, round(source_art_r))
    widths_mm = [math.radians(width) * target_art_r for width in widths_deg]

    if args.preview:
        write_preview(args.preview, blocked)

    print(f"source center/radius: ({cx:.2f}, {cy:.2f}) / {source_outer_r:.2f} px")
    print(f"artwork clip radius: {source_art_r:.2f} px -> {target_art_r:.2f} mm")
    print(f"blocked area: {blocked_percent:.2f}% of the {args.od:g} mm disc")
    print(f"ring attachments: {zones} ({', '.join(f'{width:.2f} mm' for width in widths_mm)})")
    print(f"trace complexity: {contours} contours, {segments} curve/corner segments")
    print("Fusion coordinates: 96 SVG units/inch, center at (0, 0)")
    print(f"SVG: {args.output} ({args.output.stat().st_size:,} bytes)")
    if args.preview:
        print(f"preview: {args.preview}")


if __name__ == "__main__":
    main()
