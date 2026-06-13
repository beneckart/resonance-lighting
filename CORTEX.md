# Resonance Tree — Lighting Cortex (controller / show system)

This branch (`elliots-controller`) adds the **optional "cortex" layer** on top of Ben's
autonomous solar-mesh fixtures (the "brainstem", on `main` / upstream `beneckart/resonance-lighting`).

**It rides Ben's architecture — it never replaces it.** Control-plane only over ESP-NOW,
never pixels; the cortex "wakes at dusk and dies invisibly" — the lights run without it.

## What lives here
- `app/`   — React-Three-Fiber PWA: the hardware-true digital twin + console (jam → lock cue → schedule), installed on iPad, offline-first.
- `cortex/` — Python services for the Jetson hub: twin-server, occupancy, env, voice, camp bridge. PowerFeather master = ESP-NOW USB radio-modem.
- `sim/`   — firmware pattern core compiled C++ → WASM (golden-frame parity with real fixtures; develop without hardware).
- `docs/research/` — the design corpus: **PRD-lighting-environment.md** (the job) + the 5-doc dossier (`01…05-*.md`) + Ben's existing research.

## Owned by
`lighting-architect` agent (boot: `boot-lighting`). Consumes `fixtures.json` from the
Blender placement workflow; never places fixtures itself. Coordinates with Ben via this
repo (LOG/PR) and upstream `beneckart/resonance-lighting`.

## Remotes
- `origin`   = `resonanceart/resonance-lighting` (our fork — push here)
- `upstream` = `beneckart/resonance-lighting` (Ben's live repo — read-only; PR here once collaborator access lands, see upstream issue #1)

## Keystone dependency
The whole cortex is gated on **OQ-1**: the 3-D model → `fixtures.json` + glTF export.
First task once that lands: lock the `fixtures.json` schema + round-trip, scaffold the
R3F twin viewer, stub Protocol v1 against the WASM sim. See `docs/research/PRD-lighting-environment.md` §5–8.
