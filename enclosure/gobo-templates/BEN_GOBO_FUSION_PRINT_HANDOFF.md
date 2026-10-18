# GOBO Fusion and Print Handoff for Ben

Steve supplied these answers on 2026-08-11. This package complements
[`BEN_GOBO_ART_HANDOFF.md`](BEN_GOBO_ART_HANDOFF.md), which contains the full
art-preparation rules, mechanical lessons, and example catalog.

## Reference files

### Tree logo and production geometry

- [Successful tree-logo Fusion design](fusion-print-reference/tree-aperture-v10.f3d)
- [Tree-logo Bambu Studio project](fusion-print-reference/tree-aperture%20v8.3mf)
- [50 mm tree-logo SVG](tree-logo-50mm.svg)
- [50 mm tree-logo preview](tree-logo-50mm-preview.png)
- [Native Fusion Castle Nut body](fusion-print-reference/Castle.f3d)
- [Castle Nut STEP export](fusion-print-reference/Castle.step)

### Bamboo-5 structural iteration

- [Successful Bamboo-5c Fusion design](fusion-print-reference/Bamboo-5c%20v1.f3d)
- [Bamboo-5c negative Fusion design](fusion-print-reference/Bamboo-5c-neg%20v2.f3d)
- [Bamboo-5a failed print photo](fusion-print-reference/bamboo-5a-print.jpg)
- [Bamboo-5b Fusion screenshot](fusion-print-reference/Bamboo-5b-Fusion-Screenshot.png)
- [Bamboo-5b failed print photo](fusion-print-reference/Bamboo-5b-print.jpg)

No successful Tree Logo print photo is included. The Bamboo-5 sequence is the
better documented example because the Fusion screenshot and print photos show
where weak connections failed and how the later revisions addressed them.

## Questions and Steve's answers

### Q1 - Successful tree-logo Fusion `.f3d` and exported SVG

**A1:** Creation of a good SVG from inspiration artwork is step one; the SVG is
not exported from Fusion. Codex developed Python tools to crop, reconnect,
scale, boolean-union, and simplify the SVG while applying the current GOBO
rules.

In Fusion, the SVG is inserted into a sketch on the XY plane and extruded 3 mm
in the Z direction. A second 3 mm-tall Castle Nut body sits on top. Its segmented
tines help lock the GOBO into the bamboo tube.

Use these files:

- `fusion-print-reference/tree-aperture-v10.f3d`
- `tree-logo-50mm.svg`
- `tree-logo-50mm-preview.png`

### What makes a good Fusion-ready SVG

- Use bold, high-contrast filled shapes. Clean SVG is preferred; a high-
  resolution PNG is useful when the source SVG contains excessive trace noise.
- Fit one of the exact production interfaces: 55 mm OD / 51 mm ID or 50 mm OD /
  46 mm ID. Both rings have a 2 mm radial wall.
- Center the circle on `(0,0)` and use Fusion's 96-DPI SVG coordinate scale.
- Finish with a single boolean-unioned printed profile. There must be no loose
  islands, overlapping tie outlines, or internal lines for Steve to trim.
- Positive patterns need at least five approximately distributed connections to
  the outer ring. Natural cropped contacts are preferable to long radial ties.
- Use 1.4 mm nominal connections for ordinary island or stalk repairs. Use a
  centered 2.0 mm tab for a critical stalk joint that has failed before.
- Give important stalks redundant load paths. A tiny leaf tip must not be the
  only connection supporting a large section.
- Preserve identifying features such as bamboo joints, but small leaves may be
  simplified or treated as sacrificial.
- Design for the print to survive slicing, handling, and removal from a sticky
  PETG plate, not merely to appear connected on screen.
- Keep curves smooth and reasonably simplified so Fusion does not bog down on
  thousands of unnecessary facets.

### Positive and negative (`-neg`) rules

- A positive design prints the subject and outer ring; the surrounding areas
  are open for light.
- A `-neg` design prints the background and exact outer ring; the subject's
  stalks and leaves become openings.
- Reversing a design reverses the structural problem. Any background regions
  isolated by the open subject must be reconnected with printed passages,
  normally 1.4 mm wide.
- Open selected stalk joints or leaf contacts when necessary so printed plastic
  can flow through and every background region belongs to one solid profile.
- Report open area both inside the 51 or 46 mm aperture and over the complete
  disc because the solid ring lowers the full-disc open percentage.
- Use `-neg-55mm` or `-neg-50mm` in filenames so polarity and ring size are
  unambiguous.

### Q2 - Bambu Studio `.3mf`

**A2:** Use [tree-aperture v8.3mf](fusion-print-reference/tree-aperture%20v8.3mf).
The 3MF carries more useful print context than an STL.

### Q3 - Printer model and nozzle diameter

**A3:** Bambu Lab H2D with a 0.4 mm nozzle.

### Q4 - Material and brand

**A4:** Bambu Lab PETG Basic, White.

### Q5 - Layer height and extrusion-line width

**A5:** Slice in Bambu Studio using `0.20mm Standard @BBL H2D`:

- layer height: 0.20 mm;
- line width: 0.42 mm.

### Q6 - GOBO Z thickness

**A6:** The GOBO design is 3 mm thick. The Castle Nut body/tines add another
3 mm in height.

### Q7 - Plate, release method, and print orientation

**A7:**

- heated Textured PEI Plate;
- no adhesive;
- release by flexing the plate and carefully prying;
- print with the design side down and locking tines pointing up.

PETG adhesion is a meaningful mechanical load. The part must be robust enough
that prying it from the plate does not detach stalks or leaves.

### Q8 - Successfully printed tree-logo size

**A8:** Both the 50 mm and 55 mm versions printed successfully after the
interior design was properly attached to the outer ring.

### Q9 - Fusion flange/tine template

**A9:** Steve normally uses SVG and native Fusion rather than STEP, but both
requested forms are supplied:

- [Castle.f3d](fusion-print-reference/Castle.f3d)
- [Castle.step](fusion-print-reference/Castle.step)

### Q10 - Successful and failed-print examples

**A10:** There is no Tree Logo print photo in this package. Bamboo-5 is the best
recorded example of the iteration process. Phone photos and markups documented
the Bamboo-5a and Bamboo-5b failures; Bamboo-5c was the successful reinforced
revision.

Use these Fusion models:

- [Bamboo-5c v1.f3d](fusion-print-reference/Bamboo-5c%20v1.f3d)
- [Bamboo-5c-neg v2.f3d](fusion-print-reference/Bamboo-5c-neg%20v2.f3d)

The accompanying photos above show the prior plate-removal failures and the
Fusion geometry used to diagnose them.
