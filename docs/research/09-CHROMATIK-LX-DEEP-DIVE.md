# 09 — Chromatik / LX deep dive (architecture + API the agent will use)

> Researched 2026-06-13 by reading the actual repos (cloned to `~/code/_ref/lx-ecosystem/`).
> **License (precise):** Chromatik/LX is **NOT open-source.** Free for non-commercial use under a
> **$25K/yr total-revenue cap** (a BM art collective qualifies). **Source redistribution is PROHIBITED**
> — "Licensee shall not provide any third party with access to the source code… only object code may be
> distributed." → That is why LX/GLX/Chromatik/TE source is **cloned locally, NOT committed to this repo.**
> License: http://chromatik.co/license/ · commercial: licensing@chromatik.co

## The three repos (cloned to `~/code/_ref/lx-ecosystem/`)
- **`LX`** (4.8 MB) — the **engine** (Java lib). "Core library for 3D LED lighting engines." Runs **headless** on any Java device (Pi/Jetson). This is what we build on.
- **`GLX`** (21 MB) — LWJGL+bgfx GUI harness: the **3D simulator/visualizer + UI controls**. Embeds LX in a window.
- **`Chromatik`** (732 KB) — the polished app metadata/license (the actual app is a **download** from chromatik.co, v1.2.1 Nov 2025).
- **`LXStudio-TE`** (475 MB — Titanic's End) — a complete real-world app built on LX. **Our template.** Maven, Temurin **JDK 21**, module `te-app/`, project files `te-app/Projects/*.lxp`, plus `assets/` + `audio-stems/`.

## LX engine API surface (packages — read `~/code/_ref/lx-ecosystem/LX/src/main/java/heronarts/lx/`)
What we need, mapped to LX packages (all confirmed present in the clone):
- **`model`** — `LXModel` / `LXPoint`: the 3-D geometry. Each point has a real xyz. **This is where our `fixtures.json` becomes the tree** (build an LXModel of ~100–150 points). LX is "a sparse vertex shader" — perfect for scattered fixtures, not a 2D screen.
- **`structure`** — fixtures / fixture files (`.lxf`) and the model structure layer.
- **`pattern`** — `LXPattern`: subclass to **write a pattern** (the animation that runs per-frame over the model). Patterns are our "light instruments."
- **`effect`** — `LXEffect`: post-process a pattern's output.
- **`modulation` / `modulator`** — LFOs, envelopes, noise, step-sequencers; **route any source → any parameter** (this is the magic: audio/MIDI/time drive pattern params).
- **`audio`** — real-time audio analysis (FFT/bands/beat) → **sound reactivity**.
- **`midi` / `osc`** — control surfaces + bidirectional OSC.
- **`mixer` / `clip`** — channels, faders, cues, A/B crossfade, tempo-synced clips.
- **`color` / `blend`** — color composition + blend modes.
- **`output`** — `LXOutput` + protocol outputs (ArtNet/OPC/E1.31/DDP/KiNet). **All pixel-streaming.** ← the seam (below).
- `dmx`, `snapshot`, `scheduler`, `command`, `transform`, `parameter`, `clipboard`.

## How a real app is structured (from Titanic's End)
- **Maven** (`pom.xml`), JDK 21 (Temurin). Build: `mvn clean -U package && mvn install`. Run via IntelliJ or `java -XstartOnFirstThread -jar target/te-app-*-jar-with-dependencies.jar`.
- Custom **model defined in code** + saved **`.lxp` project files** (Projects/*.lxp). Patterns in **Java or GLSL shaders**. Sound via a virtual-audio loopback (BlackHole) or mic. Control via MIDI (APC40) + OSC.
- Takeaway: a custom LX app = `{ our LXModel from fixtures.json } + { our LXPatterns } + { GLX UI } + { our output }`, built with Maven.

## THE SEAM (LX pixel-streaming vs Ben's params-only mesh)
LX's `output` package streams per-pixel color (ArtNet/OPC/…). Ben's mesh runs patterns **on the fixture** and takes **control params only — never pixels.** Two clean ways to reconcile:
1. **Authoring/preview only (now):** use LX/Chromatik's **simulator** (GLX) to design patterns + sound on our 3-D model. **No output needed** — nothing leaves the laptop. This is the whole sim-first environment, free, today.
2. **Driving the real tree (later):** write a **custom `LXOutput` subclass** that, instead of streaming pixels, emits **ESP-NOW control-params** (which pattern + params per fixture) — i.e., LX becomes the authoring brain, the mesh executes. (Equivalent to the Show Compiler in Addendum A9.) Confirm with Ben his pattern engine can ingest these.

## How WE use it
- **Today (no hardware):** load our placeholder `fixtures.json` as an LXModel → open in Chromatik (or a GLX app) → author patterns + audio modulation → preview the tree in 3D. That's the sim environment Elliot asked for.
- **Decision to make with Ben:** custom `LXOutput`→ESP-NOW vs Show-Compiler export. Until then, authoring/preview is fully unblocked.
- **Our live iPad "mirror" twin** (reported-state, R3F) stays separate — LX's sim shows what you *author*; the mirror shows what the tree *reports*.

## Reference clones (local, do NOT commit — see REFERENCES.md)
`~/code/_ref/lx-ecosystem/{LX, GLX, Chromatik, LXStudio-TE}`
