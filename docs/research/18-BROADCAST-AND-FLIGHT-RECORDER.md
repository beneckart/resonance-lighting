# 18 · Broadcasting commands + watching/recording the interactive tree

*2026-07-09 · lighting-architect · answers Elliot: "how is this controller
going to broadcast the commands?" and "how will it show what the tree is doing
and record it for bug logging and tracking in interactive mode?"*

## A · How commands reach the fleet (the broadcast path)

A browser cannot speak ESP-NOW — the path is already designed (bridge.ts,
verified 1:1 against Ben's net_bench.ino) and runs like this:

```
 iPad/laptop twin (this app)
      │  JSON lines over USB serial (Web Serial API; Safari fallback = a tiny
      │  local WebSocket daemon, same frames)
      ▼
 BRIDGE PowerFeather (Ben's master role, NB_FRAME_HZ=0 = pure bridge)
      │  packed ESP-NOW structs, channel-pinned broadcast (ADR-0004)
      ▼
 the fleet — every fixture hears the same ~250 B frame at radio airtime
```

What actually goes over the air (control plane ONLY, never pixels):
- **NB_SHOWFRAME** — param packets `{pattern_id, bri, hue, …}` (protocol.ts):
  the recipe; each fixture renders it locally.
- **Rules flash** — one compiled ≤240 B ruleset frame (doc 17): behavior that
  keeps running with the radio off.
- **NB_IDENTIFY / NB_SET_RATE / cal_%** — commissioning + self-map.

Three properties worth saying out loud to camp: (1) *one* broadcast frame
drives 118+ lights — no per-light session, no pairing; (2) the tree keeps
running its last rules if the controller walks away (cortex dies invisibly);
(3) uplink truth rides the 2 Hz heartbeats the fleet already sends — knowing
what the tree is doing costs zero extra radio duty.

## B · Watching what the tree is doing (interactive mode)

Already built, worth knowing where to look:
- **The twin IS the mirror** — it renders REPORTED state (sim heartbeat now,
  real heartbeats later through the same interface). What you see is what the
  fleet says it's doing, not what we hope it's doing.
- **📊 data log panel** — live per-light table (telemetry.ts, ~5 Hz): number,
  brightness, RGB, plus lit/total summary.
- **HUD** — fps · broadcast calls · N/118 lit; fleet monitor adds reporting/
  dead/stale counts (mock today, real heartbeats later).

## C · Recording it — the FLIGHT RECORDER (to build next)

Bug reports from interactive mode are currently "it looked wrong a minute
ago" — unreproducible. Plan: a black-box ring buffer in the twin.

**What it captures (per entry, compact):**
- `t` — session-relative ms
- **inputs**: every trigger (tap/walk/presence ping: fixture idx + rule
  snapshot), mode/rule/theme/speed changes, arm/disarm, show start/stop
- **outputs**: 2 Hz telemetry keyframes (per-light bri quantized to 16 levels
  + hue byte — ~250 B/keyframe ≈ 1.8 MB/hour, fine for a ring of 30 min)
- **engine marks**: GoL generation count, births/deaths per turn, watchdog
  reseeds, extinction events

**How it's used:**
1. **🐞 "Flag a bug" button** (always visible in interactive mode): freezes
   the last 120 s of the ring into a `bug-YYYYMMDD-HHMM.json` download +
   a note field. One tap at the moment something looks wrong.
2. **Replay in the twin**: load a bug file → the twin re-renders the recorded
   output timeline (and can re-fire the recorded inputs against the CURRENT
   engine to check "is it still wrong?"). Deterministic CA + recorded inputs
   ⇒ honest repro.
3. **Session summary** on disarm: triggers seen, generations run, reseeds,
   extinctions, min/max lit — pasted into the bug ledger.

**Why ring-buffer, not always-on logging:** Burning Man ops = an iPad with
finite storage and no ops person; 30 min of rolling memory + explicit flag
moments is the honest budget. (Same black-box pattern as aviation: record
always, persist on incident.)

Build queue: `flightrec.ts` (ring + capture hooks in store.triggerAt /
setUiMode / updateLife tick) → 🐞 button + download → replay driver (reuse
ShowPlayer's timeline pattern) → session summary card.
