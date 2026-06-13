# 12 — Festival / immersive-art pro techniques → our R3F + Web-Audio build

> From a deep web-research pass (2026-06-13). The implementation playbook for "super
> professional, festival-grade." One-line: architect like **Chromatik/LX** (3D-positional
> patterns → blend-mode channels → audio-fed modulation bus), drive motion with
> **GrandMA3-style Phasers** locked to a **self-built beat-grid**, feed it **Meyda +
> realtime-bpm-analyzer** with asymmetric envelope followers + percentile AGC, nail
> **McCurdy's bloom recipe**, and gate intensity by an **energy-state machine**.

## Architecture to adopt (one app, three layers — keep distinct)
```
MODULATION BUS  ← Web-Audio features (bands, onsets, RMS, beat-grid) write NAMED signals
      ↓ patched to params
PATTERN CHANNELS ← each pattern = fn(fixture xyz, time, signals) → color   (we have xyz!)
      ↓ blend modes (Add = festival default) + per-channel fader + A/B crossfader + master
COMPOSITE → bloom → ACES/AgX tonemap → fixtures (twin now; Art-Net/DDP→ESP-NOW later)
```
Key: **audio analysis does NOT live in patterns** — a modulator layer writes named signals; patterns read them. Re-patch "kick→bloom" without touching pattern code.

## TIER 1 (amateur→pro fastest)
1. **Bloom done right** (McCurdy): emissive ≫1.0; threshold-keyed bloom in LUMINANCE (high threshold ~0.3–0.4, strength ~0.5 — low threshold = haze = amateur); **tonemap AFTER bloom (ACESFilmic, or AgX to keep neon saturated)**; `<Bloom mipmapBlur>`. Tune live with **leva**. https://www.donmccurdy.com/2024/04/27/emission-and-bloom/
2. **Asymmetric attack/release envelope followers** per band (musical, not jittery): fast attack / slow release via `alpha = 1 - exp(-dt/(tau/1000))`. bass atk 5–15ms/rel 150–350ms; treble atk 2–5ms/rel 80–150ms; RMS 300–800ms. Set AnalyserNode.smoothingTimeConstant LOW (0–0.3), do own asymmetric smoothing.
3. **Self-built beat-grid** (Ableton-Link model): keep {bpm, beatAnchorTime}; `beatAtTime`/`timeAtBeat`; **predict nextBeatTime + fire visuals on it** (detection lags 10–50ms); PLL phase nudge (10–20%, never snap); tap-tempo overrides.

## TIER 2 (the look + motion)
4. **Phasers** (GrandMA3): every effect = waveform (sin/tri/sq) + per-fixture PHASE OFFSET + speed in BPM. 90° spread across fixtures = rolling wave/chase → 100 fixtures look like one organism. https://help.malighting.com/grandMA3/2.0/HTML/phaser.html
5. **Beams + haze**: drei `<SpotLight>` volumetric, share ONE `useDepthBuffer()`; thin `FogExp2`/`<Cloud>` haze. `<GodRays>` only ≤1 hero source. (We have additive cones now — upgrade later.)
6. **Color in OKLCH not HSL** (HSL hue passes through muddy gray) — `culori`/`colorjs.io`; crossfade named palettes on sections. → directly serves "full spectrum / many colors".
7. **Spring-damper (ζ=1) + lateral diffusion** on final fixture values (flow_k~0.18) → fluid wave across the tree, not jumpy bars.
8. **Cinematic camera**: damped orbit + GSAP-eased cuts to preset shots on downbeats.

## TIER 3 (auto-VJ director)
9. **Meyda** (AudioWorklet) rms/energy/spectralFlux/spectralCentroid; onset = spectral flux + adaptive median + refractory ~60–100ms; **realtime-bpm-analyzer** for live BPM; essentia.js OFFLINE only; mel/log bands + A-weighting (lift highs).
10. **AGC**: percentile normalization (track ~95th=max, ~5th=floor, slow asymmetric decay) + noise gate (~−55dB). Major pro/amateur separator.
11. **Drop detection**: energy build→trough→broadband spike on downbeat → biggest look.
12. **Director state machine**: classify section (ambient/build/drop/breakdown) → gate eligible patterns; switch ONLY on phrase boundaries (bar%8|16|32); **shuffle-bag** select; crossfade; drop overrides.
13. **LLM-VJ = slow loop only**: emits a cue-sheet (section→pattern-pool+palette+transition); fast state machine executes. Never block the frame loop on the LLM. ← maps onto our H3 command console as the LLM tool surface (F1).

## Cross-cutting: "less is more" is programmable — gate intensity (bloom/active-beams/CA/cut-rate/saturation) by the energy state; never visibly loop (seed phase with noise+audio). teamLab principle.

## Library shortlist
Audio: **meyda**, **realtime-bpm-analyzer** (essentia.js offline only). Render: `@react-three/postprocessing` (Bloom/GodRays/LUT/Tonemapping), `@react-three/drei` (SpotLight/useDepthBuffer/Cloud/CameraControls), `three-custom-shader-material`, **leva** (live tuning). Color: **culori**. Camera: **gsap** + camera-controls. Read: `heronarts/Chromatik` source.

## Our adoption order (folds into Phase I)
- NOW: ACES tonemapping after bloom (quick pro win) · OKLCH/spectrum + tricolor "dancing" patterns (Elliot's ask) · Phaser-style per-fixture phase in patterns.
- NEXT (B spine): Meyda + realtime-bpm-analyzer + envelope followers + AGC + beat-grid → modulation bus; test on real songs.
- THEN: director/auto-VJ (shuffle-bag, phrase switching, drop) → C DJ controller + crossfader → F LLM-VJ over the command console.
