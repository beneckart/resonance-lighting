# 08 — BUILD NOW: software-only lighting environment (no hardware, no real fixtures)

> Elliot, 2026-06-13: "we need to design without the lights built yet… get the lighting
> environment built out so we can start working with it and testing different controls without the lights."

## Goal
A **runnable simulated tree** we can drive and test controls / patterns / timing / sound on
**today** — before Ben's physical fixtures or the Blender export exist. Swap placeholders → real
later with **no interface changes**.

## Principle: SIM-FIRST. Zero hardware.
Fixtures are software objects. The mirror architecture runs entirely in software:
`control → sim-fixture updates its state → reports state → visualizer renders the reported state.`
Nothing here needs ESP-NOW, a Jetson, or a single LED.

## Step 0 — Placeholder fixture layout (unblocks everything; no Blender needed)
Generate a `fixtures.json` that approximates the tree procedurally: trunk axis + N rings +
M limbs, ~100–150 points, each `{fixture_id, role(down/up/chandelier), position[xyz], zone}`.
Mark it `"source":"placeholder"`. This is the stand-in for OQ-1's real export — so we don't
wait on Mia/Ed to start. Schema is identical to the real one, so the real export drops in later.

## Two parallel tracks

### Track 1 — fastest sandbox (off-the-shelf): Chromatik
Stand up **Chromatik** with the placeholder model. Instantly gives a 3-D preview + modular
pattern engine + **audio-reactive modulation** + OSC/MIDI controls, with **zero custom code**.
Use it to explore *what controls and patterns feel right* and to design sound reactivity now.
(Study `titanicsend/LXStudio-TE` for structure.) This is the "play with it today" path.

### Track 2 — the deployable target (custom): R3F mirror twin v0
- Load placeholder `fixtures.json`; render simulated fixtures in 3-D.
- A simple control surface: master brightness · color · pattern select · speed/timing · (later) audio in.
- **The mirror loop in software:** control sets a sim-fixture's `{color, brightness, pattern, phase}`
  → the fixture "reports" that state → the visualizer renders **only the reported state**
  (so the viz is always truthful, exactly as the real system will be).
- Placeholder patterns now (a few JS patterns: solid, chase, ripple, sparkle, breathe).
- One volumetric god-ray pass on top for the beam/gobo look (reuse Codrops / Maxime Heckel shaders).

## Sim fixture model
```
SimFixture = { id, role, position, zone,
               commanded: {pattern, params},        // what the controller pushed
               reported:  {color, brightness, pattern, phase} }  // what the viz renders
tick(): advance each fixture's pattern → update reported{}  (the "firmware" stand-in)
```
The viz reads `reported`, never `commanded`. That single rule = always-accurate mirror.

## Swap path (later, no rework — interfaces stay identical)
- placeholder `fixtures.json` → real export (Blender/Rhino)
- JS placeholder patterns → **Ben's firmware compiled to WASM** (golden-frame parity)
- sim `reported{}` → real **ESP-NOW heartbeat** telemetry
- in-process control → WS/ESP-NOW control plane (Protocol v1)

## First deliverable (definition of done for "working with it without lights")
Open the R3F twin in a browser → see the placeholder tree → push a control → watch the
fixtures mirror it in real time, with beams. That's a live lighting environment with no lights.

## Stack (per docs 06/07)
TypeScript · React Three Fiber + drei · zustand · Vite + PWA · @react-three/postprocessing ·
Web Audio (sound) · CBOR. Chromatik (Java) as the parallel authoring sandbox.
