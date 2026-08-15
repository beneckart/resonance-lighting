# Solar visualizer — source

The code that produces everything in the folder above. Two stages: a SketchUp
raytracer that measures sun access against the real woven geometry, and an HTML
viewer/report that applies the v5 power chain and draws it.

## Files

| File | Stage | What it does |
|---|---|---|
| `solar_lights_setup.rb` | 1 · geometry | Places the movable `SOLAR_LIGHT` component instances on the tree (trunk-ring, roots, doors; canopy lanterns in the phase-2 variant). Positions derive from `tree_split.blend` part centroids scaled 0.0985 m/unit. Every instance z-snaps onto the weave via `raytest`. Run once to seed a model; fixtures are then draggable. |
| `solar_access_analysis.rb` | 1 · measure | The raytracer. Reads the **live** fixture positions (re-run freely after dragging), geolocates ShadowInfo to BRC, steps the sun in 10-min slots Aug 30–Sep 8, and `raytest`s each panel point along the sun ray. Writes per-panel per-slot % face lit + full-sun minutes → the dataset. |
| `solar_3d_template.html` | 2 · viewer | The interactive 3D viewer template. Loads a `DATA` blob (the JSON above), applies the **v5 power chain in JS**, colors each panel by live watts, animates the sun. Baking `DATA` inline produces the shipped `Resonance_Solar_3D.html`. |
| `Resonance_Solar_Report.html` | 2 · report | Standalone HTML written report (surface grids, shade windows, per-panel angle optimizer). Same power chain, tabular output. |

## The v5 power chain (lives in the HTML JS)

Per panel, per 10-min slot, from measured plane-of-array irradiance (POA) off the rays:

```
panel_DC = POA × 5W_nameplate × 0.77 tolerance/controller
                × playa_excess_heat(/0.93 ref, capped at 1)
                × 0.95 dust
wh_day_batt = Σ(panel_DC over day) × 0.63     # ADR-0026 + July field-cycle chain → energy into the cell
runtime_h   = wh_day_batt ÷ 1.364             # your measured full-RGBW battery-side draw
```

`wh_day_batt` is the planning number. Constants are Ben's bench (ADR 0026 panel-side
INA + `SOLAR_FIELD_CYCLE_P105_P126_2026-07.md`). Change a constant in one place in
the HTML JS and the whole viewer re-derives.

## To reproduce

1. Open `Resonance_Solar_Study.skp` (SUNEAST `Resonance_BM_Ops/3D_Models/`) in SketchUp
   with the sketchup-mcp bridge, or paste the `.rb` into the Ruby console.
2. Run `solar_lights_setup.rb` (seed fixtures), drag any you want to move, then
   `solar_access_analysis.rb` (measure). It writes the access dataset.
3. Fold that dataset into `solar_3d_template.html`'s `DATA` blob → open in a browser.

## Gotchas (hard-won — see script headers for the long form)

- **Sun timezone:** pass BRC wall-clock as `Time.utc(2026,8,30,12)`. SketchUp applies
  `TZOffset` to a Time object's wall-clock fields; a machine-local `Time.new` gets the
  offset applied twice and puts the sun ~7 h off. `SunDirection` points *toward* the sun.
- **Raytest through the weave:** offset the start point 0.15" along the sun ray so it
  doesn't self-hit; a `nil` hit = lit. Single rays slip through weave gaps — the panel
  probe uses a fan (rays × heights) so it lands on the real outer bark, not inner trunk.
- **Label geometry shades panels:** hide the `SOLAR_REF` / name-label layer during a
  sweep or it silently drops output (caught a 33.6→28.9 Wh drop this way).

Questions → Elliot.
