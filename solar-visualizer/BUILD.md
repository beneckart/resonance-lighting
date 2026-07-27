# How this was built + how to run and modify it locally

Everything here runs **fully offline** — no internet, no live site, no build step.
three.js and the Draco decoder are vendored under `vendor/`, and the tree geometry
is embedded in the viewer. This is the complete, self-contained package: open it,
read it, change it, re-run it.

---

## Run it locally (30 seconds)

The viewer is an ES-module page, so browsers won't load it over `file://` (module
CORS). Serve the folder over a trivial local server instead:

```bash
cd solar-visualizer
./run.sh                 # = python3 -m http.server 8777
# then open http://127.0.0.1:8777/Resonance_Solar_3D.html
```

Any static server works (`python3 -m http.server`, `npx serve`, VS Code Live
Server). No network needed once the folder is on disk. Verified offline with the
vendored deps — the sun animates, panels color by live watts, the tree renders.

The spreadsheet (`Resonance_Solar_Access_Study.xlsx`) and the PDF report open
directly with no server.

---

## What each file is

```
solar-visualizer/
├─ Resonance_Solar_3D.html        the interactive viewer (geometry + data inline; vendored three.js)
├─ Resonance_Solar_Access_Study.xlsx   full dataset as a spreadsheet
├─ Resonance_Solar_Report_Phase2.pdf   2-page written report
├─ solar_phase2_data.json         THE dataset: 94 panels × 84 slots, real+ideal W, losses, Wh/day
├─ run.sh                         one-line local server
├─ vendor/                        three.js 0.160 + addons + Draco 1.5.7 decoder (offline deps)
└─ src/                           the source that generates everything
   ├─ solar_lights_setup.rb       SketchUp: places the movable fixtures on the tree
   ├─ solar_access_analysis.rb    SketchUp: the raytracer — measures sun access → data
   ├─ solar_3d_template.html      viewer template (DATA is injected here at bake time)
   ├─ Resonance_Solar_Report.html HTML report source
   ├─ model/Resonance_Solar_Study.skp   the geometry the raytracer runs against (1.3 MB)
   └─ README.md                   pipeline + the v5 power-chain formula + gotchas
```

---

## How it was created (provenance)

**1 · Geometry.** The tree + build-site geometry originates in Blender
(`tree_split.blend`, the master woven model — available from Elliot; not in git
because it's a large binary). It was scaled **0.0985 m/unit** and brought into
SketchUp as `src/model/Resonance_Solar_Study.skp` — that `.skp` is the exact
geometry the raytracer sees, so it's included in full. Canopy lantern positions
come from the lighting `fixtures.json` in this repo. The tree shown in the 3D
viewer is a Draco-compressed GLB of that same model, embedded in the HTML.

**2 · Measurement (the raytracer).** `src/solar_access_analysis.rb` runs inside
SketchUp against that model. It geolocates the sun to Black Rock City, steps it in
10-minute slots Aug 30 – Sep 8 2026, and `raytest`s each panel against the real
woven strut geometry (gaps and all). Output per panel per slot: fraction of the
panel face lit + full-sun minutes. Fixtures are live component instances — drag
one and re-run, the numbers update. Sun-timezone and raytest gotchas are documented
in the script header and `src/README.md` (they will bite you otherwise).

**3 · Power model (v5, calibrated on Ben's bench).** The raytested access is turned
into watts and Wh/day by the chain that lives in the viewer/report JS:

```
panel_DC   = POA × 5W_nameplate × 0.77 tolerance/controller
                 × playa_excess_heat(/0.93 ref, capped 1) × 0.95 dust
wh_day_batt = Σ(panel_DC over day) × 0.63     # ADR-0026 + July field-cycle chain, energy into the cell
runtime_h   = wh_day_batt ÷ 1.364             # measured full-RGBW battery-side draw
```

Constants are Ben's bench (ADR 0026 panel-side INA + `SOLAR_FIELD_CYCLE_P105_P126_2026-07`).

---

## How to modify it

- **Change a power assumption** (panel nameplate, dust, the ×0.63 chain, LED draw):
  edit the constants block in `Resonance_Solar_3D.html` (and `src/solar_3d_template.html`
  / `src/Resonance_Solar_Report.html` to keep source + report in sync). The viewer
  re-derives every panel's watts on reload. One place, whole model updates.
- **Move a light / change the tree:** open `src/model/Resonance_Solar_Study.skp` in
  SketchUp, drag fixtures or edit geometry, re-run `src/solar_access_analysis.rb`.
  It writes a fresh access dataset.
- **Rebuild the viewer from new data:** drop the new dataset into
  `src/solar_3d_template.html`'s `DATA` blob (replace the `__DATA__` token) and save
  as `Resonance_Solar_3D.html`.

Questions → Elliot.
