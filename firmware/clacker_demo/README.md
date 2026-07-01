# Clacker Demo

Arduino bench sketch for relay/noisemaker and 8002A speaker experiments on the
Adafruit Metro ESP32-S3.

It connects to the shared WiFi AP and serves a small dashboard with:

- relay pulse buttons for A0, A1, and both;
- adjustable A/B auto-clack timing;
- short tone, sweep, and melody buttons for an 8002A amplifier/speaker module.

## Wiring

```text
Metro 3V3 or 5V -> relay module VCC pins
Metro GND       -> relay module GND pins
Metro A0        -> relay module A IN
Metro A1        -> relay module B IN

Metro 3V3       -> 8002A V
Metro GND       -> 8002A G
Metro D5/GPIO5  -> 8002A S
```

The relay outputs assume high-trigger relay modules: LOW is idle, HIGH energizes the
relay. If a relay module is silent with `3V3 -> VCC` and the relay can is marked
`SRD-05VDC-*`, try `5V -> VCC` from USB power, keep grounds common, and leave the MCU
pin connected to `IN`. Do not drive a bare relay coil directly from an MCU pin.

For the speaker module, `D5/GPIO5` is a PWM/tone output. The 8002A README shows classic
ESP32 DAC pins GPIO25/GPIO26, but the Metro ESP32-S3 does not expose the old ESP32 DAC
peripheral. PWM square-wave tone output is enough for this click/tune bench. If the
module is too loud or harsh, put a 1k-10k resistor in series with `S`.

## WiFi

Create `firmware/clacker_demo/wifi_secrets.h` with the shared AP credentials:

```cpp
#pragma once

#define RES_WIFI_SSID "..."
#define RES_WIFI_PASSWORD "..."
```

The local `wifi_secrets.h` file is ignored by git. This checkout has the BubbyNet
bench credentials in that ignored file.

Open the serial monitor at 115200 baud after upload. The sketch prints its IP address
and also tries to advertise:

```text
http://clacker.local/
```

## Build/upload

Use the helper script, which builds through a dedicated path so Arduino's shared compile
cache does not collide with other bench builds:

```sh
firmware/clacker_demo/build.sh
firmware/clacker_demo/build.sh /dev/ttyACM1
```

Equivalent direct commands:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:adafruit_metro_esp32s3 \
  --build-path firmware/clacker_demo/build/metro-esp32s3 \
  firmware/clacker_demo

arduino-cli upload \
  -p /dev/ttyACM1 \
  --fqbn esp32:esp32:adafruit_metro_esp32s3 \
  --input-dir firmware/clacker_demo/build/metro-esp32s3 \
  firmware/clacker_demo
```
