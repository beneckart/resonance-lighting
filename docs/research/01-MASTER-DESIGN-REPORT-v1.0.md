# DOCUMENT 1 of 5 — MASTER DESIGN REPORT v1.0

## PART 0 — EXECUTIVE SUMMARY
The Resonance Tree carries **150 solar-autonomous bamboo light fixtures** (100 downlights, 24 uplights, 16 chandelier — count to reconcile) on a 7.5 m bamboo pavilion at Burning Man 2026. Each fixture is a **PowerFeather V2 (ESP32-S3)** driving SK6812 LEDs, joined by an **ESP-NOW mesh carrying control parameters only — never pixels**. Patterns run on the fixtures themselves.

The interactive system above the mesh: (1) **Controller/visualizer** — iPad PWA (React Three Fiber), live 3D twin, console workflow (jam → lock cue → schedule), two-path control plane for <50 ms finger-to-photon. (2) **Central brain ("cortex")** — Jetson Orin Nano Super at the trunk base; voice, occupancy, show hosting, logging; PowerFeather master as its ESP-NOW radio modem. Mesh stays an autonomous **brainstem**; cortex wakes at dusk, its death invisible to the lights. (3) **Sensing** — per-fixture PIR on all 150, crown weather suite, mmWave sentinels. (4) **Element modes** — wind-ride, ember, rain, amber forecast, beacon (whiteout → lighthouse). (5) **Voice of the Tree** — conch + DJI mic + shell-exciter speaker, local 8B LLM, passcode-gated. (6) **Camp bridge** — Meshtastic LoRa + BM Internet.

## PART 1 — PRINCIPLES & TOPOLOGY
Binding principles: two-tier brain (brainstem complete & autonomous; cortex adds night cognition, never required); control plane only (no pixel streams); reflex local, cognition central; energy truth (SOC-gated, harvest-derived); truth loop (twin renders *reported* state); serviceability over elegance (brain at trunk base); degrade gracefully, never brick.
Topology: HAT at the crown (shared solar, LiFePO4+MPPT, BME280, anemometer/chime-IMUs, mmWave sentinel, WiFi+LoRa antennas, 12V bus to 16 chandelier lights) → one trunk cable (12V+USB+audio) → BASE BOX at trunk (Jetson cortex, PowerFeather master as ESP-NOW USB modem, warden for rails/SOC/temp, Meshtastic, optional BM-Internet radio, DJI RX, amp) → ESP-NOW mesh to 150 fixtures. Camp at 0.5–3 km via LoRa; portal via BM Internet. iPad joins cortex AP at night / master SoftAP fallback.

## PART 2 — PRD
**P0:** iPad PWA 3D twin, live heartbeat states (60fps) · two-path control (<50ms, cues survive reboot) · master brightness + zone/global control · cue lock + editable schedule mirrored to master flash (plays cortex-off) · commissioning tap→blink→bind (150 in ≤1 day, 2 people) · Monitor screen · fleet blackout (GPIO4) + SOC-gated OTA · ~150KB lite fallback page.
**P1:** cortex hub · per-fixture PIR + occupancy heat map · directional door counters · weather suite + beacon · energy-truthful + firefly modes · camp bridge Tier 1 · music sync · client-scoped tokens.
**P2 (off critical path):** Voice 2026 pilot · heart-adoption · esp-csi spike · touch lanterns · BM portal sync · public twin.
**Non-goals:** no pixel streaming over radio, no cloud on-playa, no raw audio retention, no cameras, no Starlink, no Mac mini/AC inverters, no per-fixture mmWave.

## PART 3 — CONTROLLER / VISUALIZER
R3F PWA, installed on the iPad (full offline). Master flash serves only WS + fixtures.json + OTA + lite page; cortex serves the full app at night. Screens: Jam, Cues, Schedule, Monitor, Commission, Twin.
Latency: tweak path = touch → coalesced binary frames ~20Hz over WS → one ESP-NOW broadcast (idempotent, seq, 3×); local echo immediate. <50 ms finger-to-photon. Commit path = JSON desired-state docs (sticky, epoch, reapply-on-rejoin, flash-mirrored). Upgrade if jitter: WebRTC unreliable channel.
Music sync: Web Audio — envelope-follow (classical) + beat mode; events scheduled ahead on synced clocks.
Truth loop: command → ghosted (pending) → broadcast → heartbeat echoes applied epoch+hash → solid (confirmed) ≤~1s → repair stale. The app can't silently disagree with the tree.
Identity: factory MAC ↔ fixture_id ↔ designed position; registry = fixtures.json from Grasshopper (designed data, not surveyed). Bind = tap → identify-blink → confirm → MAC bound.
Model pipeline: Rhino 8 native glTF + Grasshopper fixtures.json; gltfpack compression. Sim: firmware core C++ → WASM (zero drift). OQ-1 blocks C0.

## PART 4 — PROTOCOL v1
Transports: WS/WiFi · framed USB serial (cortex↔master) · ESP-NOW broadcast+unicast (channel-pinned, APSTA) · LoRa · BM Internet HTTPS.
Messages: heartbeat (1Hz packed: SOC, V/I, temp, applied epoch+hash echo, motion, faults; PDR ≥97% bench) · param (binary, ≤10Hz broadcast, idempotent, 3×) · state_doc (JSON sticky, epoch, reapply) · cue/schedule (JSON) · event@T (clock-scheduled) · interaction (PIR/touch/wand/heart) · identify (blink) · time_sync (±10ms) · ota/blackout (SOC-gated/GPIO4).
Traps (Ben's bench): no broadcast ACK → 3× + heartbeat-echo repair; channel-lock mismatch silently kills sends → pin everywhere; low-SOC peers drop on WiFi association; MAX17260 needs DesignCap + learn cycle or SOC lies; GPIO4 = free blackout.
Auth: guest / operator / voice (15-min passcode elevation, hashed at master, never in LLM prompt) / camp (HMAC + counter).

## PART 5 — CENTRAL BRAIN
Jetson Orin Nano Super 8GB ($249), 67 TOPS, fed directly from 12V LiFePO4 (9–20V input — bench-verify; fused 7.5A). 1TB NVMe; M.2 WiFi (night AP). Upgrade: Orin NX 16GB module (~$699, NVIDIA silicon; Nano carrier accepts it). Hot spare: 16GB Android flagship (~$400). Ben @ NVIDIA: ask discount/eval.
Services: voice, twin-server, occupancy, env, bridge, db (SQLite+sqlite-vec). Pull-plug ×20 acceptance gate.
Radio plane: PowerFeather master = ESP-NOW modem over USB serial (Ben's T7, validated). Day/cortex-dead = master SoftAP + lite page.
Energy (→ Ben): night ~85 (quiet) / ~115 (typical) / ~170 (heavy) / ~250 Wh (envelope). Day ~1 Wh. Interlocks: no cortex power-on >90°F box; no charge >45°C cell.
Enclosures (pre-designed): Pelican 1450 / Apache 3800 + snap-in cabinet filter fans + IP68 glands; or NEMA 4X polycarbonate; or NX in Seeed reComputer aluminum case. MERV-13, parts ≥150°F, white shell.
Failure ladder: cortex dies → SoftAP + mesh + lite page (schedule plays from master flash); master dies → promote a fixture; LoRa dies → tree fine; battery low → shed BM radio → Voice → chandelier → cortex; sensor dies → schedule fallback.

## PART 6 — SENSING & OCCUPANCY
AM312 PIR ×150 (~$1, ~15µA — free; mmWave on 150 would out-draw the LEDs), recessed in the skirt down the beam. mMWave sentinels (LD2410) at 2–4 chokepoints. BME280 (baro = storm early-warning). Wind via chime IMUs (sway IS the reading).
Occupancy: heat map (150 cells on coords + dwell) · zone estimates (clustering, ±30% gauge) · directional door counting (paired PIRs, interior count, self-zeros at dawn) · fusion roadmap (PIR→mmWave→esp-csi). **PIR can't see a motionless person** — door counters + sentinels carry stillness; "quiet ≠ empty." Feeds crowd-adaptive patterns, beacon awareness, Voice lines, nightly analytics.

## PART 7 — ELEMENT MODES
Gusts → wind-ride + 20% dim · cold → ember · heat → pause charging · rain → silver-blue cascade · pressure drop + wind → BEACON lighthouse · falling baro → amber forecast.

## PART 8 — VOICE OF THE TREE
Conch + DJI TX in shell (RX → Jetson USB) + Dayton surface exciter (the conch IS the speaker, ~$25). Night-only. Stack: VAD → Whisper distil → 8B LLM → streaming TTS, <2s, all local. Three tools: query_knowledge (RAG over portal snapshot) · save_story (append-only — writes stories, never reads others back) · control_lights (passcode = 15-min token at master). No raw audio ever. October "letter from the tree" typed on the lite page. Gate: 7 boring-stable nights by Aug 1.

## PART 9 — CAMP BRIDGE
Tier 1 Meshtastic LoRa (~$35/node, <1W, km-range): 10-min digests + alerts out; signed commands in. Kilobits only. Extra: crew texting, GPS-in-container, weather broadcast, emergency channel, off-season link, 2027 meshing. Tier 2 BM Internet (8.5W): journal + analytics → portal + Living Memory. Starlink rejected.

## PART 10 — PRIOR-ART BASIS
Console (fader/cue split) · xLights→FPP (lock-then-schedule) · drone shows (params + synced clocks, GCS Monitor) · Firestorm (sticky reapply-on-rejoin) · WLED (ESP-NOW sync, 3×) · Ténéré/Titanic's End (scale, wired path not ours) · Entwined/Canopy (adoption UX) · Lozano-Hemmer Pulse (heart-per-light queue, phone PPG) · MIRA (all-local LLM, MERV-13, 150°F, night-only) · esp-csi · CrowdClock/Mirollo-Strogatz (firefly sync).

## PART 11 — BOM (battery/solar → Ben)
Jetson+NVMe+WiFi ~$360 · 150 PIR ~$200 · mmWave ~$20 · weather ~$50 · 2 Meshtastic ~$90 · enclosures/fans ~$250 · cable/fusing ~$120 → ~$1,090 core. Voice: exciter+amp ~$50, conch rigging ~$50, spare phone ~$400. NX upgrade ~$699. Conformal coat ~$1/board.

## PART 12 — BUILD PLAN
App: C0 model pipeline (BLOCKED on OQ-1) → C1 twin+Monitor vs sim → C2 control plane on bench → C3 cues/schedule + commissioning → C4 polish + lite page. Brain: B0 bench → B1 integration → B2 soak (7 nights, Aug 1 go/no-go; miss = ship brainstem-only).

## PART 13 — SOPs
1 Commissioning · 2 Nightly ops · 3 Show programming (jam→lock→schedule) · 4 Dust storm (beacon → occupancy check) · 5 Brain failure (swap to spare phone, debug at camp) · 6 Voice care · 7 Power discipline · 8 Data/privacy (no raw audio, append-only stories, names burn with the week).

## PART 14 — DECISIONS LOCKED
DC-only · Jetson now/NX slot/phone spare · conch Voice night-only Aug 1 gate · PIR ×150 + sentinels · two-path control · console workflow · Meshtastic + BM Internet · pre-designed enclosures · two-tier brain PF-as-modem · designed-position binding · energy-truth + firefly = P1.

## PART 15 — OPEN QUESTIONS
OQ-1 Rhino/Grasshopper export (blocks C0) · 150 vs 140 count · Jetson DC window · DJI+LoRa+mesh coexistence · placement bearing to camp · chandelier centralize-vs-autonomous · BM sound + beacon rules · enclosure SKUs · Ben/NVIDIA discount + ADR.
