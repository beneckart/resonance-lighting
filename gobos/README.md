# Gobos — bamboo-leaf projection discs for the lantern fleet

3D-printed gobo discs (Bambu Lab P1S) that project bamboo-branch shadows.
Positive design: the ring + branches + leaves are the printed material; light
floods the open field. 50 mm OD, 2 mm ring, 3 mm thick.

## Inventory — `designs/`

15 verified designs, `chatgpt-01` … `chatgpt-15`. Per design:

| file | use |
|---|---|
| `*.stl` | drag into Bambu Studio and print — watertight, one body |
| `*.svg` | vector master (mm units, centred origin, evenodd) — Fusion / laser |
| `*.png` | preview rendered from the final print vectors |

`chatgpt_contact.png` is the 15-up overview. Source art in `source-art/`.

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
