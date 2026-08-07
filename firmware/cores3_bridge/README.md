# CoreS3 desk bridge

Dedicated M5Stack CoreS3 USB bridge for the Resonance ESP-NOW fleet. It replaces
the temporary PowerFeather `net_bench --serial-bridge` board without touching the
PowerFeather charger/gauge path.

The bridge:

- initializes the CoreS3 AXP2101 and LCD through M5Unified;
- stays unassociated from infrastructure WiFi and pins ESP-NOW to channel 11;
- tracks up to 192 fixture heartbeats using the canonical packet contract in
  `firmware/fixture/src/core/packet.h`;
- emits the same `nb-master`, `nb-peer`, and `nb-scanap` serial lines consumed by
  `ops/bench/net_bench_dashboard.py` and the JSONL logger;
- accepts the existing dashboard serial controls for maintenance, resume,
  identify, rate, charger settings, sleep/park, drawdown, and solenoid strike;
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

Expected boot identity:

```text
=== Resonance net-bench cores3-bridge-2026-08-06.4 ===
role=master channel=11 frame_hz=0 hb_hz=0
mode: SERIAL BRIDGE (CoreS3; no WiFi; relaying nb-* to USB serial)
```

The USB port number is an observation, not identity. On the first Nevada City
unit the stable USB parent serial/MAC is `44:1B:F6:E3:9F:1C` and the derived
bridge ID is `E39F1C`.
