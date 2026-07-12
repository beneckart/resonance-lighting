# Solenoid Demo

PowerFeather V2 bench sketch for the noisemaker shootout, candidate B: a 3 V
mini solenoid bamboo-strike, driven through an Adafruit MOSFET driver board off
the standard fixture LED header (switchable 3V3 rail + GND + A0). Candidate A
(STEMMA speaker percussion synth) lives in `../speaker_demo`.

## What this bench answers

The switchable 3V3 header rail is not stiff (~2.96-2.97 V at ~290 mA, LOG
2026-07-02), and a small solenoid pull-in is a hundreds-of-mA-to-~1 A pulse.
Open questions this sketch exists to measure:

- Minimum pulse width for a reliable strike (fixed-width test buttons sweep
  10-120 ms).
- Rail/MCU stability during and after strikes -- on USB first, then on battery
  (watch `reset_reason` and the strike/failsafe counters across a session).
- Whether repeated strikes (burst / auto mode) upset the charger/gauge or WiFi.

## Wiring

```text
PowerFeather 3V3 header (GPIO4-gated) -> MOSFET driver load supply / solenoid +
PowerFeather GND                      -> MOSFET driver ground
PowerFeather A0 / GPIO10              -> MOSFET driver signal (gate) input

solenoid across the MOSFET driver's load output
```

A flyback diode across the coil is MANDATORY -- the Adafruit driver board has
one on-board; check it is populated before the first strike. Do not drive a
bare coil from the GPIO.

## Coil safety in firmware

- Every pulse is ended by an esp_timer one-shot AND a loop() failsafe deadline
  (`failsafes` in `/state` should stay 0).
- Pulse width is hard-clamped to 5-300 ms; a minimum 80 ms coil-rest gap is
  enforced between strikes.
- The gate pin is driven LOW first thing in `setup()`, before the SDK powers
  the 3V3 rail, and forced LOW when an OTA upload starts.
- The dashboard "Coil power" button toggles the 3V3 header rail itself via
  `Board.enable3V3()` -- the production-style hard kill.

## Dashboard

`http://solenoiddemo.local/` (or the IP from the serial banner at 115200):
STRIKE button, fixed-width test strikes, double/burst patterns, auto-repeat
with interval + pulse-width sliders, coil power toggle, strike/blocked/failsafe
counters, and the usual battery/charger telemetry. If the shared AP is
unreachable it falls back to SoftAP "ResonanceSolenoid" (pw `resonance`) at
`192.168.4.1`.

HTTP API: `/strike[?ms=N]`, `/burst?n=N`, `/stop`,
`/set?pulse=&interval=&auto=&coil=`, `/state`, `/update` (OTA).

## WiFi

Create `firmware/solenoid_demo/wifi_secrets.h` with the shared AP credentials
(`build.sh` copies it from a sibling sketch if missing):

```cpp
#pragma once

#define RES_WIFI_SSID "..."
#define RES_WIFI_PASSWORD "..."
```

## Build/flash

```sh
firmware/solenoid_demo/build.sh                        # compile only
firmware/solenoid_demo/build.sh --port /dev/ttyACM1    # first flash over USB
firmware/solenoid_demo/build.sh --ota <ip>             # OTA thereafter
```

Builds with `-DPOWERFEATHER_BOARD_V2=1` (mandatory -- see
`../POWERFEATHER_NOTES.md`). `--pin N` overrides the gate pin (default
GPIO10/A0).
