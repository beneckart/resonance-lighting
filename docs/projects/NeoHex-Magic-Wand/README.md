# NeoHex-Magic-Wand

Steve Eckart's portable 20-NeoHex light is a truncated icosahedron: one M5Stack
NeoHex A045-B on each of the 20 hexagonal faces and no LEDs on the 12 pentagons.
It is a one-off Resonance fleet peer, not one of the production lantern classes.

## Field identity

- Fixture ID: `F40344`
- WiFi MAC: `68:EE:8F:F4:03:44`
- ESP-NOW channel: 11
- PowerFeather V2 data pin: A0 / GPIO10
- Sensors on Wire1 at 100 kHz: MSA311 (`0x62`) and BMP581 (`0x47`)
- Pixel count: 20 boards x 37 pixels = 740 WS2812 pixels
- Default animation: a low-light red row moves bottom-to-top through four
  five-board rows every 0.4 seconds; inactive rows are orange, yellow, green,
  and blue.

## Power and signal topology

The PowerFeather supplies data and fleet/OTA behavior. It does not power the
NeoHex LED rail.

```
Gotion 33140 15 Ah LFP
  -> BatterySpace 1S 10 A PCM (B+/B- cell, P+/P- protected bus)
     -> PowerFeather battery JST (charging and controller power)
     -> Pololu U3V70F5 VIN/GND
        -> regulated 5.1 V LED bus
           -> four separately fused injection pairs
              -> Hex 1, Hex 6, Hex 11, Hex 16

PowerFeather A0/GPIO10
  -> 74AHCT125 level shifter powered from 5.1 V
  -> 330 ohm series resistor
  -> Hex 1 DATA IN
  -> one continuous DATA chain through Hex 20
```

All LED grounds and the PowerFeather ground are common. The Pololu toggle
connects `ENABLE` to GND for OFF and leaves it open for ON.

The NeoHex Grove wire convention used on this wand is:

- black = GND
- red = +5 V
- yellow = DATA
- white = unused

Hex numbering follows four geometric rows:

- Row 1, around the bottom pentagon: Hex 1-5
- Row 2: Hex 6-10
- Row 3: Hex 11-15
- Row 4, around the top pentagon: Hex 16-20

DATA and GND stay continuous across the entire chain. At the three power-zone
boundaries (Hex 5 -> 6, Hex 10 -> 11, and Hex 15 -> 16), the Grove red +5 V
conductor is cut and individually insulated. This prevents neighboring fused
zones from being paralleled through the small Grove conductors.

## Firmware

The fleet role is selected at compile time:

```sh
./firmware/net_bench/build.sh --role peer --channel 11 --magic-wand
```

`--magic-wand` implies the sensor diagnostic and these fixed first-boot safety
defaults:

- Generic LiFePO4
- 15,000 mAh gauge capacity
- 500 mA maximum charge current
- 4.6 V supply-maintain setting

The renderer is in `firmware/net_bench/magic_wand_mode.h`. It uses four
ESP32-S3 RMT TX memory blocks; one block under-ran once the commissioning chain
grew beyond one NeoHex. The role blanks the external LED string before WiFi
maintenance or deep sleep, rejects the ordinary 37-pixel drawdown command, and
restores the default pattern when ESP-NOW communications resume.

`firmware/magic_wand_test/magic_wand_test.ino` preserves the standalone bench
test used to commission the 1-, 2-, 5-, 10-, and 20-board chain. It includes
full RGB fills, a complete-chain white chase, a per-board LED-1 orientation
test, and the animation prototypes.

## Validated hardware state on 2026-08-19

- All 20 boards passed full red, green, and blue fills.
- A single white chase crossed every board and every isolated power boundary.
- The default pattern starts automatically on battery-only power.
- All four injection zones operate without visible flicker or resets.
- MSA311 and BMP581 passed live reads.
- Cell, PCM, PowerFeather, Pololu, fuses, WAGOs, level shifter, sensors, and
  wiring remained cool.
- Pololu output measured 5.1 V; protected battery and PowerFeather battery
  measurements were about 3.3 V during commissioning.

## Deployed image and OTA caveat

The image physically installed on the wand at handoff is:

- Network version: `net-bench-2026-08-19.1`
- Wand version: `magic-wand-2026-08-19.1`
- Binary bytes: 1,058,448
- SHA-256:
  `2617A33C47FE526AC01840149F091812DCDE37723D52C7281F07B7B273FFAB0B`

Steve completed a shared-WiFi OTA upload/reboot bench test and saw the wand
rejoin, restart its pattern, and return live wand/sensor telemetry. That test
predated the current immutable-artifact completion contract. Treat it as a
successful transport/reboot test, not as proof of the full production
pending-verify/A-B rollback gate.

The source on this branch is the later `.2` port onto Ben's current `main`; it
is compile-checked but is not the image currently installed. Before replacing
the working playa image, Ben should build one immutable artifact, record its
manifest/SHA, target only MAC `68:EE:8F:F4:03:44`, use the installed LFP for OTA
ride-through, and verify fresh post-reboot telemetry after the pending window.

## Open work

- Map MSA311 movement and BMP581 relative elevation into intentional pattern
  changes; readings are presently telemetry only.
- Replace the Tennessee maintenance WiFi profile with the agreed playa router
  profile before attempting shared-WiFi OTA on site.
- Mechanically strain-relieve and insulate every board, fuse, WAGO, PCM, and
  battery connection before sustained handheld use.
