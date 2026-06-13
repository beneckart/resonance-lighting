# 07 — Visualizer + Controller: Landscape, Inspiration, and Recommendation

> Author: portal-strategist (deep-dive at Elliot's request) · 2026-06-13
> Expands: 01-Master Report PART 3/10 · 04-Addendum C · the PRD.
> Question: what app/stack for the visualizer + controller; custom vs off-the-shelf; how to NOT start from scratch; accurate light rays + color + patterns + timing + sound.

## THE KEY INSIGHT (Elliot, 2026-06-13): visualizer MIRRORS reported state
The visualizer is **responsive to the tree, not the controller of it.** Flow:
`controller → pushes param to a fixture → fixture renders it → fixture reports its actual status (heartbeat) → visualizer renders the reported status.`
The viz is a **pure mirror** — always accurate to real light state, never assumes. It is also the Monitor (desync/dead fixtures are visible because only reported state is drawn). This is the dossier's "truth loop" promoted to the core architecture and cleanly decoupled from control.

### How the mirror stays exact (the elegant mechanism)
ESP-NOW heartbeats are tiny (~250B, ~1Hz) — can't stream per-pixel color. Instead each fixture reports `{active pattern_id, params, phase/epoch, brightness, SOC}`. The visualizer feeds those into the **same firmware pattern engine compiled to WASM** and re-renders the exact output. The viz literally runs the fixture's code with the fixture's reported inputs → guaranteed parity. (This is "golden-frame parity" used live, not just for dev.) Volumetric ray/gobo rendering is a visual layer on top of that accurate per-fixture state.

## THE LANDSCAPE (3 families)

### A. Authoring / control / sound engines — "don't start from scratch" core
- **Chromatik / LX Studio** (heronarts) — a digital lighting workstation: 3-D fixture model + modular pattern/effect engine + **modulation routing where audio & MIDI drive any parameter** + real-time preview + bidirectional **OSC** + output to ArtNet/OPC/E1.31/DDP/KiNET. **The Tree of Ténéré (Burning Man 2017, 100k LEDs, the literal predecessor "Tree") ran on LX**, driven live by audio + MIDI + heart-rate + EEG. This is the single strongest reuse candidate. **NOT open-source — but source-available + free for our use:** the LX license grants free non-commercial use with a **$25K/yr total-revenue commercial cap** (above that → license@chromatik.co). Engine = `heronarts/LX` (Java, source on GitHub, runs **headless** on any Java device incl. Pi/Jetson); GUI harness = `heronarts/GLX`; the polished app = Chromatik (download at chromatik.co, v1.2.1 Nov 2025). Built FOR non-uniform 3D pixel layouts ("sparse vertex shader", each pixel has a real 3D position) — exactly our tree. Java/desktop (laptop or Jetson, NOT an iPad PWA). NOTE: LX's native outputs are pixel-streaming (OPC/ArtNet/E1.31/DDP/KiNet) — use it for AUTHORING+PREVIEW+SOUND; bridge to Ben's control-params mesh via the Show Compiler or a custom LXOutput.
  - Refs: https://lx.studio/ · https://lx.studio/tenere · https://github.com/heronarts/Chromatik · https://chromatik.co/ · https://heronarts.lx.studio/guide/osc/
- **titanicsend/LXStudio-TE** — Titanic's End's full open-source LX app for a giant BM LED sculpture. A complete, modern, real-world codebase to **study/fork patterns + project structure** from. Ref: https://github.com/titanicsend/LXStudio-TE
- **TouchDesigner** — node-based real-time, superb sound-reactive + projection prototyping, outputs Art-Net/DMX/OSC. Free non-commercial. Desktop. Best as a **pattern/sound prototyping lab**, not the shipped runtime. Refs: https://alltd.org/touchdesigner-led-panel-pixel-mapping-tutorial/
- **MADRIX** — commercial pixel-mapping control; powerful but proprietary/streaming-oriented. Ref: https://www.madrix.com/products/software

### B. Accurate-ray pre-visualization — for hero/donor renders, NOT the live tool
- **Depence² / R4** — **physically-based real-time raytraced light beams**, gobos, prisms, IES, adjustable haze. Exactly "accurate lighting rays + controls." BUT stage-fixture-oriented (movers/conventional), Windows, paid, awkward for bespoke addressable bamboo. Use as a reference / optional hero pre-viz. Ref: https://help.depence.com/ · https://pangolin.com/products/depence-stage-lighting-module
- **Unreal Engine 5 DMX Previs** — Lumen + volumetric fog + DMX = gorgeous real-time rays; heavy, big build, not iPad. 2027 stretch at most. Ref: https://dev.epicgames.com/documentation/unreal-engine/dmx-previs-sample-project-for-unreal-engine
- **Blender Cycles** — most accurate offline rays/volumetrics we already have; perfect for **hero stills/video**, useless as a live control/sound surface.

### C. Custom web twin — the on-playa live MIRROR (iPad)
- **React Three Fiber** + volumetric techniques for the real-time beams/god-rays. Mature, copy-able implementations (don't write god-rays from scratch):
  - Codrops volumetric light rays: https://tympanus.net/codrops/2022/06/27/volumetric-light-rays-with-three-js/
  - Maxime Heckel raymarched volumetric lighting for R3F: https://blog.maximeheckel.com/posts/shaping-light-volumetric-lighting-with-post-processing-and-raymarching/
  - drei `<SpotLight>` volumetric helper; `@react-three/postprocessing` bloom/godrays.
- LED layout/util references: jasoncoon/led-mapper (layout+viz for FastLED/Pixelblaze) https://github.com/jasoncoon/led-mapper · PWRFLcreative/Lightwork (computer-vision LED mapping) https://github.com/PWRFLcreative/Lightwork · cyberboy666/artnet_led_mapper

## RECOMMENDATION — a hybrid, so we reuse the hard parts

Three roles, three tools — do NOT force one app to do all of it:

1. **Design / authoring / sound + pattern engine → Chromatik (LX).** It already is the Tree-of-Ténéré stack: model, modular patterns, audio/MIDI modulation, real-time preview, OSC. We import our `fixtures.json` as the model and author patterns + sound-reactive modulation here. This deletes months of "pattern engine + audio engine + design preview" work.
2. **Bridge to Ben's autonomous mesh → our Show Compiler (the one custom integration that matters).** KEY FORK: Chromatik's native output *streams pixels*; Ben's architecture is *control-params-only, patterns run on the fixture, no pixel streaming.* Resolution: don't stream Chromatik's pixels to the playa. Use Chromatik to **author + preview**, then the **Show Compiler exports per-fixture pattern params / keyframe files** that run on Ben's mesh (the drone-show model from Addendum A9). Chromatik authors; the mesh executes autonomously.
3. **On-playa live MIRROR visualizer → custom R3F web twin**, driven by reported heartbeat state through the WASM sim, with volumetric god-ray rendering. iPad-deployable, offline. This is the truth-mirror you described.
4. **Hero/donor rays → Blender Cycles** (offline) and/or Depence² if we want real-time accurate beams for a pitch.

**Net:** reuse Chromatik (authoring/patterns/sound) + Blender (placement + hero rays) + proven web volumetric code; build custom only the **Show Compiler bridge** and the **reported-state mirror twin**. That's the minimum bespoke surface.

## Answers to Elliot's direct questions
- **Export Blender (with lights) into another environment?** Yes — export geometry as **glTF** + fixture positions/roles as **`fixtures.json`** (one export, imported everywhere: Chromatik model, the R3F twin, optional Depence). Blender stays source-of-truth for geometry + placement + hero renders. The Blender "lights" become the fixture map, not light objects you re-create.
- **What app?** Not one — the hybrid above. Live authoring engine = Chromatik; live mirror = R3F twin; hero rays = Blender. For a fast 2026 v1 you could even run **Chromatik on a laptop/Jetson at the trunk as the whole control+preview**, and add the iPad mirror later.
- **Not from scratch?** Chromatik + Titanic's End repo (authoring/patterns/sound), Ben's firmware → WASM (sim/mirror), proven web volumetric shaders (rays), Blender (placement+rays). We write glue, not engines.
- **Accurate rays (Blender lacks live):** correct — Blender's rays are offline. Live accurate-ish rays = web volumetric raymarching (customizable, sound-able, real-time) for the mirror; Depence² if you want PBR-accurate beams for a pitch. Blender Cycles for the gorgeous hero shots.

## Next steps (ordered)
1. **Lock `fixtures.json` schema** (the contract everything imports). Get the first one from Blender (Mia's `.blend`) / Rhino (Ed) — this is OQ-1, the keystone.
2. **Stand up Chromatik** with `fixtures.json` as the model; study `titanicsend/LXStudio-TE`. Prototype 2-3 patterns + one audio-reactive modulation. (Proves the authoring path fast.)
3. **Compile Ben's pattern engine → WASM** (with Ben). This unlocks BOTH the dev sim and the live mirror.
4. **R3F mirror twin v0:** load glTF + `fixtures.json`, render WASM-sim output seeded by (mock then real) heartbeat state; add one volumetric god-ray pass.
5. **Show Compiler v0:** Chromatik/authored show → per-fixture param/keyframe export for the mesh (params, not pixels).
6. Defer Jetson/voice/LLM-VJ to 2027 (per 06-stack-opinion). 2026 = author in Chromatik → compile → autonomous mesh → mirror twin.

## The one fork to decide
**Chromatik-centralized vs Ben-on-fixture patterns.** They reconcile via the Show Compiler (author centrally, execute on-fixture). Confirm with Ben that his pattern engine can ingest compiled param/keyframe files — that's the seam that makes the whole hybrid work.
