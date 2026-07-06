# 18 · DEEP DIVE — improving the controller, field realities, rules-to-fleet

*2026-07-05 · lighting-architect · Elliot's audit ask: improvements, likely
issues, tablet reality, plug-in controllers, sensors, fleet rule-writing, bugs.*
*Shipped alongside this doc: the Fleet Rules engine (`rules.ts`, `RulesPanel`),
bug fixes listed in §6.*

---

## 1 · Running on a TABLET — the real story

The app is already a **PWA** (installable, offline-cached — `vite-plugin-pwa`
precaches the model, fixtures, audio). "Add to Home Screen" on an iPad gives a
full-screen controller with no server needed after first load. But three
browser APIs the desktop flow uses **do not exist on iPadOS** (any browser —
they're all Safari underneath):

| capability | desktop Chrome | iPad | consequence |
|---|---|---|---|
| Web Serial (USB bridge) | ✅ | ❌ | can't plug the bridge PowerFeather into an iPad |
| WebMIDI (DJ controller) | ✅ | ❌ | physical MIDI decks are desktop-only |
| Web Bluetooth | ✅ | ❌ | BLE bridge not an option either |

**The tablet answer: the bridge becomes the access point.** The bridge
PowerFeather runs a soft-AP **on the fleet's pinned channel** (channel
constraint satisfied by construction — the AP and ESP-NOW share the radio),
serves a WebSocket, and speaks the *same JSON-lines protocol* we already
defined. The iPad joins "ResonanceBridge" WiFi and the twin connects to
`ws://192.168.4.1`. Code impact: one more `BridgeLink` implementation
(`WsBridge` — ~40 lines, mirrors `SerialBridge`); zero changes downstream.
This is also the *multi-operator* path: two iPads can hold the same WebSocket.

Recommended setup: **laptop + USB bridge for bench/commissioning** (Web Serial
today), **iPad + WS bridge for show operation** (needs the AP sketch — added
to Ben's ask list). Touch ergonomics are already handled (TouchConsole 375px+,
dock layout); remaining tablet nits: hover-only `title` hints get lost (add
long-press labels), and `<select>` popovers are fine on iPadOS.

## 2 · Plug-in controllers & sensors

- **MIDI decks** (already integrated, `midi.ts`): desktop-only per above. For
  iPad shows, the TouchConsole is the controller; a future option is a small
  desktop "MIDI relay" that forwards deck input over the same WebSocket.
- **Fleet sensors we consume** (per node, in the heartbeat): battery/soc,
  solar supply, ToF presence, RSSI. The rules engine (§4) turns these into
  behavior *without* the cortex in the loop.
- **Controller-side sensors**: the twin's mic (audio-reactive modes) works on
  iPad after one tap (autoplay policy — needs a "tap to arm audio" affordance,
  listed in §5).
- **Wand** (Ben's BACKGROUND idea): arrives as just another mesh event
  (`evt: tap/proximity`) — the Fleet panel's event feed and the rules engine
  both consume it with no new plumbing.

## 3 · Where we'll have issues (ranked, with mitigations)

1. **Radio at 118 nodes** — 2 Hz × 118 ≈ 236 heartbeats/s aggregate. net_bench
   validated ~50 Hz aggregate cleanly on 5 nodes; airtime says fine (sub-ms
   frames) but collision behavior at 118 senders needs the scale test. Default
   the fleet to 0.5 Hz + instant events; keep 2 Hz for bench/commissioning
   (both already knobs).
2. **Channel pinning is a footgun** — one board built with the wrong
   `--channel` is invisible and "broken". Mitigation: the ledger's
   first-heard/never-heard view makes the symptom obvious (a flashed-but-mute
   MAC never registers); add channel to the bench run sheet step 0.
3. **RSSI table size** — full-fleet tables don't fit a frame; top-16 cap
   (doc 16 §6) must be in the firmware from day one.
4. **Coincident fixtures** (real export has some) — unresolvable by ranging,
   by design surfaced at the bottom of the confidence queue; photogrammetry or
   a human decides. Also: two entries in the model at the SAME position may be
   a Blender export artifact — worth checking with the placement workflow.
5. **localStorage as the only persistence** — registry, calibration map, rule
   drafts all live in one browser profile. An iPad Safari data-clear loses the
   install map. Mitigation now: CSV/JSON export buttons (registry has CSV; add
   map export/import). Real fix: the bridge PowerFeather carries an SD/flash
   copy of the locked map — the TREE holds its own truth (each node already
   stores its own slot; a fresh controller can rebuild the map by asking).
6. **Sim-to-hardware drift** — MockBridge is a model, not hardware. Every
   claim it makes (latency, PDR) gets re-measured in bench-10 steps 1–7 before
   we trust it.
7. **Multi-operator conflicts** — two controllers both broadcasting shows =
   last-writer-wins flicker. Near-term: one bridge = one operator by
   convention; the rule epoch counter (below) at least makes "whose rules won"
   visible.

## 4 · Scripts & rules for the fleet (SHIPPED this session)

The ask: *"write script and rules to give to the fleet, flashing the updates
on how we want them to behave in different conditions and modes."*

**How it works** (`rules.ts` + 📜 Fleet Rules panel):
- A rule program is text — one rule per line, first match wins, last line is
  the default:
  ```
  when hour >= 1 and hour < 6 and presence = 0 -> pattern=ember bri=30
  when soc < 20 -> pattern=ember bri=25
  when presence > 0 -> pattern=ripple bri=255 speed=3
  -> pattern=breathe bri=140
  ```
- Sensors: `hour · soc · presence · sound · supply · mode` — each node
  evaluates against its OWN battery and ToF; behavior needs no radio.
- The compiler packs the program into **≤ 240 bytes — one ESP-NOW broadcast
  reprograms the whole fleet** (enforced; the night-saver preset is 44 B).
  Rules are DATA in node flash, never firmware (ADR-0010-clean) — "flashing
  the updates" is a broadcast + flash write, not an OTA.
- Epoch counter versions every push; nodes ack; heartbeats carry the pattern
  the rule engine chose, so the ledger SHOWS the program running.
- Verified live: flash night-saver → 118 nodes settle on `breathe`; slide
  presence up → the entire fleet flips to `ripple` within one tick, announced
  by instant events.
- Firmware ask: `NB_RULESET` NbType + a ~60-line C evaluator (the TS
  `evalRules` is written to be transliterated 1:1).

**Where this can go next**: per-group rule programs (crown vs rings), a
`rand` sensor for organic variation, time-blended transitions between rule
actions (fade, not snap), and rule simulation against a recorded sensor day.

## 5 · Improvement backlog (prioritized)

1. `WsBridge` + bridge-AP sketch → iPad operation (§1). **Biggest unlock.**
2. Calibration-map JSON export/import buttons (backup + share between devices).
3. "Tap to arm audio" affordance for iPad mic modes.
4. Undo for manual re-slot (one-level is enough — installers fat-finger).
5. Slot-conflict surfacing: if a manual re-slot displaces another MAC's
   assignment (the map keeps 1:1 silently), show WHO got bumped.
6. Rule transitions (fade between actions) + per-group programs.
7. Registry retention policy (prune MACs unheard for 30+ days, keep CSV).
8. Long-press tooltips for touch (identify/tap buttons rely on hover titles).

## 6 · Bugs found & fixed this session

| bug | fix |
|---|---|
| SelfMapPanel `busy` flag stuck forever if the solver throws (panel freezes at "solving…") | try/finally around both async blocks |
| FleetPanel `flash` map grew unbounded (one entry per event, never pruned) | pruned on each sweep tick |
| Fleet ledger showed a stale calibration map after SelfMap solves (needed reconnect) | fresh map read per render (2 Hz, cheap) |
| Ledger table clipped on narrow screens | `overflow-x` on the table container |
| Registry timestamps used `performance.now()` → nonsense across reloads (108 ghost-"online" nodes) | wall-clock `Date.now()` for ledger, `performance.now()` only for latency |
| RulesPanel preset buttons missing React keys (console warning) | keyed wrapper |
| Event feed showed raw epoch-seconds after the wall-clock switch | time-of-day format |
| (design) per-row `<select>` with 118 options × 118 rows tanked rendering | single click-to-edit control |

Known non-issue: headless-browser fps readings (~2 fps) are rAF throttling in
the test harness, not app performance — flagged so nobody chases it later.
