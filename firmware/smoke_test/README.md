# COTS Smoke Test

Minimal Arduino smoke firmware for first-arrival COTS boards.

Targets:

- Adafruit Feather ESP32-C6 + Adafruit IS31FL3741 13x9 matrix over STEMMA-QT.
- UnexpectedMaker FeatherS2 Neo built-in 5x5 LED matrix.
- M5Stack Atom Matrix built-in 5x5 LED matrix.

The sketch prints a boot report over serial, scans I2C, runs a conservative
LED test, and can start either a home-WiFi or temporary AP-hosted web OTA
updater from the serial console.

## WiFi Secrets

For station-mode OTA, create `wifi_secrets.h` next to the sketch. That file is
ignored by git.

```cpp
#pragma once

#define RES_WIFI_SSID "your-network"
#define RES_WIFI_PASSWORD "your-password"
#define RES_WIFI_AUTO_CONNECT 0
```

Set `RES_WIFI_AUTO_CONNECT` to `1` only for bench sessions where every boot
should enter WiFi OTA maintenance mode automatically.

## Build

Use Arduino CLI with the ESP32 core.

```sh
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32c6:CDCOnBoot=cdc,PartitionScheme=min_spiffs firmware/smoke_test
arduino-cli compile --fqbn esp32:esp32:um_feathers2neo:PartitionScheme=min_spiffs firmware/smoke_test
arduino-cli compile --fqbn esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs firmware/smoke_test
```

## Flash

Current bench serial mapping:

```sh
arduino-cli upload -p /dev/ttyACM1 --fqbn esp32:esp32:adafruit_feather_esp32c6:CDCOnBoot=cdc,PartitionScheme=min_spiffs firmware/smoke_test
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:um_feathers2neo:PartitionScheme=min_spiffs firmware/smoke_test
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs firmware/smoke_test
```

## Serial Commands

- `h` or `?` — print help.
- `r` — print boot/status report again.
- `i` — run I2C scan.
- `l` — run conservative LED test.
- `c` — clear LEDs.
- `w` — connect to configured WiFi and start web OTA updater.
- `o` — start temporary AP web OTA updater.

When AP OTA mode is started, connect to the printed `resonance-smoke-*` WiFi AP
and open `http://192.168.4.1/` in a browser. When station OTA mode is started,
open the printed `http://<board-ip>/` URL.
