# REFERENCES — external source + how to get the software

> **License gate:** Chromatik/LX (and Titanic's End) are **source-available, NOT open-source**, and
> **redistribution of their source is prohibited.** Do **NOT** commit their source into this (public) repo.
> They are cloned **locally** for our own reference + the app is a download. This file is the manifest so
> any machine can reproduce the setup.

## Local reference clones (this Mac)
Path: `~/code/_ref/lx-ecosystem/`
| Dir | Repo | What | Reproduce |
|---|---|---|---|
| `LX/` | github.com/heronarts/LX | the engine (Java lib) | `git clone --depth 1 https://github.com/heronarts/LX.git` |
| `GLX/` | github.com/heronarts/GLX | GUI/simulator harness | `git clone --depth 1 https://github.com/heronarts/GLX.git` |
| `Chromatik/` | github.com/heronarts/Chromatik | app metadata/license | `git clone --depth 1 https://github.com/heronarts/Chromatik.git` |
| `LXStudio-TE/` | github.com/titanicsend/LXStudio-TE | **template** real BM app (Titanic's End) | `git clone --depth 1 https://github.com/titanicsend/LXStudio-TE.git` |

## Get the software (to run the sim environment)
1. **JDK:** Temurin 21 — `brew install --cask temurin@21` (Titanic's End's required SDK).
2. **Chromatik app (easiest sim):** download from **https://chromatik.co/** (v1.2.1, Nov 2025). Free for our use.
3. **Maven** (to build a custom LX app): `brew install maven`. Build TE: `cd ~/code/_ref/lx-ecosystem/LXStudio-TE && mvn clean -U package && mvn install`.

## Key links
- LX engine: https://github.com/heronarts/LX · GLX: https://github.com/heronarts/GLX
- Chromatik app + guide: https://chromatik.co/ · license: http://chromatik.co/license/ · OSC guide: https://heronarts.lx.studio/guide/osc/
- Titanic's End (template): https://github.com/titanicsend/LXStudio-TE
- Tree of Ténéré (the predecessor, ran on LX): https://lx.studio/tenere
- Web volumetric rays (for our R3F mirror twin): https://tympanus.net/codrops/2022/06/27/volumetric-light-rays-with-three-js/ · https://blog.maximeheckel.com/posts/shaping-light-volumetric-lighting-with-post-processing-and-raymarching/
- LED layout util: https://github.com/jasoncoon/led-mapper · CV LED mapping: https://github.com/PWRFLcreative/Lightwork
- Accurate-ray pre-viz (optional, paid): Depence² https://pangolin.com/products/depence-stage-lighting-module

## How to stand up the sim TODAY (no hardware)
1. Generate placeholder `fixtures.json` (see `08-BUILD-NOW-software-sim.md`).
2. Either: open **Chromatik app** + build an LXModel from it (fast), OR clone **LXStudio-TE** and adapt its model to our tree (full custom path).
3. Author 2–3 patterns + one audio-reactive modulation; preview in the 3D simulator.
4. In parallel, build the custom **R3F mirror twin** (the iPad/live target).
