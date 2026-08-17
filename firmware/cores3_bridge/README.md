# CoreS3 desk bridge

Dedicated M5Stack CoreS3 USB bridge for the Resonance ESP-NOW fleet. It replaces
the temporary PowerFeather `net_bench --serial-bridge` board without touching the
PowerFeather charger/gauge path.

This is also the implemented fallback/reference for the received PUCA
performance-audio hardware selected in ADR 0035. PUCA is a separate original-
ESP32 + WM8978 target and does not run this CoreS3 binary; its hardware and
bring-up record is `hardware/puca-audio-bridge/README.md`.

The bridge:

- initializes the CoreS3 AXP2101 and LCD through M5Unified;
- stays unassociated from infrastructure WiFi and pins ESP-NOW to channel 11;
- tracks up to 192 fixture heartbeats using the canonical packet contract in
  `firmware/fixture/src/core/packet.h`;
- emits the same `nb-master`, `nb-peer`, and `nb-scanap` serial lines consumed by
  `ops/bench/net_bench_dashboard.py` and the JSONL logger;
- length-gates the fixture heartbeat tails and exposes profile, lifecycle, power
  tier, sensor-derived class/signature, class mismatch, LED-rail state, actual
  rendered RGBW average, lit-pixel count, and low-VBAT recovery state;
- accepts the existing dashboard serial controls for maintenance, resume,
  identify, rate, charger settings, sleep/park, drawdown, and solenoid strike;
- accepts `i<fixture-id>:<seconds>` for an exact 1-255 second fixture locator
  (`iF40268:60`), while bare `i` retains next-peer cycling;
- accepts `T<fixture-id>:1` for a renewable 255-second steady-green tag at
  linear level 128 and `T<fixture-id>:0` to release it immediately;
- accepts `F0` / `F1` to persistently place all reachable fixtures in
  commission / field profile, or `F<fixture-id>:0|1` for one fixture;
- shows bridge health and fresh fixtures on the built-in screen using a
  PSRAM-backed framebuffer so periodic updates do not visibly blink.

It also has a build-time Cambium modem mode. In that mode the same CoreS3
relays Cambium's COBS/CRC serial contract instead of emitting dashboard text,
while retaining the on-device health display and heartbeat tracking. Binary
mode never writes bare diagnostic text to USB; diagnostics are Cambium LOG
frames so they cannot corrupt the serial stream.

An independent audio-reactive build mode turns a live microphone envelope into
10 Hz `NB_DIRECT_FRAME` colors for every fixture heard in the last five
seconds. Each fixture gets a stable red, green, or blue slot based on sorted
fixture ID. The bridge performs a two-second ambient-noise calibration, then
uses fast attack and slow release. Tap the screen or send `A` over USB serial
to pause/resume. Direct-frame staleness still returns each fixture to its
autonomous program after three seconds; this mode does not persist a lifecycle
override on any fixture. Peers whose full heartbeat identifies non-fixture
firmware are omitted so an old bench node cannot consume a color slot.

The CoreS3 itself has two built-in microphones. The M5Stack Module Audio has no
microphone of its own: it provides external analog inputs through a TRS
LINE/MIC jack and a TRRS headset jack. The two mic routes share one codec input
and must not be enabled together. The Resonance module build selects the TRS
LINE/MIC jack for the Rode mic. If the module is not detected it falls back to
the CoreS3 microphones and reports the active source on screen and serial.

For the physical hookup, Rode VideoMic NTG control reference, recommended gain
and filter settings, display interpretation, daylight bench procedure, and
troubleshooting, see
[`docs/howto/CORES3_AUDIO_REACTIVE.md`](../../docs/howto/CORES3_AUDIO_REACTIVE.md).

The Thread Border Router kit's ESP32-H2 Gateway module is not used. Leaving the
Gateway/DIN stack installed is harmless; the Resonance bridge runs only on the
CoreS3's ESP32-S3 radio.

## Build and flash

Install `M5Unified` (Arduino CLI also installs its M5GFX dependency), then build
once into a named path and flash that exact build:

```sh
arduino-cli lib install M5Unified
bash ./build.sh --channel 11 --build-path build/nc-cores3-bridge-r1
arduino-cli upload --fqbn esp32:esp32:m5stack_cores3 --port COM40 \
  --build-path build/nc-cores3-bridge-r1 .
```

Or compile and flash in one invocation:

```sh
bash ./build.sh --channel 11 --port COM40 --build-path build/nc-cores3-bridge-r1
```

For Cambium, build a separate artifact with `--cambium` and keep the normal
dashboard artifact available for restoration:

```sh
bash ./build.sh --cambium --channel 11 \
  --build-path build/nc-cores3-cambium-r1
arduino-cli upload --fqbn esp32:esp32:m5stack_cores3 --port COM40 \
  --build-path build/nc-cores3-cambium-r1 .
```

The Cambium status identity is `cores3-cb-0.1`. The mode implements RADIO_TX,
RADIO_RX (including the full source MAC and RSSI), status, channel selection,
and reboot. Use it with the serial transport in the Cambium repo; do not run
the ASCII net-bench dashboard against a binary-mode artifact.

For a built-in-microphone audio artifact:

```sh
bash ./build.sh --audio --channel 11 \
  --build-path build/nc-cores3-audio-builtin-r1
```

For Module Audio, first power the stack off and set the module's physical A/B
I2S selector to configuration B; A is for Basic/Core2 and conflicts with the
CoreS3's onboard audio path. Then install M5Stack's `M5Module-Audio` library and
build the separate module artifact. The library selects the matching CoreS3
software pin map, but it cannot change that physical selector. This repo was
validated against upstream commit
`d8649a4863fea4a47d690781eb330f7c28a434b3`:

```sh
git clone https://github.com/m5stack/M5Module-Audio.git \
  "$HOME/Documents/Arduino/libraries/M5Module_Audio"
bash ./build.sh --audio-module --channel 11 \
  --build-path build/nc-cores3-audio-module-r1
arduino-cli upload --fqbn esp32:esp32:m5stack_cores3 --port COM43 \
  --build-path build/nc-cores3-audio-module-r1 .
```

For a daylight bench test, use each fixture's serial `N1` command to force
night in RAM, and always restore `N2` (automatic lifecycle) afterward. Neither
command is persisted across reboot.

After the boot banner, launch the existing dashboard:

```sh
python ../../ops/bench/net_bench_dashboard.py --port COM40
```

The primary view is a dense fleet-health grid. Each light is one composite glyph:
the center battery fill uses ADR 0023's load-compensated thresholds, the top sun or
plug shows a live charger input, the thin top bar is the fixture's reported rendered
color, and the whole tile fades when its expected heartbeat is late or silent. The
reported fixture class (normally from the Stemma probe, with an override available)
sets the center shape: circle for canopy/downlight, hexagon for perimeter, triangle
for trunk/uplight, diamond for chandelier, and a
rounded square when class telemetry is unknown. Each tile also has a small
checkbox that persists a green half-brightness location tag in the browser and
renews its bounded fixture lease every two minutes. IDs use the last two MAC digits;
only collisions expand to `DC-1`, `DC-2`, and so on. Select a tile for exact values.
The older solar metrics, controls, table, and raw serial console remain available
under `Detailed diagnostics`.

With `All` selected, the solenoid control becomes `Strike all (N)`. After an explicit
confirmation, the dashboard queues one `K<id>:<ms>` command for every fresh fixture.
This deliberately remains a series of addressed commands rather than adding a wire
broadcast strike. Boards with `sol_en=0` ignore the request, and the fixture's local
lifecycle, power, pulse-width, rest-time, and mechanism gates remain authoritative.

The BQ telemetry proves whether a charger input is present but does not universally
distinguish a panel from USB. The glyph therefore uses fixture class: panel-bearing
classes get a yellow sun and chandelier-class fixtures get a blue external-power
plug. A crossed-out sun means one panel-class fixture is missing input while a
daylight fleet consensus says comparable fixtures have input; it is a diagnostic
lead, not a standalone electrical verdict.

Expected boot identity:

```text
=== Resonance net-bench cores3-bridge-2026-08-16.1 ===
role=master channel=11 frame_hz=0 hb_hz=0
mode: SERIAL BRIDGE (CoreS3; no WiFi; relaying nb-* to USB serial)
```

The USB port number is an observation, not identity. On the first Nevada City
unit the stable USB parent serial/MAC is `44:1B:F6:E3:9F:1C` and the derived
bridge ID is `E39F1C`.
