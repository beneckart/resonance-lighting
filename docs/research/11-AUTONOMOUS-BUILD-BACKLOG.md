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
- [x] D1. Bloom/glow ✓ — @react-three/postprocessing EffectComposer + Bloom; fixtures rendered with HDR gain (×2.6) so they glow as lanterns. Also: bamboo glb now a VISIBLE warm lit material (#9c7a44) + key/rim directional lights + hero 3/4 camera framing the whole tree. **Now unmistakably reads as the Resonance Tree** (screenshots/cycle9-hero.png).
- [ ] D2. Volumetric beam per fixture (drei volumetric SpotLight or a cone+raymarch pass). (Verify: visible beams.)
- [ ] D3. Gobo projection: spotlight `.map` with a mandala texture casting a pattern on a ground plane. (Verify: mandala shadow visible.)

### Phase E — sound
- [x] E1. Web Audio (`audio.ts`): mic OR loaded song → FFT → bass/mid/treble/level/beat. ✓
- [x] E2. Reactive mapping (in `litFor`): level→brightness, bass→swell, beat→flash, treble→hue. ✓ (refine per-band→zone + beat-cue triggers = H4)

### Phase F — timing / show / monitor
- [ ] F1. Cue system: capture current look as a cue; list + recall cues.
- [ ] F2. Simple timeline/schedule with tempo (cues fire on time).
- [x] F3. Monitor view ✓ — `monitor` toggle: dead fixtures render as red "no-signal" markers; live readout `reporting N · dead D · stale S` (store.monitorStats updated ~2Hz from the tick). Proves the truth-mirror surfaces desync/dead.

### Phase G — swap-readiness + polish
- [x] G1. Mock heartbeat feed ✓ — `mock heartbeat` toggle: each fixture reports its color at a jittered 0.6–1.2s interval (held between → real reported-state staleness), `deadCount` fixtures stop reporting. TreeLights renders REPORTED buffers, not the instantaneous commanded render. Swap this transport for ESP-NOW heartbeats and nothing else changes.
- [ ] G2. `fixtures.json` schema doc + runtime validation; ready to swap in the real Grasshopper export.
- [ ] G3. PWA offline config (vite-plugin-pwa) + README with run instructions.
- [~] G4. Perf pass — instanced mesh already in place (TreeLights InstancedMesh, 1 draw call); beams/bloom perf TBD.

### Phase H — Elliot's live asks (2026-06-13, via conductor)
- [x] H1 **Sequencer** ✓ — 7 modes on azimuth order (`seq`): fill (one-after-another around tree, then recede, then dark) · single (one light travels around) · snake (two heads sweep out from center, ping-pong) · groups (blocks of groupSize) · everyN (every 2nd/4th, animating) · allOn · allOff. Step-delay slider (default 200ms) + group-size + every-N. UI sub-panel shows when pattern=sequence. (`patterns.ts` sequence case + Controls.tsx)
- [ ] H2 **Full fleet**: scan `06_Unplaced_REVIEW` (256) for uplight/chandelier light sources; include if present → 100–150 fixtures (pending conductor scope confirm). fixtures.json re-reads on change.
- [x] H3 **"Any possible lighting command"** ✓ — command console (`command.ts` + Controls input). Grammar: `clear`/`on`/`off`; globals `hue/bri/sat/speed/pattern <v>`; targeted overrides `<target> <color #hex|name | on | off>` where target = `all | zone <low/mid/high> | range <a-b> | every <n> | fixture <id/seq>` (range/every use azimuth order). Per-fixture override layer applied in the tick after the pattern. Verified live: "every 4 color red" → 20 fixtures red around the tree.
- [ ] H4 **Audio depth**: per-band → zone mapping, beat-synced cue triggers, spectrum→color; song scrub/transport.
- [ ] H5 **Feature-coverage audit** vs PRD + docs 01–11 + Ben's repo (Monitor go/no-go, cues+schedule, gobo/beams D-phase, MIDI APC-mini, commissioning, element modes) — ensure ALL interactions covered (Elliot: "check our repo + Ben's, all the features").
- [x] H6 **Tree context geometry** ✓ — bg agent exported 01_Structure+02_Bamboo (2364 meshes, decimate 0.03 + Draco) → `app/public/tree-context.glb` (10.4MB, scratch on SUNEAST). Wired into Scene.tsx as a faint (#26303f, opacity 0.22, depthWrite off) backdrop; aligns with the 78 fixtures (Y-up). Now reads as a tree. (export_context.py committed.) Also covers A4.

### Phase I — PRO-GRADE SCOPE (Elliot + conductor, 2026-06-13) — "super professional, festival/immersive quality"
> Bar: must look + behave like a pro festival-stage / immersive-art lighting system. Spine = REAL MUSIC driving a BEAUTIFUL tree. Always cross-check PRD + docs 01–11 + Ben's repo for features; everything TESTED + VISUALLY VERIFIED.
- [~] I1 VISUAL FIDELITY polish — D1 bloom ✓ (c9), visible bamboo + hero cam ✓ (c9), **D2 volumetric beams ✓ (c10)** — additive cones per fixture, beam-angle sized, colored by reported state. REMAINING: D3 gobo/mandala projection, ground plane + shadows, multi-visualizer, full-spectrum/tricolor patterns, per-fixture beam DIRECTION from model.
- [ ] I2 FULL AUDIO SYNC (the spine) — real beat/BPM/tempo/onset/DROP detection (not just FFT bands); TESTED with real songs for rhythm/timing accuracy. (need test tracks → app/public/audio/.)
- [ ] I3 DJ CONTROLLER — on-screen crossfader + EQ-band→light + intensity faders, wired; MIDI-ready (APC mini mk2 / Midi Fighter Twister mapping).
- [ ] I4 RANDOM / AUTO-VJ — shuffle + generative modes.
- [ ] I5 MULTIPLE VISUALIZERS — several distinct viz looks to switch between.
- [ ] I6 SEQUENCES — many more sequence patterns, built + tested.
- [ ] I7 LLM SMART SOUND→LIGHT MODE — an LLM "VJ" that reads audio features + crowd/section and arranges the pattern/visualizer vocabulary live (Addendum C Layer 2); multiple visualizations.
- [ ] I8 ESP-NOW FREQUENCIES / CHANNEL config — channel-pinned control surface in the app (honor Ben's ADR 0004/0010 + SYSTEM.md); wire the param-output path (custom LXOutput-equivalent) toward the real mesh.
- [ ] I9 FULLY-FUNCTIONAL APP CONTROLS — every control wired + tested; polished, professional UI.
- [ ] I10 DEEP RESEARCH — how big festival stages + immersive art pieces do real-time music-reactive lighting/visuals (tools, pipelines, beat-sync, pro techniques) → fold into the build. (bg research agent dispatched cycle 9.)

## BUILD LOG (append one line per cycle — newest at bottom)
<!-- template: [cycle N] <increment> — verified-by <shot> — commit <sha> — next <X> -->
- [cycle 1] A0.1+A1 — Vite+React+TS+R3F+drei+zustand scaffold + full test env (Vitest + Playwright e2e + build) — verified-by screenshots/cycle1-scaffold.png (emissive icosahedron + grid render) — next A0.2 real model
- [cycle 2] A0.1 full — Playwright config + e2e spec (canvas mounts, sized, 0 console errors, 543 distinct colors) + `npm run check` (build+test+e2e all green) — verified-by e2e pass — next A0.2 real model (EJF blend, 76 canopy lights)
- [cycle 3] A0.2+A3 — headless Blender export of EJF blend → 78 real canopy fixtures in fixtures.json + render as emissive point cloud at true positions (Z-up→Y-up), Bounds auto-frame — verified-by screenshots/cycle3-real-tree.png (78 distinct colored points) — next B1-B4 controllable mirror
- [cycle 4] B1-B4 + C1-C6 + E1-E2 — FULLY CONTROLLABLE twin: zustand store (commanded→tick→reported mirror), InstancedMesh render, control overlay (5 patterns + sliders), Web-Audio reactivity (mic/song→FFT). store/patterns/audio/TreeLights/Controls/Scene/App — verified-by screenshots/cycle4-controllable.png (78 lights + live control panel) — next H1 sequencer
- [cycle 5] H1 sequencer — 7 modes (fill/single/snake/groups/everyN/allOn/allOff) on azimuth order, step-delay 200ms + group-size + every-N sliders, UI sub-panel — verified-by screenshots/cycle5-sequencer.png (sequence mode UI + on/off fill state) — next H3 any-command console
- [cycle 20] F1 LLM-console — multi-line command runner (textarea → store.runScript → each line via runCommand, the LLM's tool surface) + parseScript (drops blanks/# comments) unit-tested + command.test.ts (grammar) + docs/research/13 LLM grammar tool-spec — verified-by build✓ npm test 15 passed + screenshots/cycle20-llmscript.png (3-command script: solid + high-zone blue + every-4 red) — next more sequences / E cortex-bridge / G2 validation
- [cycle 19] D auto-VJ — shuffle-bag look switching (pattern+visualizer+hue) on phrase boundary (phraseSeconds from BPM × autoBars, fallback timer); autovj.ts pure (ShuffleBag/phraseSeconds/LOOKS) unit-tested; 🤖 auto-VJ toggle + bars in Controls — verified-by build✓ npm test 11 passed + screenshots/cycle19-autovj.png (auto-switched to spectrum) — next F LLM-console / more sequences
- [cycle 18] C DJ controller — crossfader A↔B look blend (2× litFor lerp) + 3-band EQ→tree-zone gain + master intensity + strobe gate; dj.ts pure helpers (eqGain/strobeGate/lerp) unit-tested; scrollable panel — verified-by build✓ npm test 8 passed + screenshots/cycle18-dj.png — next D auto-VJ / F LLM-console
- [cycle 17] P0 FIX (conductor caught) — app was BLANK on fresh clone: gobo-mandala.png was untracked → texture 404 → Suspense hang. Committed the asset + added per-asset ErrorBoundary around GoboFloor/TreeContext so a missing asset degrades gracefully (never blanks). — verified-by build✓ + screenshots/cycle17-assetfix.png — next C DJ controller
- [cycle 16] A7 multiple visualizers — render-style switch: lanterns (spheres+beams) / orbs (big blooming) / wire (wireframe data look, beams off); viz selector in Controls — verified-by screenshots/cycle16-{orbs,wire,lanterns}.png (3 distinct looks) — next C DJ crossfader
- [cycle 15] B live wiring — BPM→sequencer tempo (sync-to-beat toggle + ¼/⅛ div; stepMs=beatStepMs(bpm,div)) + onset→flash + DROP detection→burst; beatStepMs unit-tested (124→484, 140→429); 140bpm-drop test track added — verified-by build✓ npm test 4 passed + screenshots/cycle15-beatsync.png — next: real-browser audio confirm + C DJ crossfader / A7 visualizers
- [cycle 14] A5 gobo projection — ground plane (receiveShadow) + downward SpotLight map=gobo-mandala.png (shadows on, target below tree), mandala projected on the ground + lantern/structure shadows — verified-by screenshots/cycle14-gobo.png (radial mandala on ground) — next: BPM→sequencer tempo + drop→burst (with unit test), then A7 visualizers / C DJ
- [cycle 13] B BPM PROVEN — extracted beat math to pure BeatTracker (beat.ts) + Vitest: detects 124 BPM from a 124-bpm impulse train AND 90 from a 90-bpm train (deterministic, hardware-independent). Confirms the headless ~83 was the audio path, not the algorithm. audio.ts refactored to use it. — verified-by `npm test` 3 passed — next A5 gobo + real-browser audio confirm
- [cycle 12] B audio spine — audio.ts rewrite: spectral-flux onset (bass band) + interval BPM + asymmetric attack/release envelope followers (bass/mid/treble) + AGC + track player + beat→flash; "🎶 test track" button + live BPM readout — verified-by screenshots/cycle12-audio.png (reactive, 0 errors). ⚠ BPM read ~83 on the 124bpm track IN HEADLESS (headless audio timing unreliable) → needs real-browser verification + BPM-accuracy tuning (next). onset-reactivity works. — next: real-browser BPM verify + A5 gobo
- [cycle 11] spectrum + tricolor-dance patterns + ACES tonemapping — full-spectrum rainbow around the tree + 3-color dancing + filmic tonemap after bloom — verified-by screenshots/cycle11-spectrum.png (gorgeous full-rainbow tree, colored beams) — next A5 gobo (conductor supplied gobo-mandala.png) + B audio (Meyda/BPM)
- [cycle 10] A4 volumetric beams — additive downward cones per fixture, beam-angle sized (beamDeg), colored by reported state; tuned bloom/gain so it's pro not blown-out — verified-by screenshots/cycle10-beams.png (glowing lanterns + tasteful beam wash, reads beautifully as the tree) — next: full-spectrum/tricolor patterns + B audio beat/BPM
- [cycle 9] D1 bloom + VISUAL FIDELITY — bloom (HDR gain), visible warm bamboo material + key/rim lights + hero camera — verified-by screenshots/cycle9-hero.png (NOW LOOKS LIKE THE RESONANCE TREE: bamboo lattice + glowing lanterns) — next I2 real audio beat/BPM sync + I1 beams/gobo
- [cycle 8] G1+F3 truth loop — mock-heartbeat transport (held reports + jitter + dead fixtures) + Monitor (red no-signal markers + reporting/dead/stale readout); TreeLights renders reported buffers — verified-by screenshots/cycle8-monitor.png ("reporting 72 · dead 6 · stale 0", 6 red markers) — next D1 bloom
- [cycle 7] H6 tree-context backdrop — bg agent's decimated bamboo glb (2364 meshes, 10.4MB) wired as faint backdrop; tree now reads as a tree — verified-by screenshots/cycle7-tree-context.png (canopy silhouette behind 78 lights) — next G1 mock-heartbeat + F3 Monitor
- [cycle 6] H3 any-command console — command.ts parser (clear/on/off/globals + targeted overrides by all/zone/range/every/fixture) + per-fixture override layer + Controls input/chips/log — verified-by screenshots/cycle6-cmd-every4-red.png (live "every 4 color red" → 20 fixtures red, 0 errors) + build+e2e green — next: G1 mock-heartbeat + F3 Monitor (top audit pick), then D1 bloom + H6 tree geometry (bg agent running) / H6 tree context geometry
