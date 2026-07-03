# Clacker Demo

Arduino bench sketch for relay/noisemaker and 8002A speaker experiments on the
Adafruit Metro ESP32-S3.

It connects to the shared WiFi AP and serves a small dashboard with:

- relay pulse buttons for A0, A1, and both;
- adjustable A/B auto-clack timing;
- Qwiic/STEMMA relay click and repeat-clack controls;
- selectable noisemaker output on D5, D6, D7, or Modulino Buzzer over I2C;
- Modulino Vibro pulse/buzz controls over I2C;
- short tone, sweep, and melody buttons for the selected noisemaker.

## Wiring

```text
Metro 3V3 or 5V -> relay module VCC pins
Metro GND       -> relay module GND pins
Metro A0        -> relay module A IN
Metro A1        -> relay module B IN

Metro STEMMA/Qwiic -> SparkFun Qwiic Omron relay
Metro STEMMA/Qwiic -> Arduino Modulino Buzzer
Metro STEMMA/Qwiic -> Arduino Modulino Vibro

Metro 3V3       -> 8002A V
Metro GND       -> 8002A G
Metro D5/GPIO5  -> 8002A S

Metro D6/GPIO6  -> passive piezo lead 1
Metro GND       -> passive piezo lead 2

Metro 3V3       -> RedBot buzzer POW
Metro GND       -> RedBot buzzer GND
Metro D7/GPIO7  -> RedBot buzzer signal
```

The relay outputs assume high-trigger relay modules: LOW is idle, HIGH energizes the
relay. If a relay module is silent with `3V3 -> VCC` and the relay can is marked
`SRD-05VDC-*`, try `5V -> VCC` from USB power, keep grounds common, and leave the MCU
pin connected to `IN`. Do not drive a bare relay coil directly from an MCU pin.

The Qwiic relay is controlled over the Metro ESP32-S3 STEMMA bus (`SDA`/GPIO47,
`SCL`/GPIO48). The firmware supports both SparkFun protocols seen on these relay boards:
the older Qwiic Single Relay at `0x18`/`0x19` and the newer TCA9555-based relay at
`0x20`/`0x21`. The dashboard reports which one was detected.

The Arduino Modulino boards are also controlled on the same STEMMA bus. The firmware uses
the Arduino Modulino protocol directly, without adding the Arduino_Modulino library:
Modulino Buzzer default firmware address `0x3C` appears on the 7-bit bus as `0x1E`, and
Modulino Vibro default firmware address `0x70` appears as `0x38`. A fallback Vibro
address `0x1D` is also probed because some Arduino docs/datasheets list `0x3A`/`0x1D`.
If the I2C boards are plugged in after boot, press `Scan I2C`.

For these noisemakers, `D5/GPIO5`, `D6/GPIO6`, and `D7/GPIO7` are PWM/tone outputs. The
dashboard drives only one selected tone output at a time. The Modulino Buzzer is an I2C
tone output and can also play the same beep/sweep/melody buttons. The 8002A README shows
classic ESP32 DAC pins GPIO25/GPIO26, but the Metro ESP32-S3 does not expose the old ESP32
DAC peripheral. PWM square-wave tone output is enough for this click/tune bench. If the
8002A module is too loud or harsh, put a 1k-10k resistor in series with `S`; a
100 ohm-1k series resistor is also fine on the passive piezo lead.

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
