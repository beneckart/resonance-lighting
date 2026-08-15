# 0037 -- Claude mesh bridge handheld ("cricket console")

**Date:** 2026-08-15

**Status:** Proposed direction. **Hardware on hand** (2x LilyGO **T-Deck Plus**,
LCD variant -- **not** T-Deck Pro; 1x M5Stack Cardputer ADV; confirmed
2026-08-15). No firmware written, no board bring-up done. Hardware arrival is not
firmware completion. Post-2026-event work unless Ben explicitly re-prioritizes it.

**Owners:** Ben + Claude

## Context

The handhelds available today are either tethered or dumb. A CoreS3 on USB gives
Claude (running on the laptop) full fleet visibility through
`net_bench_dashboard.py`, but requires an open laptop. An Atom clicker is
untethered but can do exactly one thing. Neither lets someone standing under the
tree ask a question in English and get an answer from the mesh.

Merging the two produces a different tool. The value is in three places:

- **Development.** A device that hears every ESP-NOW frame with per-packet RSSI
  turns "which nodes went quiet in the last ten minutes, and what were their
  last signal strengths" into a tool call against a ring buffer instead of an
  evening with a logic analyzer.
- **Field operations.** Camp will have Starlink but not necessarily an open
  laptop. A lanyard-sized device with a keyboard that reaches both the Anthropic
  API and the mesh means diagnosis happens at the tree, in gloves-off seconds.
- **Performance.** "Make the east quadrant chirp denser and quieter for the next
  hour" as show control, typed on a granola-bar keyboard. The absurdity is
  intentional and correct for this project.

The design brief this ADR records is
`docs/research/CLAUDE_MESH_BRIDGE_DESIGN_2026-08-15.md`. It was drafted in a
chat session without repo access; the corrections below are the result of
checking it against the actual firmware.

## Decision

Record the direction, with these constraints binding any implementation.

1. **Depends on ADR 0036.** The device associates to an AP *and* runs ESP-NOW on
   one radio. It cannot work unless the AP is pinned to the mesh channel. The
   channel guard from ADR 0036 is mandatory, in its mesh-preserving form: on
   mismatch, drop WiFi and stay on the mesh.
2. **`firmware/fixture/src/core/packet.h` is the wire contract. Do not write a
   new `mesh_protocol.h`.** The header is already platform-independent (no
   Arduino includes), already native-testable, and already pins golden
   `sizeof`/`offsetof` values so an accidental reorder fails at build time. It is
   parsed by 24 commissioned fixtures, `cores3_bridge`, and all host tooling. A
   parallel definition forks the fleet contract, which is the single most
   expensive mistake available here.
3. **Build the census on the ordinary ESP-NOW receive callback, not promiscuous
   mode.** All fleet traffic is broadcast -- targeting is a 3-byte `target_id`
   *inside* the payload, not unicast addressing -- so `esp_now_register_recv_cb`
   already sees every frame, and `esp_now_recv_info_t.rx_ctrl->rssi` already
   carries per-packet RSSI. `cores3_bridge` already tracks 192 peers with seq,
   gaps, PDR, RSSI, and age on exactly this path. Promiscuous mode is an
   optional build flag for non-fleet frames and malformed-frame debugging, not
   the foundation.
4. **Stay on the repo toolchain: arduino-cli + Arduino-ESP32, with M5Unified /
   LovyanGFX for display and input.** Arduino-ESP32 3.x sits on ESP-IDF 5.x and
   exposes `esp_wifi_*`, promiscuous mode, ESP-NOW, and `esp_http_client`-class
   TLS. A pure-IDF target for one device costs easy reuse of `packet.h`, the
   native test suite, and every build/flash convention in `firmware/*/build.sh`.
5. **Addressing follows the fleet, not the compass.** There are no
   `north/south/east/west/canopy` groups. There is a 3-byte short ID, with
   `00:00:00` meaning all, and four fixture classes (`downlight`, `perimeter`,
   `uplight`, `chandelier`) that fixtures self-detect. Per-fixture color already
   exists as `NB_DIRECT_FRAME` (18 entries per frame); pinned adjacency exists as
   `NB_NEIGHBOR_SET`. A named-group layer is a new wire-format decision and needs
   its own ADR -- it is not a lookup table on top of what exists.
6. **Safety rails live in firmware, not in the prompt.** Clamp every parameter to
   the limits the fixture already enforces -- the solenoid path is a 5-300 ms
   bounded pulse with a 40 ms default and an 80 ms coil rest
   (`firmware/fixture/src/esp32/solenoid.h`), and the fixture releases D7 on
   every exit path. Nothing the chat layer emits may hold a coil energized.
   Fleet-wide mode changes require a one-keypress confirm on the device. **No
   OTA, reboot, profile, or lifecycle-override opcode is exposed to the tool
   surface in v1.**
7. **Mesh functions work with the WAN fully down.** Claude is an enhancement, not
   a dependency. Ship three or four hardwired quick commands -- all-off,
   default-day, default-night, census-to-screen -- reachable with no API round
   trip. Queue outbound user messages in RAM with capped backoff and show
   *queued* rather than an error.
8. **The unauthenticated-command problem is inherited, not solved.** Anyone on
   channel 11 can already spoof a fleet command; this is a known open item
   against the Atom clicker work in `TODO.md`. A lanyard device that can command
   the fleet widens the same surface. Acceptable for a bench and for v1, but it
   is a precondition to be closed before this device is trusted at the event,
   and it belongs to the same authenticated-command work item, not a separate
   one.
9. **Primary target is the T-Deck; Cardputer ADV is the secondary.** Two T-Decks
   are on hand against one Cardputer, so the primary target has a spare -- which
   matters for a device carried in dust. It also has the larger display and the
   better keyboard for sustained typing, and 8 MB PSRAM / 16 MB flash for the
   TLS-plus-census pinch point. Prove milestones 0-4 on it, then port behind the
   display/input HAL. The HAL structure from the brief is right; building both
   at once is not.
10. **Variant confirmed: T-Deck Plus (LCD).** ESP32-S3FN16R8 with 8 MB PSRAM and
    16 MB flash, 2.8 in ST7789 IPS 320x240 with GT911 capacitive touch,
    BlackBerry-style keyboard on an ESP32-C3 auxiliary MCU over I2C (commonly
    0x55), trackball, SX1262 LoRa as standard, a GPS receiver, a bundled
    2000 mAh battery, and a case with an antenna break-out. The brief's board
    table is correct for this hardware and can be used as written.
    **Do not port from T-Deck Pro documentation** -- the Pro is a different
    device (3.1 in e-paper, CST328 touch, TCA8418 keypad controller) whose
    display and input drivers do not transfer. The names are one word apart and
    this is the easiest available mistake.
    An IPS panel means streamed text renders per delta with no special handling
    -- but **direct-sun readability is a real open risk** for the milestone 5
    sunglasses criterion. Check it early with the actual panel outdoors rather
    than at the bench.
11. **Battery life is a design constraint, not a given.** The 2000 mAh cell makes
    the device untethered, but this repo has already measured that an always-on
    ESP-NOW peer is radio-RX-dominated at roughly **168 mA / 0.55 W** on an
    ESP32-S3 (LOG 2026-06-08, the sizing campaign that made deep sleep mandatory
    for fixtures). A handheld is by design an always-on receiver, plus a backlit
    IPS panel, plus periodic TLS. Expect well under a full night of continuous
    operation, and treat runtime as a milestone-0 measurement rather than an
    assumption.
    The consequent design decision: **continuous census is the expensive mode.**
    Duty-cycling the radio to extend runtime trades census completeness for
    battery, and "how long was I not listening" must then be visible in the
    census output rather than silently degrading it -- a quiet node and an
    unobserved node must not look the same. Screen blanking is the cheap win;
    radio duty-cycling is the one with a correctness cost.

## Consequences

- The sniffer path doubles as a permanent mesh-health instrument: even with no
  chat session open, the device accumulates a per-MAC census (last seen, packet
  count, RSSI EMA) that any future tooling can read.
- Reusing `packet.h` means the handheld inherits the append-only tail discipline
  for free, and any protocol change is a single-file change that the native test
  pins.
- Reusing the `cores3_bridge` census path means milestone 3 is largely a port,
  not a new subsystem.
- TLS plus sniffer plus UI on one ESP32-S3 is the memory pinch point. Budget
  roughly 50 KB heap per TLS session, keep exactly one in flight, put
  conversation state and the frame ring buffer in PSRAM, and log heap and PSRAM
  watermarks at every milestone rather than assuming. The T-Deck Plus's 8 MB
  PSRAM / 16 MB flash is comfortable on paper; measure it anyway.
- The T-Deck Plus carries a **GPS receiver and a battery-backed power path**.
  That is a real adjacency to the ADR 0031 sparse-time-anchor work (four SAM-M8Q
  soft anchors and four DS3231 RTC anchors bought, qualification open): a
  battery-powered handheld that already knows UTC could act as a walking time
  anchor, and `NB_TIME_QUALITY` (type 20) is already defined and parse-stubbed
  in `packet.h` for exactly this kind of source. Recorded as an opportunity to
  evaluate later and explicitly **not** adopted here. ADR 0031's anchor plan
  does not depend on this device existing and must not come to -- a show clock
  that needs someone to be holding a handheld is not a production time source.
- The **SX1262 LoRa radio is out of scope.** The fleet link is ESP-NOW on
  channel 11 and nothing here changes that. Noted only so it is not mistaken for
  a second mesh path.
- API key, SSID/PSK, model, and mesh channel live in NVS, provisioned over a
  serial CLI. No secret is compiled in or committed; any local config header is
  gitignored, matching the existing `wifi_secrets.h` convention.
- This is a nice-to-have against the open production gates listed in
  `README.md`. Recording the direction now is cheap; building it before the tree
  is lit is not.

## Validation required

Milestones, each gated on measured heap and PSRAM watermarks:

| M | Scope | Accept when |
|---|-------|-------------|
| 0 | T-Deck Plus bring-up: ST7789 display, GT911 touch, I2C keyboard, trackball, WiFi STA, SNTP, channel guard | Status bar live; a wrong-channel AP drops WiFi, keeps mesh, and says so |
| 0b | Measure runtime and direct-sun readability on the real hardware | Hours of continuous census on the 2000 mAh cell, recorded as a number; test pattern legible outdoors through sunglasses |
| 1 | Claude client: streaming chat, NVS key, trimmed history, offline queue | Sustained chat over the Beryl; survives a two-minute WAN pull |
| 2 | `packet.h` integration + mesh TX | Bridge triggers a bench fixture's strike and LED mode from a local menu |
| 3 | Census on the ESP-NOW receive path | Census matches known bench fixtures; RSSI sane; one-hour soak with no heap creep |
| 4 | Agent tool loop | "Which fixtures are quiet?" and "make the bench fixture chirp twice" work end to end from typed English |
| 5 | UI polish, three-state indicators, quick-command menu, Cardputer ADV port | Readable in direct sun with sunglasses; operable with gloves; both boards from one codebase |

Take the direct-sun readability check out of milestone 5 and do it in milestone
0, with nothing more than a test pattern on the actual panel outdoors. It is the
cheapest possible falsification of the whole concept, and an IPS panel in desert
daylight is the plausible way this device turns out to be unusable in the field
despite working perfectly on the bench.

SNTP must land before any TLS attempt -- certificate validation needs wall-clock
time, and a device that boots with a 1970 clock fails handshake in a way that
looks like a network problem.

## References

- `docs/research/CLAUDE_MESH_BRIDGE_DESIGN_2026-08-15.md` -- full design brief
- `docs/decisions/0036-camp-network-channel-11-ap.md` -- the hard prerequisite
- `firmware/fixture/src/core/packet.h` -- the wire contract
- `firmware/cores3_bridge/README.md` -- census and bridge precedent
- `firmware/fixture/src/esp32/solenoid.h` -- the actuator clamp to mirror
- `TODO.md` -- authenticated/authorized strike commands (open)
