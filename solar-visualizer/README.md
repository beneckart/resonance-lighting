# Solar Visualizer — Resonance Tree solar-light study (v5, Ben-bench-calibrated)

**v5 (2026-07-17): the model is now calibrated against YOUR bench data, Ben** —
ADR 0026 panel-side INA measurements (P105 "5W" → 3.85 W real full sun, ×0.90
charger, ≈3.2 W battery-side peak) and the July field-cycle ledger (×0.63 daily
input→cell chain). Panel roles per ADR 0026 (P105 on downlights + wings). LED
draw = your measured 1.364 W battery-side full RGBW. **The planning number is
now `wh_day_batt` (Wh into each light's own cell)**: canopy median 10.5 Wh/day
→ ≈7.7 h full-brightness vs the ~10 h dark window (dim profile covers the
night) — which independently agrees with your field-cycle margin conclusion.

Solar access study for the tree's **94 solar-powered lights** — 16 ground/trunk
panels + **78 hanging canopy lights** — measured against the real woven geometry
at Black Rock City, Aug 30 – Sep 8 2026. From Elliot / blender-architect for
lighting + charging design (Ben: built for you to revise your calcs against).

## What's here

| File | What it is |
|---|---|
| `Resonance_Solar_3D.html` | **Open this first.** Self-contained interactive 3D viewer — the tree with all 94 panels colored by live measured output, clock slider (10-min steps) + play, sun riding its path, **"canopy lights" layer toggle**, click any light for its numbers (ring, bearing, hang height, drop, recommended height). Needs internet once (three.js CDN). |
| `solar_phase2_data.json` | The full dataset: per light — position (m, tree-centered, Z-up), aimed unit normal, sky-view factor, 84 × 10-min arrays (real W, no-loss ideal W, % face lit, shading-loss %, orientation-loss %), daily Wh real+ideal. Plus per-slot sun vectors and per-lantern metadata (ring, bearing, drop, 7-ft clearance, recommended hang height). |
| `Resonance_Solar_Access_Study.xlsx` | Same data as a spreadsheet: ground tab, canopy tab (ring/bearing/drop/7ft/rec-height), per-slot output + face-lit + shading-loss + orientation-loss grids, assumptions. |
| `Resonance_Solar_Report_Phase2.pdf` | 2-page written report: method, both phases, hang-height findings + the 7-ft rule, recommendations. |
| `src/` | **The full codebase.** The SketchUp raytracer that measures sun access + the HTML viewer/report that applies the v5 power chain. See `src/README.md` — built so you can re-run the model and revise the constants against your bench. |

## ⚡ Wiring reality (v4): every light is an ISLAND
Each light = its own panel + battery + LED. **Nothing is wired together** — no pooling,
no remote panels. Per-light runtime is the governing metric:
`runtime = daily Wh × 0.85 battery round-trip ÷ draw` (4 W RGBW full / 1 W dim).
Canopy median ≈ **4.3 h of full-brightness light per night** (all night dimmed);
16 lanterns < 3 h; the six inner-south lights get 6–30 min — they must MOVE
(relocating a light relocates its power plant) or stay decorative.

## Power model v3 — physics-only, no generic trim

- **Shading (most conservative):** beam × (fraction of face lit)² × a further **0.75
  series-string mismatch penalty whenever the face is partially shaded**. Full-sun
  faces take no penalty; shaded faces still collect sky diffuse.
- **Heat (calculated per 10-min slot):** cell temp = ambient (17→35 °C daily curve)
  + irradiance self-heating; −0.4 %/°C above 25 °C.
- **Dust:** ×0.95 (occasional wipe-downs).
- Rated 5 W appears only in brief best-case windows (cool, clean, dead-on sun); practical peak **≈4.0 W**.
- `w_ideal` / "ideal" columns = pure clear-sky nameplate math, zero losses (upper bound).
- Not modeled: wiring/charge-controller losses, battery acceptance, shade sails, vehicles.

## Headlines (real Wh/day per 5 W panel)

- **System ≈ 1,733 Wh/day** — canopy 1,424 + ground 309.
- Canopy rings: outer 17.8–25.9 (median 21.5) · middle 13.3–24.8 (median 20.8) ·
  inner 0.4–23.6 (median 13.1).
- Ground: roots 18–27.5 (RT-SSW best), S/SW trunk-ring arc 18–26, N side 9–13.
- **Six inner-south low lights are effectively solar-dead** (CL-I11/12/14/16/17/18,
  0.4–2.5 Wh ⇒ 6–30 min of light) and also hang below the 7-ft head-clearance floor.
  Remote panels are NOT possible (standalone lights) — relocate them or accept them dark.
- 24 lights gain 5–20 % by lengthening cords (per-light rec heights in the sheet),
  min hang = 7 ft to panel bottom.

## Provenance

~500k rays vs the Rhino-derived woven model in SketchUp (`Resonance_Solar_Study.skp`,
SUNEAST/Resonance_BM_Ops/3D_Models). Canopy positions = lighting `fixtures.json`.
Sun verified vs almanac (solar noon 12:57 PDT, alt 56.9°, noon shadow due north).
Scripts: resonance-os-v2 `apps/tree-blender/scripts/sketchup/`. Questions → Elliot.

## Run it offline (Ben — no live site needed)

Everything runs locally with no internet: `cd solar-visualizer && ./run.sh`, then open
`http://127.0.0.1:8777/Resonance_Solar_3D.html`. three.js + Draco are vendored under
`vendor/`. Full build + how-to-modify notes in **BUILD.md**; source + geometry in `src/`.
