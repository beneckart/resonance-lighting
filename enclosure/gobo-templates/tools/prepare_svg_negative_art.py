#!/usr/bin/env python3
"""Prepare selected black SVG paths as square artwork for a negative gobo.

This helper is useful when downloaded stencil art includes its own circular
border. It can discard leading path elements, replace the source viewBox with a
chosen square crop, and force all retained artwork to solid black on white.
The resulting SVG remains an intermediate; render it to a square PNG before
passing it to ``negative_art_gobo.py``.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--drop-leading-paths",
        type=int,
        default=0,
        help="number of leading path elements to remove",
    )
    parser.add_argument(
        "--crop",
        required=True,
        metavar="MIN_X,MIN_Y,SIZE",
        help="square source-coordinate crop",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    crop_fields = [float(value.strip()) for value in args.crop.split(",")]
    if len(crop_fields) != 3 or crop_fields[2] <= 0:
        raise ValueError("crop must be MIN_X,MIN_Y,SIZE with a positive size")

    tree = ET.parse(args.source)
    root = tree.getroot()
    paths = list(root.iter(f"{{{SVG_NS}}}path"))
    if args.drop_leading_paths < 0 or args.drop_leading_paths > len(paths):
        raise ValueError("drop-leading-paths is outside the available path range")

    parents = {child: parent for parent in root.iter() for child in parent}
    for path in paths[: args.drop_leading_paths]:
        parents[path].remove(path)
    for path in paths[args.drop_leading_paths :]:
        path.set("fill", "#000000")
        path.set("stroke", "none")
        path.attrib.pop("class", None)
        path.attrib.pop("style", None)

    minimum_x, minimum_y, size = crop_fields
    root.set("viewBox", f"{minimum_x:g} {minimum_y:g} {size:g} {size:g}")
    root.set("width", "1200")
    root.set("height", "1200")
    root.set("style", "background:#ffffff")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    tree.write(args.output, encoding="utf-8", xml_declaration=True)
    print(f"retained paths: {len(paths) - args.drop_leading_paths}")
    print(f"square crop: {minimum_x:g},{minimum_y:g} size {size:g}")
    print(f"prepared SVG: {args.output}")


if __name__ == "__main__":
    main()
