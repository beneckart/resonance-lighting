# PUCA performance-audio bridge

**Status: powered-Pod20 hardware baseline passed 2026-08-26; waveform and show
qualification remain open.** Development firmware `0.4.1-dev` now boots on the
received PUCA into standalone HEARTBEAT + line input with controls locked. The
WM8978, stereo I2S capture, faceplate paw, powered knob ADCs, channel-11 receive,
ADR 0040 `fx-*` fixture filtering, and more-than-18-fixture transmit chunking all
ran on hardware. A 207 s soak reached 70+ eligible fixtures and 8,318 successful
send callbacks with zero send failures, audio read failures, receive drops, I2C
errors, or clipped blocks. The performer's DG1022Z waveform, visible fixture
response/stale fallback, full knob sweeps, mixed-output fidelity, multi-hour
stability, and field-range geometry remain unverified.

Target: **PUCA DSP Original Edition** (ESP32-PICO-D4 + WM8978 codec, 8 MB
PSRAM) on the Ohmic 6 HP Eurorack expansion -- the ADR 0035 primary
performance-audio bridge. Classic dual-core ESP32; this is **not** a
CoreS3/S3 binary, and Original/Strawberry edition binaries are not
interchangeable.

What it does:

- initializes the WM8978 over I2C with a minimal RX-only register set
  (mic-or-line input path into the ADC; DAC/headphone/speaker paths left
  powered down; ALC/AGC left at its power-on OFF default);
- runs stereo I2S RX at 16 kHz with the ESP32 mastering MCLK/BCLK/LRCLK,
  reports raw peak/clipping telemetry, and averages L+R for the envelope
  (MCLK = 256 x fs on GPIO0, matching the vendor examples);
- feeds 100 ms audio blocks to the **shared** envelope tracker
  `firmware/cores3_bridge/audio_reactive.h` (relative include, not a copy);
- tracks live fixtures from `NB_HEARTBEAT` (same 5 s window and ADR 0040
  `fx-*`/legacy fixture-identity filter as the CoreS3 audio mode);
- broadcasts the existing `NB_DIRECT_FRAME` contract at ~10 Hz on fixed
  ESP-NOW channel 11 (`firmware/fixture/src/core/packet.h` via the build's
  `-I` to the firmware root -- never forked), chunking the sorted live census
  across 18-entry packets so the publisher is not capped at 18 fixtures.
- provides a **HEARTBEAT** line-input look for deterministic generator
  waveforms. It follows raw block peak instead of the room-audio noise learner,
  so a continuously repeating waveform is not calibrated away. Narrow pulses
  produce a shared deep-red fleet pulse with a fast release.

## ADR 0035 constraints honored

- Existing ~10 Hz `NB_DIRECT_FRAME` contract only; **no new packet type**
  (ADR 0035 item 6 forbids one until its semantics are specified).
- Fixture safety unchanged: frames carry the 10 s micro-lease + hard-cut
  flags (0x03) exactly like the CoreS3 bridge. OFF/audio-pause sends a final
  zero frame and stops publishing; reboot or bridge loss also stops frames, so
  the 3 s stale-frame fallback returns fixtures to autonomy. While publishing,
  silence produces zero-valued frames; automatic cable-loss detection is not
  yet claimed.
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
| VBAT sense (unused) | 14 via BT_LVL jumper | [main README](https://github.com/ohmic-net/puca_dsp/blob/main/README.md) -- note it collides with TRIG2 | High |
| Amp/enable GPIO | -- | none found in any upstream source or the [datasheet](https://github.com/ohmic-net/puca_dsp/blob/main/documentation/puca_dsp_datasheet_v1.1.pdf); the 1 W speaker driver hangs off WM8978 LOUT2/ROUT2, which this firmware leaves off | Medium (absence of evidence) |

Line-in electrical limits (main README): AC-coupled, 1 Mohm, **3.3 Vpp max
before clipping**. Never feed it a speaker output.

## Knob / button / CLI map

| Control | Function |
|---|---|
| KNOB1 (top pot) | input sensitivity: 0.25x-4x multiplier on the envelope level (log taper, 1x at center) |
| KNOB2 (bottom pot) | CLASSIC + HEARTBEAT + EMBER: brightness ceiling 0-100%; HUE: hue, one full wheel per turn |
| Paw touch, normal boot | status display only; it cannot change or stop the performance |
| Paw held continuously at boot | opens a 20 s setup window after a 1.2 s hold |
| Paw short touch in setup | HEARTBEAT -> CLASSIC -> EMBER -> HUE -> HEARTBEAT; OFF is excluded |
| Paw long hold in setup | confirms the selection and locks immediately; inactivity also locks after 20 s |
| LED1 (onboard) | lit when ESP-NOW is up |
| LED2 (carrier top) | lit while the bridge is actively publishing (mode != OFF and audio on) |
| LED3 (carrier bottom) | status code: LINE = 1 long pulse, MIC = 2 long; then HEARTBEAT = 1 short, CLASSIC = 2, EMBER = 3, HUE = 4 |

The no-laptop boot default is **HEARTBEAT + line input + LOCKED**. Modes:
**CLASSIC** = per-slot R/G/B envelope (same slot colors as the CoreS3 bridge);
**HEARTBEAT** = raw line-waveform peak driving a shared deep-red pulse without
quiet-room calibration; **EMBER** = shared warm-white envelope (CoreS3 EMBER
ratios); **HUE** = knob-set hue, envelope drives the value. **OFF** remains a
service/serial state, never a paw-cycle accident. It sends one black frame and
stops publishing so the 3 s staleness + micro-lease expiry can return fixtures
to autonomy.

Serial CLI at 115200 (boot banner `=== Resonance puca-bridge 0.4.1-dev ===`,
plus a 1 Hz `puca ...` status line):

| Key | Action |
|---|---|
| `t` | one-line JSON status including mode, RMS, peak/clipping, gain, hue, peers, and send counters |
| `M` | next live mode; skips OFF, like an unlocked setup-window paw touch |
| `A` | audio on/off toggle (off sends one zero frame; on re-runs the 2 s noise calibration) |
| `I` | input path toggle: onboard MEMS mics (boot default) <-> 3.5 mm line-in |
| `H` | select 3.5 mm line input and HEARTBEAT mode in one step |

## Build / flash

```bash
firmware/puca_bridge/build.sh                 # build only
firmware/puca_bridge/build.sh --port COMx      # build + explicit USB flash
```

FQBN `esp32:esp32:pico32` (ESP32-PICO-D4, 4 MB flash; the 8 MB PSRAM is
unused by this development build). The script runs the native PUCA tests, uses
a unique build dir, checks the binary/build-options outputs, reports the exact
binary SHA-256, and passes
`-I<firmware root>` so `fixture/src/core/packet.h` is the one canonical wire
contract. Verify the enumerated port really is the PUCA (`lsusb`, then match
the serial device) before flashing -- other bench devices also enumerate as
serial ports. A build-only result is a compile-check, not a promoted shared-bench
artifact under ADR 0040.

The 2026-08-26 accepted development candidate is
`build/puca-bridge-20260826-standalone-heartbeat-v041-c2/puca_bridge.ino.bin`,
964,752 bytes, SHA-256
`e8ec74680564f96f10c2f6e87b37eb807b9d9ba3b355ccf41c72f8301c4984b6`.
It is an exact bench-recovery identity, not a show-release claim.

## Bring-up checklist

1. DONE 2026-08-26: exact Original Edition identity, 4 MB flash, CP2102N USB
   serial, MAC, boot banner, codec, stereo I2S, channel 11, powered Pod20, and
   locked HEARTBEAT/line defaults are recorded in the hardware README.
2. DONE 2026-08-26: normal paw touches produced status only and did not change
   HEARTBEAT. Verify the boot-hold setup gesture during a deliberate service
   session, not during a performance.
3. Finish both knob full sweeps: `gain` must cover 0.25-4.00 and `ceil` 0-1.00;
   note whether patched CV2/CV3 cables disturb them.
4. Clap-test the MEMS path if it will be used, then test the RODE through the
   chosen faceplate input. Record unclipped working gains.
5. One named fixture: confirm visible HEARTBEAT response and autonomous return
   within about 3 s after PUCA stops publishing.
6. Mixed HEX/RGBW group: verify visible output and stale fallback. The current
   full-census sender already passed 70+ eligible peers and 8,318/0 callback
   success/failure during a 207 s powered-Pod20 soak.
7. Repeat packet/PDR and range checks at the intended tree/Shiftpod geometry,
   then run the multi-hour soak from ADR 0035.
8. Confirm no "PUCA DSP" WiFi AP exists while this firmware runs.
9. Work the no-human DG1022Z procedure in
    `docs/howto/DG1022Z_PUCA_HEARTBEAT.md`; verify two visible pulses and zero
    clip blocks before considering any connection near the ceremony system.

## Known UNVERIFIED items / assumptions

- Boot-hold setup entry and long-hold confirmation feel; normal locked paw
  status-only behavior is hardware-proven.
- Full knob end-to-end range and rotation direction; powered ADC values are
  hardware-proven.
- MICBEN requirement for the Knowles MEMS pair (enabled because upstream does).
- Field-distance ESP-NOW/PDR from the Pod20 at the intended placement. Current
  bench geometry reached 70+ eligible fixture heartbeats and clean local send
  callbacks, but is not the ADR 0035 field-range test.
- DG1022Z faceplate input, HEARTBEAT response, and the 10 Hz light rendering of
  the performer's actual waveform.
- Visible fixture response and three-second autonomous fallback from this PUCA
  publisher; send callbacks alone do not prove receiver application.

## Bench and schematic findings (2026-08-20/21/26)

These findings were established live on hardware and from the vendor schematics
during pre-playa bench work, then recovered from the Claude session record.

### Verified on hardware

- The codec and onboard MEMS microphones are live: the first boot reported
  `codec=1 i2cerr=0`, and the envelope tracked room sound.
- One boot entered EMBER unexpectedly, consistent with a spurious paw edge.
  The current test-covered guard ignores the first 2.5 s and baselines the
  settled input before accepting a press; that delta still needs hardware proof.
- Knobs reading zero on USB-only power is expected. The Eurorack back PCB powers
  the pots' reference rail and TL072 buffers from the Eurorack header. Knobs,
  faceplate audio/CV inputs, and amplified outputs require Eurorack power; USB
  alone powers the ESP32, codec, and onboard microphones.
- On 2026-08-26, the powered Pod20 and USB combination booted `0.4.1-dev` with
  `codec=1`, line input, HEARTBEAT, and LOCKED controls. Normal paw touches
  replayed status without changing mode. Powered knobs reported stable nonzero
  values; complete sweeps remain open.
- The received unit identifies as ESP32-PICO-D4 revision 1.0 with 4 MB embedded
  flash and MAC `4C:75:25:A4:EB:10`; Windows CP2102N identity is
  `USB\\VID_10C4&PID_EA60\\0EC45B486617EC1183509E9D47486EB0` on the current
  bench.
- The corrected `fx-*` fixture filter sustained a 207 s full-census run, ending
  at 70+ eligible fixtures and 8,318 successful send callbacks, with every
  reported error/clipping counter at zero.
- The service `A` stop sent one final eight-chunk black census, then held
  `active=0` with transmit counters unchanged. This releases the live stream;
  visible receiver-side autonomous fallback remains to be observed directly.

### Eurorack v0.5 faceplate wiring

| Jack | Net | Notes |
|---|---|---|
| J1 | V/OCT -> GPIO32 | Clamp diodes, dedicated pitch CV. |
| J2 | CV + top pot -> GPIO33 (KNOB1) | Jack and pot are summed by an op-amp mixer. |
| J3 | CV + bottom pot -> GPIO34 (KNOB2) | Same summing topology. |
| J4 / J7 | Triggers -> GPIO13 / GPIO14 | Unused by this firmware. |
| J5 / J6 | Audio in L / R | About 4:1 attenuation (100k/24k, about -12 dB) onto WM8978 LINE_IN_L/R. |
| J8 / J9 | Audio out L / R | About 18x gain back to modular level; unused here. |
| AUX pair | GPIO-header pass-through | No faceplate jack; WM8978 AUX pass-through remains unconfigured. |

The CV2/CV3 jacks add to the pot values; they do not disconnect the knobs. A
consumer/DJ line source arrives roughly 12 dB quieter through J5/J6 than through
the PCB stereo jack, so gain staging must be recorded for the chosen path.

### Power and source hookup

- The board charger is an MCP73831. With the battery connector populated, a
  LiPo can provide USB/ext-5 V ride-through; charge current is resistor-fixed.
- The battery-sense jumper shares GPIO14 with TRIG2. Closing it sacrifices that
  otherwise-unused trigger input.
- Prefer fixed-level mixer record out, then booth out. Avoid a +4 dBu balanced
  master feed into the codec. For faceplate J5/J6, use dual RCA to dual 3.5 mm TS
  mono; do not put a TRS plug into a mono jack. Pack a ground-loop isolator.
- The RODE NTG uses its internal battery and analog output here. Its USB-C port
  is a device-mode computer interface; PUCA cannot host or control it.
