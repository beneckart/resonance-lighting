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

After the boot banner, launch the existing dashboard:

```sh
python ../../ops/bench/net_bench_dashboard.py --port COM40
```

Expected boot identity:

```text
=== Resonance net-bench cores3-bridge-2026-08-06.2 ===
role=master channel=11 frame_hz=0 hb_hz=0
mode: SERIAL BRIDGE (CoreS3; no WiFi; relaying nb-* to USB serial)
```

The USB port number is an observation, not identity. On the first Nevada City
unit the stable USB parent serial/MAC is `44:1B:F6:E3:9F:1C` and the derived
bridge ID is `E39F1C`.
