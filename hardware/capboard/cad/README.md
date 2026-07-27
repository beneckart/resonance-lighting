# Cap-bank v1.0 — CAD exports (for plate / enclosure fitting)

Generated from `../build/capbank.kicad_pcb`. Regenerate with:

```bash
python3 generate_capboard.py
kicad-cli pcb export step --subst-models --no-dnp -o cad/capbank-v1.0.step build/capbank.kicad_pcb
kicad-cli pcb export step --board-only -o cad/capbank-v1.0-board-only.step build/capbank.kicad_pcb
kicad-cli pcb export dxf --layers Edge.Cuts --output-units mm -o cad/ build/capbank.kicad_pcb
kicad-cli pcb export vrml -o cad/capbank-v1.0.wrl build/capbank.kicad_pcb
```

| File | What it is | Use for |
|---|---|---|
| `capbank-v1.0.step` | full board + all component solids | clearance, plate fitting, assembly mock-ups |
| `capbank-v1.0-board-only.step` | bare PCB, no components | quick reference / lightweight import |
| `capbank-Edge_Cuts.dxf` | outline, slots, holes (2D, mm) | plate cutouts, drill patterns, laser |
| `capbank-v1.0.wrl` | VRML with colors | viewers that prefer VRML over STEP |

## Key dimensions

Origin is the board's **top-left corner** in KiCad convention: +X right, +Y
**down**. Most CAD packages flip Y on import — check which corner your origin
landed on before trusting hole positions.

- **Board**: 88.0 × 40.0 mm, 1.6 mm FR-4, 2 layer.
- **Mounting holes**: 4 × Ø3.2 mm (M3 clearance), non-plated, at
  **(3.2, 3.2), (84.8, 3.2), (84.8, 36.8), (3.2, 36.8)** — one per corner,
  3.2 mm in from each edge. Intended for nylon standoffs.
- **Tallest components**: the three 22,000 µF cans, Ø18 × 35.5 mm, standing on
  the top face → **≈37.1 mm total stack height** (1.6 mm board + 35.5 mm can).
  Can centers sit at y = 18.3 mm, x = 16.25 / 37.25 / 58.25 mm (the footprint
  origin is the + lead; can bodies are centered ≈3.75 mm right of that).
- **Zip-tie slots**: 4 × 1.8 × 10 mm through-slots at x = 9.7, 30.5, 51.5,
  72.3 mm, centered on y = 18.3 mm — they flank the cans so the can *bodies*
  can be lashed down, which they need on washboard roads.
- **Connectors**: all along the bottom edge, centered on y = 35.5 mm, **vertical
  entry** — allow ~20 mm of clearance *above* them for the mating housings and
  wire bend radius, in addition to the 37 mm cap height.
- **RX480E receiver** (optional, docks in the RECVR socket): tops out ≈20 mm
  above the board and stays entirely **within** the board footprint, so it does
  not grow the envelope in X or Y. Not included in these exports — see
  `../viz/` for a board with the receiver modeled in place.

## ⚠ Note on cap height

Through 2026-07-26 these renders and exports used KiCad's default 3D model for
this footprint, which is a **20 mm** can — 15 mm shorter than reality. Fixed
2026-07-27; the exports here use the true 35.5 mm model. If Steve pulled any
STEP or screenshot before that date, re-pull.
