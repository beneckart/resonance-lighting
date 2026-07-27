# ops/solarsim -- native solar-access sim (Python port of solar-visualizer)

Runs the solar-visualizer study (branch `solar-visualizer`, Elliot) natively:
no SketchUp, seconds per full sweep, so candidate light layouts can be scored
for solar access in the same loop as gobo spacing and localization geometry.

- `raytrace.py` -- embree raycasting of panel sample grids (7x5 per panel)
  against the woven-tree mesh, per shipped 10-min sun slot; 128-ray
  cosine-weighted sky-view factor.
- `power.py` -- the v5 power chain verbatim (Meinel clear-sky, lit^2 x 0.75
  partial-shade mismatch, calculated heat derate normalized to Ben's bench
  full-sun measurement, x0.95 dust, x0.63 field-cycle chain into the cell,
  /1.364 W measured full-RGBW draw).
- `solar_score.py` -- CLI: `--validate` (compare against the shipped 88-panel
  phase-2 dataset) or `--placement <json>` (score any placement; format =
  `canopy_positions_corrected.json` on the solar-visualizer-lights branch).
- `data/tree_draco.glb` + `data/tree_mesh.ply` -- the woven-tree occluder
  extracted from the viewer's embedded Draco blob (treev4: tower, rings,
  roof, limbs, shell, nets, roots, windchimes; 741k faces).
  `data/solar_phase2_data.json` -- Elliot's shipped dataset (sun vectors +
  reference numbers).

Deps: numpy, trimesh, embreex, rtree, DracoPy (all user-site installed
2026-07-17).

## Calibration status -- read before quoting numbers

Panel footprint fixed 2026-07-27: true 5"x3.5" (0.127x0.089 m) SOLAR_LIGHT
panel; the old 0.50x0.35 m stand-in (the .rb fallback branch) cost ~16 pts of
absolute energy. Validated against the regenerated 88-panel corrected-canopy
reference (branch solar-visualizer-lights, RERUN_2026-07-27.md):

- position RANKING: good (wh_day_batt Spearman 0.82, Pearson 0.85).
- ABSOLUTE energy: ~15% conservative (median wh ratio 0.85), localized:
  - power chain: near-exact given the reference lit+svf (ratio 0.95,
    Pearson 0.98; remainder is the shipped arrays' rounded lit ints and the
    mismatch flip at lit=1.0 -- bit-exact impossible per RERUN note).
  - raytrace: our occluder mesh (web-viewer Draco extract) over-occludes vs
    the now-pinned .skp reference state (SOLAR_REF hidden, everything else
    visible incl SITE_CONTEXT): mean lit -9 pts, svf -9 pts.

Use this tool to COMPARE layouts and rank positions. For bankable Wh, re-run
the SketchUp pipeline (turnkey on branch solar-visualizer-lights:
`src/canopy_setup_phase2_corrected.rb` + `solar_access_analysis.rb`).

First result (data/corrected-canopy-score-2026-07-17.json, old 0.50x0.35
footprint): the corrected 72-position canopy scores median 7.1 Wh/day into the
cell; the 6 relocated ex-stray lights go from Elliot's "6-30 min sun, must
move" to 1.4-2.4 h full-brightness runtime. SUPERSEDED by the SketchUp
reference rerun (RERUN_2026-07-27.md): the fills bank 3.3-8.7 h; with the
fixed footprint this sim scores them 2.0-7.7 h in the same order (weakest
CL-I29 NNW).
