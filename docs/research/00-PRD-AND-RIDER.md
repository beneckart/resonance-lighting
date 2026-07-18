# Resonance Tree Lighting Environment — PRD + RIDER (living, check EVERY cycle)

> Canonical source of truth for `lighting-architect` (builder) + `portal-strategist` (conductor/Elliot-proxy).
> **RULE: both agents read this top-to-bottom every cycle, update the LEDGER (Part 3), and never declare a feature done until its acceptance criteria + visual verification pass.** Cross-checked vs dossier 01–11 + Ben's repo + Elliot's asks 2026-06-13.

---
## PART 1 — PRODUCT (what we're building)

**Vision:** a real-time, hardware-true **digital twin + control/show system** for the Resonance Tree — the actual bamboo tree rendered beautifully, its lanterns glowing, driven by **real music**, playable by a DJ/VJ, automatable by an **LLM**, and ready to bridge to Ben's real solar-mesh fixtures. Not a control harness — a living instrument that looks like the Tree.

### A. VISUAL FIDELITY (must look like the Resonance Tree)
- Bamboo tree shell clearly visible + lit (not dark-on-black). Acc: a fresh viewer instantly recognizes "a tree."
- Lanterns GLOW — emissive + **bloom** + per-light glow/halo. Acc: lights read as lanterns, not flat dots.
- Volumetric **beams** from fixtures. Gobo/**mandala** projection on ground.
- Hero camera framing + orbit; night world; physical-unit (lumens/beam/IES) calibration.
- **Multiple visualizers** — several distinct looks to switch between.
- Acc gate: a screenshot that unmistakably reads as the Resonance Tree.

### B. AUDIO SYNC (the spine)
- Real **beat / BPM / tempo**, onset, and **drop** detection — not just FFT bands.
- **Tested with real songs** (multiple genres) — beat/rhythm/timing visibly accurate.
- Audio → modulation matrix (bass→brightness, mids→motion, highs→sparkle, beat→flash, drop→burst).
- Line-in / DJ-booth path + mic. Acc: lights move convincingly to a real track on the beat.

### C. DJ / VJ CONTROLLER
- Wired DJ sliders: **crossfader** (A/B look blend), EQ-band→light faders, master intensity.
- **MIDI controller** support (Akai APC-class via a mido/WebMIDI bridge or host).
- Guest-DJ scoped mode. AI-VJ hooks (see F).

### D. MODES · PATTERNS · SEQUENCES
- Pattern library (solid/breathe/chase/ripple/sparkle + more) + parameters.
- **Random / auto-VJ** modes (shuffle, generative, CA/firefly fields).
- Sequencer (done: 7 modes) + **many testable sequences** + element modes (wind/ember/rain/beacon).
- Cue system + **timeline/schedule** (tempo-synced) + **Show Compiler** (per-fixture keyframes, drone-show model).

### E. MASTER BRAIN INTEGRATION (cortex)
- Cortex (Jetson) integration: twin-server, show hosting, sensor fusion; two-tier (mesh brainstem autonomous + cortex optional, dies invisibly).
- **PowerFeather-as-ESP-NOW-modem** bridge → drive REAL fixtures (control-params, NOT pixels).
- Protocol v1 (heartbeat/param/state_doc/cue/event/time_sync) + truth-loop over the wire.

### F. LLM RUNS THE CONTROLS
- **LLM operator:** natural language → emits the H3 command-console grammar → drives the tree. (The command console IS the LLM's tool surface.)
- AI-VJ over live audio (arranges the pattern vocabulary to the music). Voice-of-the-Tree control_lights.
- Deliverable: a documented tool/grammar spec the LLM is prompted/trained on.

### G. SENSING / OPS / HARDWARE-FIDELITY (later, keep on radar)
- PIR occupancy / heat-map (sim) · presence→ripple · weather/beacon · energy-truth.
- fixtures.json ↔ Ben's mesh contract · Show-Compiler export to fixture params · failure ladder.
- Count reconcile: 78 canopy vs ~150 (down+up+chandelier). Tests: grow coverage as features land.

---
## PART 2 — RIDER (how we operate — both agents, every cycle)

1. **Read this PRD every cycle.** Update the LEDGER (Part 3): mark ✅/🟡/❌ + commit SHA per feature.
2. **Priority order (current):** A visual fidelity → B real-song audio sync → C DJ+random → D visualizers/sequences → F LLM-control → E master brain → G later. (Elliot can re-order.)
3. **Definition of DONE (per feature):** builds + tests pass **and** visually verified (screenshot for anything visual) **and** the acceptance criterion above is met. No "done" on code-read alone. No over-claiming.
4. **Builder cadence:** build one increment → verify (npm build/test + screenshot) → commit+push origin/elliots-controller → report to thread → re-arm ≤60s. Never stop with backlog left; never end a turn without a thread status.
5. **Conductor cadence:** every ≤60s — read thread + answer questions, git-check, re-verify new commits (build/test + screenshot visual ones, assess honestly), feed assets (gobo, test songs), and **completeness cross-check vs this PRD every ~5 cycles** (flag any uncovered idea).
6. **Comms:** hot thread e5fd2727; builder routes all questions to conductor (Elliot-proxy); both post status; identity collapses to 'resonance-test' — read by content, not tag.
7. **Repo:** builder owns ~/code/resonance-tree-lighting (push to fork origin/elliots-controller, no MR/PR). Conductor is read-only on the repo + produces assets/verification.
8. **Escalate to Elliot:** a genuine visual leap (screenshot looks like the Tree), build/test FAIL, >30min stall, or morning summary. Honesty always — flag gaps, don't inflate.

---
## PART 2b — CHECK PROCESS (run EVERY wake, both agents — never idle)
The build is NEVER "done" until **every integration + every control works, full app, all tested**. Each wake:
1. **READ** this PRD + the LEDGER (Part 3).
2. **VERIFY** the last increment is truly done (builds + tests + visual + acceptance). If not, finish it first.
3. **CROSS-CHECK** vs the repo + dossier 01–11 + Ben's repo: is any idea NOT captured here? add it. Any ❌/🟡 now unblocked?
4. **PICK** the next work from the three lanes below (Building → Testing → Research, priority order), or if a lane is dry, pull the next ❌ from the LEDGER.
5. **DO IT → verify → commit/push → report to thread → update the LEDGER → re-arm ≤60s.**
6. **"Nothing to do" is never true.** If you think you're done: add a test, test a control with a real song, research the next feature, or take the next ❌. Keep building + testing + researching until Elliot says stop.

## PART 2c — WORK LANES (always pull from here)
**🔨 BUILDING** — feature backlog in priority order: A visual-fidelity (bamboo visible→glow/bloom→beams→gobo→cameras→multi-visualizer) → B audio (beat/BPM/drop→matrix) → C DJ sliders+MIDI → D random/auto-VJ + more sequences + element modes + cue/timeline/Show-Compiler → F LLM-runs-console + AI-VJ → E cortex/master-brain + ESP-NOW bridge → G sensing/ops.
**🧪 TESTING** — *every control, every integration, with real songs.* Build a CONTROL MATRIX test (every pattern × every sequence mode × every slider × command-console grammar). Audio: test with multiple real tracks (beat accuracy, drop timing). Add Vitest unit + Playwright e2e for each feature (coverage is thin — grow it every cycle). Visual regression screenshots. Verify nothing regresses on each commit.
**🔬 RESEARCH** — investigate the next feature's best approach BEFORE/while building: web-audio beat/BPM libs (e.g. realtime tempo), three.js volumetric beam + gobo techniques, browser MIDI (WebMIDI/iOS limits), LLM-control grammar design, Pixelblaze/Firestorm + cortex bridge patterns, gobo/mandala generation. Conductor owns most research + hands findings/assets to builder.

---
## PART 3 — LIVE FEATURE LEDGER (update + check every cycle)

| # | Feature | Status | Last commit / note |
|---|---|---|---|
| A1 | Tree model + 78 lights at true positions | ✅ | b70feea |
| A2 | Bamboo shell VISIBLE + lit | ❌ | near-black; H6 backdrop too faint |
| A3 | Lanterns glow + bloom | ❌ | dim dots |
| A4 | Volumetric beams | ❌ | |
| A5 | Gobo/mandala projection | ❌ | conductor can supply texture |
| A6 | Hero camera / night world / IES | ❌ | |
| A7 | Multiple visualizers | ❌ | |
| B1 | FFT bands + mic/song | 🟡 | bb0da17 |
| B2 | Beat/BPM/onset/drop detection | ❌ | |
| B3 | Tested with REAL songs | ❌ | need audio assets |
| B4 | Audio→modulation matrix tuned | 🟡 | basic |
| C1 | DJ sliders (crossfader/EQ/intensity) | ❌ | |
| C2 | MIDI controller | ❌ | |
| C3 | Guest-DJ / AI-VJ hooks | ❌ | |
| D1 | Pattern library (5) | ✅ | bb0da17 |
| D2 | Random / auto-VJ modes | ❌ | |
| D3 | Sequencer (7 modes) | ✅ | 2e91701 |
| D4 | Many sequences + element modes | 🟡 | |
| D5 | Cue/timeline/Show-Compiler | ❌ | |
| E1 | Cortex (master brain) integration | ❌ | |
| E2 | PowerFeather ESP-NOW bridge / Protocol v1 | ❌ | |
| F1 | LLM runs the command console | ❌ | H3 console = tool surface |
| F2 | AI-VJ over live audio | ❌ | |
| H3 | Any-command console | ✅ | d9fdae6 |
| G* | Sensing/ops/hardware-fidelity | ❌ | later |

> Builder: append commits + flip statuses here each cycle. Conductor: verify + cross-check each item; nothing ships ✅ without acceptance + visual proof.

---
## PART 4 — TASK BACKLOG (work all night, top-down; pull the next unchecked)

### 🔨 BUILDING — Visual fidelity (DO FIRST)
- [ ] 1. Bamboo shell visible: add ambient + key + rim lights; lit/standard material on the glb (it's near-black now).
- [ ] 2. Verify glb loaded + scaled + ALIGNED with the 78 light positions; fix scale/orientation/origin if off.
- [ ] 3. Lanterns: bigger emissive spheres + per-light glow sprite/halo.
- [ ] 4. **Bloom** postprocessing (@react-three/postprocessing UnrealBloom), tuned.
- [ ] 5. Volumetric **beam** cones per fixture (additive).
- [ ] 6. **Gobo mandala** projection on a ground plane (spotlight.map) — conductor supplies texture.
- [ ] 7. Night world: dark gradient bg + subtle ground/reflection.
- [ ] 8. Hero camera + smooth orbit + saved viewpoints (front/3-4/top).
- [ ] 9. ACES tonemapping + exposure control.
- [ ] 10. Multiple **visualizers**: realistic / abstract-particles / wireframe-data — switchable.

### 🔨 BUILDING — Audio sync
- [ ] 11. Integrate beat/**BPM** detector (web-audio-beat-detector or custom autocorrelation).
- [ ] 12. Onset detection (spectral flux).
- [ ] 13. **Drop** detection (energy buildup→collapse→burst) → drop-bomb cue.
- [ ] 14. Configurable audio→param **modulation matrix** (bass→bri, mids→motion, highs→sparkle, beat→flash).
- [ ] 15. Load + play **real song files** from app/public/audio + a track picker; test sync per track.
- [ ] 16. Tempo-locked sequencer (snap steps to BPM/phrase).

### 🔨 BUILDING — DJ/VJ controls
- [ ] 17. **Crossfader** A/B look blend. 18. 3-band **EQ→light zones**. 19. Master intensity + strobe.
- [ ] 20. **WebMIDI** mapping (faders→params, pads→cues, bidirectional LED feedback).
- [ ] 21. Guest-DJ scoped mode (caps, locks).

### 🔨 BUILDING — Modes / sequences / show
- [ ] 22. Random/shuffle auto-VJ (switch on phrase). 23. CA field (reaction-diffusion/GoL). 24. Firefly sync.
- [ ] 25. Element modes: wind-ride, ember, rain, **beacon**. 26. More patterns: spiral, comet, wave, twinkle, bloom.
- [ ] 27. Cue capture + recall list. 28. Timeline/**schedule** (tempo + absolute). 29. **Show Compiler** (per-fixture keyframe export).

### 🔨 BUILDING — LLM control + master brain
- [ ] 30. Document the command-console **grammar as a tool spec** (from H3).
- [ ] 31. **LLM operator**: natural language → console commands → drives tree (endpoint/local).
- [ ] 32. **AI-VJ**: audio digest → LLM picks pattern/params.
- [ ] 33. Protocol v1 message types in code (heartbeat/param/state_doc/cue/event/time_sync).
- [ ] 34. **Mock heartbeat feed** (sim fixtures report state w/ jitter) → mirror renders reported.
- [ ] 35. ESP-NOW/WS bridge stub (cortex↔master modem); fixtures.json schema validation + version.

### 🧪 TESTING (grow every cycle)
- [ ] 36. Vitest: store reducers, pattern math, audio→param mapping, command parser.
- [ ] 37. Playwright e2e: load app → select each pattern → drive each slider → run console commands.
- [ ] 38. **CONTROL MATRIX** test: every pattern × every sequence mode × every slider × console grammar.
- [ ] 39. **Real-song** test harness: assert lights respond to beats/drops on ≥3 tracks.
- [ ] 40. Visual-regression screenshots per visualizer/pattern.
- [ ] 41. Perf: hold 60fps with 78 lights + bloom + beams (instanced meshes).
- [ ] 42. Regression check on every commit (build+test+screenshot stays green).

### 🔬 RESEARCH (conductor leads, hands findings to builder)
- [ ] 43. Beat/BPM/onset library survey (accuracy, latency, browser).
- [ ] 44. Volumetric beam + gobo technique (drei SpotLight volumetric, raymarch).
- [ ] 45. Browser MIDI feasibility + iOS limits; host-bridge fallback.
- [ ] 46. LLM-control grammar + prompt/tool design; AI-VJ arrangement strategy.
- [ ] 47. Cortex bridge patterns (Pixelblaze/Firestorm, ESP-NOW control-plane).
- [ ] 48. Fixture count reconcile (78 canopy vs ~150 down+up+chandelier) — pull up/chandelier from .blend if needed.
- [ ] 49. Gobo/mandala texture generation (conductor produces PNGs).
- [ ] 50. Source royalty-free real test tracks → app/public/audio (for task 15/39).

**When this list is exhausted: generate the next 25 (polish, more visualizers, more patterns, deeper LLM, hardware bridge). The work does not end until Elliot says stop.**
