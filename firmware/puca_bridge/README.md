# PUCA performance-audio bridge (proof of concept)

**Status: UNVERIFIED PoC — never run on hardware.** Written 2026-08-19 while
the PUCA was not USB-enumerated; it compiles clean against esp32:esp32 3.3.7
but no register write, pin, or radio behavior below has been observed on the
board. Work the bring-up checklist before believing anything here.

Target: **PUCA DSP Original Edition** (ESP32-PICO-D4 + WM8978 codec, 8 MB
PSRAM) on the Ohmic 6 HP Eurorack expansion — the ADR 0035 primary
performance-audio bridge. Classic dual-core ESP32; this is **not** a
CoreS3/S3 binary, and Original/Strawberry edition binaries are not
interchangeable.

What it does:

- initializes the WM8978 over I2C with a minimal RX-only register set
  (mic-or-line input path into the ADC; DAC/headphone/speaker paths left
  powered down; ALC/AGC left at its power-on OFF default);
- runs I2S RX at 16 kHz mono with the ESP32 mastering MCLK/BCLK/LRCLK
  (MCLK = 256 x fs on GPIO0, matching the vendor examples);
- feeds 100 ms audio blocks to the **shared** envelope tracker
  `firmware/cores3_bridge/audio_reactive.h` (relative include, not a copy);
- tracks live fixtures from `NB_HEARTBEAT` (same 5 s window and
  `fixture-` firmware filter as the CoreS3 audio mode);
- broadcasts the existing `NB_DIRECT_FRAME` contract at ~10 Hz on fixed
  ESP-NOW channel 11 (`firmware/fixture/src/core/packet.h` via the build's
  `-I` to the firmware root — never forked).

## ADR 0035 constraints honored

- Existing ~10 Hz `NB_DIRECT_FRAME` contract only; **no new packet type**
  (ADR 0035 §6 forbids a feature packet until its semantics are specced).
- Fixture safety unchanged: frames carry the 10 s micro-lease + hard-cut
  flags (0x03) exactly like the CoreS3 bridge; silence, clipping, cable pull,
  reboot, OFF mode, or bridge loss just stops frames and the 3 s stale-frame
  fallback returns every fixture to autonomous behavior.
- No raw audio on the air.
- No WiFi AP: the factory image's "PUCA DSP" softAP does not exist here. STA
  stays unassociated, the channel is pinned to 11 and printed at boot.
- Codec/I2S/carrier specifics live in this PUCA-specific sketch; the feature
  extraction and the wire contract are the shared fleet files.

## Pin table (with provenance)

All sources fetched 2026-08-19 from
[github.com/ohmic-net/puca_dsp](https://github.com/ohmic-net/puca_dsp) @ main.

| Function | GPIO | Source | Confidence |
|---|---|---|---|
| I2C SDA (WM8978 ctrl) | 19 | [WM8978.h](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_CV_test/WM8978.h) `I2C_MASTER_SDA_IO` | High |
| I2C SCL (WM8978 ctrl) | 18 | same, `I2C_MASTER_SCL_IO` | High |
| WM8978 I2C address | 0x1A (7-bit) | same, `WM8978_ADDR`; 9-bit reg writes packed per [WM8978.cpp](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_CV_test/WM8978.cpp) `writeReg` | High |
| I2S MCLK | 0 | [esp-idf main.cpp](https://github.com/ohmic-net/puca_dsp/blob/main/puca_dsp-esp-idf/main/main.cpp) `.mclk = GPIO_NUM_0`; also CLK_OUT1 trick in the [Faust arch](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_CV_test/sine_add.cpp) | High (two sources) |
| I2S BCLK | 23 | esp-idf main.cpp `.bclk`; Faust arch `PICO_DSP` branch (`sine_add.h` sets `PICO_DSP true`) | High (two sources) |
| I2S LRCLK/WS | 25 | same two sources | High |
| I2S DOUT (to DAC, idle) | 26 | same two sources | High |
| I2S DIN (ADC to ESP32) | 27 | same two sources | High |
| KNOB1 = top pot (CV2) | 33 (ADC1_CH5) | [CV test](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_CV_test/Puca_Eurorack_CV_test.ino) `//IO33 CV2 Top Pot` | High |
| KNOB2 = bottom pot (CV3) | 34 (ADC1_CH6) | same, `//I034 CV3 Btm Pot` | High |
| CV1 jack (unused) | 32 (ADC1_CH4) | same, `//IO32 CV1 V/Oct Input` | High |
| Paw (capacitive touch) | 15 | [trigger test](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_trigger_test/Puca_Eurorack_trigger_test.ino) `#define TOUCH 15`, digital read | Pin high; **polarity UNCONFIRMED** (HIGH=touched inferred from the test's LED behavior) |
| TRIG1 / TRIG2 (unused) | 13 / 14 | trigger test; "HIGH after boot" (active low) | High |
| Onboard button (unused) | 36 | both test sketches `#define BUTTON 36` | High |
| LED1 (onboard) | 5 | trigger test | High |
| LED2 / LED3 (carrier) | 2 / 4 | trigger test; "LOW after boot" | High |
| VBAT sense (unused) | 14 via BT_LVL jumper | [main README](https://github.com/ohmic-net/puca_dsp/blob/main/README.md) — note it collides with TRIG2 | High |
| Amp/enable GPIO | — | none found in any upstream source or the [datasheet](https://github.com/ohmic-net/puca_dsp/blob/main/documentation/puca_dsp_datasheet_v1.1.pdf); the 1 W speaker driver hangs off WM8978 LOUT2/ROUT2, which this firmware leaves off | Medium (absence of evidence) |

Line-in electrical limits (main README): AC-coupled, 1 Mohm, **3.3 Vpp max
before clipping**. Never feed it a speaker output.

## Knob / button / CLI map

| Control | Function |
|---|---|
| KNOB1 (top pot) | input sensitivity: 0.25x–4x multiplier on the envelope level (log taper, 1x at center) |
| KNOB2 (bottom pot) | CLASSIC + EMBER: brightness ceiling 0–100%; HUE: hue, one full wheel per turn |
| Paw touch | next mode: CLASSIC → EMBER → HUE → OFF → … (30 ms debounce, rising edge) |
| LED1 (onboard) | lit when ESP-NOW is up |
| LED2 (carrier top) | lit while the bridge is actively publishing (mode != OFF and audio on) |

Modes: **CLASSIC** = per-slot R/G/B envelope (same slot colors as the CoreS3
bridge); **EMBER** = shared warm-white envelope (CoreS3 EMBER ratios);
**HUE** = knob-set hue, envelope drives the value; **OFF** = stop sending —
fixtures go dark on one final zero frame, then return to autonomous behavior
via the 3 s staleness + micro-lease expiry.

Serial CLI at 115200 (boot banner `=== Resonance puca-bridge 0.1.0-poc ===`,
plus a 1 Hz `puca ...` status line):

| Key | Action |
|---|---|
| `t` | one-line JSON status `{mode, level, gain, hue, peers, sendok, …}` |
| `M` | next mode (same cycle as the paw) |
| `A` | audio on/off toggle (off sends one zero frame; on re-runs the 2 s noise calibration) |
| `I` | input path toggle: onboard MEMS mics (boot default) ↔ 3.5 mm line-in |

## Build / flash

```bash
firmware/puca_bridge/build.sh                 # build only
firmware/puca_bridge/build.sh --port /dev/ttyACM?   # build + USB flash
```

FQBN `esp32:esp32:pico32` (ESP32-PICO-D4, 4 MB flash; the 8 MB PSRAM is
unused by this PoC). The script uses a unique build dir and passes
`-I<firmware root>` so `fixture/src/core/packet.h` is the one canonical wire
contract. Verify the enumerated port really is the PUCA (`lsusb`, then match
the serial device) before flashing — other bench devices also enumerate as
ttyACM.

## Bring-up checklist (all UNVERIFIED until checked on hardware)

1. Confirm the unit is Original Edition and record its USB identity and MAC
   (`hardware/puca-audio-bridge/README.md` checklist).
2. Flash; confirm the boot banner, `node id=… mac=…`, `esp-now up, ch=11`,
   `codec: WM8978 @0x1A init ok`, and `i2s: 16 kHz mono RX ready` lines.
3. Confirm no "PUCA DSP" WiFi AP exists while this firmware runs.
4. Clap test on the MEMS mics: `t` should show `level` moving after the ~2 s
   calibration. Then `I` to line-in with the RODE NTG (TRS–TRS cable, mic
   powered manually) and repeat.
5. Verify the paw: each touch prints `mode=…`. If touches misfire or invert,
   the polarity assumption is wrong — flip the comparison in `touchTick`.
6. Verify both knobs sweep `gain` 0.25→4.00 and `ceil` 0→1.00 in `t` output,
   and note whether patched CV2/CV3 cables disturb them.
7. One fixture on channel 11: confirm it follows the envelope, and that it
   returns to autonomous output within ~3 s of `M`-ing into OFF (or pulling
   power).
8. Mixed HEX/RGBW group, then packet rate / PDR / overrun / multi-hour soak,
   and the in-Pod20 range test (ADR 0035 validation list).
9. Record the working input gains (codec + RODE dial) back into
   `hardware/puca-audio-bridge/README.md`.

## Known UNVERIFIED items / assumptions

- Every WM8978 register write (values derived from the vendor driver + the
  WM8978 v4.5 datasheet; the codec is write-only so failures surface as sound,
  not errors).
- Which stereo slot the mono I2S RX captures, and which physical mic/line
  channel that is.
- Paw touch polarity and debounce feel; knob rotation direction vs ADC value.
- Whether CV2/CV3 jack inputs share the ADC nets with the pots.
- MICBEN requirement for the Knowles MEMS pair (enabled because upstream does).
- ESP-NOW TX from inside the metal Pod20 case (ADR 0035 requires a range test).
- The `esp32:esp32:pico32` profile against the real 4 MB/PSRAM module (PSRAM
  deliberately untouched here).

## Bench + schematic findings (2026-08-20/21, recovered to repo 2026-08-25)

These were established live on hardware and from the vendor schematics during
the pre-playa bench sessions but only existed in conversation until now.

### Verified on hardware (first boot, 2026-08-20)

- **Codec + MEMS mics are LIVE**: `codec=1 i2cerr=0`, envelope tracked room
  sound on the first boot (write-only WM8978 init sequence is correct).
- **Boot-into-EMBER glitch observed once** — consistent with one spurious paw
  touch at startup; firmware now ignores touches for the first 2.5 s. True paw
  polarity remains unconfirmed.
- **Knobs read zero on USB-only power — NOT a firmware bug.** The eurorack
  back-PCB schematic (`puca-eurorack/schematics/eurorack_v0.5_schematic.pdf`)
  shows the pots' reference rail and their TL072 buffer op-amps are powered
  from the EURORACK power header rails. USB alone powers only the ESP32 +
  codec + MEMS mics. **Knobs, faceplate audio ins, CV/V-OCT, and the
  amplified outs all require eurorack power.**

### Eurorack v0.5 faceplate wiring (from the schematic, 2026-08-20)

| Jack | Net | Notes |
|---|---|---|
| J1 | V/OCT -> GPIO32 | clamp diodes, dedicated pitch CV |
| J2 | CV + top pot -> GPIO33 (KNOB1) | jack and pot are **SUMMED** via op-amp mixer — a patched CV adds to the knob; knobs never go dead |
| J3 | CV + bottom pot -> GPIO34 (KNOB2) | same summing topology |
| J4 / J7 | triggers -> GPIO13 / GPIO14 | unused by this firmware |
| J5 / J6 | audio in L / R | **~4:1 op-amp attenuators (100k/24k, about -12 dB)** onto the LINE_IN_L/R nets — the same WM8978 line path as the PCB stereo jack, scaled for hot modular signals. DJ/consumer line arrives ~12 dB quieter here than via the PCB jack; the PGA + KNOB1 absorb it. |
| J8 / J9 | audio out L / R | ~18x gain back up to modular level |
| AUX pair | GPIO-header pass-through | goes to a further expansion header, wired to **no faceplate jack**; the WM8978 AUX bus can also be routed as a zero-latency analog pass-through to the outputs (unconfigured here) |

### Power / battery (datasheet v1.1 + main schematic, 2026-08-20)

- Charger is an **MCP73831**: solder the JST and a LiPo behaves as a UPS —
  runs from USB/ext 5 V while charging, seamless takeover on power loss,
  recharges on return. Charge rate is program-resistor-fixed (size the cell
  for hours of ride-through); charge termination is imprecise under load
  (known MCP73831 trait, harmless here).
- The battery-voltage sense **solder jumper shares GPIO14 with TRIG2** —
  closing it costs that trigger input (acceptable; triggers unused).

### DJ deck / RØDE NTG hookup doctrine (with `docs/research/AUDIO_INGEST_NTG_PUCA_2026-08-04.md`)

- Tap the mixer's **record out** (fixed level, master-independent) first,
  booth out second; avoid balanced XLR master outs (+4 dBu clips the codec).
- Cable: dual RCA -> dual 3.5 mm **TS mono** into the faceplate ins (Eurorack
  vocabulary: "Hosa CMR" class), or 3.5 mm TRS -> 2x RCA into the PCB stereo
  jack if the case exposes it. Never TRS into the mono faceplate jacks (ring
  shorts to sleeve), never TRRS to the NTG (flips it to smartphone mode).
- Pack a cheap RCA/3.5 mm **ground-loop isolator** (PUCA on USB power + DJ rig
  on generator = classic hum).
- NTG: gain at the mic (run its dial hot, keep the codec PGA low), powered
  from its internal battery; bench-verify it stays on into the bias-less
  line-in. Its USB-C is a computer audio interface — the PUCA/CoreS3 USB
  ports are device ports and cannot host, power, or control it.

### Firmware deltas landed 2026-08-25 (compile-verified only — NOT yet run on hardware)

- I2S capture switched MONO/LEFT-slot -> **STEREO with L+R averaged** into
  the envelope: either faceplate jack, both, or the PCB jack now behave
  identically (single-jack reads ~half level; KNOB1 absorbs it).
- Paw touches ignored for the first 2.5 s of boot (boot-EMBER guard).
