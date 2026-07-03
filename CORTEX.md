# Resonance Tree — Lighting Cortex (controller / show system)

## ▶ Run it (2 minutes, no hardware needed)
```bash
git clone --branch Lighting-Controller https://github.com/beneckart/resonance-lighting.git
cd resonance-lighting/app
npm ci
npm run dev        # → http://localhost:5173  (it also prints a Network URL for iPads/laptops on your WiFi)
```
Requirements: **Node 18+** and a WebGL2 browser (Chrome/Edge/Safari). Everything the app
needs (tree model, `fixtures.json`, MIDI scores, test audio) is in the repo — a fresh
clone runs the full digital twin: **118 real fixtures**, four operator modes
(🌱 Interactive · 🎬 Light Show · 🎵 Sound · 🔧 Calibrate), the Game-of-Light presence
system with colour themes, auto-calibration & testing, DJ decks, the self-playing piano,
and one-click video+audio recording. Full guide + 60-second tour: [`app/README.md`](app/README.md).

This branch adds the **optional "cortex" layer** on top of Ben's autonomous solar-mesh
fixtures (the "brainstem", on `main`).

**It rides Ben's architecture — it never replaces it.** Control-plane only over ESP-NOW,
never pixels; the cortex "wakes at dusk and dies invisibly" — the lights run without it.

## What lives here
- `app/`   — React-Three-Fiber PWA: the hardware-true digital twin + full control
  system (this is the thing you run — see above). Split-screen dock UI: tree on the
  left, one organized mode-first panel on the right; groups can each run their OWN
  mode simultaneously (canopy interactive while the chandelier follows sound).
- `cortex/` — Python services for the Jetson hub: twin-server, occupancy, env, voice, camp bridge. PowerFeather master = ESP-NOW USB radio-modem.
- `sim/`   — firmware pattern core compiled C++ → WASM (golden-frame parity with real fixtures; develop without hardware).
- `docs/research/` — the design corpus: **PRD-lighting-environment.md** (the job) + the 5-doc dossier (`01…05-*.md`) + Ben's existing research.

## Status (2026-07-02)
The keystone (`fixtures.json` from the Blender export — 118 fixtures, schema 0.3) landed
and the twin is feature-complete against it. Highlights, all sim-verified + unit-tested
(160 tests):
- **Interactive mode** — decentralised cellular automata on each fixture's pre-baked
  k-nearest-neighbour list (Ben's BACKGROUND.md mesh spec): a true Game of Life plus
  excitable-media / reaction-diffusion / firefly-sync rules. Tap the tree = a presence
  sensor firing (per Ben's PRESENCE_SENSING doc: ToF downward eye on downlights);
  disturbances trickle outward hop-by-hop; a rules editor sets colour / brightness /
  time-on / spread (with never-same-as-last + brightness-range constraints); colour
  THEMES hold a mood; the Game-of-Light lifecycle (first-visitor ignition → live
  nodes by quadrant → organic Unity/community celebration when a chain rings the tree).
- **Light Show mode** — scope first (whole tree → groups → single lights), patterns,
  three authored ~5-min shows, the sampled piano playing real scores.
- **Sound mode** — beat-tracked audio engine, DJ decks, AI auto-pilot.
- **Calibrate mode** — commissioning (MAC↔slot), and auto-calibration: all off → every
  light solo-steps (group by group) with confirm-or-timeout against its own reported
  heartbeat (provably catches dead fixtures), ToF height-histogram self-location, JSON
  report export that doubles as the photogrammetry frame-sync log.

## Owned by
`lighting-architect` agent (boot: `boot-lighting`). Consumes `fixtures.json` from the
Blender placement workflow; never places fixtures itself. Coordinates with Ben via this
repo (LOG/PR) and upstream `beneckart/resonance-lighting`.

## Remotes
- `origin`   = `resonanceart/resonance-lighting` (our fork — working pushes)
- `upstream` = `beneckart/resonance-lighting` (Ben's repo — this work lives on its `Lighting-Controller` branch)
