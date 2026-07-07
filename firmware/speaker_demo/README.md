# Speaker Demo

Arduino bench sketch for the Adafruit STEMMA speaker (#3885, PAM8302 amp + mini
speaker, $4.76) on the PowerFeather V2 -- candidate A in the 2026-07 noisemaker
shootout. The speaker plugs into the repurposed LED header.

The shootout feedback said square-wave `tone()` output ("nintendo sounds")
clashes with the bamboo-tree aesthetic. The #3885 is an analog amplifier, so
this sketch does not use `tone()` at all: it runs a small fixed-point synth
(39 kHz sample ISR into a 78 kHz / 9-bit LEDC PWM carrier, integer-locked at
exactly 2 carrier periods per sample so clock intermod beats stay ultrasonic,
with voice-gated high-pass TPDF dither so decaying tails neither limit-cycle
into tonal whine nor hiss between notes; the carrier A/B button probes each
amp unit's free-running class-D oscillator for audible beat products;
the amp input filtering + speaker roll off the carrier) and renders organic
percussion -- decaying-sine bamboo knocks, marimba, chime, water drip, noise
shaker -- plus one square "beep" kept as the A/B baseline against the earlier
buzzers.

The web dashboard has:

- percussion buttons: Knock, Tock, Tick, Shaker;
- tonal buttons: Marimba (random pentatonic), Chime, Drip, Beep (square baseline);
- "Ripple" -- a ~2.4 s cascade of ~20 knocks swelling through, a single-fixture
  preview of a wave passing through 150 fixtures;
- "Grove" -- free-running sparse random knocks at an adjustable events/min rate;
- Volume / Pitch (x0.25-x4) / Decay (x0.25-x4) sliders;
- "Amp power" -- toggles the switchable 3V3 header rail (GPIO4) itself, the same
  software kill-switch a production power policy would use;
- PowerFeather battery/supply telemetry (guarded charging, sway_demo pattern).

## Wiring

```text
PowerFeather 3V3 header (GPIO4-gated) -> STEMMA speaker V+
PowerFeather GND                      -> STEMMA speaker G
PowerFeather A0 / GPIO10              -> STEMMA speaker signal
```

The speaker's JST PH 3-pin plugs onto the same three header pins the LED
studios used (3V3, GND, A0). The 3V3 header rail is 0 V until GPIO4 goes HIGH;
the sketch (or the PowerFeather SDK init) enables it at boot.

If the speaker hisses at idle: the sketch already ramps the PWM to a full-low
mute after 3 s of silence, and the Amp power button cuts the rail entirely. If
it is too loud or harsh at vol 100, put 1k-10k in series with the signal pin.

## WiFi

Create `firmware/speaker_demo/wifi_secrets.h` with the shared AP credentials
(`build.sh` auto-copies it from `../sway_demo` if present):

```cpp
#pragma once

#define RES_WIFI_SSID "..."
#define RES_WIFI_PASSWORD "..."
```

Open the serial monitor at 115200 after upload; the sketch prints its IP and
advertises `http://speakerdemo.local/`. Without secrets it falls back to a
SoftAP `ResonanceSpeaker` (pw `resonance`) at `192.168.4.1`.

## Build/upload

```sh
firmware/speaker_demo/build.sh                       # compile only
firmware/speaker_demo/build.sh --port /dev/ttyACM1   # compile + USB flash
firmware/speaker_demo/build.sh --ota <ip>            # OTA re-flash
```

## HTTP API

```text
GET /                 dashboard
GET /play?snd=knock   knock|tock|tick|shaker|marimba|chime|drip|beep
                      optional &p=<pitch mul>&v=<vol mul>
GET /ripple           the cascade scene
GET /set?vol=0..100&pitch=1..100&decay=1..100&grove=0|1&grate=2..120&amp=0|1
GET /state            JSON state + battery telemetry
GET|POST /update      OTA (curl -F "firmware=@speaker_demo.ino.bin" http://<ip>/update)
```
