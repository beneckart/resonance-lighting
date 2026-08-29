# PUCA performance-audio bridge

**Status: powered-Pod20 `0.4.1-dev` hardware baseline passed 2026-08-26;
credentialed `0.5.0-dev` USB bootstrap and safe OTA passed 2026-08-27; exact-
target `0.5.2-dev` onboard-mic mode and time-calibration proof passed; exact-
target `0.5.3-dev` clockwise carrier-control normalization passed build and OTA
2026-08-28; exact-target `0.5.5-dev` capacitive paw and held-paw boot arming
passed 2026-08-29.** The runtime sends no lighting frames unless the paw is held
continuously for 1.2 s during the five-second steady-red boot opportunity. An
armed boot starts in DJ mode (the previous CLASSIC per-slot
look), with line input and a 20 s setup window. PUCA advertises its `A4EB10`
identity/revision to Bridge OS and accepts only an exact-target maintenance
request before leaving ESP-NOW for the standard shared-WiFi `/telemetry`,
`/update`, and `/resume` flow. It never responds to fleet-wide maintenance and
never creates the factory softAP.

The prior WM8978, stereo I2S capture, faceplate paw, powered knob ADCs,
channel-11 receive, ADR 0040 fixture filtering, and more-than-18-fixture
transmit chunking all ran on hardware. A 207 s soak reached 70+ eligible
fixtures and 8,318 successful send callbacks with zero reported errors. The
2026-08-28 service-started onboard-mic run exercised DJ, HEARTBEAT, EMBER, and
HUE across about 75-92 fresh fixtures with clean transport and capture counters;
Ben visibly confirmed audio-reactive fixture behavior. A repeat laptop waveform
with the TS plugs corrected from J8/J9 AUDIO OUT to J5/J6 AUDIO IN raised RMS
from 6-12 to 37-41 with zero clipping/errors, proving the faceplate line path.
The paw electrode is proven through the ESP32 T3/GPIO15 capacitance peripheral;
a held-paw Pod20 power cycle produced `bootarmed=1` and active DJ. Timed setup
gestures, line-route continuity, forced rollback, `/resume`/timeout, fleet-wide-
maintenance rejection, stale fallback, mixed-output fidelity,
multi-hour stability, and field-range geometry remain unverified.

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
- emits its own tail-7 `NB_HEARTBEAT` with `puca-bridge-*` identity so Bridge OS
  can show and exact-target the one-off publisher without mistaking it for a
  fixture;
- enters standard shared-WiFi OTA only on exact `NB_TARGET_ENTER_MAINT` for
  `A4EB10`; a fleet-wide zero target is deliberately ignored;
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
- No WiFi AP: the factory image's "PUCA DSP" softAP does not exist here. COMMS
  stays unassociated on channel 11. Exact-target maintenance deliberately
  stops publishing, releases the mesh, joins the shared maintenance WiFi, and
  auto-resumes dark COMMS after 10 minutes if no update completes.
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
| Paw (capacitive touch) | 15 / ESP32 T3 | Vendor [trigger test](https://github.com/ohmic-net/puca_dsp/blob/main/puca-eurorack/hardware_test_arduino/Puca_Eurorack_trigger_test/Puca_Eurorack_trigger_test.ino) names GPIO15 but its digital read does not work on this exact unit; `touchRead(15)` is hardware-proven | High: released 867-870, held through 216; press <=650, release >=750, invalid <50 fails safe |
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
| KNOB1 (top pot) | input sensitivity: 0.25x-4x multiplier on the envelope level (log taper, 1x at center); clockwise increases, counterclockwise decreases |
| KNOB2 (bottom pot) | DJ + HEARTBEAT + EMBER: brightness ceiling 0-100%, clockwise toward maximum; HUE: hue, clockwise through one full wheel |
| No paw hold at boot | SAFE-IDLE: mesh identity/maintenance remain available, but PUCA emits no lighting frames |
| Paw held continuously at boot | after a 1.2 s hold, arms DJ + line input and opens a 20 s setup window |
| Paw touch after locked boot | status display only; it cannot arm, change, or stop the performance |
| Paw short touch in setup | DJ -> HEARTBEAT -> EMBER -> HUE -> DJ; OFF is excluded |
| Paw long hold in setup | confirms the selection and locks immediately; inactivity also locks after 20 s |
| LED1 (onboard) | lit when ESP-NOW is up |
| LED2 (carrier top) | lit while the bridge is actively publishing (mode != OFF and audio on) |
| LED3 (carrier bottom) | status code: LINE = 1 long pulse, MIC = 2 long; then DJ = 1 short, HEARTBEAT = 2, EMBER = 3, HUE = 4 |

The no-laptop boot default is **SAFE-IDLE + line input + LOCKED**. It sends no
`NB_DIRECT_FRAME`, so plugging in or OTA-rebooting PUCA cannot seize/darken the
tree. Deliberately hold the paw during boot to arm **DJ + line input**. Modes:
**DJ** (internal `MODE_CLASSIC`) = per-slot R/G/B envelope, the same slot colors
as the CoreS3 CLASSIC look;
**HEARTBEAT** = raw line-waveform peak driving a shared deep-red pulse without
quiet-room calibration; **EMBER** = shared warm-white envelope (CoreS3 EMBER
ratios); **HUE** = knob-set hue, envelope drives the value. **OFF** remains a
service/serial state, never a paw-cycle accident. It sends one black frame and
stops publishing so the 3 s staleness + micro-lease expiry can return fixtures
to autonomy.

Serial CLI at 115200 (boot banner `=== Resonance puca-bridge 0.5.5-dev ===`,
plus a 1 Hz `puca ...` status line):

| Key | Action |
|---|---|
| `t` | one-line JSON status including mode, RMS, peak/clipping, gain, hue, peers, and send counters |
| `M` | next live mode; skips OFF, like an unlocked setup-window paw touch |
| `A` | audio on/off toggle (off sends one zero frame; on re-runs the 2 s noise calibration) |
| `I` | input path toggle: 3.5 mm line-in (boot default) <-> onboard MEMS mics |
| `H` | select 3.5 mm line input and HEARTBEAT mode in one step |
| `P` | inactive-only four-second raw GPIO15 capacitive probe; normal capacitive paw handling resumes afterward |

## Build / flash

```bash
firmware/puca_bridge/build.sh                 # build only
firmware/puca_bridge/build.sh --wifi-source /secure/wifi_secrets.h --port COMx
firmware/puca_bridge/build.sh --ota 192.168.1.123
```

FQBN `esp32:esp32:pico32:PartitionScheme=default` (ESP32-PICO-D4, 4 MB flash,
explicit dual-app OTA layout; the 8 MB PSRAM is unused by this development
build). The script runs the native PUCA tests, uses
a unique build dir, checks the binary/build-options outputs, reports the exact
binary SHA-256, and passes
`-I<firmware root>` so `fixture/src/core/packet.h` is the one canonical wire
contract. Verify the enumerated port really is the PUCA (`lsusb`, then match
the serial device) before flashing -- other bench devices also enumerate as
serial ports. A build-only result is a compile-check, not a promoted shared-bench
artifact under ADR 0040.

`wifi_secrets.h` is gitignored. `--wifi-source` copies an explicit local header
with `RES_WIFI_SSID` and `RES_WIFI_PASSWORD`; otherwise the build reuses an
existing fixture/net-bench/power-bench secrets file. A missing file is a hard
runtime OTA refusal, and the build prints a warning. Do not install such a build
as the supposed USB bootstrap for an enclosed PUCA.

### USB rescue on the received board

Routine updates use exact-target OTA; the internal USB-C connector can remain
behind the faceplate. The received Original Edition board does not expose a
normal BOOT button. The visible onboard button is GPIO36, not the ESP32 download
strap, and the accessible four-pin header is `VIN`, `RST`, `VDD`, `GND`.

The CP2102N automatic reset entered normal boot (`0x13`) rather than the ROM
downloader on this unit. The proven rescue sequence was:

1. Verify the exact CP2102N identity/port and keep stable Pod20 power present.
2. Use a normal jumper to hold `RST` to `GND`; never use a meter in ammeter mode
   as a jumper, and never short `VIN` or `VDD`.
3. Assert the USB serial adapter's DTR/download control, then release `RST`.
4. Use esptool with `--before no_reset`; write bootloader at `0x1000`, partition
   table at `0x8000`, `boot_app0` at `0xe000`, and the application at `0x10000`.
5. Hard-reset and confirm the exact MAC, firmware revision, and SAFE-IDLE status.

This is an emergency bench procedure, not the routine deployment path. Do not
attempt it from labels alone if the header order on a different PUCA revision
does not match.

## Exact-target OTA from Bridge OS

PUCA follows the fixture transport contract, but remains a protected one-off
publisher. Firmware bytes travel over ordinary WiFi from the laptop; Bridge OS
only sends the exact-target maintenance request.

1. Declare the one firmware operator and stop any competing publisher.
2. In Bridge OS Health, select live peer `A4EB10` (`puca-bridge-*`) and request
   maintenance, or issue serial `UA4EB10`. PUCA ignores fleet-wide maintenance.
3. Discover an endpoint whose `/telemetry` reports `fixture_id=A4EB10`,
   `role=puca_audio_bridge`, and the expected current firmware. Wait for/freeze
   the bridge command tail before uploading so the reboot cannot be re-caught.
4. Upload only a PUCA Original Edition binary. The reusable verified path is:

   ```bash
   python ops/bench/field_cycle_ota.py A4EB10 \
     --bin <puca_bridge.ino.bin> \
     --expect-fw puca-bridge-0.5.0-dev \
     --notes "PUCA exact-target OTA"
   ```

   That helper sends `UA4EB10`, identity-matches `/telemetry`, waits out the
   35 s request tail, calls the standard uploader, then requires a fresh exact
   revision heartbeat after the 25 s pending-verify survival gate.
5. After OTA, PUCA reboots SAFE-IDLE. A 20 s self-test requires codec, I2S,
   ESP-NOW heartbeat, and writable NVS before cancelling A/B rollback. Arm DJ
   only with a later deliberate paw-held power cycle. USB remains recovery.

The installed and exact-target OTA-proven 2026-08-29 candidate is
`build/puca-bridge-20260829-paw-cap-touch-v055-r1/puca_bridge.ino.bin`,
1,038,176 bytes, SHA-256
`b7d4db31f339a14d079a273170544f6b4218a367075799736f1229a6c1c2f2c1`.
It replaces the vendor example's nonfunctional GPIO15 digital read with the
hardware-proven ESP32 capacitance channel, preserves the fail-safe boot gate,
and passed a real held-paw Pod20 power cycle with `bootarmed=1`, `active=1`.
This remains a development bench identity, not a promoted show release.

The superseded exact-target OTA-proven 2026-08-28 diagnostic candidate is
`build/puca-bridge-20260828-paw-telemetry-v052-r1/puca_bridge.ino.bin`,
1,024,608 bytes, SHA-256
`3e3e8d9fa0ce3c8d60950abc027840eb345c95cf1f9438d1f0ff44cbac5a20e8`.
It adds active-high raw paw telemetry, a five-second steady-red arm opportunity,
and elapsed-time calibration proven on hardware at about two seconds. The paw
signal itself remains intermittent, so this is a diagnostic bench identity, not
a promoted show release.

The prior OTA-proven 2026-08-27 bootstrap candidate is
`build/puca-bridge-20260827-ota-safe-v050-bootstrap-r3/puca_bridge.ino.bin`,
1,024,128 bytes, SHA-256
`1e90f6f1731a622b11274fa91abbc6eeebb17c35abe90bd86337c915cb99e8da`.
It was built explicitly with `esp32:esp32:pico32:PartitionScheme=default` and
local gitignored WiFi credentials. It is an exact bench-recovery identity, not
a promoted shared-fleet/show release under ADR 0040.

The prior 2026-08-26 accepted development candidate is
`build/puca-bridge-20260826-standalone-heartbeat-v041-c2/puca_bridge.ino.bin`,
964,752 bytes, SHA-256
`e8ec74680564f96f10c2f6e87b37eb807b9d9ba3b355ccf41c72f8301c4984b6`.
It is an exact bench-recovery identity, not a show-release claim.

## Bring-up checklist

1. DONE 2026-08-26: exact Original Edition identity, 4 MB flash, CP2102N USB
   serial, MAC, boot banner, codec, stereo I2S, channel 11, powered Pod20, and
   historical locked HEARTBEAT/line defaults are recorded in the hardware README.
2. DONE 2026-08-27/29 on `0.5.0-dev`, `0.5.2-dev`, and `0.5.5-dev`: no-hold USB and OTA/software boots
   reported `active=0`, `bootarmed=0`, locked controls, healthy codec, and zero
   direct frames while Bridge OS received the PUCA heartbeat. Raw capacitance
   isolated the vendor digital-read mismatch; a held-paw full power cycle on
   `0.5.5-dev` returned `bootarmed=1`, `active=1`, and DJ + line. Timed short-
   touch mode cycling and long-hold setup lock remain to be accepted.
3. Finish both knob full sweeps: `gain` must cover 0.25-4.00 and `ceil` 0-1.00;
   note whether patched CV2/CV3 cables disturb them.
4. DONE 2026-08-28 for onboard MEMS capture, all four renderers, and the complete
   laptop-to-J5/J6 line path. The initial silent run had both TS plugs in J8/J9
   AUDIO OUT; corrected J5/J6 routing raised RMS from 6-12 to 37-41 at only 10
   percent laptop level with zero clipping/errors. Test the RODE and record its
   unclipped working gain.
5. One named fixture: confirm visible HEARTBEAT response and autonomous return
   within about 3 s after PUCA stops publishing.
6. Mixed HEX/RGBW group: verify visible output and stale fallback. The current
   full-census sender already passed 70+ eligible peers and 8,318/0 callback
   success/failure during a 207 s powered-Pod20 soak.
7. Repeat packet/PDR and range checks at the intended tree/Shiftpod geometry,
   then run the multi-hour soak from ADR 0035.
8. DONE in part 2026-08-27: exact-target `UA4EB10` exposed identity-matching
   telemetry, accepted the exact retained binary, rebooted dark, supplied a
   fresh same-revision heartbeat with reset uptime/sequence, and survived the
   25 s host gate. Still prove `/resume`, the 10-minute timeout, forced-self-test
   rollback, and hardware rejection of fleet-wide maintenance without moving
   the live fixture fleet into maintenance.
9. Confirm no "PUCA DSP" WiFi AP exists in COMMS or maintenance.
10. Work the no-human DG1022Z procedure in
    `docs/howto/DG1022Z_PUCA_HEARTBEAT.md`; verify two visible pulses and zero
    clip blocks before considering any connection near the ceremony system.

## Known UNVERIFIED items / assumptions

- `0.5.0-dev` paw-held DJ-first arming, setup gestures, `/resume`, maintenance
  timeout, fleet-wide-maintenance rejection on hardware, and forced rollback.
  No-hold SAFE-IDLE, exact-target maintenance, shared-WiFi OTA, pending-verify
  survival, and post-OTA dark rejoin are hardware-proven on `A4EB10`.
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
- One older digital-input boot entered EMBER unexpectedly. That path is
  superseded: exact-unit capacitance plus hysteresis now drives the paw, and a
  held-paw `0.5.5-dev` power cycle is hardware-proven.
- Knobs reading zero on USB-only power is expected. The Eurorack back PCB powers
  the pots' reference rail and TL072 buffers from the Eurorack header. Knobs,
  faceplate audio/CV inputs, and amplified outputs require Eurorack power; USB
  alone powers the ESP32, codec, and onboard microphones.
- On 2026-08-26, the powered Pod20 and USB combination booted `0.4.1-dev` with
  `codec=1`, line input, HEARTBEAT, and LOCKED controls. Normal paw touches
  replayed status without changing mode. Powered knobs reported stable nonzero
  values; both complete clockwise endpoint sweeps later passed on `0.5.3-dev`.
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
