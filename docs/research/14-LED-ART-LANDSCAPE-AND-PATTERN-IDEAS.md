# Research: Large-Scale LED-Art Software & Volumetric Tree Pattern Design

*For the Resonance Tree (Burning Man 2026) R3F visualizer/controller. Focus: steal pattern ideas + design vocabulary for our parameter-driven (not pixel-driven) engine. Compiled cycle 35 (2026-06-13) via web research.*

---

## Part 1 — Burning Man / Large-Scale LED-Art Software & Controllers

### The Tenere → Entwined → Chromatik/LX lineage (our closest ancestor)

A giant illuminated **tree** at Burning Man, driven by a **parametric pattern engine sampling 3D point positions** — exactly our model.

- **Tree of Ténéré (BM 2017)** — Zachary Smith. ~100k+ LEDs / 15k+ leaves. Driven by **LX Studio** with live audio, MIDI surfaces, heart-rate sensors, Muse EEG headsets as real-time inputs. Confirms "parameters + live external input." ([lx.studio/tenere](https://lx.studio/tenere))
- **Entwined (`squaredproject/Entwined`)** — descendant codebase (~10k LEDs). Pattern authoring = **per-point sampling** (a pattern returns a colour per 3D point). **No clear OSS license (NOASSERTION)** — reference only.
- **Chromatik / LX (`heronarts/LX`, Mark Slee)** — the engine underneath. **15 built-in pattern types** as a vocabulary checklist: Chase, **Chevron** (multi-axis angular shape w/ motion), DMX, **Gradient** (3D colour gradient over geometry), **Image** (project 2D→3D), **Life** (Conway), **Noise** (Perlin/Ridge/FBM/Turbulent), **Orbox**, **Planes** (overlapping 3D planes), **Script** (live JS), Slideshow, Solid, **Sound Object** (audio-reactive field), Sparkle, Test. Params: Motion/Timing (manual OR tempo-synced divisions), Level/Brightness, Spatial (XYZ, yaw/pitch/roll, scale), Modulation depth. Modulators (LFOs, audio) patch onto any param — a DAW-for-light model. **License: NOT open source** (Heron Arts custom license) — reimplement ideas, do not vendor.

### WLED + WLEDtubes (the mesh-firmware lineage)
- **WLED (`wled/WLED`)** — ESP32 firmware, **EUPL-1.2 (true OSS)**. 100+ effects + 50 FastLED palettes, segment-based. Core idea = **palette + effect-on-segment**.
- **WLEDtubes (`craiglink/WLEDtubes`)** — **MIT**, BM 2019. WLED usermod with an **ESP-Now mesh** → all tubes **auto-sync without WiFi** via a shared global beat/phase clock; a **particle library** composites moving particles over a base FX layer.

### Pixelblaze — live **expression-language** engine (`render3D(x,y,z)` edited live, 100s fps). Spiritual match to our author-once/position-sampled approach.

### FadeCandy / OPC — transport, not engine. OPC = stream an RGB list to a controller; FadeCandy adds **temporal dithering/interpolation** for ultra-smooth low-end fades. OPC is the obvious output protocol if the twin ever drives real hardware.

### TouchDesigner / Resolume — "treat light as video" (map a 2D/3D texture onto fixture positions). The paradigm we are *not* using, but the **texture-sampling idea** is worth one pattern.

---

## Part 2 — Projection / Gobo / Beam Design

- **Breakup gobos = dappled-canopy texture.** Irregular organic breakup (light through leaves), NOT hard logos — lean into this for our gobo floor.
- **Beam character: shaft vs flood.** Tight angle → discrete **shafts/rays**; wide → **wash** but loses definition. Sweet spot for textured beams ~30–40°, focused just soft of hard-edge. `beamAngle` should drive a continuum godray↔glow. Haze makes beams volumetric — we fake it with additive cones. *(We already tightened our rays to a 15° crisp shaft, cycle 33.)*
- **Depth = warm/cool + angle variety.** Height-based warm/cool split reads instantly as depth.

### Colour cheat-sheet
- **Analogous** = calm (breathe/ambient). **Complementary** = max contrast (pop on beats). **Triadic** = our `tricolor` home (orbit 3 hues in azimuth). **Warm/cool** = depth. **Energy = saturation + contrast** → drive saturation off audio energy (bloom on drops).

### 10 concrete new pattern ideas (inputs: `pos.xyz`, `height 0-1`, `azimuth seqOrder`, `beamAngle`, `bass/mid/treble/beat/bpm`)

1. **Canopy Dapple** — FBM noise at `(x,z,t*speed)`, threshold to sparse soft spots; `mid` raises threshold. *(Noise + breakup gobo.)*
2. **Godray Shafts** — N azimuth sectors; fixtures in a narrow cone go bright, rotate at `bpm`. Tight beamAngle = lasers. *(shaft principle.)*
3. **Rising Sap** — `bri = pulse(height - (t*speed mod 1))`; band climbs trunk→canopy; `beat` injects a new pulse from base. *(Planes vertical.)*
4. **Tricolor Orbit** — three hues at `az = base + k*120°`, triad rotates at `bpm`; each hue's sat = its band's audio. *(triadic.)*
5. **Warm/Cool Depth Split** — `hue = lerp(warm,cool,height)`, moving boundary `0.5+0.3sin(t)`; `bass` pushes warm up. *(depth layering.)*
6. **Plane Wipe** — rotating 3D plane normal `n`; `bri = falloff(dot(pos,n) - t*speed)`; beat re-randomizes `n`. *(Planes/Chevron.)*
7. **Spectrum Spiral** — `hue = (az/360 + height*turns + t*speed) mod 1`, full sat — rainbow barber-pole up the tree, `bpm`-synced. *(WLED rainbow + twist.)*
8. **Bloom (audio saturation)** — analogous base; `sat = baseSat + energy`, `bri += beatEnv`. Blooms saturated on drops. Overlay on any base. *(energy=sat.)*
9. **Particle Embers v2 (blit layer)** — sparse particles spawn at base, drift up (vel ∝ treble), fade; composited over a dim wash; spawn rate ∝ mid. *(WLEDtubes particle-over-base.)*
10. **Texture Scroll** — project to UV `(az,height)`, sample a scrolling aurora/gradient texture; scroll `bpm`-synced, gain `bass`. Video-like auroras cheaply. *(Image pixel-mapping.)*

### Cross-cutting engine recommendations (high-leverage)
- **Shared `phase` clock + BPM-synced timing divisions on every pattern** (Chromatik + WLEDtubes both center on this) — musical coherence almost free.
- **Layered compositing** (base wash + particle/sparkle overlay) instead of monolithic patterns — makes ember/sparkle reusable as overlays.
- **Live JS expression pattern** (`render3D` style) for at-the-tree prototyping — Pixelblaze/Chromatik's biggest workflow win.
- **Temporal dithering on slow fades** (FadeCandy) — kills banding on breathe/solid at low brightness. *(We added a 110ms colour slew cycle 35; dithering is the next step.)*

### License note (CRITICAL)
Only **WLED (EUPL-1.2)** and **WLEDtubes (MIT)** are safe to copy code from. **Chromatik/LX and Entwined are NOT OSS** — reimplement ideas, never vendor their source. *(Consistent with our existing rule: LX/TE cloned at `~/code/_ref/` only, never committed.)*

**Sources:** lx.studio/tenere · squaredproject/Entwined · chromatik.co/guide/patterns · heronarts/LX · wled/WLED · craiglink/WLEDtubes · electromage.com/pixelblaze · opc-java/FadeCandy · HARMAN concert color theory · On Stage Lighting (gobos).
