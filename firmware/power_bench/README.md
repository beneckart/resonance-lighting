# Power Bench (PowerFeather V2)

Arduino firmware that turns an **ESP32-S3 PowerFeather V2** into a WiFi power-
telemetry bench for the Resonance Lighting workstream. It reads battery and
supply telemetry through the PowerFeather SDK and serves it as JSON at
`/telemetry`, so a host poller (`ops/bench/power_logger.py`) can log power data
over WiFi across three test axes: **battery × LED option × solar panel**.

Forked from `firmware/smoke_test`; LED measurement modes (`0`-`5`, `q`) and the
web-OTA scaffolding are unchanged.

## Toolchain (verified 2026-06-02)

- `arduino-cli` 1.4.1, ESP32 Arduino core `esp32:esp32` **3.3.7**.
- **FQBN:** `esp32:esp32:esp32s3_powerfeather`
- **Board macro:** `ARDUINO_ESP32S3_POWERFEATHER` (compile-time board detection)
- **SDK:** `PowerFeather-SDK` **2.1.0** — `arduino-cli lib install "PowerFeather-SDK@2.1.0"`
  - namespace `PowerFeather`, singleton `PowerFeather::Board`, header `<PowerFeather.h>`
- LED libs (already installed): Adafruit NeoPixel, IS31FL3741, GFX, BusIO.

## Battery chemistry — software only, no jumpers

Chemistry and capacity are set entirely in firmware via the SDK; there are no
jumpers or solder bridges. Override at build time:

| Macro | Default | Meaning |
|---|---|---|
| `RES_PF_BATTERY_CAPACITY_MAH` | `2000` | Cell capacity in mAh — **set to your actual cell** |
| `RES_PF_BATTERY_TYPE` | `Mainboard::BatteryType::Generic_3V7` | `Generic_3V7` Li-ion/LiPo; `Generic_LFP` LiFePO4; `ICR18650_26H` / `UR18650ZY` (capacity ignored, 2600 mAh) |
| `RES_PF_ENABLE_CHARGING` | `1` | Call `enableBatteryCharging(true)` (SDK leaves charging **off** by default) |
| `RES_PF_MAX_CHARGE_MA` | `1000.0` | Charge-current cap (mA), 40-2000. Charger self-limits to what the supply gives. <=0.5C for >=2000 mAh cells; lower (`--charge-ma`) for smaller cells |
| `RES_PF_MAINTAIN_V` | `4.6` | Supply maintain / charger VINDPM (V). Set to panel MPP for solar runs (4.6-16.8 V) |

Switching Li-ion → LiFePO4 is a one-line change to
`RES_PF_BATTERY_TYPE=Generic_LFP` (and the matching capacity).

## LED options (build variants)

LED option is chosen at build time, like smoke_test's `RES_ATOM_GROVE_NEOHEX`:

| Build flag | LED module | Driver / pin |
|---|---|---|
| *(none)* | none (telemetry-only bring-up) | — |
| `RES_PF_LED_NEOHEX` | M5Stack NeoHEX 37px | WS2812 `NEO_GRB`, data GPIO16 (D6) |
| `RES_PF_LED_RGBW1` | single high-power SK6812 RGBW | `NEO_GRBW`, data GPIO16 (D6) |
| `RES_PF_LED_IS31` | IS31FL3741 13x9 | I2C over STEMMA-QT (see caveat) |

**Physical / Phase-B caveats (confirm before wiring):**
- NeoHEX/RGBW data pin is GPIO16 (D6) by default; the WS2812 power rail (VSQT
  3.3 V vs Vbat) needs choosing per module current. Verify against the
  PowerFeather pinout.
- On PowerFeather V2 the STEMMA-QT bus is GPIO47/48 (`Wire1`, port 1), which the
  SDK uses for the charger/fuel-gauge. The IS31 driver currently begins on the
  default `Wire`; routing it onto the STEMMA-QT bus (shared with the SDK) is an
  open Phase-B item.

## Build + flash

Use `build.sh` — it **always passes `-DPOWERFEATHER_BOARD_V2=1`**, which is
REQUIRED: the SDK picks the fuel gauge at compile time, and without this flag it
silently uses the V1 `LC709204F` instead of the V2 `MAX17260`, so SOC/health/cycles
fail. The sketch has a `#error` guard so a bare `arduino-cli compile` will refuse to
build.

```sh
# IS31FL3741 13x9, 4400 mAh Li-ion, build + USB flash
./build.sh --led is31 --cap 4400 --port /dev/ttyACM0

# Same, but flash WIRELESSLY over WiFi (no USB) -- for deployed/outdoor harnesses
./build.sh --led is31 --cap 4400 --ota 192.168.4.185

# NeoHEX, 1500 mAh LiFePO4, no charging (clean LED-current run), build only
./build.sh --led neohex --cap 1500 --chem lfp --no-charge

# options: --led is31|neohex|rgbw1|none  --cap MAH  --chem 3v7|lfp
#          --charge-ma MA  --no-charge  --maintain VOLTS
#          --port /dev/ttyACM0   (USB)  |  --ota <board-ip>   (WiFi)
```

### OTA (wireless) flashing

`--ota <ip>` compiles, then POSTs the binary to the firmware's web `/update`
endpoint over WiFi — no USB. Keep USB as the recovery path: if an OTA build is
broken, re-flash once over USB. The board must be on WiFi and serving (it is at
boot when `RES_WIFI_AUTO_CONNECT=1`); a board put into mode `q` drops WiFi and
can't be OTA'd until reset.

Equivalent raw command (if not using build.sh, the V2 flag is mandatory):

```sh
arduino-cli compile -u -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3_powerfeather \
  --build-property "compiler.cpp.extra_flags=-DPOWERFEATHER_BOARD_V2=1 -DRES_PF_LED_IS31=1 -DRES_PF_BATTERY_CAPACITY_MAH=4400" \
  firmware/power_bench
```

## WiFi

For untethered telemetry, copy `wifi_secrets.h.example` to `wifi_secrets.h`
(gitignored) and set `RES_WIFI_AUTO_CONNECT 1` so each boot joins WiFi and starts
the web server automatically.

## Endpoints

- `GET /telemetry` — JSON: `battery_v`, `battery_ma` (− discharge / + charge),
  `soc_pct`, `health_pct`, `cycles`, `time_left_min`, `supply_v`, `supply_ma`,
  `supply_good`, plus `board`, `fw`, `fixture_id`, `led_option`, `led_mode`,
  `uptime_ms`, `heap_free`, `reset_reason`, `pf_ready`, `battery_type`, and a
  `telemetry_errors` array (fields that returned non-`Ok`).
- `GET /mode?m=<0-5|q>` — set LED measurement mode.
- `GET /` — status page + OTA upload form.
- `POST /update` — multipart firmware upload (web OTA).

## Serial commands

`h` help · `r` report · `t` telemetry JSON · `i` I2C scan · `0`/`c` LEDs off ·
`1`-`5` LED modes · `q` quiet baseline · `w` join WiFi + web server · `o` AP web server.

## Notes

- The SDK getters can block ~100 ms each on the charger ADC (supply V/I), so a
  `/telemetry` response can take a few hundred ms — fine for polling.
- Continuous WiFi inflates active current vs the production ESP-NOW + light-sleep
  duty cycle. For LED-current runs, subtract the mode-`0` baseline. For autonomy/
  solar runs, record the WiFi-on confound in run notes.
- Use mode `0` (LEDs off, radio on) as the logged baseline, NOT mode `q` — `q`
  stops WiFi (a USB-meter-only idle baseline) and drops the board off the network.
- Fuel-gauge SOC/health/cycles require the `-DPOWERFEATHER_BOARD_V2=1` build flag
  (see above) so the SDK uses the MAX17260; build.sh sets it. With the flag,
  `soc_pct/health_pct/cycles/time_left_min` populate. SOC may read low/rough until
  the gauge learns over a charge/discharge cycle.
- The default 200 mA charge current dominates and masks small LED-current deltas;
  build `-DRES_PF_ENABLE_CHARGING=0` for clean LED measurement on battery discharge.
