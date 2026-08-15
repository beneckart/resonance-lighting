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

## Production minimum-feature baseline

As of 2026-08-11, Steve's successfully printed "Resonance Tree" gobo source is
not yet present in this repository. Do not interpret the 0.9-1.1 mm prototype
stem-slot width above as a proven structural or manufacturing minimum.

When the native source arrives, measure and record these independently:

- the thinnest connected solid web, which controls structural integrity;
- the narrowest open slot that survives slicing and printing, which controls the
  projected line detail;
- the plate thickness in Z;
- nozzle diameter, extrusion line width, material, layer height, print orientation,
  and slicer profile.

Use the proven solid-web measurement as the empirical lower bound for generated
patterns, subject to a production margin chosen after a small calibration coupon.
The generator should reject disconnected solid islands and apertures that create
unsupported pieces even when every local feature passes the width check.
