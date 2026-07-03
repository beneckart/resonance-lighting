# Resonance Tree — Mirror Twin (`app/`)

A real-time, hardware-true 3D **digital twin + control/show system** for the Resonance
Tree: the actual bamboo tree rendered with its lanterns glowing, driven by music, playable
by a DJ/VJ, scriptable by an LLM, and ready to bridge to Ben's solar-mesh fixtures.
Built offline-first (PWA), control-plane only (params, never pixels).

## Run (fresh clone → live twin in ~2 min)
```bash
git clone --branch Lighting-Controller https://github.com/beneckart/resonance-lighting.git
cd resonance-lighting/app
npm ci             # Node 18+ required
npm run dev        # http://localhost:5173 (+ a Network URL for other devices)
```
Everything needed is in the repo — the 3-D tree geometry (`tree-context.glb`), the real
lantern + chandelier bodies, `fixtures.json` (118 fixtures from the Blender export),
gobos, photometry (`downlight.ies`), MIDI scores and test audio — so a clone runs the
exact same system and controls. Two footnotes: (1) the piano's *sampled* grand-piano
sound streams once from a CDN on first play (the lights and the built-in synth fallback
are fully offline); (2) layouts/cues/themes persist per-browser (localStorage), so each
operator starts from clean defaults. Other scripts: `npm run build` · `npm test`
(Vitest) · `npm run e2e` (Playwright) · `npm run check` (build + test + e2e).

## The four operator modes (pick first, top of the panel)
- 🌱 **Interactive** — the tree is reactive; you only set the rules. Tap the tree =
  a presence sensor fires; the Game of Life trickles the disturbance outward.
  Rules editor (colour/brightness/time-on/spread + never-same-as-last, ranges),
  colour THEMES (⚡ Energize · 🕯 Intimate · 💗 Love · 🌊 Ocean …), and the
  Game-of-Light lifecycle: arm → first-visitor ignition → visitors drop living
  nodes by quadrant → a chain all the way around = 🌈 Unity (organic only).
- 🎬 **Light Show** — scope first (whole tree · custom groups · single lights),
  sliders + patterns, three authored shows, the piano playing real scores.
  Groups can each run their OWN mode simultaneously (canopy interactive while
  the chandelier follows sound) — pick a group chip, assign it a mode.
- 🎵 **Sound** — beat-tracked audio engine, DJ decks (only visible here), AI-VJ.
- 🔧 **Calibrate** — commissioning (MAC↔slot) + AUTO-CALIBRATION: all lights off,
  each light solo-steps group-by-group with confirm-or-timeout against its own
  reported heartbeat (catches dead/wrong fixtures), ToF self-location histogram,
  JSON report = the photogrammetry frame-sync log.

## Play with it from another computer / iPad (LAN — no deploy)
The dev server listens on the whole network (`server.host: true`). From the machine
running it, find its address, then open that from any device on the same WiFi:
```bash
npm run dev              # prints  ➜ Network: http://192.168.x.x:5173
```
Open `http://<that-ip>:5173` on Ben's laptop / an iPad. Full-screen on iPad: share →
"Add to Home Screen" (it's a PWA; it launches chromeless and keeps working offline).

### 60-second tour (for Ben)
- **Tap the tree** (in 🌱 Interactivity with a CA rule active) — fires a simulated
  presence sensor at the nearest downlight; the disturbance propagates via each
  fixture's pre-baked k-nearest-neighbour list (your flash neighbour table).
- **🎇 Game of Light**: `Arm` → tap → ignition (off → flourish → off) → live: dark at
  rest, each visitor-tap drops a persistent node in its quadrant colour. Ring the
  whole tree with outer nodes → 🌈 Unity (rainbow + fanfare). Rules editor sets what
  a sensor firing does (colour/brightness/time-on/spread).
- **🎬 Light Shows**: three authored ~5-min timed shows + the piano (real scores).
- **🎛 Controls**: patterns, sequencer, colour, speed; **DJ** panel: crossfade/EQ/strobe;
  **AI-VJ**: audio-reactive auto-looks (mic or built-in track).
- Everything renders the fixtures' *reported* state — the mirror-twin contract; the
  ESP-NOW seam is `protocol.ts` (control params only, never pixels).

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
