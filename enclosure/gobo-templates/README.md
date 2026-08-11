# Gobo Templates

Traceable and printable 2D pattern sources for replaceable shadow apertures.

For the source-art brief, mechanical rules, print lessons, and example catalog
to share with Ben, see [`BEN_GOBO_ART_HANDOFF.md`](BEN_GOBO_ART_HANDOFF.md).

## Production geometry

There are now two downlight gobo interfaces:

- large: 55 mm outside diameter / 51 mm inside diameter, used by 14 of the
  100 bamboo tube shades;
- small: 50 mm outside diameter / 46 mm inside diameter, expected for most of
  the remaining shades;
- both use a 2 mm radial ring wall;
- artwork joined to the ring in at least five approximately evenly spaced places;
- one connected printed solid, with no loose stencil islands; and
- 40-45% total blocked area for ordinary positive patterns, measured over the
  complete disc including the ring.

The filename is part of the interface contract: `-55mm` means 55/51 mm and
`-50mm` means 50/46 mm. A `-neg` pattern prints the background and ring while
leaving the subject open; a file without `-neg` is a positive pattern.

Fusion 360 imports SVG user coordinates as 96-DPI CSS pixels even when the SVG
declares millimeter dimensions. Production SVGs therefore use 96 / 25.4 SVG
units per millimeter. Their viewBox and geometry are centered on `(0,0)`, with
the 55 mm disc spanning approximately `-103.937` to `+103.937` SVG units. This
imports as `-27.5` to `+27.5` mm in Fusion instead of the erroneous 14.552 mm
diameter produced by a 55-unit viewBox.

The centered 50 mm files use a `188.976378`-unit viewBox spanning approximately
`-94.488189` to `+94.488189`, so Fusion imports them as exactly -25 to +25 mm.

`tree-logo-55mm.svg` is the first normalized design under these rules. Gray is
printed material and white is open for light. Its accompanying PNG is a visual
preview only; import the SVG into Fusion 360.

`bamboo-stencil-55mm.svg` is a repaired low-resolution stencil image containing
two bamboo stalks and a wreath of leaves. The source contained 60 disconnected
printed regions. `tools/repair_bamboo_stencil.py` joins neighboring regions with
short branch-like bridges and adds six ties to the production ring before tracing
the result into optimized Bezier curves. The revised artwork reaches a 25 mm
radius, making the ring ties very short, and blocks approximately 42.4% of the
full disc. Each 1.4 mm ring tie terminates at the 25.5 mm-radius opening. The
generator boolean-unions the ring, ties, stalks, and leaves before tracing, so
the SVG contains one printed-solid path with no buried intersection lines for
Fusion to trim manually.

`bamboo-2-55mm.svg` starts from a wide paint-stencil fan containing free-floating
leaves. `tools/repair_bamboo_fan.py` reconnects the leaf/stem islands, fits the
artwork by height, and deliberately crops the oversized left/right leaf tips at
the circular opening. Like the repaired bamboo stencil, its ring and artwork are
boolean-unioned into one Fusion profile before export.

`bamboo-3-55mm.svg` starts from a green bamboo stock photograph rather than a
black stencil. `tools/repair_bamboo_photo.py` uses color separation to reject the
pale watermark, reconnects the branch clusters, reinforces fine photographic
stems to at least 1.4 mm, and circularly crops the wide composition. It blocks
approximately 44% of the full disc and contains one boolean-unioned Fusion
profile.

`bamboo-4-55mm.svg` applies the same photo workflow to an upright cluster of
three bamboo stalks but deliberately disables the Bamboo-3 centerline-width
reinforcement. The source's naturally broad stalks are retained while leaf tips
remain tapered. Four selected ties at approximately noon, 2, 6, and 10 o'clock
supplement the natural cropped contacts; the former 4 and 8 o'clock braces are
omitted. The artwork is enlarged to 52 mm high, producing approximately 41%
blocked area in the rendered SVG.

`bamboo-5-55mm.svg` starts from a clean 400 x 400 black-and-white vector bamboo
grove. The square SVG is rendered losslessly at 800 x 800 for boolean processing,
then circle-cropped directly to the 51 mm opening. The crop produces 21 natural
ring-contact zones, so it has no generated circle ties. Eighty-two cropped stalk,
joint, and leaf regions are joined with 81 minimum-length 0.8 mm bridges before
the complete artwork and ring are boolean-unioned into one Fusion path. The
rendered SVG blocks approximately 44.5-45.0% of the full disc.

`bamboo-5a-55mm.svg` is the full-density comparison requested by Steve. It uses
the same circle crop and Fusion cleanup but applies no silhouette erosion. The
original black stalk and leaf contours remain full width; only the 55 required
0.8 mm island repairs and boolean union alter the source. It has 29 natural ring
contacts, no generated ties, and blocks approximately 51.5-52.1% in the rendered
SVG.

`bamboo-5b-55mm.svg` is the mechanically reinforced response to the first
Bamboo-5a print. The visible bamboo-joint notches remain, but all 55 required
island bridges increase from 0.8 to 1.4 mm. Six additional distributed 1.4 mm
cross-links create redundant load paths between substantial stalk/leaf regions,
especially through the middle, so one broken joint should not release an entire
stalk. It retains the full-density silhouettes and natural ring contacts.

`bamboo-5c-55mm.svg` targets the one center-right stalk still lost in the second
PETG print. Comparison with the Fusion screenshot showed that its long middle
section was not directly connected across either bamboo joint; it relied mainly
on a sideways leaf. Bamboo-5c retains Bamboo-5b's reinforcement and adds centered
2.0 mm tabs across that stalk's upper and lower joints. The surrounding white
side notches preserve the bamboo-joint appearance.

`bamboo-5c-neg-55mm.svg` is the internal negative of Bamboo-5c. The 55/51 mm
ring remains printed, former light openings become the printed interior, and the
bamboo stalks/leaves become openings. Seventeen short 1.4 mm passages through
narrow stalk joints and leaf contacts connect 18 negative-space regions into one
printed solid. It is approximately 45% open within the 51 mm aperture and 38.5%
open over the complete 55 mm disc once the solid ring is included.

`bamboo-6-neg-55mm.svg` establishes the new `-neg` name for negative-pattern
tests. It starts from portrait black-on-white vector art, fits the full source
canvas 54 mm wide, circle-crops it at the 51 mm aperture, and prints the white
background plus the exact outer ring. The bamboo becomes the opening. A 0.35 mm
opening expansion preserves the source composition while making its fine stems
useful for light transmission; one tiny supplemental open leaf removes an
otherwise enclosed printed sliver. Sixteen butt-ended 1.4 mm passages reconnect
17 printed background regions. The independently rendered result is one
connected printed component, approximately 45.7% open within the aperture and
39.3% open over the complete disc.

`bamboo-7-neg-55mm.svg` uses a cleaner single-path portrait bamboo source. The
full source canvas is again fitted 54 mm wide, with a 0.45 mm expansion applied
to the open bamboo silhouette. Its broad stalks and leaves already fill the
circle well, so no supplemental leaves are added. Four butt-ended 1.4 mm
passages connect five printed background regions. Independent rendering verifies
one connected printed component, approximately 45.3% open within the aperture
and 39.0% open over the complete disc.

### 50/46 mm production set

The first 50 mm family refits selected finished 55 mm artwork by the 46/51
aperture ratio (`0.901961`), then replaces the old ring with an exact 50/46 mm
ring. This is deliberately not a 50/55 whole-part scale, which would leave an
incorrect 46.36 mm opening. Each SVG below independently renders as one path and
one connected printed component:

| File | Pattern | Full-disc open | 46 mm aperture open |
| --- | --- | ---: | ---: |
| `tree-logo-50mm.svg` | positive | 57.66% | 68.10% |
| `bamboo-2-50mm.svg` | positive | 55.41% | 65.46% |
| `bamboo-2-neg-50mm.svg` | negative | 28.51% | 33.68% |
| `bamboo-2a-neg-50mm.svg` | negative, thin upper leaves | 31.16% | 36.82% |
| `bamboo-5c-50mm.svg` | positive | 45.51% | 53.76% |
| `bamboo-5c-neg-50mm.svg` | negative | 38.13% | 45.05% |
| `bamboo-6-neg-50mm.svg` | negative | 38.51% | 45.49% |
| `bamboo-7-neg-50mm.svg` | negative | 38.28% | 45.23% |
| `bamboo-8-neg-50mm.svg` | negative, clean segmented vector | 31.70% | 37.45% |
| `bamboo-stencil-50mm.svg` | positive | 56.50% | 66.75% |
| `bamboo-stencil-2-neg-50mm.svg` | negative color-stencil crop | 31.29% | 36.96% |

`bamboo-2-neg-55mm.svg` is the newly requested 55/51 mm negative companion to
Bamboo-2. It is 29.11% open over the full disc and 33.86% open within the
51 mm aperture.

`bamboo-2a-neg-55mm.svg` adds four open leaves near 10 o'clock and four near
2 o'clock, visually balancing the lower leaf fans near 7 and 5 o'clock. After
markup review, the new leaves were rebuilt as narrow, staggered lance shapes
with gray separation between them. They do not divide the printed background,
so no added repair passages are needed. Independent rendering verifies one
connected printed component and raises open area from 33.86% to 37.01% within
the aperture (31.82% over the full disc).

`bamboo-stencil-2-neg-50mm.svg` starts from the downloaded color
`bamboo-stencil-2.jpeg`. `tools/prepare_color_stencil.py` rejects the white
background and blue paintbrush, retaining 136 green stencil regions as the clean
black-on-white `bamboo-stencil-2-bw.png`. The negative crop enlarges that canopy
to 58 mm, shifts it 2 mm left and 0.5 mm down, and lets the 46 mm circle discard
the unimportant lower-left stem while retaining the strongest leaf clusters. A
light 0.08 mm opening expansion improves transmission without merging the leaf
slots. No background passages are required. The higher-resolution trace is
approximately 133 KB with 2,488 fitted segments and independently verifies as
one connected printed component.

`bamboo-8-neg-50mm.svg` starts from a clean vector logo whose source includes
its own heavy circular border. `tools/prepare_svg_negative_art.py` removes that
first border path and square-crops the 21 retained stalk/leaf shapes before the
negative conversion. The exact 50/46 mm production ring prints; the bamboo is
open. Because every source opening is already separated, the printed background
is one connected component without generated repair passages. It is 37.45% open
within the aperture and 31.70% open over the complete disc.

### Rebuilding a traced design

`tools/trace_gobo.py` replaces the ring in a gray-on-white source PNG with an
exact-size ring and traces only the interior artwork into optimized cubic Bezier
curves. It requires Python plus `numpy`, `Pillow`, and `potracer`.

```powershell
python -m pip install numpy Pillow potracer
python tools/trace_gobo.py `
  C:\path\to\source.png `
  tree-logo-55mm.svg `
  --preview tree-logo-55mm-preview.png
```

For disconnected dark-on-light stencil artwork, use the repair tool. It builds
a minimum-length network of short internal bridges, then adds six evenly spaced
ring ties:

```powershell
python tools/repair_bamboo_stencil.py `
  C:\path\to\source.jpg `
  bamboo-stencil-55mm.svg `
  --preview bamboo-stencil-55mm-preview.png
```

For a wide branching fan that should be fitted by height and cropped at the
circle sides:

```powershell
python tools/repair_bamboo_fan.py `
  C:\path\to\bamboo-2.png `
  bamboo-2-55mm.svg `
  --preview bamboo-2-55mm-preview.png
```

For green bamboo photographed on a light background:

```powershell
python tools/repair_bamboo_photo.py `
  C:\path\to\bamboo-3.png `
  bamboo-3-55mm.svg `
  --preview bamboo-3-55mm-preview.png
```

The upright Bamboo-4 source uses a lighter color threshold, natural widths, and
selected attachment angles:

```powershell
python tools/repair_bamboo_photo.py `
  C:\path\to\bamboo-4.png `
  bamboo-4-55mm.svg `
  --preview bamboo-4-55mm-preview.png `
  --art-height 52 `
  --green-excess 6 `
  --maximum-source-hole-pixels 1000 `
  --minimum-stem-width 0 `
  --thicken-radius 0 `
  --ring-bridge-angles=-90,-30,90,210
```

For a square black-and-white vector pattern, first render the source SVG to a
square 800 px PNG with a white background, then run:

```powershell
python tools/repair_bamboo_square.py `
  C:\path\to\bamboo-5-render-800.png `
  bamboo-5-55mm.svg `
  --preview bamboo-5-55mm-preview.png
```

This square-pattern workflow checks the number of natural ring contacts and
stops if fewer than five exist instead of silently adding ties.

To produce the untouched-black Bamboo-5a comparison, add:

```powershell
  --erosion-passes 0
```

For the plate-removal-reinforced Bamboo-5b variant, use:

```powershell
  --erosion-passes 0 `
  --internal-bridge-width 1.4 `
  --redundant-bridge-count 6 `
  --redundant-bridge-width 1.4
```

Bamboo-5c adds these targeted centered-millimeter joint bridges to the Bamboo-5b
options:

```powershell
  --target-bridge=5,-13,5,-11.5,2 `
  --target-bridge=5,0.5,5,1.9,2
```

To build a negative internal pattern from a finished positive gobo, render the
positive SVG to a square PNG and run:

```powershell
python tools/flip_gobo.py `
  C:\path\to\positive-gobo-render.png `
  bamboo-5c-neg-55mm.svg `
  --preview bamboo-5c-neg-55mm-preview.png
```

New negative-pattern work uses the `-neg` suffix. For direct black-on-white art,
render the source SVG to a high-resolution PNG with a white background, then run:

```powershell
python tools/negative_art_gobo.py `
  C:\path\to\bamboo-6-neg-render.png `
  bamboo-6-neg-55mm.svg `
  --preview bamboo-6-neg-working-preview.png `
  --art-width 54 `
  --open-expansion 0.35 `
  --leaf=-5,3.7,2.7,1.2,90,0.1 `
  --minimum-component-pixels 300 `
  --passage-width 1.4 `
  --trace-pixels-per-mm 12 `
  --trace-tolerance 1.1
```

The source black becomes open; source white and the ring become one printed
solid. The tool reports aperture and full-disc open percentages separately.

For a flat green stencil downloaded on a white product background, create the
clean black-on-white intermediate first:

```powershell
python tools/prepare_color_stencil.py `
  bamboo-stencil-2.jpeg `
  bamboo-stencil-2-bw.png
```

This color separation rejects non-green objects before circular fitting and
negative conversion.

For vector art that includes an unwanted source border, isolate the interior
paths into a square intermediate before rendering it to PNG:

```powershell
python tools/prepare_svg_negative_art.py `
  C:\path\to\bamboo-8.svg `
  C:\path\to\bamboo-8-interior.svg `
  --drop-leading-paths 1 `
  --crop 942,318,1860
```

This keeps the downloaded artwork's clean vector silhouettes while preventing
its decorative border from becoming an open annulus in the negative gobo.

To refit any finished 55/51 mm design to the exact 50/46 mm interface, render
the 55 mm SVG to a square raster and run:

```powershell
python tools/resize_gobo.py `
  C:\path\to\finished-55mm-render.png `
  design-50mm.svg `
  --source-od 55 --source-id 51 `
  --target-od 50 --target-id 46
```

The tool scales only the interior by 46/51, rebuilds the exact ring, repairs any
sub-pixel contact gaps, and refuses output unless the printed result is one
connected component.

To add leaf-shaped openings to a finished negative while retaining its exact
ring, use `tools/add_openings_gobo.py`. Repeated `--leaf` arguments use
`X,Y,LENGTH,WIDTH,ANGLE[,CURVE]` in centered millimeter coordinates. The tool
reconnects any printed background islands before tracing the final path.

The command reports blocked area, attachment count/width, and SVG curve
complexity. A traced file still needs a Fusion/Bambu slicer review for minimum
web widths and final mesh quality before production printing.

## Bamboo Leaves, 50 mm

- `bamboo-leaves-50mm-trace.svg` is the stroke-only drawing for tracing or manual CAD sketching.
- `bamboo-leaves-50mm-aperture.svg` has filled black slot shapes intended as cut-through apertures.

Geometry assumptions:

- Finished aperture disc diameter: 50 mm.
- Dashed guide circle: 46 mm diameter, leaving a 2 mm radial keep-out from the edge.
- Stem slots are roughly 0.9-1.1 mm wide.
- Leaf openings are intentionally simple almond shapes so they should still read after projection.

Print notes:

- Start with a thin disc, roughly 0.8-1.2 mm, and test against the actual LED throw.
- For FDM, review every slot in the slicer before printing; scale slot widths up if the slicer drops them.
- Treat this as an aesthetic prototype, not a final production gobo, until the shadow has been photographed through the lantern rig.
