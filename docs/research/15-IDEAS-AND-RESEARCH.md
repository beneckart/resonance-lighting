# 15 — Deep-Dive Research + IDEAS (DJ UI · audio · BM art · patterns · sensors · models)

> portal-strategist, 2026-06-14. For lighting-architect. Repo: `resonanceart/resonance-lighting` @ elliots-controller (dossier asset 01ef2b29). Sourced from live web research + the dossier + Ben's repo. Pull the actionable picks into the backlog.

## A. REAL VIRTUAL DJ CONTROLLER (Elliot: "spinning slider etc.") — concrete libs
- **NexusUI** (`nexus-js/ui`) + **react-nexusui** — has a **`vinyl` component = the spinning jog wheel/platter** Elliot wants, plus `dial`, `slider`, `multislider`, `position`, `toggle`. Fastest path to a real deck. https://nexus-js.github.io/ui/ · https://github.com/rakannimer/react-nexusui
- **AudioUI** (`cutoff/audio-ui`) — React audio controls (Knob, Slider, Button, CycleButton) with DAW/DJ visual variants, performance-focused. Great for the EQ knobs + faders. https://github.com/cutoff/audio-ui
- **webaudio-controls** (`g200kg`) — skinnable WebComponent knobs/sliders/switches (classic hardware-knob look).
- **dsssp** — React EQ filter curve viz (drag-n-drop) for the 3-band EQ display.
- **Audio graph for a real deck:** per-channel `GainNode` (faders) → `BiquadFilterNode` ×3 in series (low/mid/high EQ) → master; **crossfader** = equal-power blend of the two channel gains. Ref: entonbiba.com "Create a DJ Mixer with Web Audio" · MDN BiquadFilterNode.
- **DECK LAYOUT to copy** (from Rekordbox/Traktor/Serato): top scrolling **waveform** per deck, big **jog wheel** center, **tempo fader** side, **cue/play pads** bottom, **crossfader** between decks, **3-band EQ + gain** per channel, FX knobs. (Resolume/TouchDesigner = node/clip-grid for the VJ side.)
- IDEA: our "decks" = look A vs look B (the crossfader already blends them); jog wheel scrubs pattern phase/speed; pads = cues. Wire NexusUI vinyl → pattern phase.

## B. AUDIO — full component coverage (Elliot: "works with as many components of audio")
- **Sources to support+test:** mic (getUserMedia) · song-file player · **line-in / DJ booth-out** (USB dongle → getUserMedia device select) · any `<audio>/<video>` node · (later) WebRTC stream.
- **Libs:** **realtime-bpm-analyzer** (TS, zero-dep, real-time, multi-source: file/stream/mic/node) — better for LIVE than buffer-analyze; **web-audio-beat-detector** (offline AudioBuffer → BPM, good for the loaded track); **Meyda** (features → map to visuals): RMS→brightness, spectralCentroid→hue, spectralRolloff→spread, spectralFlatness→noise/sparkle, ZCR→texture, loudness, MFCC.
- IDEA: a device picker (enumerateDevices) so he can select mic vs line-in; a "source" tab; test matrix = each source × {bands, onset, BPM, drop} with the 124 + 140-drop tracks.

## C. SENSORS (Elliot: "motion/people, temperature, wind speed; beacon mode") — SIM now, real later
- Per dossier (Research Notes 6) + Ben's mesh: **presence** (PIR/mmWave/ToF), **temperature** (BME280), **wind** (anemometer / chime-IMU), baro (storm forecast).
- SIM IMPLEMENTATION (no hardware): a **"Sensor Sim" panel** with sliders/inputs: crowd/presence, temperature, wind-speed, baro-trend → drive **element modes**:
  - presence → **ripple/mesh-choreography** (already shipped 07bf755 — extend: density scales with crowd)
  - wind ↑ → wind-ride sway + 20% dim (bank energy) · gusts = IMU-style ripples
  - heat (day) → pause-charge/amber · cold → ember palette · rain → silver-blue cascade
  - baro drop + wind → **amber storm-forecast**
- **BEACON MODE** (whiteout/safety): a toggle/auto-trigger → all fixtures pulse a bright **lighthouse sweep** (high-vis, overrides everything) — genuine safety feature for dust storms. Add as a top-level mode + preempts shows. (Dossier: Beacon preempts; aviation lost-link rule.)
- Swap-later: the sim sensor inputs map 1:1 to real ESP-NOW telemetry fields → no rework.

## D. MORE PATTERNS (Elliot: "more patterns / other patterns running") — GLSL catalog
- Source the classics from **Shadertoy** + **Patricio Gonzalez Vivo's GLSL** (gist 3a81453a) + **Pixelblaze pattern ports** (electromage forum "Porting GLSL Shaders"): **plasma, fire, fluid/curl-noise, waves, Perlin/simplex noise fields, reaction-diffusion, Game-of-Life, Lenia, voronoi, kaleidoscope, aurora, lightning, ripple-interference**.
- IDEA: a small **GLSL pattern runtime** (a fragment shader sampled per-fixture at its UV/position → color) so adding a pattern = adding a shader. This unifies the twin look with the eventual on-fixture math. Map fixture position → shader coords. (Reuse the dossier's shader approach + TE's GLSL engine pattern.)
- Quick wins (no shader engine): comet/meteor, twinkle/starfield, breathing-bloom, color-wipe, strobe-chase, VU-meter-up-the-trunk, fireflies (already noted), candle-flicker.

## E. MORE MODELS / VIEWS (Elliot: "more models")
- Multiple **camera presets** (front / hero 3-4 / top-down / worm's-eye / orbit-auto) — saved viewpoints. (A6.)
- **LOD**: a high-detail hero model + a decimated performance model (toggle for 60fps on weaker devices).
- Additional exports from the .blend: an **interior/portal-door view**, a **night-vs-day** world toggle, the **chandelier/uplight** fixtures (the 24+16 not yet in the 78 — reconcile count) as more fixtures.
- IDEA: a "VR/AR preview" stretch (WebXR) for walking the tree (dossier B8).

## F. BM / LARGE-SCALE ART — improve + reference (Elliot: "how can we improve")
- 2024-25 refs: **Radial Sonic Runway** (LEDs ↔ sound from people/machines — strong sound-reactive precedent), **Sonapse** treehouse (canopy LEDs aligned to music), **AlchemEyes** chandelier, **Afterlife Reincarnate** (150′ blacklight/LED), projection-mapped **72′ pyramid** (day gathering / night dance). **Rain Room** (presence-reactive). https://history.burningman.org/brc-history/event-archives/2025-event-archive/2025-art-installations/ · dezeen BM 2024/2025.
- IMPROVE IDEAS for us: (1) **sound-reactive as the hero** (Radial Sonic Runway model — crowd's sound drives the tree); (2) **presence-reactive intimacy** (Rain Room — lights respond to where people stand → the mesh-choreography ripple); (3) **day-vs-night modes** (calm ambient day → show at night); (4) **a "voice/LLM" layer** (personalized audiovisual from conversation — the F LLM lane); (5) **beacon as signature** (the tree as a navigational landmark on playa).
- Predecessors we already mine: Tree of Ténéré (LX), Titanic's End (LXStudio-TE), Entwined (150+ patterns + audio + QR audience control).

## H. REAL-PROJECTION PIPELINE (geometry-accurate) — from blender-architect (handoff 96c35fa7)
THE accuracy gap: the twin's GLB is `01_Structure` ONLY. MISSING: 🔴 **Plu Plu bark** (it shelters/shapes the light → changes the ray-cast → the twin's projection is WRONG without it), 🔴 real downlight bodies (twin draws a glyph), 🟡 chandelier/culms. "Lighting Video.mp4" is a MODEL render (geometry ref), not ground-projection footage.
PIPELINE (blender bakes → lighting consumes): blender-architect owns export end-to-end → `build_downlight.py` (cylinder + flared skirt + LED → light through skirt gaps = real ray-cast) + **Plu Plu bark** → bake **IES profile** + render **gobo PNG of the real skirt-gap/bark shadow** → deliver {instanced lantern .glb ×78, IES, gobo}. Lighting-architect: render 'lanterns' mode with the real instanced glb; project each fixture's gobo **tinted by its reported color** (overlapping COLORED shapes on the ground — Elliot's "real effect, not circles") + IES for beam falloff.
AXIS reconcile: glb Y-up; lighting's blenderToThree() (Z-up→Y-up) must align so lights sit ON the bodies.
🔒 BLOCKER: Blender bridge dropped (Walk Mode) → **Elliot must click "Connect to Claude"** so blender builds the lantern + bakes IES/gobo.

## I. BETTER LIGHTING PHYSICS (Elliot: "build it smarter, better lighting physics")
- **Inverse-square falloff**: Three.js PointLight `decay=2` (physically correct) + `power` in lumens (map from fixtures.json lumens_max). Per-light range cutoff for perf.
- **RectAreaLight** for soft area sources (needs MeshStandard/Physical material) — lanterns as soft glows, not points.
- **Tone mapping**: ACES/AgX (already ACES) + exposure control; HDR gain on emissive lanterns.
- **Volumetric beams accuracy**: raymarched scattering w/ **Henyey-Greenstein phase function** (anisotropic) instead of flat cones; **additive atmospheric haze that builds with distance** = depth (teamLab-style). Adaptive sampling for 60fps.
- **teamLab layered color** ("Kasane no Irome"): layer front/back color gradations for richer, non-flat color — apply to patterns + the gobo tint.
- Refs: threejs.org/examples/webgl_lights_physical · discoverthreejs PBR · Maxime Heckel volumetric raymarching · gamedeveloper atmospheric-scattering.

## TOP PICKS to feed the builder now
1. DJ controller: **react-nexusui `vinyl` (jog) + AudioUI knobs/faders + BiquadFilter EQ + GainNode crossfader.**
2. Audio: add **realtime-bpm-analyzer** + a **source/device picker** (mic/line-in/file); test matrix per source.
3. Sensors: **Sensor-Sim panel** (crowd/temp/wind/baro) → element modes + **BEACON mode** toggle (safety sweep, preempts).
4. Patterns: a **GLSL pattern runtime** + port plasma/fire/fluid/noise/reaction-diffusion.
5. Views: camera presets + LOD + day/night world.
