# PRD — Resonance Tree Custom Lighting Environment ("the Cortex")

> **Owner:** Elliot Fabri · **Author:** portal-strategist · **Date:** 2026-06-13 · **Status:** v1 (build-ready spec for the `lighting-architect` agent)
> **Build agent:** `lighting-architect` (boot prompt: `stack/claude/boot-prompts/lighting-architect.md`)
> **Source corpus:** Ben's repo `github.com/beneckart/resonance-lighting` + the 5-doc dossier at `~/Resonance Tree Lighting Controls/01…05-*.md`

---

## 0. Mission (one sentence)
Build the **custom lighting environment for the Resonance Tree off the 3-D model** — a hardware-true **digital twin + control/show system** that drives the Tree's solar-mesh light fixtures via **control parameters only (never pixels)**, integrating every controller and research finding from the dossier, as an **optional "cortex" layer that rides Ben's autonomous mesh and dies invisibly**.

## 1. Context — the two-tier system
The Resonance Tree lighting is ONE coherent system in two tiers:
- **Brainstem (Ben Eckart, build-ready, the critical path):** ~100+ fungible, solar-powered, **autonomous** bamboo fixtures. Per fixture: **PowerFeather V2 (ESP32-S3)** + LiFePO4 + **SK6812 / RGBW** LEDs + gobo. Joined by an **ESP-NOW mesh that carries control parameters only — never pixels**; patterns render on the fixtures. Standard OTA. *This must be 100 fixtures in-hand & operational by ~Aug 20, 2026.* Source of truth = Ben's repo.
- **Cortex (THIS PRD, optional night layer):** a Jetson at the trunk base + a **PowerFeather as an ESP-NOW USB radio-modem** + an **iPad React-Three-Fiber twin/console** + MIDI/DJ-VJ + Voice + sensing + camp bridge. It "wakes at dusk, dies invisibly" — the lights run perfectly without it.

**This agent builds the cortex.** It does not build fixtures/firmware (Ben) and does not place fixtures in 3-D (blender-architect).

## 2. The job, precisely
1. **Off the 3-D model:** consume `fixtures.json` (designed fixture positions/roles/lumens, exported from the Rhino/Blender model by blender-architect) as the canonical fixture registry.
2. **Digital twin:** a hardware-true 3-D visualizer (R3F PWA) rendering the model + every fixture's *reported* state in physical units (lumens/beam/CCT), at 60fps.
3. **Control/show system:** the two-path control plane, console workflow (jam → lock cue → schedule), Show Compiler, MIDI + DJ/VJ modes, element modes, sensing/occupancy, Voice, camp bridge.
4. **Integrate the research:** implement against the locked decisions in the dossier (Protocol v1, failure ladder, PACE, security-by-physics, energy-truth).

## 3. Goals & non-goals (from Master Report PRD)
**P0 (must):** iPad PWA 3-D twin w/ live heartbeat states · two-path control (<50ms finger-to-photon; cues survive reboot) · master brightness + zone/global · cue-lock + editable schedule mirrored to master flash (plays cortex-off) · commissioning tap→blink→bind (150 in ≤1 day, 2 people) · Monitor go/no-go screen · fleet blackout (GPIO4) + SOC-gated OTA · ~150KB lite fallback page.
**P1:** Jetson cortex hub · per-fixture PIR + occupancy heat-map · directional door counters · weather suite + beacon · energy-truthful + firefly modes · camp bridge Tier 1 (Meshtastic) · music sync · client-scoped tokens.
**P2 (off critical path):** Voice pilot · heart-adoption · esp-csi spike · touch lanterns · BM portal sync · public twin.
**Non-goals (hard):** NO pixel streaming over radio · NO cloud on-playa · NO raw-audio retention · NO cameras · NO Starlink · NO AC/inverters.

## 4. Architecture
Mirror Ben's monorepo so it ports cleanly:
- `app/` — React-Three-Fiber PWA (TypeScript, zustand, CBOR). Screens: Jam, Cues, Schedule, Monitor, Commission, Twin. Installed on iPad, fully offline.
- `cortex/` — Python services on the Jetson: twin-server, occupancy, env, voice, bridge, db (SQLite+sqlite-vec). The PowerFeather master is its ESP-NOW modem over framed USB serial.
- `sim/` — firmware pattern core compiled C++ → WASM (zero-drift "golden-frame parity" against real fixtures); lets the app/twin develop without hardware.
- `firmware/`, `hardware/`, `enclosure/` — **Ben's**, read-only reference.
**Truth loop (core principle):** command → ghosted(pending) → ESP-NOW broadcast (idempotent, seq, 3×) → heartbeat echoes applied epoch+hash → solid(confirmed) ≤~1s → repair stale. The app can never silently disagree with the tree.

## 5. The 3-D-model pipeline (the keystone) — OQ-1
The entire cortex is gated on a model → data export:
- **Inputs (already on the Mac):** `~/Downloads/Tree_Rhino7.3dm` (Ed) · `Tree_Resonance_packed_2026-05-29.blend` (Mia).
- **Produced by blender-architect** (Addendum C §C3.2–C3.8): import-audit → Fixture Tools add-on → skeleton overlay → **`fixtures.json`** (+ glTF for the twin).
- **`fixtures.json` = the contract** between blender-architect (producer) and lighting-architect (consumer). Versioned; re-read on change.
- **Lumens as a calibration field:** per-fixture `lumens_max` + `beam_deg` + `role` + designed position + zone. The twin renders physical units; an IES profile from Vishnu's lantern makes shadows real; `lumens_max` maps brightness-% → PWM per fixture type and feeds the energy-truth math.

## 6. Integration surface (what the cortex must speak)
- **Ben's ESP-NOW mesh** — Protocol v1 messages: heartbeat (1Hz packed: SOC/V/I/temp/applied-epoch+hash/motion/faults) · param (binary ≤10Hz broadcast, idempotent, 3×) · state_doc (JSON sticky, epoch, reapply-on-rejoin) · cue/schedule (JSON) · event@T (clock-scheduled) · interaction · identify(blink) · time_sync(±10ms) · ota/blackout (SOC-gated / GPIO4). Channel-pinned APSTA. Honor Ben's bench traps (no broadcast ACK → heartbeat-echo repair; channel-lock; low-SOC drop; MAX17260 learn cycle).
- **Show Compiler (drone-show model, P1.5):** locking a schedule compiles per-fixture keyframe show-files (`[t, pattern_id, params, transition]`, ~4–16KB/fixture) referencing firmware pattern engines; distributed SOC-gated across the afternoon; at showtime master sends only time-sync → jitterless; total master failure mid-show is invisible.
- **MIDI console** (Addendum C): class-compliant USB controller → `mido` bridge → existing binary tweak path (1–3ms). Akai APC mini mk2 + Midi Fighter Twister. Bidirectional pad feedback = the truth loop in your fingertips.
- **Guest DJ / AI VJ** (Addendum C): USB line-in → two-layer brain (Layer 1 DSP reflexes incl. drop-detection; Layer 2 LLM VJ *arranges* the curated pattern vocabulary, never streams pixels; human override wins).
- **Sensing/occupancy** (Doc 5 / Addendum A5): PIR ×fixtures + mmWave sentinels + BME280 + chime-IMU wind. Occupancy heat-map / zone estimates / directional door-counting. Encode "quiet ≠ empty" (PIR can't see stillness).
- **Element modes:** wind-ride+dim · ember · pause-charge (heat) · rain cascade · BEACON lighthouse (whiteout safety) · amber storm-forecast.
- **Voice of the Tree** (P2): conch + DJI mic + shell exciter + local 8B LLM, night-only, passcode-gated; 3 tools (query_knowledge / save_story / control_lights). No raw audio.
- **Camp bridge:** Meshtastic LoRa (digests+signed commands) + BM Internet (journal→portal). Starlink rejected.

## 7. Build phases
**App:** C0 model pipeline (BLOCKED on OQ-1) → C1 twin + Monitor vs sim → C2 control plane on bench → C3 cues/schedule + commissioning → C4 polish + lite page.
**Brain:** B0 Jetson bench → B1 integration (PF-as-modem) → B2 7-night soak (**Aug 1 go/no-go; miss = ship brainstem-only**).
**First milestone for this agent:** C0/C1 — lock `fixtures.json` schema + round-trip; R3F twin scaffold loading glTF + `fixtures.json`; stub Protocol v1 against the WASM sim.

## 8. Contracts & interfaces (lock these early)
- **`fixtures.json` schema:** `{fixture_id, mac?, role(down/up/chandelier), designed_position[xyz], zone, lumens_max, beam_deg, led_type, ring/limb ref}`. Identity chain: factory MAC ↔ fixture_id ↔ designed position. Bind = tap→identify-blink→confirm→MAC bound.
- **Protocol v1** (§6) — the cortex↔mesh wire format; co-own with Ben.
- **Golden-frame parity:** the WASM sim render must be byte-identical to the real fixture render.

## 9. Constraints & guardrails
1. **Ben's 100 fixtures by Aug 20 is THE critical path. The cortex is additive and must never jeopardize it.**
2. **Control-plane only** — no pixel streaming over radio (architectural; honor Ben).
3. **DC-only** (no AC/inverters). **Offline-first** (no cloud on-playa). **Energy-truth** (SOC-gated, harvest-derived).
4. **Degrade gracefully, never brick** — the failure ladder (A full → B master-only → C fixtures finish on own clocks → D compiled ambient = 150 autonomous lanterns) is a requirement, not a nicety.
5. **Security by physics** — confidentiality is worthless; authority + self-healing (1Hz state beacon convergence ≤1s) are everything. Destructive ops (blackout/OTA) never ride open radio.
6. **Reliability-per-dollar wins already banked:** conformal-coat all boards (~$1) · Victron MPPT telemetry to the warden · brightness-envelope schedule layer.

## 10. Repos & access
- **Reads on boot:** Ben's repo (brainstem source-of-truth, per its AGENTS.md) + our repo (this PRD + the 5 docs + system docs + memory).
- **Writes:** our monorepo lane `apps/tree-lighting/` (we have push). Land via [MERGE-REQUEST]→qb→PR→Elliot. Never main, never `gh pr create`.
- **Contribute to Ben's repo:** `resonanceart` is **pull-only** on `beneckart/resonance-lighting` → fork to `resonanceart/resonance-lighting`, branch `elliots-controller`, PR to Ben (don't merge). Prefer a shared branch if Ben grants collaborator access.

## 11. Open decisions (need Elliot / team)
1. **Fixture count & ownership:** dossier = 150 (100 down + 24 up + 16 chandelier); Ben's repo scopes ~100 downlights only. **Who owns the 24 uplights + 16 chandelier?** Affects `fixtures.json` + power.
2. **Access path** to Ben's repo: fork+PR now vs request collaborator access.
3. **PIR-to-Bali window** (Addendum B B9 #1): per-fixture sensing may need to be in fixtures *before* they ship from Bali, or it slips to 2027. Hard near-term date.
4. **OQ-1 export owner:** who produces the first `fixtures.json` — blender-architect from Mia's .blend, or from Ed's Rhino model.

## 12. Success criteria (cortex acceptance)
- `fixtures.json` round-trips; twin renders the real fleet from it in physical units.
- Finger-to-photon <50ms measured (240fps camera, not vibes).
- Cue lock → schedule plays with cortex powered OFF (master-flash).
- Chaos drills pass: kill cortex mid-jam, kill master mid-show, plug-pull ×20, forced beacon — show degrades gracefully each time.
- The whole cortex can be removed and the tree is still 100+ beautiful autonomous solar lanterns.

## 13. Source index
Ben's repo (AGENTS.md read-order) · `~/Resonance Tree Lighting Controls/01-MASTER-DESIGN-REPORT-v1.0.md` · `02-ADDENDUM-A` · `03-ADDENDUM-B` · `04-ADDENDUM-C` · `05-RESEARCH-NOTES-6` · memory `project_lighting_two_visions_2026_06_13`.
