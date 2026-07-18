# Canopy placement fix for the phase-2 solar study

From Ben + Claude (lighting workstream), 2026-07-17. For Elliot / blender-architect.

## What we found

The 78-light canopy set in `solar_phase2_data.json` inherits three artifacts
from the raw lantern export (`fixtures.json` 0.3.1, "PROCEDURAL first-pass"):

1. **6 trunk strays** -- CL-I11, CL-I12, CL-I14, CL-I16, CL-I17, CL-I18 sit
   within 12 cm of the trunk axis at z 0.05-0.94 m (the vertical stack of
   panels on the ground inside the tree). These are export glitches, not
   lantern positions.
2. **6 stacked duplicates** -- CL-O04/O05, CL-I04/I05, CL-I08/I09,
   CL-O12/O13/O14, CL-I17/I18, CL-I19/I20 are pairs (one triple) at literally
   identical coordinates: 78 lights but only 72 distinct positions.
3. **6 ring holes** -- the rings should be 24/24/24 but come out 20/22/24
   distinct (4 inner + 2 middle slots missing; visible as gaps).

We hit the identical artifacts in the auto-localization study on `main`
(`docs/tests/AUTOLOCATE_RSSI_SIM_FEASIBILITY_2026-07-12.md`) and patched them
there: the 6 strays move into the 6 ring holes (slot positions inferred from
each ring's angular gaps), duplicates stay flagged.

## The fix input

`canopy_positions_corrected.json` (this directory): the corrected canopy as
**72 distinct attach positions** (24/24/24, radii ~2.60 / 4.12 / 4.97 m), in
your convention -- meters, tree-centered, Z-up, at your calibrated 0.0985
m/unit scale. Six entries are marked `moved_from_trunk_stray` (these need
fresh raytracing -- their old sun-access data described the trunk floor);
six are marked `duplicate_of` (drop or ignore -- their data double-counts
their twin).

Everything else (the 16 ground/trunk lights, the power chain, the viewer) is
untouched and great -- we are in fact adopting your TB-01..12 + RT-* spots as
the uplight candidate positions for the lighting layout.

## Ask

Re-run the `src/` raytracer with the corrected canopy placement (only the 12
affected positions strictly need new rays: 6 moved + 6 hole-fills are the
same thing here; the other 60 are unchanged) and regenerate
`solar_phase2_data.json`. Happy to adapt naming/format if the Ruby side wants
it different -- ping Ben on WhatsApp.
