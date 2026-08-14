# 19 · HOW THE SYSTEM TALKS TO THE LIGHTS, HOW THEY TALK BACK, AND EVERY STARTUP ORDER

*2026-08-13 · lighting-architect · written for the Nevada City integration (Justin on-site Fri 08-15).*
*Everything in here is verified against running code — cambium `bea2a2d`, our `elliots-controller`,
Ben's `main` `9312973` — not aspiration. The e2e suite (`app/src/cambium.e2e.test.ts`) and the
fakefleet rehearsal reproduce all of it with zero hardware.*

---

## 1 · DOWN: how the system talks to all the lights

```
 twin (browser)             cambium daemon              CoreS3 modem            fleet (≤130)
 ───────────────            ──────────────              ────────────            ─────────────
 render loop writes         normalize:                  Cambium binary-modem    each fixture:
 telemetry.states           id→MAC join (roster),       build (Ben's, primary   ESP-NOW RX →
 {id, rgb} per fixture      clamp, quantize 8-bit,      08-06); screen shows    render @10 Hz cap
      │ 5 Hz                white-extract per class     radio/serial health
      ▼                          │                           │                       ▲
 RealDriveDriver            packetize: type-25           COBS-framed bytes          │
 (📡 drive real toggle)     NB_DIRECT_FRAME, its OWN     over USB serial            │
      │                     monotonic wire seq               │                       │
      ▼  WS JSON :8600           ▼                           ▼                       │
 CambiumBridge  ────────►  daemon  ──────────────────►  CoreS3  ─── ESP-NOW ch 11 broadcast
                                                             (unencrypted, unacked)
```

Two kinds of talking, deliberately different:

- **Parameters (the doctrine path)** — `show {phase, hue, flags}`, `night`, `identify`,
  `set_rate`. Tiny broadcast frames; patterns run ON the fixtures. This is Ben's
  control-plane-only contract and it works with no daemon at all (fleet is autonomous).
- **Direct frames (the cambium path, type 25)** — per-fixture RGB at ≤8 Hz from the twin,
  daemon-paced to the fleet's 10 Hz render cap. This is Justin's proposal, adopted into
  Ben's firmware 08-06. It is a *lease*, not a takeover: the moment we go silent the
  fixture's own ladder takes back over.

**The failure ladder is the design, not an accident:** frame flow stops → fixture holds
last colors ~1 s → crossfades to autonomous pattern by 3 s → **never blank**. Closing the
laptop mid-show is a graceful act. We verified the arm side (blackout propagates) and
cambium's fakefleet reproduces the fallback side.

**Identity join:** the twin speaks `fixture_id` ("F000"); the roster (built from OUR
`fixtures.json`) joins it to the 3-byte MAC every radio packet uses. Unknown ids never
black out the rest — they come back as an `err` listing, which the FleetPanel surfaces.

## 2 · UP: how the lights talk back

```
 fixture ──2 Hz±30% heartbeat──► CoreS3 ──serial──► daemon fleetstate ──WS──► twin
```

- **Heartbeat (2 Hz, jittered):** `batt_mv, batt_ma (signed: − = discharging), soc_pct,
  dl_rssi, mode, life_state, program, power_tier` + per-packet `rssi`. State rides the
  heartbeat — knowing the whole fleet costs ZERO extra radio duty. Short beats omit
  tail fields; the daemon keeps last-known instead of flapping to null.
- **Choreo edges (`evt`):** the moment a fixture's local state flips, one tiny frame —
  instant, no polling. First sighting counts as an edge.
- **Charging census:** daemon counts nodes with `batt_ma > 0` and pushes `charging
  {count, macs}` on change. **This feeds `store.solarPanelsCharging()` — the Solar Ray
  show's handoff trigger now fires from REAL panels** (the sim edge remains for demo).
- **Bridge health:** `bridge_status` (serial/channel/txOk/txFail/fleetHeard) — transport
  meta, surfaced in the FleetPanel note line, never mixed into fleet state.

In the app all of it lands behind the same `BridgeLink` seam as the mock and USB
transports: `hb`/`evt` flow to the MAC registry/FleetPanel; meta stays on a side channel.

## 3 · Startup protocols — the full matrix, each cell tested

| # | order / event | what happens | verified by |
|---|---|---|---|
| A | daemon → app (happy) | connect, drive on, frames flow | live e2e 4/4 + causal blackout |
| B | **app → daemon (late daemon)** | first connect fails FAST, toggle disarms, **no zombie retry loop** (retry only fights for a once-established session) | unit test "startup case B" — bug found & fixed 08-13 |
| C | daemon dies mid-show, returns | backoff redial 0.5→8 s; on reopen RealDriveDriver **re-arms drive automatically**; fleet rode its autonomy ladder meanwhile | reconnect unit test (open→close→open) |
| D | fleet boots DAY-gated (ignores frames) | the #1 bench trap: **🌙 night on button** in FleetPanel (no CLI memory needed); `cambium doctor` names the fix too | NightDown wire test; gate rehearsed in fakefleet |
| E | app up before fixtures load | pump sends nothing on an empty scene (no empty-frame spam) | pump unit test |
| F | wrong fixtures file (id mismatch) | daemon answers `err` with the unknown ids; surfaced in FleetPanel; rest of tree unaffected | live e2e (err channel proven both ways) |
| G | page reload mid-drive | `driveReal` is NOT persisted → reboots disarmed, no surprise auto-drive; fleet fell back gracefully ~3 s in | store audit (no persist on `net`) |
| H | seq after either side restarts | daemon stamps its OWN wire seq (starts 1, monotonic); browser seq is informational — reload/daemon-restart cannot strand frames as "stale" | cambium `packetize.py` read |

## 4 · The runbooks

**Bench / Nevada City (Justin, Friday):**
1. Plug CoreS3 (Cambium binary-modem build, **channel 11** — same as fleet, or nothing hears).
2. `cambium serve` (real) — or `cambium fakefleet run --fixtures app/public/fixtures.json --start-night` (rehearsal).
3. `cambium doctor --daemon http://localhost:8600 --listen 2` → expect `READY`; it names its own fixes.
4. App: `npm run dev` → open `http://localhost:5173/?cambium=ws://localhost:8600/ws`.
5. If doctor warned DAY: tap **🌙 night on** (or `cambium night on`).
6. Toggle **📡 drive real** → the tree follows the twin. Kill anything, in any order: the fleet never blanks.

**iPad / phone on the LAN:** `npm run dev -- --host` (http, avoids the https→ws mixed-content
block), open `http://<laptop-ip>:5173/?cambium=ws://<laptop-ip>:8600/ws`. Same toggle.

**Branch truth:** everything above lives on `elliots-controller` (Justin's
`cambium-ws-bridge` is merged; his quickstart's branch pointer is stale — pull ours).

## 5 · Open, honestly

- fixtures.json is 118 (06-13 export) vs ADR-0032's 130 w/ perimeter class — re-export
  requested from the Blender side (handoff `dbba3930`); cambium's roster inherits it free.
- `bridge_status` fields render as a note line only — a proper health strip is unbuilt.
- The RGBW **white channel** is extracted daemon-side per fixture class; the twin renders
  RGB only, so pure-white looks slightly different on hardware than on screen. Known,
  accepted for launch.
