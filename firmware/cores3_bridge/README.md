# CoreS3 Bridge OS

Standalone M5Stack CoreS3 bridge for the Resonance ESP-NOW fleet. The ordinary
image is now a small, touch-first Bridge OS with two switchable apps:

- **Listener:** a read-only, paged fleet-health grid and fixture detail view;
- **Audio:** one microphone publisher with envelope and short-window FFT looks,
  a fast scrolling spectrogram, and on-screen start/stop/input/mode controls.

It runs from the CoreS3 battery with no laptop or infrastructure WiFi. USB is
still available for the full host dashboard, telemetry logging, and the legacy
serial controls, but it is optional for both on-device apps. Cambium remains a
separate binary artifact.

For a device-comparison, field workflow, and first-line troubleshooting guide,
start with the illustrated
[`Bridge field manual`](../../docs/howto/BRIDGE_OS_FIELD_MANUAL.md). This file
retains the detailed CoreS3 build and protocol record.

This is also the implemented fallback/reference for the received PUCA
performance-audio hardware selected in ADR 0035. PUCA is a separate original-
ESP32 + WM8978 target and does not run this CoreS3 binary; its hardware and
bring-up record is `hardware/puca-audio-bridge/README.md`.

The bridge:

- initializes the CoreS3 AXP2101 and LCD through M5Unified;
- stays unassociated from infrastructure WiFi and pins ESP-NOW to channel 11;
- tracks up to 192 fixture heartbeats using the canonical packet contract in
  `firmware/fixture/src/core/packet.h`;
- boots to an on-device launcher instead of requiring a mode-specific reflash;
- adapts the bench dashboard's class, raw-VBAT health, freshness, and reported-
  output cues to a 24-fixture-per-page touch grid, with a detail page for exact
  values;
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
- accepts `B<seconds>` for a RAM-only, hard-cut fleet-dark program lease and
  lowercase `b` to release any active program lease immediately; neither
  command changes the fixture profile, lifecycle, or persisted configuration;
- accepts `Q<hours>` (1-168) for multi-day rails-off transport sleep; timer wake
  returns radio/telemetry but fixtures remain electrically dark until a program
  command such as bare `b` releases the retained latch;
- accepts `L[seconds]` (default 120, maximum 900, `L0` stop) for a bounded full
  heard-roster RSSI survey and emits one `nb-rssi` line per directed observation;
- shows bridge health and fresh fixtures on the built-in screen using a
  PSRAM-backed framebuffer so periodic updates do not visibly blink.

It also has a build-time Cambium modem mode. In that mode the same CoreS3
relays Cambium's COBS/CRC serial contract instead of emitting dashboard text,
while retaining the on-device health display and heartbeat tracking. Binary
mode never writes bare diagnostic text to USB; diagnostics are Cambium LOG
frames so they cannot corrupt the serial stream.

The Audio app analyzes a live microphone at 25 Hz and turns the result into
10 Hz `NB_DIRECT_FRAME` colors for every fixture heard in the last five seconds.
The faster local path drives a 24-row log-frequency spectrogram with about three
seconds of history plus live bass, mid, and high meters; it does not increase
fleet airtime. Its fixture selector
recognizes the ADR 0040 `fx-*` artifact identity, the older `fixture-*` identity,
and fixture `dev-local`, while excluding identified legacy net-bench/bridge
peers that cannot consume direct frames. Starting Audio first
sends a one-shot, RAM-only fleet program release so an earlier CA, Contagion,
or Dark lease cannot block the direct stream. It does not change the autonomous
default, profile, lifecycle, or NVS. Each fixture gets a
stable red, green, or blue slot based on sorted fixture ID. The bridge performs
a two-second ambient-noise calibration, then maintains a broadband envelope and
independently normalized 60-250 Hz bass, 250-2000 Hz mid, and 2000-8000 Hz high
bands from a 512-sample Hann-windowed FFT. Seven modes are available: CLASSIC
per-slot R/G/B, EMBER warm-white, HUECYCLE (20 s shared hue rotation), PULSE
(broadband transient flashes over a dim floor), BANDS RGB (shared bass/red,
mid/green, treble/blue), BANDS SPLIT (stable fixture thirds each follow one
band), and TIMBRE HUE (spectral centroid selects color while energy selects
brightness). A RAM-only output gain applies after color generation so every
mode keeps its hue while becoming easier to see: **1X**, **1.5X**, **2X**, and
**3X**, with **2X** as the brighter field default. Channel values saturate at
255; fixture battery DIM/OFF/PROTECT policy remains downstream and authoritative.
The Audio footer has **Start/Pause**, **Input**, **Mode**, and **Gain**. Input
cycles between the CoreS3's ambient microphones and Module Audio's Aux input;
USB `A`, `N`, `M`, and `V` remain optional compatibility controls for those
same actions. Leaving Audio sends a zero frame and stops publishing, so a
hidden app cannot fight another artistic publisher.
Direct-frame staleness still returns each fixture to its autonomous program
after three seconds; the app does not persist a lifecycle override. Peers whose
full heartbeat identifies non-fixture firmware are omitted so an old bench node
cannot consume a color slot.

The CoreS3 itself has two built-in microphones. The M5Stack Module Audio has no
microphone of its own: it provides external analog inputs through a TRS
LINE/MIC jack and a TRRS headset jack. The two module routes share one codec
input and must not be enabled together. The Resonance module build uses the TRS
LINE/MIC jack for Aux and can switch at runtime to the CoreS3 microphones. It
prefers Aux at boot, falls back to Ambient if the module is not ready, and lets
the operator retry Aux later with **Input**. A source handoff pauses publishing,
sends zero, resets the two-second noise-floor calibration, and restores the
prior publishing state only after the new input starts. The built-in-only image
shows Ambient and has no Aux input.

M5Unified configures the built-in pins/callback in both hardware variants but
does not start capture during boot; `setupAudioInput()` gives Module Audio first
choice and starts only the selected source. Module Audio's LEDs are green for
Aux and dark blue for Ambient.

For the physical hookup, Rode VideoMic NTG control reference, recommended gain
and filter settings, display interpretation, daylight bench procedure, and
troubleshooting, see
[`docs/howto/CORES3_AUDIO_REACTIVE.md`](../../docs/howto/CORES3_AUDIO_REACTIVE.md).
The staged sound-to-photon latency work, including the no-fixture-flash path and
the gated future feature-packet fixture path, is specified in
[`docs/projects/LOW_LATENCY_AUDIO_REACTIVITY_DEV_PLAN.md`](../../docs/projects/LOW_LATENCY_AUDIO_REACTIVITY_DEV_PLAN.md).

The Thread Border Router kit's ESP32-H2 Gateway module is not used. Leaving the
Gateway/DIN stack installed is harmless; the Resonance bridge runs only on the
CoreS3's ESP32-S3 radio.

## Build and flash

Install `M5Unified` (Arduino CLI also installs its M5GFX dependency), then build
the unified Listener + built-in-mic Audio image once into a named path and flash
that exact build:

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

For Cambium, build a separate artifact with `--cambium` and keep the ordinary
Bridge OS artifact available for restoration:

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

For Module Audio, first power the stack off and set the module's physical A/B
I2S selector to configuration B; A is for Basic/Core2 and conflicts with the
CoreS3's onboard audio path. Then install M5Stack's `M5Module-Audio` library and
build the same two-app firmware with the module hardware layer. The library
selects the matching CoreS3 software pin map, but it cannot change that physical
selector. This repo was
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

Listener carries a compact on-device form of the primary fleet-health view. For
the complete host dashboard, connect USB and launch the command above. Each
dashboard light is one composite glyph:
the center battery fill uses ADR 0046's load-compensated thresholds, the top sun or
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
=== Resonance net-bench cores3-os-0.2.0-dev ===
role=master channel=11 frame_hz=0 hb_hz=0
mode: BRIDGE OS (CoreS3; wireless Listener + Audio apps; USB optional)
```

The USB port number is an observation, not identity. The hardware-validated
Bridge OS unit is full MAC `80:45:6B:4D:5D:B0`, short ID `4D5DB0`. The second
historical Nevada City CoreS3 identity is full MAC `44:1B:F6:E3:9F:1C`, short
ID `E39F1C`.
