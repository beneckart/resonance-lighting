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
- `solar_score.py` -- CLI: `--validate` (compare against the shipped 94-panel
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

Validated against the SketchUp reference on the shipped 94-panel set:

- position RANKING: excellent (daylight-lit Spearman 0.92-0.96 by class;
  wh_day_batt Spearman 0.87).
- ABSOLUTE energy: ~30% conservative (mean lit ~10 points lower than the
  reference on canopy; the lit^2 term compounds it). Likely cause: occluder-
  set differences between the web-viewer mesh and the .skp model state used
  for the reference run (layer visibility unknowable from here).

Use this tool to COMPARE layouts and rank positions. For bankable Wh, re-run
the SketchUp pipeline (turnkey on branch solar-visualizer-lights:
`src/canopy_setup_phase2_corrected.rb` + `solar_access_analysis.rb`).

First result (data/corrected-canopy-score-2026-07-17.json): the corrected
72-position canopy scores median 7.1 Wh/day into the cell (our conservative
scale); the 6 relocated ex-stray lights go from Elliot's "6-30 min sun, must
move" to 1.4-2.4 h full-brightness runtime -- viable but still the weakest
positions (inner/south fills).
