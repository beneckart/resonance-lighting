# Claude mesh bridge -- design brief and implementation plan

**Date:** 2026-08-15

**Status:** Design brief. Hardware on hand (2x T-Deck LCD, 1x Cardputer ADV);
nothing built, no bring-up done. Direction recorded in
`docs/decisions/0037-claude-mesh-bridge-handheld.md`.

**Origin:** Drafted by Ben + Claude in a chat session without repo access, then
reconciled against the actual firmware. Section 9 lists what the original draft
got wrong; the rest of this document is the corrected version.

---

## 1. What it is

A standalone handheld that is simultaneously:

- a Claude chat client over WiFi/TLS,
- an ESP-NOW command transmitter into the Resonance mesh, and
- a passive observer of all mesh traffic,

so that Claude, through tool use, can query and operate the tree from a pocket
device.

Today the handhelds are either tethered or dumb: a CoreS3 on USB gives Claude
full fleet visibility through `net_bench_dashboard.py` but needs an open laptop;
an Atom clicker is untethered and does exactly one thing.

**For development**, the observer sees every ESP-NOW frame with per-packet RSSI,
so "which fixtures went quiet in the last ten minutes and what were their last
signal strengths" becomes a tool call against a ring buffer.

**For field operations**, camp has Starlink but not necessarily an open laptop.
Diagnosis happens at the tree, in gloves-off seconds.

**For performance**, natural language becomes the show-control surface. The
absurdity is intentional and correct for this project.

---

## 2. The one physics constraint

The ESP32-S3 has **one 2.4 GHz radio**. WiFi STA and ESP-NOW coexist on it but
must share a channel, and in STA mode the AP dictates that channel. The fleet is
pinned to channel 11.

This is why every existing bridge avoids the problem: `cores3_bridge` "stays
unassociated from infrastructure WiFi and pins ESP-NOW to channel 11", and
fixtures entering OTA maintenance have already left ESP-NOW.

This device cannot dodge it. The entire architecture therefore rests on
**ADR 0036**: the camp AP is pinned to channel 11, HT20, WPA2-PSK, on a dedicated
2.4 GHz SSID. Setup runbook: `docs/howto/CAMP_NETWORK_SETUP.md`.

**Channel guard (required).** After STA association, read the actual operating
channel. If it is not the compiled mesh channel, **drop the WiFi association and
keep the mesh**, then display the mismatch.

The original draft had this inverted -- it refused to start mesh TX/RX. That is
backwards: the mesh is the primary function and Claude is the enhancement, which
is the same principle section 7 applies to WAN outages. A field tool that
silences its mesh to hold a WiFi link has the priority wrong.

---

## 3. Hardware targets

One codebase, board-specific code confined to a display/input HAL.

| | Cardputer ADV | T-Deck |
|---|---|---|
| SoC | ESP32-S3 (M5Stamp-S3A), PSRAM | ESP32-S3, 8 MB PSRAM |
| Display | 1.14 in ST7789, 240x135 | 2.8 in ST7789, 320x240, capacitive touch |
| Input | 56-key matrix | BlackBerry-style keyboard on an aux MCU over I2C (commonly 0x55), plus trackball |
| Role | pocket/lanyard, walking-around console | primary chat terminal (about 50x20 chars) |

**Both are on hand** as of 2026-08-15: **2x T-Deck Plus (LCD variant) and
1x Cardputer ADV**. The table above is accurate for these units.

**T-Deck Plus, confirmed:** ESP32-S3FN16R8, 8 MB PSRAM, 16 MB flash, 2.8 in
ST7789 IPS 320x240 with GT911 capacitive touch, BlackBerry keyboard on an
ESP32-C3 auxiliary MCU over I2C, trackball, SX1262 LoRa as standard, a GPS
receiver, a bundled 2000 mAh battery, and a case with an antenna break-out and
tripod mount. The Plus is the variant that makes LoRa standard and adds GPS and
the battery; the base T-Deck is a bring-your-own-cell devkit.

**T-Deck Pro is a different device** -- 3.1 in *e-paper* 320x240, CST328 touch,
TCA8418 keypad controller. Its display and input drivers do not transfer, and
streamed chat would need paragraph-boundary repaints instead of per-delta
rendering. Not what is in hand; recorded so nobody ports to the wrong driver set
after reading a spec page one word off from theirs.

Still verify pin maps and the keyboard controller address against LilyGO's
documentation for the actual revision rather than trusting this table.

**Two on-board radios that are not the fleet link.** SX1262 LoRa is out of scope
-- the fleet link is ESP-NOW on channel 11. The GPS is more interesting: see
section 10.

**Build the T-Deck first.** Two units means the primary target has a spare,
which matters for a device carried in dust; it also has the larger display and
the better keyboard for sustained typing. Prove milestones 0-4 there, then port
to the Cardputer ADV behind the HAL.

**The IPS panel's real risk is direct sun, not refresh rate.** Streaming text
renders per delta with no special handling, but a 2.8 in IPS in desert daylight
through sunglasses is the plausible way this device works perfectly on the bench
and is unusable at the tree. Check it outdoors with a test pattern during
milestone 0 -- it is the cheapest possible falsification of the concept.

**Toolchain: arduino-cli + Arduino-ESP32, with M5Unified / LovyanGFX.** Not pure
ESP-IDF. Arduino-ESP32 3.x sits on ESP-IDF 5.x and exposes `esp_wifi_*`,
promiscuous mode, ESP-NOW, and TLS-capable HTTP clients. A separate IDF target
for one device costs easy reuse of `packet.h`, the native test suite, and every
convention in `firmware/*/build.sh`. Per `AGENTS.md`, give any uncached
ESP32-S3 build at least 300 seconds and a unique `--build-path`.

---

## 4. Firmware architecture

Modules, each with a narrow header:

**`net_mgr`** -- STA bring-up, reconnect with backoff, SNTP, the section 2
channel guard, RSSI/IP for the status bar. `esp_wifi_set_ps(WIFI_PS_NONE)` is
mandatory: modem power-save drops ESP-NOW frames arriving between DTIM beacons.
**SNTP must succeed before any TLS attempt** -- certificate validation needs
wall-clock time, and a 1970 clock fails the handshake in a way that looks like a
network fault.

**`claude_client`** -- HTTPS to `https://api.anthropic.com/v1/messages` using the
ESP-IDF x509 certificate bundle. Headers: `x-api-key` (from NVS),
`anthropic-version: 2023-06-01`, `content-type: application/json`. See section 5.

**`mesh_tx`** -- thin wrapper over `esp_now_send`. **Includes
`firmware/fixture/src/core/packet.h` directly; does not redefine the protocol.**
The fleet broadcasts, with targeting carried as a 3-byte `target_id` inside the
payload, so a broadcast peer covers both group and per-node commands.

**`census`** -- built on the ordinary `esp_now_register_recv_cb`, **not**
promiscuous mode. The callback runs in WiFi context: timestamp, copy
`{ts, src_id, rssi, len, first N payload bytes}` into a PSRAM ring buffer, and
return. A worker task on core 1 drains the ring into a rolling raw tail and a
per-node census (last seen, packet count, RSSI EMA, seq gaps, PDR).
Promiscuous mode is an optional build flag for non-fleet frames; if the fleet
ever enables ESP-NOW long-range mode, the sniffer must enable LR too or those
frames are invisible.

**`agent`** -- the tool-use loop (section 6).

**`ui`** -- chat scrollback, input line, status bar (WiFi RSSI, channel, IP,
SNTP, live-node count, TX/RX ticks). Three states must be visually distinct:
*thinking* (request in flight), *link down* (queued, retrying), *mesh silent*
(no frames in N seconds).

**`store`** -- NVS: SSID/PSK, API key, model, mesh channel. Provisioned over a
serial CLI (`set wifi ...`, `set key ...`). No secret compiled in or committed;
gitignore any local config header, matching the `wifi_secrets.h` convention.

**Concurrency:** WiFi/LWIP own core 0; UI, agent, and census-drain tasks pin to
core 1.

**Memory:** TLS + census + UI is the pinch point. Budget roughly 50 KB heap per
TLS session, keep exactly one in flight, put conversation state and the ring
buffer in PSRAM, and log heap/PSRAM watermarks at every milestone.

---

## 5. Claude API details

Verified against the current API, 2026-08-15.

**Endpoint and headers** as above. Requests use `"stream": true`; the client
parses SSE:

| Event | Use |
|---|---|
| `content_block_start` / `content_block_stop` | block boundaries; begin tool-use assembly |
| `content_block_delta` | `text_delta` to the UI; `input_json_delta` accumulates tool input |
| `message_delta` | carries `stop_reason` and usage |
| `message_stop` | end of message |

**Model.** The original draft named `claude-sonnet-4-6`. That model is still
active, but the current generation is `claude-sonnet-5` (balanced) and
`claude-opus-5` (most capable). Default to `claude-sonnet-5` here and keep it
NVS-overridable.

**Two embedded-specific gotchas the draft missed:**

1. **Thinking is on by default on the current models**, and `max_tokens` caps
   thinking *plus* response text together. A `max_tokens` of 1024 with adaptive
   thinking on can truncate mid-answer. Either set
   `"thinking": {"type": "disabled"}` -- valid on Sonnet 5, and on Opus 5 at
   effort `high` or below -- or raise `max_tokens` well above the answer length
   you expect. For a device rendering to a 240x135 screen over a playa uplink,
   thinking off is usually the right call.
2. **`output_config: {"effort": "low"}`** is the cheap, low-latency setting and
   suits short operator questions. Reserve higher effort for genuinely analytical
   asks.

**Conversation state:** system prompt plus a trimmed window of roughly the last
12 turns, in PSRAM.

---

## 6. Tool surface

Keep it small; grow it only when a session actually wants something. The schema
below is grounded in real opcodes from `packet.h`, not invented groups.

```json
[
  {
    "name": "mesh_census",
    "description": "Summarize mesh liveness from the passive census: per-fixture short ID, seconds since last frame, packet count, smoothed RSSI, PDR, fixture class. Optionally filter to fixtures silent for more than quiet_s seconds.",
    "input_schema": { "type": "object", "properties": {
      "quiet_s": { "type": "integer" } } }
  },
  {
    "name": "node_status",
    "description": "Return the census entry and last decoded heartbeat fields for one fixture, by 6-hex-digit short ID (e.g. F40268).",
    "input_schema": { "type": "object", "properties": {
      "id": { "type": "string" } }, "required": ["id"] }
  },
  {
    "name": "identify",
    "description": "Blink or color-identify a fixture so a human can find it physically. Maps to NB_IDENTIFY.",
    "input_schema": { "type": "object", "properties": {
      "id":    { "type": "string" },
      "secs":  { "type": "integer" },
      "color": { "type": "string", "enum": ["none","red","green","blue","yellow","white"] } },
      "required": ["id"] }
  },
  {
    "name": "set_program",
    "description": "Lease a choreography program to one fixture or the whole fleet. Maps to NB_PROGRAM_SET; the lease expires on its own after lease_s, so a lost bridge never strands the fleet.",
    "input_schema": { "type": "object", "properties": {
      "target":     { "type": "string" },
      "program_id": { "type": "integer" },
      "lease_s":    { "type": "integer" },
      "params":     { "type": "object" } },
      "required": ["target", "program_id"] }
  },
  {
    "name": "strike",
    "description": "One bounded solenoid pulse on one fixture. Maps to NB_TARGET_SOLENOID. Pulse length is clamped in firmware to 5-300 ms regardless of what is requested.",
    "input_schema": { "type": "object", "properties": {
      "id":       { "type": "string" },
      "pulse_ms": { "type": "integer" } }, "required": ["id"] }
  },
  {
    "name": "sniffer_tail",
    "description": "Return the last n observed frames (timestamp, source, type, rssi, len, payload head) for debugging.",
    "input_schema": { "type": "object", "properties": {
      "n": { "type": "integer" } } }
  }
]
```

**`target` semantics -- the open decision.** `all` and a 6-hex fixture ID map
directly to `target_id` (`00:00:00` means all). **Class or spatial targeting
does not exist on the wire.** Fixtures self-detect into `downlight`, `perimeter`,
`uplight`, `chandelier`, and that class rides in telemetry -- it is not an
address. Three options, in increasing cost:

1. **Client-side expansion** -- the handheld knows each fixture's class from the
   census and emits per-ID commands, or fills `NB_DIRECT_FRAME` entries (18 per
   frame). Works today, no wire change, costs airtime at fleet scale.
2. **A class byte in the target field** -- small, but a wire-format change to a
   contract 24 commissioned fixtures already parse.
3. **Named spatial groups** (the draft's `north/south/east/west/canopy`) --
   requires per-fixture group assignment, which requires knowing where each
   fixture physically is. `NB_NEIGHBOR_SET` (pinned adjacency) and the reserved
   `NB_NEIGHBOR_REPORT` locate work are the existing seams for this.

**Start with option 1.** Options 2 and 3 need their own ADR.

**Safety rails live in firmware, not in the prompt.** Clamp every parameter to
the limits the fixture already enforces: the solenoid path is a bounded 5-300 ms
pulse, 40 ms default, 80 ms coil rest, with D7 released on every exit path
(`firmware/fixture/src/esp32/solenoid.h`). Nothing the chat layer emits may hold
a coil energized. Fleet-wide changes require a one-keypress confirm on the
device. **No OTA, reboot, profile, or lifecycle-override opcode is exposed in
v1** -- those are exactly the commands whose blast radius exceeds what a
conversational surface should reach.

**Agent loop:** on `stop_reason: "tool_use"`, dispatch to the local handler,
append the `tool_result` block, re-POST, and repeat until an end turn, rendering
streamed text throughout. Cap iterations so a loop cannot run away.

**On-device system prompt, roughly:**

```
You are the operator console for a roughly 130-fixture solar lantern tree
("Resonance"). Tools let you observe the ESP-NOW mesh passively and send
commands. Fixtures are addressed by 6-hex-digit short ID, or "all". Classes:
downlight, perimeter, uplight, chandelier. Prefer census/status before
commanding. Be terse: the display is small. Firmware clamps all actuator
limits regardless of what you request.
```

---

## 7. Behavior under bad networks

This is Burning Man; playa Starlink drops for minutes at a time.

- Outgoing user messages queue in RAM and retry with capped exponential backoff.
  The UI shows *queued*, not an error.
- Half-finished SSE streams are abandoned after a stall timeout (about 20 s with
  no delta) and retried once before surfacing.
- **Mesh functions work with the WAN fully down.** Census and commands from the
  local menu must not require an API round trip. Ship three or four hardwired
  quick commands: all-off, default-day, default-night, census-to-screen.

Claude is an enhancement, not a dependency -- the same principle as the channel
guard in section 2, and the same principle that keeps the fleet autonomous with
no infrastructure at all (ADR 0004).

---

## 8. Security posture

**ESP-NOW commands are unauthenticated.** No PMK/LMK is set anywhere in the
firmware; anyone on channel 11 with the packet layout can spoof any command.
This is already a known open item against the Atom clicker work in `TODO.md`
("add authenticated/authorized strike commands because the current ESP-NOW packet
is unauthenticated").

A lanyard-sized device that commands the fleet widens the same surface: it can
be lost, and it holds an Anthropic API key in NVS. Acceptable for bench work and
v1. **Not** acceptable as a trusted event tool until the authenticated-command
work lands -- and it is the *same* work item, not a new one. Practical mitigations
in the meantime: keep OTA/reboot opcodes off the tool surface (section 6), keep
the API key scoped and revocable, and treat the device as untrusted inventory.

---

## 9. Corrections against the original draft

Recorded so the reasoning is not lost:

| Draft said | Reality |
|---|---|
| "Extract a shared `mesh_protocol.h`; that header extraction is task one" | Already done. `firmware/fixture/src/core/packet.h` is the contract -- platform-independent, native-testable, with golden `sizeof`/`offsetof` pins. Include it; a second definition forks the fleet contract. |
| Promiscuous-mode sniffer as the foundation | Largely redundant. All fleet traffic is broadcast, and `esp_now_recv_info_t.rx_ctrl->rssi` gives per-packet RSSI on the ordinary receive callback. `cores3_bridge` already tracks 192 peers this way. Promiscuous is an optional flag. |
| Groups `all|north|south|east|west|canopy|perimeter` | No group addressing exists. 3-byte short ID, `00:00:00` = all; classes are `downlight/perimeter/uplight/chandelier` and are telemetry, not an address. See section 6. |
| "~150 ESP32-S3 nodes" | About 130 fixtures in four classes (ADR 0032). |
| "Solenoid coils are 20-30 ms pulse devices" | Repo clamp is 5-300 ms, 40 ms default, 80 ms coil rest. |
| "ESP-IDF 5.x recommended" | Repo is arduino-cli + Arduino-ESP32, which is IDF 5.x underneath and sufficient. Pure IDF forks the toolchain for one device. |
| Channel guard refuses to start mesh TX/RX | Inverted. Drop WiFi, keep the mesh. |
| `claude-sonnet-4-6` | Still valid, but previous-generation. Use `claude-sonnet-5`. |
| `max_tokens` about 1024 | Fine only with thinking disabled; `max_tokens` caps thinking plus text on current models. |
| Two board targets from the start | One first (T-Deck -- two units on hand vs one Cardputer ADV), then port. The brief's board table is otherwise correct for the LCD T-Deck actually in hand. |
| "Anyone on ch11 can spoof; acceptable for v1, revisit later" | True, but it is an existing tracked open item, not a new footnote. See section 8. |

---

## 10. Open questions

- Who owns the build. (Board is settled: T-Deck Plus first, Cardputer ADV port
  after.)
- Whether a 2.8 in IPS is readable in direct playa sun through sunglasses. This
  is the cheapest falsification of the whole concept -- do it in milestone 0.
- **Runtime on the 2000 mAh cell.** This repo measured an always-on ESP-NOW peer
  at roughly 168 mA / 0.55 W on an ESP32-S3, radio-RX-dominated (LOG 2026-06-08)
  -- the finding that made deep sleep mandatory for fixtures. A handheld is an
  always-on receiver plus a backlit panel plus periodic TLS, so expect well under
  a night of continuous use. Measure it; do not assume it. The design question
  that follows: duty-cycling the radio buys runtime but costs census
  completeness, and the census must then distinguish "this node was quiet" from
  "I was not listening" rather than letting the two look identical.
- **GPS as a possible ADR 0031 adjacency.** The Plus has a GPS receiver, and
  `NB_TIME_QUALITY` (type 20) is already defined and parse-stubbed in `packet.h`
  with a `source` field that includes `bridge`. A battery-powered handheld that
  knows UTC could act as a walking time anchor or a locate aid alongside the four
  purchased SAM-M8Q soft anchors and four DS3231 RTC anchors. Worth evaluating
  after the core device works -- and explicitly not a production time source: a
  show clock that depends on someone carrying a handheld is not a clock.
- Group/class addressing: client-side expansion (start here) vs a wire change.
- Whether the census should also be exposed over serial so the existing
  `net_bench_dashboard.py` can consume it.
- Whether ESP-NOW long-range mode will ever be enabled fleet-wide (it changes
  what a sniffer must configure).
- Where this sits against the open production gates in `README.md`. As recorded
  in ADR 0037, this is post-2026-event work unless Ben re-prioritizes.

## References

- `docs/decisions/0036-camp-network-channel-11-ap.md`
- `docs/decisions/0037-claude-mesh-bridge-handheld.md`
- `docs/howto/CAMP_NETWORK_SETUP.md`
- `firmware/fixture/src/core/packet.h`
- `firmware/fixture/src/esp32/solenoid.h`
- `firmware/cores3_bridge/README.md`
