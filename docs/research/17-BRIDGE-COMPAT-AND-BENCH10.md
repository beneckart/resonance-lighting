# 17 · TWO-WAY BRIDGE, COMPATIBILITY REVIEW & THE 10-LIGHT BENCH TEST

*2026-07-05 · lighting-architect · answers Elliot's bridge/battery/bench questions.*
*Code: `app/src/bridge.ts` (seam + mock + Web Serial) · `app/src/macregistry.ts`
(MAC ledger) · `app/src/FleetPanel.tsx` (console) · `public/fixtures-bench10.json`.*
*Companion: `16-SYNC-CALIBRATION-PROTOCOL.md` (the staged self-mapping).*

---

## 1 · Compatibility review — is the controller compatible with Ben's design?

I read the live upstream (`firmware/ARCHITECTURE.md`, `net_bench/net_bench.ino`,
`NETWORKING_FEASIBILITY_5NODE_2026-06-07.md`, POWERFEATHER notes). Verdict:
**yes — and better than expected: most of what we need already exists.**

| controller need | Ben's design already has | gap |
|---|---|---|
| unique per-light identity | `NbHeader.src_id[3]` — 3-byte compact MAC in EVERY packet | none — our calibration map keys on exactly this |
| know fleet state cheaply | `NbHeartbeat` @ **2 Hz ±30% jitter** already carries `ca_state`, `mode`, `batt_mv/ma/soc`, `supply_*`, `dl_rssi`, `dl_pdr`, `reset_reason` | none — state rides the heartbeat |
| the bridge | master role **bridges peer stats to the host**; `NB_FRAME_HZ=0` = pure bridge/mesh node | transport: net_bench bridges over WiFi UDP:54321; on the playa there's no AP → **USB serial bridge sketch needed** (below) |
| find a physical light | `NB_IDENTIFY` (locate-blink) exists | none |
| battery-conserving rates | `NB_SET_RATE` (heartbeat/frame Hz) exists | none |
| params-not-pixels | `NbShowFrame {phase, hue, flags}` broadcast | our ShowFrame is richer (pattern id, per-group) — grows via append-only versioning |
| survey (RSSI tables) | heartbeat carries only `dl_rssi` (RSSI of MASTER frames) | **NB_CAL_RSSI proposal**: per-neighbor top-16 table during a survey session (doc 16 §6) |
| instant edge events | nothing edge-triggered today (2 Hz bound = ≤ ~650 ms staleness) | **NB_STATE_EVT proposal**: tiny frame on local state change |
| seq/dedup/reboot detection | `seq` + `uptime_ms` in every header | none — ledger uses both (PDR + reboot counting) |
| packet hygiene | packed little-endian, versioned, **append-only fields**, jittered sends, "keep lighting functional under loss" | our foldSession/registry already treat the wire as untrusted + lossy |

Two constraints we must keep honoring (ADR 0004/0010): ESP-NOW carries only
small state/control metadata (never firmware, never pixels), and **the channel
is pinned fleet-wide** — the bridge must be built with the same `--channel` as
the fleet, or nothing hears anything (validated failure mode in net_bench).

## 2 · The two-way bridge (controller ⇄ fleet)

```
  twin/controller (iPad/laptop, Chrome)
        │  USB CDC serial, JSON lines @115200      ← SerialBridge (app/src/bridge.ts)
  bridge PowerFeather (master role, NB_FRAME_HZ=0, channel-pinned)
        │  ESP-NOW broadcast, packed Nb* structs   ← Ben's existing packet codec
  fleet (peers: rules run LOCALLY; heartbeat @2 Hz; instant events on edges)
```

- **Downlink** (controller→fleet): show params, identify, set_rate, cal frames.
  The bridge unpacks a JSON line and emits the packed struct.
- **Uplink** (fleet→controller): every heartbeat/event the bridge hears becomes
  one JSON line. The twin's registry ingests them; the mirror renders REPORTED
  state — the always-truthful-mirror contract from day one.
- The seam is `BridgeLink` — the **MockBridge** (sim fleet) and **SerialBridge**
  (real port) implement the same interface, so the whole console works today
  and the hardware swap changes zero downstream code.
- **Firmware ask (small)**: a `serial_bridge` sketch = net_bench master with
  the UDP sender swapped for `Serial.println(json)` + a line-reader for
  downlink. ~an afternoon on top of Ben's existing code.

### "Instantly respond when they change state"

Two paths, both live in the Fleet panel:
1. **Heartbeat path (exists today):** worst-case staleness ≈ period + jitter ≈
   **650 ms at 2 Hz**. Zero new radio duty.
2. **Event path (proposed NB_STATE_EVT):** the node transmits one tiny frame
   the moment its local state flips (tap/presence/mode/fault). ESP-NOW airtime
   is sub-ms; serial adds ~1 ms; the twin reacts in **< 50 ms**. Measured in
   the sim console: tap→twin 0 ms.

### "Know their state without draining the battery"

The honest power story, from Ben's bench data:
- The ESP-NOW **receiver is always on anyway** — that's the price of being
  commandable at all, and it's already inside the validated power budget (the
  2026-07-05 soak: **46 h continuous on a 7.2 Ah LFP at 209 mA mean**, radio
  on, heartbeats flowing).
- **TX is the marginal cost, and it's tiny**: a heartbeat is ~50 B ≈ sub-ms of
  airtime; at 2 Hz that's duty measured in **parts per thousand**.
- Rules run on the fixture (control-plane contract), so knowing state costs
  nothing extra: it **rides the heartbeat that already exists**.
- Conservation lever when it matters: `NB_SET_RATE` drops heartbeats to
  0.2 Hz (the panel has the knob); the proposed edge events keep the twin
  instant even at low heartbeat rates. Report-by-exception, cheaply.
- What we do NOT do: poll. Nothing in this design ever asks a fixture a
  question on a timer.

### MAC mapping & logging

`macregistry.ts` is the permanent ledger: every MAC ever heard, first/last
seen, heartbeat count, seq-gap loss (→ per-node uplink PDR), reboot count
(uptime regressions), battery/state snapshot, online/offline transitions, a
capped event log — persisted across sessions, exportable as CSV per test/
install day. The calibration map (who hangs WHERE) stays separate and
version-stamped; the registry is who EXISTS and how it's doing.

### Manual adjustments

Click any light's slot in the Fleet ledger → pick the true slot → stored as
`confirmed/manual` in the calibration map. The self-map solver treats every
installer-confirmed entry as an **anchor** on its next run — human truth
always outranks the solver. (Verified live end-to-end.)

## 3 · The 10-light bench test (first hardware, not on the tree)

**Layout** (`fixtures-bench10.json`, loadable in the Fleet panel): 8 downlights
clipped to a **14 m rope/ridge line at 2.2–3.2 m height** (three height steps),
plus 2 uplights on the ground at the ends. Deliberate properties:
- **A line, not a ring** → no rotational ambiguity; the embedding's job is 1-D
  ordering, the cleanest first hardware test of RSSI topology.
- **Three height steps within VL53 ToF range (≤ 4 m!)** → every lantern gets a
  real ground return; validates the vertical stage at bench scale (on the tree,
  canopy heights exceed ToF range — bench is where ToF gets proven).
- **2 m spacing** → matches the tightest same-role spacing on the real tree
  (chandelier ≈ 1.9 m); if the solver separates 2 m neighbors on the bench, we
  know the resolution limit before install day.

**The run sheet — each step de-risks one layer of the stack:**

| # | step | validates | pass looks like |
|---|---|---|---|
| 0 | flash 10 peers + 1 bridge, same `--channel`; plug bridge into laptop | build/channel discipline | 10 MACs appear in the Fleet ledger unprompted |
| 1 | let it sit 10 min | heartbeat plumbing + ledger | 10/10 online, PDR ≥ 99%, battery sane, zero reboots |
| 2 | `identify` each row from the panel | downlink + MAC↔physical | the right lantern blinks, every time |
| 3 | survey session (raise HB rate via set_rate, collect RSSI tables) | NB_CAL_RSSI (new fw) | each node hears ≥ 8 peers; medians stable ±2 dB |
| 4 | solve with 0 anchors, then confirm the queue | the full doc-16 ladder at n=10 | ≤ 3 manual confirms to 10/10 correct |
| 5 | physically SWAP two adjacent lanterns, re-survey, re-solve | drift detection + re-map | solver flags/reassigns the swapped pair |
| 6 | tap/wave at one lantern (presence) | instant event path (NB_STATE_EVT) | its row flashes < 1 s later; latency logged |
| 7 | set_rate 0.2 Hz, run 1 h | conservation mode | ledger stays truthful; events still instant |
| 8 | photogrammetry dry run: AutoCal solo-step, phone video | stage-5 capture flow | each solo window isolates one light on camera |
| 9 | export CSV + lock map (hash) | the paper trail | hash recomputes identically on reconnect |

Step 5 is the one that makes install day safe: it rehearses exactly the "a
repair swapped two units" failure the tree WILL have.

**What the bench CAN'T tell us** (be honest, plan tree-scale checks): real
canopy RSSI obstruction (bamboo/bodies cost 35–40 dB in T4), ToF-beyond-4 m
behavior, and 118-node airtime contention (net_bench showed clean headroom to
50 Hz aggregate, so 118 × 2 Hz ≈ 236 fps aggregate needs a scale test — or a
lower default HB rate fleet-wide).

## 4 · Asks for Ben (all small, all append-only)

1. `serial_bridge` variant of the master role: UDP sender → USB serial JSON
   lines, line-reader downlink. (The controller side is already built.)
2. `NB_STATE_EVT` (new NbType): edge-triggered state report.
3. `NB_CAL_RSSI` / `NB_CAL_TOF` / `NB_CAL_ASSIGN` / `NB_CAL_LOCK` (doc 16 §4),
   with the top-16-neighbor cap so tables fit one ESP-NOW frame.
4. A word on preferred fleet-wide default HB rate at 100+ nodes (2 Hz bench
   default vs airtime at scale).
