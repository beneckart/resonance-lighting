# Resonance Tree — Mirror Twin (`app/`)

A real-time, hardware-true 3D **digital twin + control/show system** for the Resonance
Tree: the actual bamboo tree rendered with its lanterns glowing, driven by music, playable
by a DJ/VJ, scriptable by an LLM, and ready to bridge to Ben's solar-mesh fixtures.
Built offline-first (PWA), control-plane only (params, never pixels).

## Run
```bash
cd app
npm install
npm run dev        # http://localhost:5173
```
Other scripts: `npm run build` · `npm test` (Vitest) · `npm run e2e` (Playwright) ·
`npm run check` (build + test + e2e).

## What it is
- **Real tree**: loads `public/fixtures.json` (78 real canopy lights exported from the
  Blender model) + `public/tree-context.glb` (decimated bamboo) — renders as the tree.
- **Mirror / truth loop**: the render shows each fixture's *reported* state (a mock
  heartbeat transport stands in for ESP-NOW; swap it later, nothing else changes).
- **Visuals**: bloom + HDR-gain lanterns, volumetric beams (per fixture, beam-angle
  sized), gobo mandala projected on the ground, ACES tonemapping, hero camera, 3
  visualizers (lanterns / orbs / wire).
- **Patterns**: solid · breathe · chase · ripple · sparkle · sequence (7 modes) ·
  spectrum · tricolor, + element modes (wind / ember / rain / beacon).
- **Audio (the spine)**: mic / song / built-in test track → FFT, spectral-flux onset,
  interval BPM, asymmetric envelope followers, AGC, drop detection. BPM→sequencer tempo,
  onset→flash, drop→burst. (BeatTracker unit-tested at 124 & 90 BPM.)
- **DJ**: crossfader (A↔B look blend), 3-band EQ→tree-zone, master intensity, strobe.
- **Auto-VJ**: shuffle-bag look switching on the phrase. **Cues**: save/recall/delete looks.
- **Command console / LLM surface**: type or paste a command script (one per line). See
  `../docs/research/13-LLM-CONTROL-GRAMMAR.md` for the grammar.
- **Control plane**: ESP-NOW channel + Protocol-v1 param encoder (`protocol.ts`) — the
  seam to the real mesh (control params, not pixels).

## Command grammar (quick ref)
```
pattern <id> | hue 0.5 | bri 0.8 | speed 1.5
<target> color <#hex|name> | <target> on | <target> off
  target = all | zone low|mid|high | range a-b | every n | fixture <id|seq>
on | off | clear
```

## Re-export the model (optional)
`scripts/blender/export_tree.py` (fixtures) + `export_context.py` (geometry) run via
headless Blender against the source `.blend`. Commit only the small `.glb` + `fixtures.json`.

## Stack
Vite · React 18 · TypeScript · React Three Fiber + drei · @react-three/postprocessing ·
zustand · Web Audio · vite-plugin-pwa. Tests: Vitest + Playwright.
