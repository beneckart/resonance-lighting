# How the Twin Maps & Mirrors the REAL Lights

*The full twin↔hardware understanding — what's simulated, what's real, how a
commanded look reaches a physical 4W RGBW LED, and how fixtures self-identify,
self-locate, and calibrate. Grounded in Ben's `firmware/ARCHITECTURE.md`,
`fixtures.json` (schema 0.3, 118 fixtures), and `protocol.ts`. Cycle 56.*

---

## 1. The fixture (what one real light IS)
- A **4W RGBW LED** (`led_type: "rgbw_4w"`, `lumens_max ~450`) buried INSIDE a
  bamboo **downlight tube** with a flared 15-strip skirt. The LED points down; light
  escapes through the **gaps between skirt strips** → the radial **petal gobo** on
  the ground. *The geometry IS the projection* — the gobo.png we project is the
  literal skirt-gap shadow (baked from the real lantern in Blender).
- We do **not** see the LED itself (it's in the tube). We see: the soft glow at the
  tube mouth + the cast beam + the petal pattern on the ground/surfaces.
- **118 fixtures** (schema 0.3): 78 canopy **downlights** (aim −Z, straight down),
  24 **uplights** (interior rings, aim +Z), 16 **chandelier** (crown cluster).

## 2. The brain on each fixture (ESP32)
Per `firmware/ARCHITECTURE.md`: each fixture runs an ESP32 with an on-device
**pattern + CA engine** and a **MAC-derived fixture ID**. It renders its OWN light
locally from control PARAMETERS — it is NOT fed a pixel stream. ESP-NOW carries
only: heartbeat/boot, fixture state, battery summary, neighbour RSSI, global mode
hints, wand/proximity events. `led_task` enforces brightness/current caps; the
mesh stays functional under partial packet loss (two-tier: mesh brainstem
autonomous, cortex optional + dies invisibly).

## 3. The control plane — params, NOT pixels (`protocol.ts`)
The twin/cortex broadcasts a **Protocol-v1 ShowFrame**, channel-pinned + epoched:
```
{ proto:1, channel, epoch, fixtures:[ {id, pattern, bri, hue, rgb?} ] }
```
Each fixture matches its `id`, runs `pattern` at `bri/hue` on-device. This is the
ADR 0004/0010 contract: a recipe per fixture, low-rate, broadcast — survives loss,
scales to 118 nodes, no per-pixel bandwidth. The **Show Compiler** (`showcompiler.ts`)
bakes a cue list into a deterministic keyframe timeline of these frames the cortex
replays without a browser.

## 4. How a commanded look reaches a real light
```
DJ/AI/LLM/touch UI → control (pattern,hue,bri,speed,…) in the twin store
  → [twin] litFor() renders the MIRROR (what we expect each fixture to show)
  → [bridge] encodeFixture()/buildShowFrame() → Protocol-v1 frame
  → PowerFeather-as-ESP-NOW-modem broadcasts on the pinned channel
  → each ESP32 (matched by id) runs the pattern locally on its 4W RGBW LED
  → fixture reports state back on the mesh (heartbeat)
  → [twin] truth-loop renders REPORTED state (not commanded) — the mirror rule
```

## 5. The MIRROR (truth loop)
The twin renders **reported** state, never blindly the commanded state. Today a
**mock heartbeat** simulates per-fixture reporting with jitter/latency + dead
fixtures (monitor view shows reporting/stale/dead counts). On hardware this swaps
to the real ESP-NOW heartbeat: the visualizer becomes a live mirror of the actual
tree — if a fixture is dead/stale/wrong, the twin shows it.

## 6. Self-IDENTIFY & self-LOCATE (Elliot's key question)
- **Self-identify: YES, automatic.** Each ESP32 derives a stable ID from its WiFi
  MAC (`compactIdFromMac` = last 3 MAC bytes) and announces it on boot. No DIP
  switches, no manual indexing.
- **Self-locate: NO — and this is the crux.** The hardware canNOT know its physical
  position. ESP-NOW **RSSI is approximate topology only, "not exact distance"**
  (Ben's note); there's no per-fixture GPS. So a MAC tells you *which node*, not
  *where it hangs in the tree*.
- **Therefore position is AUTHORED + COMMISSIONED**, exactly like Chromatik/LX
  (fixture positions live in the model file, never auto-sensed). Our model file is
  `fixtures.json` (the 118 Blender-true positions/roles/aims). Commissioning binds
  each physical MAC to a `fixtures.json` slot.

## 7. CALIBRATION / commissioning (built in — `calibration.ts`)
A one-time (re-runnable) pass that maps **MAC ⇄ fixtureId**:
1. Twin sends **IDENTIFY** (`identifyCommand`) → the target fixture **flashes**.
2. Installer sees which physical light blinks → taps the matching slot in the twin.
3. `assign(mac → fixtureId)` (strictly 1:1; re-assign replaces). Persisted
   (localStorage now; cortex-hosted on hardware).
4. `progress()` tracks % commissioned; `unassignedFixtures()` lists what's left.
- Protocol-v1 frames stay **fixtureId-addressed**; the cortex holds the calibration
  map to translate fixtureId ⇄ MAC on the wire. Re-running re-binds after a board
  swap. Unit-tested (`calibration.test.ts`).
- Future robustness: seed the map by RSSI-clustering (coarse topology) to pre-order
  the identify sweep, but the human tap remains ground truth.

## 8. What's ACCURATE vs APPROXIMATED in the twin
| Aspect | Twin | Real |
|---|---|---|
| Fixture positions / aim / role | ✅ exact (Blender `fixtures.json` 0.3) | same |
| Beam cone angle / falloff | ✅ from baked **IES** (`downlight.ies`) | same physics |
| Ground petal pattern | ✅ real baked **gobo.png** (skirt-gap shadow), per-fixture tinted cookies | same projection |
| Bark light-shaping shell | ✅ Plu Plu bark glb, near-opaque | same occlusion |
| Per-fixture colour/pattern | ✅ exact (same engine concept) | ESP32 runs it on-device |
| Volumetric beams in air | ⚠ additive cones (no haze sim) | needs haze/dust to be visible |
| Ground cookies | ⚠ textured quads (not real raytrace) | real light + shadow |
| Colour smoothing | ⚠ 110ms display slew | LED PWM response |
| Power/thermal/current caps | ❌ not simulated | `led_task` enforces real caps |

## 9. Open gaps needing real-hardware validation
- Real ESP-NOW radio bridge (E2) + cortex host (E1) — the modem that emits our frames.
- Power/solar budget per fixture at full-brightness (Ben's `power_bench`).
- Commissioning UI in the twin (the tap-to-assign flow) — `calibration.ts` is the
  engine; the panel is next.
- Real-song audio verify (B3) in a real browser.
- Uplight/chandelier (0.3 procedural first-pass) → refine positions in Blender.
