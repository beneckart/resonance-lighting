# PUCA performance audio bridge

This is the hardware and bring-up record for the dedicated performance-audio
source for Resonance Tree. The manufacturer's name is styled with an accented
`u`; this repo uses the ASCII spelling **PUCA** so Windows shells, searches, and
agent handoffs remain reliable.

**Status (2026-08-13): hardware received; Resonance firmware not yet written or
validated.** The factory Eurorack oscillator/effect firmware is not the tree
bridge.

The illustrated
[`Bridge field manual`](../../docs/howto/BRIDGE_OS_FIELD_MANUAL.md) explains
when to use the proven CoreS3 fallback, how PUCA fits beside Bridge OS, and the
operator-facing bring-up boundary. This file remains the detailed hardware
record and qualification checklist.

## What "PUCA" means in this project

PUCA means the Ohmic Limited **PUCA DSP Original Edition** mounted on its 6 HP
Eurorack expansion. It is a small original-ESP32 audio/DSP board with:

- ESP32-PICO-D4, 4 MB flash, and 8 MB PSRAM (Original Edition);
- WM8978 stereo audio codec;
- stereo 3.5 mm line input and line/headphone output on the PUCA board;
- two onboard Knowles MEMS microphones;
- USB-C programming/power and optional LiPo support;
- 8-48 kHz codec sample-rate support.

The bare-board line input is AC-coupled, 1 Mohm, and specified for no more than
3.3 V peak-to-peak before clipping. The Eurorack carrier exposes dual-mono or
stereo audio I/O plus protected control inputs. Do not confuse an AUDIO jack
with the nearby CV or TRIG jacks.

Official references:

- <https://github.com/ohmic-net/puca_dsp>
- <https://github.com/ohmic-net/puca_dsp/tree/main/puca-eurorack>
- <https://www.ohmic.net/puca-dsp>

## Hardware on hand

- PUCA DSP **Original Edition** on the Ohmic Eurorack expansion (the R110
  Original bundle).
- 4ms **Pod20 Powered** 20 HP Eurorack case.
- 4ms **45 W Power Brick**, AC mains lead, and the Eurorack ribbon supplied with
  the PUCA bundle.
- 10 HP + 4 HP blank panels around the 6 HP PUCA module.
- RODE **VideoMic NTG** directional microphone and **WS11** furry windshield.
- Multiple M5Stack CoreS3 + Module Audio stacks as an independently implemented
  fallback; see `firmware/cores3_bridge/README.md`.

The Eurorack carrier's controls are useful rather than decorative:

| Control | Hardware mapping | Intended Resonance use (not locked) |
|---|---|---|
| Top knob | CV2 / GPIO33 ADC | sensitivity or noise threshold |
| Bottom knob | CV3 / GPIO34 ADC | effect energy, speed, or band mix |
| Paw | capacitive TOUCH input | pause/resume or next scene |
| CV1-3 | protected control-voltage inputs | future performer/controller input |
| TRIG1-2 | protected trigger inputs | future footswitch or beat trigger |

The exact control assignments belong in firmware and must be shown at boot or
recorded here after they are chosen.

## Intended signal and control path

```text
RODE VideoMic NTG --analog audio--> PUCA audio input
                                      |
                                      v
                              WM8978 -> I2S DMA
                                      |
                                      v
                         ESP32 envelope / FFT / onset
                                      |
                                      v
                          ESP-NOW, fixed channel 11
                                      |
                                      v
                      fleet direct frames -> light output
```

The PUCA onboard microphones are the no-extra-cable fallback. The RODE is the
normal source for bowls, violin, singing, and other non-DJ performances because
its directionality and active variable output make gain staging repeatable.

For a DJ or mixer, use a documented record/booth output and keep the PUCA input
below its 3.3 Vpp clipping limit. Add isolation and attenuation when the source
can produce professional line-level peaks. Never feed a speaker output into the
PUCA.

The first implementation should **not transmit raw audio**. It should reuse the
proven `NB_DIRECT_FRAME` path at about 10 Hz and retain the fixture's existing
three-second stale-frame return to autonomous behavior. A compact, reusable
audio-feature packet (RMS, broad bands, onset, tempo/confidence) may be useful
later, but it is a separate packet-contract decision and is not required for
first light.

## RODE starting configuration

This is a bring-up baseline, not a measured final preset:

1. Charge the mic and turn it on manually. Do not assume the PUCA input provides
   the plug-in power needed for the RODE's automatic power behavior.
2. Install the WS11 outdoors.
3. Start flat: high-pass off, high-frequency boost off, pad off, and safety
   channel off.
4. Start the RODE output gain conservatively, exercise the loudest expected
   source, and raise it until peaks retain roughly 12-18 dB of digital headroom.
5. Keep codec gain fixed and automatic gain control off for the first bridge.
   Avoid two independent gain loops.
6. Once measured, mark or tape the RODE dial and record both RODE and codec gain
   here. For bowls, preserve the low end unless wind/handling proves otherwise.

The included RODE shock mount has a 3/8 inch socket. Use the folding boom stand,
strain-relieve the audio cable, weight the stand outdoors, and mark the legs so
they are visible at night.

## Software constraints and implementation notes

- This is an original dual-core ESP32, **not** an ESP32-S3. Use a distinct board
  profile and do not assume the CoreS3 pin map, USB behavior, or libraries.
- Original Edition and Strawberry Edition binaries are not interchangeable.
  This received Eurorack unit is the Original/PSRAM edition.
- The factory Eurorack image creates a `PUCA DSP` WiFi access point for firmware
  upload. Resonance runtime firmware must not leave that AP running. Keep the
  radio unassociated, pin ESP-NOW to channel 11, and expose the active channel at
  boot, matching the CoreS3 bridge discipline.
- Reuse platform-independent feature extraction from
  `firmware/cores3_bridge/audio_reactive.h` where practical, but keep codec,
  I2S, control-panel, and board initialization in a PUCA-specific layer.
- Reuse the canonical fleet packet definitions from
  `firmware/fixture/src/core/packet.h`; do not fork a lookalike packet struct.
- Treat the metal Pod case and its orientation as part of the RF system. The
  first field test must include range/PDR with the module installed in the case.
- Preserve a no-audio or no-radio safe state. Silence, clipping, cable removal,
  reboot, or bridge loss must not latch the fleet in a directed show.

## Bring-up checklist

- [ ] Photograph and record the installed ribbon orientation before first power;
  the red stripe belongs at `-12 V` on the Eurorack bus.
- [ ] Confirm the board label/flash size identifies the received unit as Original
  Edition and record its USB identity and WiFi/ESP-NOW MAC.
- [ ] Run the upstream trigger/CV hardware tests and verify both knobs and the paw.
- [ ] Identify and label the exact front-panel AUDIO input/cable used by the RODE;
  record whether it reaches WM8978 LINE or AUX and the required gain.
- [ ] Log unclipped RODE input levels for bowls, violin, and singing, plus the
  onboard-microphone fallback.
- [ ] Create `firmware/puca_bridge/` with a named, reproducible build profile.
- [ ] Prove one fixture receives direct frames on channel 11 and returns to
  autonomous output within three seconds after PUCA transmission stops.
- [ ] Repeat on a mixed HEX/RGBW group, then measure packet rate, PDR, CPU load,
  audio overruns, and multi-hour stability.
- [ ] Range-test the PUCA in the closed Pod20 at the intended tree/Shiftpod
  placement and orientation.
- [ ] Record the final RODE gain, codec gain, knob functions, startup sequence,
  and known-good firmware artifact in this file.

## What PUCA is not

- It is not one of the roughly 130 solar fixture controllers.
- It is not the CoreS3 desk bridge, though both can publish to the same fleet.
- It is not a required coordinator: fixtures remain autonomous when it is absent.
- It is not yet production-validated merely because the hardware has arrived.
