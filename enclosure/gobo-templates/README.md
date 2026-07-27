# Gobo Templates

Traceable and printable 2D pattern sources for replaceable shadow apertures.

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
