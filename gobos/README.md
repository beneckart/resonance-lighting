# Gobos — bamboo-leaf projection discs for the lantern fleet

Gobo discs that project bamboo-branch shadows. Positive design: the ring +
branches + leaves are the printed material; light floods the open field.

**STANDARD (Steve, 2026-08-13): 46 mm OD / 2 mm ring / 42 mm ID, single disk.**
Deliverables are **SVG + PNG preview only** — Steve builds the 3D in Fusion,
so no STLs are produced for the 46 mm line.

## Inventory — `designs-46mm/` (CANONICAL)

37 verified designs, `chatgpt-01-46mm` … `chatgpt-37-46mm`, each as:

| file | use |
|---|---|
| `*-46mm.svg` | vector master, true 46 mm (mm units, centred origin, evenodd) — Fusion |
| `*-46mm.png` | preview rendered from the final vectors |

`contact-46mm.png` is the full-set overview. Source art in `source-art/`.
`legacy-50mm/` holds the earlier 50 mm cut (incl. STLs) — superseded, kept for
reference only.

Every design passed the automated gate before landing here:
one connected piece · watertight mesh · min printed feature ≥ ~1.4 mm
(≥3 extrusion lines at 0.42 mm) · blocked area 32–46 % of the disc.

## Print recipe (Steve's working setup)

PETG Basic White · 0.4 mm nozzle · 0.20 mm layers · 0.42 mm line width ·
textured PEI plate · **design face down** · 2 walls, high infill · no supports.

## Making more — `tools/`

- `art_to_p1s.py` — converts clean black/white art (ChatGPT etc.) into the
  verified STL+SVG+PNG triple. Detects the drawn ring and swaps in the true
  2 mm production ring, heals nicks/holes, thickens every stroke with a
  uniform 0.58 mm vector "ink bleed" (no lumpy raster dilation), Chaikin-smooths
  the outlines, auto-repairs the mesh, and refuses to emit anything that fails
  the print gate. `python3 art_to_p1s.py --png art.png --name chatgpt-12`
- `CHATGPT_PROMPT.md` — the image-generation prompt that produced these,
  plus variation levers. Target: 76 unique discs for the fleet.
- `gobo_gen.py` — parametric fallback generator (sumi-e leaf clusters on
  ring-to-ring culms) when we want variations without an art pass.

Workshop copy of everything: Elliot's `~/Downloads/gobo-p1s/` + Drive
`AI Workshop / Gobo P1S LATEST`.
