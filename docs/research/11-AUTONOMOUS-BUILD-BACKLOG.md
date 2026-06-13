# 11 — Autonomous Build Backlog + Loop Protocol (lighting-architect)

> Purpose: enough sequenced work + discipline for a self-paced **design → test → iterate** loop
> of 20+ cycles, building the software-only lighting environment (sim-first, no hardware).
> Read with: PRD + docs 06–10. Target = the R3F mirror twin (`app/`) + a sim of the tree.

## THE LOOP (every cycle — do all 6, then re-arm ≤60s)
1. **PICK** the lowest-numbered unchecked increment below (or the next unblocked one).
2. **BUILD** just that increment — one focused change. Small > big.
3. **TEST / VERIFY (mandatory — DESIGN mode):** run the dev server (`npm run dev` in `app/`) and **screenshot it** (browse/Playwright, or `python3 -m http.server` for static). **Never mark an increment done on code-read alone** — look at the pixels.
4. **CHECKPOINT:** commit (explicit paths) + `git push origin elliots-controller`. Push to **our fork directly — NO MR, NO qb, NO PR, never the OS repo.**
5. **LOG:** append one line to the BUILD LOG (bottom of this file): `[cycle N] <increment> — built / verified-by <screenshot> / commit <sha> / next: <X>`.
6. **REFLECT + RE-ARM:** note anything to improve; pick the next increment; re-arm the loop (ScheduleWakeup ≤60s / continue). **Do not stop** until the backlog is done or you're hard-blocked. If blocked, log the blocker + skip to the next unblocked increment.

**FULL AUTONOMY — no restrictions (Elliot 2026-06-13).** This is a greenfield repo; nothing is built yet, so **there is nothing to break.** Move fast, experiment freely, refactor boldly, just keep committing + pushing to `origin/elliots-controller`. Don't ask permission; don't wait. Only two hard rules: (1) don't edit Ben's `firmware/ hardware/ enclosure/` lanes or the OS repo; (2) no secrets / no giant binaries committed. If you finish the backlog, generate the next 10 increments and keep going. **Run all night.**

## BACKLOG (check off as you go)

### Phase A0 — environment + REAL model (DO FIRST)
- [x] A0.1 **Testing environment.** Dev server (`npm run dev`), **Vitest** unit tests, and a **Playwright** headless screenshot harness for the mandatory visual verification each cycle. A `npm run build` + `npm test` that must pass before each checkpoint commit. ✓ scaffolded in `app/` (Vite+React+TS+R3F+drei+zustand); `npm run build`/`npm test` green; `scripts/screenshot.mjs` harness working.
- [ ] A0.2 **Load the REAL 3-D model.** It's a **mounted-Drive folder on this Mac — read it directly, no download:**
  `"/Users/resonanceartcollective/Library/CloudStorage/GoogleDrive-resonanceartcollective@gmail.com/.shortcut-targets-by-id/1v30ZnGHSid-Xt8f4MjVkRsSfNDoqwOxM/Resonance Master/Marketing/Cowork/Resonance Marketing OS/Portal Resonance/Resonance Studio/Resonance Tree/Portal System Current Read Only/design-and-construction/design-files/Blender/"`
  **CURRENT model = `Tree_Resonance_packed_2026-06.13.ejf.blend`** (2.4 GB, newest). Also `Tree_Rhino7.3dm` (Ed).
  Export with Blender CLI: `blender --background "<that .blend>" --python export_gltf.py` → write a **decimated/compressed glTF** (`app/public/tree.glb`, gltfpack) + extract LED/fixture positions → `app/public/fixtures.json`. (Coordinate w/ blender-architect if helpful — it knows this file's object names, e.g. `treev4 Lights`.)
  ⚠ **Do NOT commit the .blend or the raw 2.4 GB** — only the exported `.glb` + `fixtures.json` (gitignore the rest). Load the `.glb` in the R3F twin. **Verify (screenshot): the actual tree renders.**
- [ ] A0.3 If the real export is briefly blocked, fall back to the **placeholder** generator (A2) so the loop never stalls — then keep retrying the real model.

### Phase A — scaffold + model
- [x] A1. Scaffold `app/`: Vite + React + TypeScript + React Three Fiber + drei + zustand. `npm run dev` serves a blank R3F canvas. (Verify: canvas renders.) ✓ live canvas: emissive icosahedron + infinite grid + OrbitControls; `preserveDrawingBuffer` on for reliable screenshots.
- [ ] A2. Placeholder `fixtures.json` generator (procedural tree: trunk axis + rings + limbs, ~120 points, schema `{fixture_id, role, position[xyz], zone, "source":"placeholder"}`). Save to `app/public/fixtures.json`.
- [ ] A3. Load `fixtures.json`; render each fixture as a small emissive sphere at its xyz; OrbitControls. (Verify: you see a tree-shaped point cloud.)
- [ ] A4. Add a simple tree-proxy mesh (trunk cylinder + canopy) or a stub glTF for spatial context.

### Phase B — the mirror loop
- [ ] B1. zustand store with `SimFixture { id, role, position, commanded:{pattern,params}, reported:{color,brightness,pattern,phase} }`.
- [ ] B2. A tick loop (`useFrame`) that advances each fixture's pattern → writes `reported`. (The "firmware" stand-in.)
- [ ] B3. Rendering reads **`reported` only** (color+brightness drive each sphere's emissive). This is the mirror rule — prove it by changing `commanded` and watching `reported` catch up.
- [ ] B4. Minimal control surface (HTML overlay): master brightness slider + global color picker → sets `commanded`.

### Phase C — patterns (one increment each)
- [ ] C1. Pattern: solid color.   - [ ] C2. chase.   - [ ] C3. ripple (radial from a chosen origin).   - [ ] C4. sparkle.   - [ ] C5. breathe/pulse.
- [ ] C6. Pattern selector UI + per-pattern params (speed, density, hue, saturation).

### Phase D — light rays + gobo (the "accurate visuals")
- [ ] D1. Add `@react-three/postprocessing` bloom/glow. (Verify: fixtures glow.)
- [ ] D2. Volumetric beam per fixture (drei volumetric SpotLight or a cone+raymarch pass). (Verify: visible beams.)
- [ ] D3. Gobo projection: spotlight `.map` with a mandala texture casting a pattern on a ground plane. (Verify: mandala shadow visible.)

### Phase E — sound
- [ ] E1. Web Audio: mic/line-in → FFT → frequency bands exposed in the store.
- [ ] E2. Modulation: map bands to params (bass→brightness, highs→sparkle); a beat pulse. (Verify: lights move to audio.)

### Phase F — timing / show / monitor
- [ ] F1. Cue system: capture current look as a cue; list + recall cues.
- [ ] F2. Simple timeline/schedule with tempo (cues fire on time).
- [ ] F3. Monitor view: show `reported` vs `commanded`; flag desync/dead fixtures (the truth-mirror as Monitor).

### Phase G — swap-readiness + polish
- [ ] G1. Mock heartbeat feed: simulate fixtures reporting state over a fake transport (jitter/latency); viz mirrors it (proves the real-telemetry path).
- [ ] G2. `fixtures.json` schema doc + runtime validation; ready to swap in the real Grasshopper export.
- [ ] G3. PWA offline config (vite-plugin-pwa) + README with run instructions.
- [ ] G4. Perf pass (instanced meshes for fixtures; 60fps with 150 points + beams).

## BUILD LOG (append one line per cycle — newest at bottom)
<!-- template: [cycle N] <increment> — verified-by <shot> — commit <sha> — next <X> -->
- [cycle 1] A0.1+A1 — Vite+React+TS+R3F+drei+zustand scaffold + full test env (Vitest + Playwright e2e + build) — verified-by screenshots/cycle1-scaffold.png (emissive icosahedron + grid render) — next A0.2 real model
- [cycle 2] A0.1 full — Playwright config + e2e spec (canvas mounts, sized, 0 console errors, 543 distinct colors) + `npm run check` (build+test+e2e all green) — verified-by e2e pass — next A0.2 real model (EJF blend, 76 canopy lights)
