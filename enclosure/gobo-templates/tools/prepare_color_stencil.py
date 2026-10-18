#!/usr/bin/env python3
"""Extract flat-color stencil artwork into a cropped black-on-white PNG.

The default separation keeps pixels whose green channel exceeds both red and
blue. This rejects white product backgrounds and unrelated blue objects such as
the paintbrush in the Bamboo Stencil 2 download.

Dependencies:
    python -m pip install numpy Pillow
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image

from repair_bamboo_stencil import component_mask, label_components


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="source color image")
    parser.add_argument("output", type=Path, help="cropped black-on-white PNG")
    parser.add_argument("--minimum-green", type=int, default=55)
    parser.add_argument("--green-excess", type=int, default=18)
    parser.add_argument("--minimum-component-pixels", type=int, default=24)
    parser.add_argument("--crop-padding", type=int, default=24, help="source pixels")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    rgb = np.asarray(Image.open(args.source).convert("RGB"), dtype=np.int16)
    red, green, blue = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    raw = (
        (green >= args.minimum_green)
        & (green - red >= args.green_excess)
        & (green - blue >= args.green_excess)
    )
    components = label_components(raw, args.minimum_component_pixels)
    if not components:
        raise ValueError("no color-separated stencil artwork found")
    clean = component_mask(raw.shape, components)
    ys, xs = np.where(clean)
    x0 = max(0, int(xs.min()) - args.crop_padding)
    y0 = max(0, int(ys.min()) - args.crop_padding)
    x1 = min(clean.shape[1], int(xs.max()) + 1 + args.crop_padding)
    y1 = min(clean.shape[0], int(ys.max()) + 1 + args.crop_padding)
    cropped = clean[y0:y1, x0:x1]
    output = np.where(cropped, 0, 255).astype(np.uint8)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(output, mode="L").save(args.output)

    print(f"source size: {clean.shape[1]} x {clean.shape[0]}")
    print(f"retained stencil components: {len(components)}")
    print(f"artwork bbox: ({x0}, {y0}) - ({x1}, {y1})")
    print(f"black/white output: {cropped.shape[1]} x {cropped.shape[0]}")
    print(f"black artwork area: {100.0 * np.count_nonzero(cropped) / cropped.size:.2f}%")
    print(f"PNG: {args.output} ({args.output.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
