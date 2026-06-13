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
- [x] A0.2 **Load the REAL 3-D model.** ✓ DONE — exported from the EJF blend (`Tree_Resonance_packed_2026-06.13.ejf.blend`, 2.42 GB) via headless Blender (`app/scripts/blender/export_tree.py`): **78 real canopy lights → `app/public/fixtures.json`** (Light_Sources collection, z 0.53–40.51, real colors). Rendered in the twin as distinct emissive points at true positions (Blender Z-up → three Y-up). Structure glb (10.4 MB ovoid shell) deferred — re-export decimated bamboo as context in A4.
  `"/Users/resonanceartcollective/Library/CloudStorage/GoogleDrive-resonanceartcollective@gmail.com/.shortcut-targets-by-id/1v30ZnGHSid-Xt8f4MjVkRsSfNDoqwOxM/Resonance Master/Marketing/Cowork/Resonance Marketing OS/Portal Resonance/Resonance Studio/Resonance Tree/Portal System Current Read Only/design-and-construction/design-files/Blender/"`
  **CURRENT model = `Tree_Resonance_packed_2026-06.13.ejf.blend`** (2.4 GB, newest). Also `Tree_Rhino7.3dm` (Ed).
  Export with Blender CLI: `blender --background "<that .blend>" --python export_gltf.py` → write a **decimated/compressed glTF** (`app/public/tree.glb`, gltfpack) + extract LED/fixture positions → `app/public/fixtures.json`. (Coordinate w/ blender-architect if helpful — it knows this file's object names, e.g. `treev4 Lights`.)
  ⚠ **Do NOT commit the .blend or the raw 2.4 GB** — only the exported `.glb` + `fixtures.json` (gitignore the rest). Load the `.glb` in the R3F twin. **Verify (screenshot): the actual tree renders.**
- [ ] A0.3 If the real export is briefly blocked, fall back to the **placeholder** generator (A2) so the loop never stalls — then keep retrying the real model.

### Phase A — scaffold + model
- [x] A1. Scaffold `app/`: Vite + React + TypeScript + React Three Fiber + drei + zustand. `npm run dev` serves a blank R3F canvas. (Verify: canvas renders.) ✓ live canvas: emissive icosahedron + infinite grid + OrbitControls; `preserveDrawingBuffer` on for reliable screenshots.
- [~] A2. Placeholder generator — SUPERSEDED: the real EJF export (A0.2) gives a true 78-fixture `fixtures.json`, so no placeholder needed.
- [x] A3. Load `fixtures.json`; render each fixture as a small emissive sphere at its xyz; OrbitControls. ✓ + Bounds auto-frame; `fixtures.ts` typed contract + loader.
- [ ] A4. Add a simple tree-proxy mesh (trunk cylinder + canopy) or a stub glTF for spatial context.

### Phase B — the mirror loop
- [x] B1. zustand store (`store.ts`): SimFixture {id,role,zone,pos,norm,seqT(azimuth order),heightT,rnd} + Control {pattern,brightness,hue,sat,speed}. ✓
- [x] B2. tick loop (`useFrame` in TreeLights.tsx) computes each fixture's reported color via `litFor` (the firmware stand-in). ✓
- [x] B3. Rendering reads computed reported state only (InstancedMesh instanceColor). ✓ (G1 will split tick→transport)
- [x] B4. Control overlay (`Controls.tsx`): pattern buttons + brightness/hue/sat/speed sliders → sets commanded. ✓

### Phase C — patterns (one increment each)
- [x] C1 solid · [x] C2 chase (travels AROUND tree by azimuth order) · [x] C3 ripple (radial) · [x] C4 sparkle · [x] C5 breathe. (`patterns.ts`)
- [x] C6. Pattern selector UI + params (brightness, hue, saturation, speed). ✓

### Phase D — light rays + gobo (the "accurate visuals")
- [ ] D1. Add `@react-three/postprocessing` bloom/glow. (Verify: fixtures glow.)
- [ ] D2. Volumetric beam per fixture (drei volumetric SpotLight or a cone+raymarch pass). (Verify: visible beams.)
- [ ] D3. Gobo projection: spotlight `.map` with a mandala texture casting a pattern on a ground plane. (Verify: mandala shadow visible.)

### Phase E — sound
- [x] E1. Web Audio (`audio.ts`): mic OR loaded song → FFT → bass/mid/treble/level/beat. ✓
- [x] E2. Reactive mapping (in `litFor`): level→brightness, bass→swell, beat→flash, treble→hue. ✓ (refine per-band→zone + beat-cue triggers = H4)

### Phase F — timing / show / monitor
- [ ] F1. Cue system: capture current look as a cue; list + recall cues.
- [ ] F2. Simple timeline/schedule with tempo (cues fire on time).
- [ ] F3. Monitor view: show `reported` vs `commanded`; flag desync/dead fixtures (the truth-mirror as Monitor).

### Phase G — swap-readiness + polish
- [ ] G1. Mock heartbeat feed: simulate fixtures reporting state over a fake transport (jitter/latency); viz mirrors it (proves the real-telemetry path).
- [ ] G2. `fixtures.json` schema doc + runtime validation; ready to swap in the real Grasshopper export.
- [ ] G3. PWA offline config (vite-plugin-pwa) + README with run instructions.
- [~] G4. Perf pass — instanced mesh already in place (TreeLights InstancedMesh, 1 draw call); beams/bloom perf TBD.

### Phase H — Elliot's live asks (2026-06-13, via conductor)
- [x] H1 **Sequencer** ✓ — 7 modes on azimuth order (`seq`): fill (one-after-another around tree, then recede, then dark) · single (one light travels around) · snake (two heads sweep out from center, ping-pong) · groups (blocks of groupSize) · everyN (every 2nd/4th, animating) · allOn · allOff. Step-delay slider (default 200ms) + group-size + every-N. UI sub-panel shows when pattern=sequence. (`patterns.ts` sequence case + Controls.tsx)
- [ ] H2 **Full fleet**: scan `06_Unplaced_REVIEW` (256) for uplight/chandelier light sources; include if present → 100–150 fixtures (pending conductor scope confirm). fixtures.json re-reads on change.
- [x] H3 **"Any possible lighting command"** ✓ — command console (`command.ts` + Controls input). Grammar: `clear`/`on`/`off`; globals `hue/bri/sat/speed/pattern <v>`; targeted overrides `<target> <color #hex|name | on | off>` where target = `all | zone <low/mid/high> | range <a-b> | every <n> | fixture <id/seq>` (range/every use azimuth order). Per-fixture override layer applied in the tick after the pattern. Verified live: "every 4 color red" → 20 fixtures red around the tree.
- [ ] H4 **Audio depth**: per-band → zone mapping, beat-synced cue triggers, spectrum→color; song scrub/transport.
- [ ] H5 **Feature-coverage audit** vs PRD + docs 01–11 + Ben's repo (Monitor go/no-go, cues+schedule, gobo/beams D-phase, MIDI APC-mini, commissioning, element modes) — ensure ALL interactions covered (Elliot: "check our repo + Ben's, all the features").
- [ ] H6 **Tree context geometry**: decimated bamboo (02_Bamboo) → faint backdrop glb so the lights read as a tree (redo of A4 with better geometry; scratch on SUNEAST).

## BUILD LOG (append one line per cycle — newest at bottom)
<!-- template: [cycle N] <increment> — verified-by <shot> — commit <sha> — next <X> -->
- [cycle 1] A0.1+A1 — Vite+React+TS+R3F+drei+zustand scaffold + full test env (Vitest + Playwright e2e + build) — verified-by screenshots/cycle1-scaffold.png (emissive icosahedron + grid render) — next A0.2 real model
- [cycle 2] A0.1 full — Playwright config + e2e spec (canvas mounts, sized, 0 console errors, 543 distinct colors) + `npm run check` (build+test+e2e all green) — verified-by e2e pass — next A0.2 real model (EJF blend, 76 canopy lights)
- [cycle 3] A0.2+A3 — headless Blender export of EJF blend → 78 real canopy fixtures in fixtures.json + render as emissive point cloud at true positions (Z-up→Y-up), Bounds auto-frame — verified-by screenshots/cycle3-real-tree.png (78 distinct colored points) — next B1-B4 controllable mirror
- [cycle 4] B1-B4 + C1-C6 + E1-E2 — FULLY CONTROLLABLE twin: zustand store (commanded→tick→reported mirror), InstancedMesh render, control overlay (5 patterns + sliders), Web-Audio reactivity (mic/song→FFT). store/patterns/audio/TreeLights/Controls/Scene/App — verified-by screenshots/cycle4-controllable.png (78 lights + live control panel) — next H1 sequencer
- [cycle 5] H1 sequencer — 7 modes (fill/single/snake/groups/everyN/allOn/allOff) on azimuth order, step-delay 200ms + group-size + every-N sliders, UI sub-panel — verified-by screenshots/cycle5-sequencer.png (sequence mode UI + on/off fill state) — next H3 any-command console
- [cycle 6] H3 any-command console — command.ts parser (clear/on/off/globals + targeted overrides by all/zone/range/every/fixture) + per-fixture override layer + Controls input/chips/log — verified-by screenshots/cycle6-cmd-every4-red.png (live "every 4 color red" → 20 fixtures red, 0 errors) + build+e2e green — next: G1 mock-heartbeat + F3 Monitor (top audit pick), then D1 bloom + H6 tree geometry (bg agent running) / H6 tree context geometry
