# PUCA performance audio bridge

This is the hardware and bring-up record for the dedicated performance-audio
source for Resonance Tree. The manufacturer's name is styled with an accented
`u`; this repo uses the ASCII spelling **PUCA** so Windows shells, searches, and
agent handoffs remain reliable.

**Status (2026-08-27): the standalone powered-Pod20 `0.4.1-dev` baseline and
credentialed `0.5.0-dev` USB bootstrap/no-hold safe boot/exact-target OTA path
pass on the received PUCA.** The installed behavior boots SAFE-IDLE and emits no
lighting frames unless the capacitive paw is held for 1.2 s during boot; an
armed boot starts in DJ mode + line input. PUCA advertises its exact `A4EB10`
identity to Bridge OS and accepts only exact-target shared-WiFi maintenance,
never a fleet-wide request or factory-style softAP. Exact waveform/light
fidelity, the paw-held DJ boot, full knob sweeps, stale fallback, rollback,
multi-hour stability, and intended-placement RF/PDR remain open.

The illustrated
[`Bridge field manual`](../../docs/howto/BRIDGE_OS_FIELD_MANUAL.md) explains
when to use the proven CoreS3 fallback, how PUCA fits beside Bridge OS, and the
operator-facing bring-up boundary. This file remains the detailed hardware
record and qualification checklist.

The DG1022Z ceremony-waveform path has its own no-human bench procedure and
explicit isolation boundary in
[`DG1022Z -> PUCA heartbeat input`](../../docs/howto/DG1022Z_PUCA_HEARTBEAT.md).

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

| Control | Hardware mapping | Current Resonance use |
|---|---|---|
| Top knob | CV2 / GPIO33 ADC | input sensitivity, 0.25x-4x |
| Bottom knob | CV3 / GPIO34 ADC | brightness ceiling; hue only in HUE mode |
| No paw hold at boot | capacitive TOUCH input | SAFE-IDLE: identity/maintenance only, no lighting frames |
| Paw held 1.2 s at boot | capacitive TOUCH input | arms DJ + line and opens setup; short touch cycles four live modes, long hold locks |
| Paw after locked boot | capacitive TOUCH input | status display only; cannot arm/change/stop the performance |
| CV1-3 | protected control-voltage inputs | future performer/controller input |
| TRIG1-2 | protected trigger inputs | future footswitch or beat trigger |

Normal no-laptop startup is SAFE-IDLE + line input + LOCKED. Hold the paw during
boot to arm line input + DJ and open setup. The bottom carrier
LED reports one/two long pulses for line/mic followed by one to four short
pulses for DJ/HEARTBEAT/EMBER/HUE. OFF is deliberately absent from the paw
cycle.

## Power-source boundary

USB-C or a LiPo can keep the PUCA main PCB's ESP32, codec/onboard-audio path,
radio, and recovery interface alive. It does not replace the Pod20's Eurorack
rails for the installed carrier's complete analog audio, CV, trigger, and panel
path. The Pod20 is therefore the normal operational supply; USB is the rescue
and serial path.

The optional main-board LiPo could provide a brief PUCA-only control-plane ride-
through, but it cannot keep the externally powered tree or full Eurorack signal
chain operating after their power is lost. The current installation therefore
does not require soldering the optional JST battery header. Revisit only if a
measured requirement emerges for PUCA telemetry/OTA ride-through independent of
the Pod20.

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
  upload. Resonance runtime never creates that AP. COMMS remains unassociated on
  channel 11; only exact-target `A4EB10` maintenance may stop publishing, leave
  ESP-NOW, and join the shared maintenance WiFi for standard A/B OTA.
- Reuse platform-independent feature extraction from
  `firmware/cores3_bridge/audio_reactive.h` where practical, but keep codec,
  I2S, control-panel, and board initialization in a PUCA-specific layer.
- Reuse the canonical fleet packet definitions from
  `firmware/fixture/src/core/packet.h`; do not fork a lookalike packet struct.
- Chunk `NB_DIRECT_FRAME` output across the full sorted live census. One packet
  carries only 18 fixtures; silently limiting a publisher to that first packet
  is not a fleet implementation.
- Treat the metal Pod case and its orientation as part of the RF system. The
  first field test must include range/PDR with the module installed in the case.
- Preserve a no-audio or no-radio safe state. Silence, clipping, cable removal,
  reboot, or bridge loss must not latch the fleet in a directed show.

## Bring-up checklist

- [ ] Photograph and record the installed ribbon orientation before first power;
  the red stripe belongs at `-12 V` on the Eurorack bus.
- [x] Confirm the board label/flash size identifies the received unit as Original
  Edition and record its USB identity and WiFi/ESP-NOW MAC. DONE 2026-08-26;
  details are below.
- [ ] Run the upstream trigger/CV hardware tests and verify both knobs and the paw.
- [ ] Identify and label the exact front-panel AUDIO input/cable used by the RODE;
  record whether it reaches WM8978 LINE or AUX and the required gain.
- [ ] Log unclipped RODE input levels for bowls, violin, and singing, plus the
  onboard-microphone fallback.
- [x] Create `firmware/puca_bridge/` with a named build profile and native
  audio/control tests. The development build reports binary SHA-256; promotion
  under ADR 0040 remains gated on hardware acceptance.
- [x] Prove powered-Pod20 stereo capture, locked-paw behavior, channel-11
  census, and more-than-18-fixture sender chunking without reported errors.
  DONE 2026-08-26 with `0.4.1-dev`; receiver-side light proof remains separate.
- [x] USB-bootstrap credentialed `0.5.0-dev` and prove no-hold SAFE-IDLE,
  `A4EB10` heartbeat in Bridge OS, exact-target maintenance, shared-WiFi OTA,
  post-OTA dark reboot, and pending-verify survival. DONE 2026-08-27; exact
  evidence is below.
- [ ] Prove the remaining ADR 0063 gates: paw-held DJ-first arming and setup
  gestures, `/resume`/10-minute timeout, fleet-wide-maintenance rejection on
  hardware, no softAP, and forced-self-test A/B rollback.
- [ ] Prove one fixture receives direct frames on channel 11 and returns to
  autonomous output within three seconds after PUCA transmission stops.
- [ ] Repeat on a mixed HEX/RGBW group, then measure packet rate, PDR, CPU load,
  audio overruns, and multi-hour stability.
- [ ] Range-test the PUCA in the closed Pod20 at the intended tree/Shiftpod
  placement and orientation.
- [ ] Record the final RODE gain, codec gain, knob functions, startup sequence,
  and known-good firmware artifact in this file.

## Powered-Pod20 acceptance (2026-08-26)

- Power: the PUCA ribbon red stripe was aligned to the PCB `-12 V` marking;
  Pod20 `+12 V`, `-12 V`, and `+5 V` LEDs were steady. USB and Eurorack power
  were present together for programming/telemetry and faceplate-rail operation.
- Target identity: Silicon Labs CP2102N
  `USB\\VID_10C4&PID_EA60\\0EC45B486617EC1183509E9D47486EB0`; ESP32-PICO-D4
  revision 1.0, 4 MB embedded flash, MAC `4C:75:25:A4:EB:10`, short ID `A4EB10`.
  Do not confuse it with the ESP32-S3 currently enumerating separately as
  `COM152`.
- Recovery image before the first Resonance flash:
  `firmware/puca_bridge/build/puca-bridge-20260826-standalone-heartbeat-v040-c1/preflash-backup/puca-com154-pre-v040-full-4mb.bin`,
  exactly 4,194,304 bytes, SHA-256
  `c4f67d01dc1c001e1e6342f7b3b604f77b5f6f701ee4962b3bb94dd5e3d0bfcf`.
- Installed development candidate:
  `firmware/puca_bridge/build/puca-bridge-20260826-standalone-heartbeat-v041-c2/puca_bridge.ino.bin`,
  964,752 bytes, SHA-256
  `e8ec74680564f96f10c2f6e87b37eb807b9d9ba3b355ccf41c72f8301c4984b6`.
  Flash writes and read-back hashes verified. This is a known recovery identity,
  not a promoted show artifact.
- Runtime: booted HEARTBEAT, line input, controls LOCKED, codec and I2S ready,
  ESP-NOW channel 11. A normal paw touch printed `paw=status only` and left the
  mode unchanged. The powered knobs produced stable nonzero readings.
- Radio soak: over 207 s the live census grew to 70+ eligible fixtures and the
  sender reached 8,318 successful callbacks with `sendfail=0`, `readfail=0`,
  `rxdrop=0`, `i2cerr=0`, and zero clipped blocks. This proves local full-census
  packet production; visible receiver application and field-distance PDR are
  still separate acceptance gates.
- Cleanup: USB service key `A` sent the final eight black chunks, changed status
  to `active=0`, and left the send counters stable. The bridge is paused for the
  bench handoff; the next power cycle restores active HEARTBEAT + line input.

## Safe-boot and OTA acceptance (2026-08-27)

- Built credentialed `0.5.0-dev` with the explicit ESP32-PICO-D4 default dual-
  app partition layout. Application identity:
  `firmware/puca_bridge/build/puca-bridge-20260827-ota-safe-v050-bootstrap-r3/puca_bridge.ino.bin`,
  1,024,128 bytes, SHA-256
  `1e90f6f1731a622b11274fa91abbc6eeebb17c35abe90bd86337c915cb99e8da`.
- The received CP2102N automatic reset did not enter the ROM downloader. There
  is no exposed BOOT button: the visible onboard button is GPIO36. With stable
  Pod20 and USB power, the proven rescue was a normal jumper from `RST` to `GND`,
  DTR/download asserted, release `RST`, then esptool `--before no_reset`. All
  four flash regions completed esptool hash verification. Never use a meter in
  ammeter mode as the jumper and never short `VIN` or `VDD`.
- Fourteen consecutive no-hold status samples reported DJ selected but
  `active=0`, `bootarmed=0`, controls locked, codec ready, and `frames=0`.
  Heartbeat callbacks alone increased, as intended.
- Primary Bridge OS `8EB508` received fresh channel-11 heartbeats for publisher
  `A4EB10`, revision `puca-bridge-0.5.0-dev`.
- The host sent only `UA4EB10`. PUCA joined shared WiFi, exposed identity-
  matching `/telemetry`, and accepted the retained 1,024,128-byte application.
  The upload was recorded in
  `ops/bench/data/ca/2026-08-27-ota-results.jsonl`.
- After OTA, the dashboard observed a fresh expected-revision heartbeat with a
  software reset and reset uptime/sequence after the 25 s survival gate. A
  separate USB status request caused another ordinary reset and again reported
  SAFE-IDLE, locked controls, healthy codec, and zero direct frames.

This accepts the routine enclosed-update path. The internal USB connector can
remain behind the faceplate, but must remain physically accessible for rescue.

## What PUCA is not

- It is not one of the roughly 130 solar fixture controllers.
- It is not the CoreS3 desk bridge, though both can publish to the same fleet.
- It is not a required coordinator: fixtures remain autonomous when it is absent.
- It is not yet production-validated merely because the hardware has arrived.
