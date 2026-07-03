# PowerFeather demo -- ported to V2 / SDK 2.x / arduino-esp32 v3.x

A port of PowerFeather's official [powerfeather-demo](https://github.com/PowerFeather/powerfeather-demo)
(the ESPUI web-telemetry app shown in their V1 video) so it builds and runs on
**our V2 board with PowerFeather-SDK 2.1.0 and arduino-esp32 core 3.3.7**.

We ported it to use as the **reference WiFi-telemetry load** when characterizing
battery brownout behavior (it's AP-mode + ~10 Hz telemetry pushed over a
websocket -- see `docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`).

## What changed from upstream

The upstream demo targets V1 + the old SDK 1.x API + arduino-esp32 v2.x. Ported:

1. **SDK 1.x -> 2.x telemetry API.** `getSupplyVoltage(uint16_t&)` (millivolts) etc.
   became `getSupplyVoltage(float&)` (volts); currents are now `float` mA. Updated
   the `loop()` reads and scaling accordingly.
2. **`setSupplyMaintainVoltage` units.** V1 took millivolts (`4600`); V2 takes
   **volts** (`4.6`). The MPP slider value (mV) is now divided by 1000.
3. **Battery type.** `Board.init(capacity)` -> `Board.init(capacity, DEMO_BATTERY_TYPE)`,
   defaulting to `Generic_LFP` (our cell). Override with `--chem 3v7` for Li-ion.
4. **Core-3.x library stack.** Uses the **ESP32Async** forks instead of the old
   (v2.x-only) ones: `Async TCP` 3.4.x + `ESP Async WebServer` 3.11.x + `ESPUI`
   2.2.4 + `ArduinoJson` 7.x. (Upstream README warned about ESPAsyncWebServer vs
   v3.x -- these forks resolve it.)

## Build / flash

```sh
# install libs once (and remove the old AsyncTCP):
arduino-cli lib uninstall AsyncTCP
arduino-cli lib install "Async TCP" "ESP Async WebServer" "ArduinoJson" "ESPUI"

./build_demo.sh --port /dev/ttyACM0            # build + flash (LFP default)
./build_demo.sh --port /dev/ttyACM0 --chem 3v7 # for a Li-ion cell
```

`-DPOWERFEATHER_BOARD_V2=1` is mandatory (set by the script) so the SDK uses the
V2 MAX17260 gauge.

## Use

On boot it creates a WiFi **access point `PowerFeather_Demo`** (no password) and
serves the UI at **http://192.168.1.1**. Connect a phone/laptop to that AP and
open the page -- live battery + supply telemetry, plus controls for charging,
temp sense, 3V3/VSQT rails, MPP voltage, ship/deep-sleep.

Status: compiles + boots + AP comes up on V2 (verified on USB, board 9E5AB8).
Web UI and on-battery behavior still to be exercised with a phone + a battery.
