# 10 — Deep findings: model export, Pixelblaze analog, gobo projection, sound, shaders

> 2026-06-13, from reading the LX/TE source (local clones) + targeted web research.

## 1. Export from Blender / Grasshopper → YES (the keystone, and it's low-lock-in)
LX ingests **arbitrary 3-D point lists natively** (confirmed in the cloned source):
- `structure/JsonFixture.java`, `PointListFixture.java`, `PointFixture.java`, `LXBasicFixture.java`
- `model/LXModelBuilder.java`, `LXModel`, `LXPoint(x,y,z)`
- `.lxf` = JSON fixture files (Titanic's End defines its whole 128k-LED model in `.lxf` — e.g. `te-app/Fixtures/TE/TE.lxf`).

**Pipeline:** `Grasshopper/Rhino (or Blender) → list of fixture points {id, role, x, y, z} → fixtures.json (our neutral contract) →` (a) LX `.lxf` / `PointListFixture`, and (b) glTF + positions for the R3F twin. **One export feeds both LX and the web twin.**

- **Grasshopper is the PREFERRED source** — parametric, exact, "designed not surveyed" (Master Report PART 3 already named it; OQ-1 = Rhino/Grasshopper export). GH → JSON/CSV is native (a JSON export component or a ~3-line GHPython script). Ed/Vishnu own that parametric model.
- **Blender** also works: a `bpy` script dumps LED-object / empty world positions → JSON (blender-architect's job).
- **Low lock-in:** `fixtures.json` is environment-neutral — it feeds LX, the R3F twin, or anything else. So the authoring-tool decision is **reversible**; the model export is never wasted. This de-risks the "important decision."

## 2. Pixelblaze + Firestorm — the proven commercial twin of Ben's design (validate + borrow)
- **Pixelblaze** (ESP32) runs patterns **on the device**; **[Firestorm](https://github.com/simap/Firestorm)** does **NTP-style time-sync across many controllers** + "select a pattern by name → it launches on every device that has it"; the **Sensor Board broadcasts sound/accelerometer features WIRELESSLY to other Pixelblazes** (firmware v3.40+ sync).
- This is **almost exactly Ben's architecture** (autonomous patterns + wireless sync + sound sensor). Implications:
  1. **Ben's design is proven** — a shipping product works this way.
  2. **Firestorm is a ready blueprint** for our control-plane / Show Compiler: time-sync + select-pattern-by-name across the fleet.
  3. **Sound on a wireless mesh, solved:** one **sensor node broadcasts audio features over ESP-NOW → every fixture reacts locally.** This is the answer to "works with sound, no wires." → **flag to Ben.**

## 3. Gobo / mandala projection + beams in the web twin → YES, accurately enough
- Three.js `SpotLight.map = texture` **projects a gobo/cookie pattern in real time** — i.e., the twin can render the actual bamboo **mandala projections**, not just glowing dots. Add a volumetric cone for the visible beam.
- Refs: [Codrops texture projection](https://tympanus.net/codrops/2020/01/07/playing-with-texture-projection-in-three-js/) · three.js `SpotLight` docs (`.map`) · [volumetric rays](https://blog.maximeheckel.com/posts/shaping-light-volumetric-lighting-with-post-processing-and-raymarching/). (Note: spotlight `.map` requires `castShadow=true`.)

## 4. Sound — strongly supported on both sides
- **LX `audio` pkg** (authoring): `FourierTransform`, `BandFilter`, `BandGate`, `GraphicMeter`, `Envelop`, `SoundObject`/`SoundStage`, Reaper integration, `GraphicEqualizerPattern` example. Rich built-in reactivity.
- **On the autonomous mesh** (live, no wires): Pixelblaze-style sensor-node-broadcast (§2).

## 5. Rich visuals — GLSL shader patterns (reusable across LX *and* our twin)
- TE ships a full **GL shader pattern engine** (`pattern/glengine/` — `GLShaderPattern`, `TEShader`, `ShaderPainter`, precompiler) + **101 `.fs` GLSL shaders**. This is the high-fidelity pattern path.
- **GLSL is portable:** the same shader math runs in TE's engine *and* in our R3F twin (Three.js shaders). Patterns authored as shaders can serve both the LX preview and the web mirror.

## 6. Sister reference — Entwined (Charles Gadeken)
LED forest, **150+ patterns + audio + QR-code audience control** ([charlesgadeken.com](https://www.charlesgadeken.com/about)). Design inspiration for interactivity (QR audience control ≈ our presence/wand interaction).

## Bottom line on the decision
- **Authoring / patterns / sound / preview:** LX / Chromatik (proven on the Ténéré "Tree"; Pixelblaze independently validates the autonomous-pattern model).
- **Live iPad mirror:** custom R3F twin (gobo texture-projection + volumetric beams, reported-state-driven).
- **Mesh control-plane + wireless sound:** Pixelblaze/Firestorm blueprint (time-sync + pattern-by-name + sensor broadcast).
- **Neutral contract = `fixtures.json` from Grasshopper** → low lock-in; the decision is reversible.
