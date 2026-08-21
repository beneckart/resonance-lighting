# Sound-Responsive Lighting — Best-in-Class + Upgrade Plan

*Research note (cycle 74) for the audio-reactive engine. Verdict: our ANALYSIS layer is already ahead of commercial DJ-lighting (rekordbox/Resolume/MadMapper can't even auto-extract BPM — operators tap it in). The gaps vs best-in-class are TIMING and TASTE, not more features.*

## What the leaders do (steal list)
- **rekordbox Lighting / PRO DJ LINK** — picks the look from the **song's phrase** (intro/verse/build/chorus/drop) on a locked beatgrid, not from instantaneous loudness. → live **section/energy-tier** model is the #1 idea.
- **Resolume** — dual-clock: *free-running FFT reactivity* vs *tempo-synced* layers, deliberately blended. Apply a contrast curve to FFT before mapping.
- **TouchDesigner** — a **normalized control bus** between analysis and visuals (named signals patterns subscribe to); normalize-before-map; different bands→different layers.
- **Chromatik/LX** — Beat Detect emits a **decay ramp** (not a boolean); a **Quantizer** holds a trigger until the next tempo division; tempo-division LFOs.
- **MadMapper** — **quantized launch** (look change waits for the next bar).
- **GrandMA / Pangolin** — tempo is an **external shared truth** (Ableton Link / MIDI clock), phase-following, with a **BeatManager** abstraction + source fallback.

## Mapping ART (design principles)
- **Contrast is the game** — keep a calm base look; transients ACCENT it. Not everything reacts (a "reactivity budget").
- **Less is more** — pull DOWN on breakdowns, slam UP on drops (build/release arc).
- **Layer = base wash + accent + beat-locked chase** (fixture roles).
- **Color = energy/emotion** — warm/red = arousal, cool = calm, saturation amplifies, brightness up = positive. Map energy-tier → warmth+sat+brightness.

## Latency/smoothing
- Human AV sync window ~80–110ms; keep light within ~50ms, err EARLY not late. Asymmetric envelopes (attack ~1ms / release ~20–80ms). Once phase-locked, **predict** the next beat + schedule at `nextBeat − pipelineLatency`.

## PRIORITIZED upgrades for us (impact÷effort)
- **P0-1 Tempo-PHASE lock (PLL)** — phase accumulator advanced by BPM/60, phase-corrected on each onset → expose `beatPhase`/`barPhase`. Chases hit ON the beat. (our #1 gap)
- **P0-2 Section/energy-tier state machine** — generalize drop-detection into {ambient,groove,build,peak} from rolling level+flux+bass+centroid → auto-pilot maps tier→whole look, quantized to the bar.
- **P0-3 Quantizer primitive** — discrete events (pattern switch/accent/strobe) fire on the next 1/4·1/2·bar division.
- **P0-4 Meyda spectral centroid→hue / rolloff→spread / flatness→gate** — timbre-aware colour (Meyda wraps our AnalyserNode, runs realtime).
- **P1-5 Per-onset decay-ramp envelopes** (Chromatik model) — onsets emit a shaped 1→0 ramp, not a boolean.
- **P1-6 Dual-clock + reactivity budget** — formal tempo-synced vs free-reactive layers + a "% fixtures allowed to react" knob (contrast).
- **P1-7 Predictive beat scheduling** — cancel pipeline latency once phase-locked.
- **P2-8 Ableton Link bridge** (Node→WS→browser) — lock to the DJ's real master clock; BeatManager fallback Link→MIDI→audio-estimate→tap.
- **P2-9 Stem proxy** — kick-band (50–120Hz) transient + vocal-presence (300–3000Hz gated by flatness) ≈ 80% of stem value, ~0 latency.
- **P2-10 Energy→colour emotion palettes** baked per tier.

**Architectural through-line:** a **normalized control bus** `{bass,mid,high,level,onsetLow/Mid/High,beatPhase,barPhase,section,centroid,rolloff,flatness,kick,vocal}` patterns subscribe to — every upgrade just adds a channel.

**SHIPPED so far from this:** cycle 74 `reactiveSpeed()` (energy+BPM+drop → live motion speed, '🎵 RX-SPD' toggle). NEXT: P0-1 phase-lock + P0-2 section tiers + P0-4 Meyda centroid.

*Sources: rekordbox Lighting guide, Resolume/TouchDesigner/Chromatik/MadMapper/Pangolin docs, Meyda, Ellis beat-tracking, ISMIR zero-latency beat, Wilms&Oberfeld colour-emotion, AV-sync PMC studies. (full list in agent transcript.)*
