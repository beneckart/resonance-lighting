# CoreS3 audio-reactive lighting how-to

This guide covers the bench setup that makes a Rode VideoMic NTG drive nearby
Resonance fixtures through an M5Stack CoreS3 and Module Audio. It explains the
mic controls, the bridge display, practical tuning, and the safe fallback path.

The control descriptions below assume the mic is a **Rode VideoMic NTG**. The
1-15 rear gain dial and its two-button control panel match the Nevada City bench
mic. Confirm the product label before using the exact button sequences with a
different Rode model.

## What the signal path does

```text
sound -> Rode mic -> Module Audio LINE/MIC input -> CoreS3 envelope
      -> 10 Hz ESP-NOW direct frames -> channel-11 fixtures
```

The bridge measures sound intensity, not musical pitch or frequency bands. It
removes the waveform's DC offset, calculates RMS level, learns the ambient noise
floor for two seconds, and applies a fast attack with a slower release. Live
fixtures receive stable ID-sorted red, green, or blue brightness slots.

The Module Audio has no microphone of its own. It is the external analog input
for the Rode. A Module Audio build exposes both choices in one app: **Aux Input**
uses the RODE through LINE/MIC, and **Ambient Mic** uses the CoreS3's two
built-in microphones. Aux is preferred at boot when ready; Ambient is the safe
fallback.

## Quick start

1. Power the CoreS3 stack completely off.
2. Set the Module Audio's physical A/B I2S selector to **B**. Configuration A is
   for Basic/Core2 and produced valid reads containing only digital zeros on the
   CoreS3 bench.
3. Connect the Rode's analog output to the Module Audio jack labeled
   **LINE/MIC** using the Rode 3.5 mm cable. Do not use the TRRS headset jack for
   this setup.
4. Turn the Rode on and start with these controls:

   - gain dial: **10**
   - high-pass filter: **75 Hz**
   - high-frequency boost: **off**
   - -20 dB pad: **off**
   - safety channel: **off**

5. Power the CoreS3, tap **Audio** in the launcher, and confirm the app says
   `AUX INPUT PUBLISHING`. If it says `AMBIENT MIC`, tap **Input** once to retry
   or select Aux.
   Starting the publisher first releases any active CA, Contagion, Dark, or
   other program lease in fixture RAM so Audio can take ownership. It does not
   flash fixtures or change their saved configuration.
6. During the first two seconds after Audio starts, leave the space at its ordinary ambient level
   without clapping, speaking into the mic, or playing intentional program
   audio. This becomes the initial noise floor.
7. Play the intended source and adjust the Rode gain dial. A useful target is:

   - ordinary room or outdoor background: about 0-10% bridge level
   - normal intended sound: about 20-80%
   - strong beats or claps: brief peaks near 80-100%

   Avoid a display that stays pinned near 100%. The bridge's percentage is an
   adaptive response value, not a clipping meter; the Rode's red peak LED is the
   authoritative warning that the microphone preamp is clipping.
8. Aim the mic at the intended source. Keep its rear and sides toward unwanted
   generator, wind, handling, or bamboo-impact noise where practical.

Opening Audio starts the stream in CLASSIC mode. Use **Look** (or USB `M`) to
cycle CLASSIC -> EMBER -> HUECYCLE -> PULSE, **Input** (or USB `N`) to cycle
Ambient Mic <-> Aux Input, and **Pause/Start** (or USB `A`) to control the
stream. An input change sends zero, pauses the publisher during the hardware
handoff, and starts a fresh two-second calibration before continuing. Leaving
Audio for the launcher also stops the publisher. After a large change to the
RODE gain or filter, pause and start once so the bridge relearns promptly.

## Rode VideoMic NTG controls

### Rear dial, 1-15

This is the microphone's analog output-level control. Higher numbers send a
larger signal to the Module Audio, which is why the bridge showed better levels
above 1. Rode recommends 10 as a general starting point.

Use the dial as the main setup control. Start at 10, make sound at the loudest
level expected in use, then adjust while watching both the CoreS3 level and the
Rode peak LED. Gain 1 is intentionally very low and is unlikely to be useful for
normal speech or music at a distance.

There are two gain stages in this bench path: the Rode dial and the fixed Module
Audio codec gain configured by the bridge firmware. Tune the Rode, not the
firmware codec, during ordinary setup.

### Power/function button

A short press cycles these function states:

- -20 dB pad
- safety channel
- both enabled
- both disabled

Hold the button for about three seconds for manual power on or off. The mic can
auto-power with compatible equipment that supplies plug-in power, but confirm
the mic's power LED rather than assuming the Module Audio woke it.

For this controller, leave both functions off unless the source is exceptionally
loud:

- **-20 dB pad:** reduces the signal before the mic preamp. Enable it for a loud
  PA, close percussion, or another source that makes the Rode's peak LED turn
  red even after lowering the rear dial.
- **Safety channel:** places a second, quieter copy on one stereo channel for a
  recorder to recover later. The current bridge does not record or separately
  recover that channel; it measures the incoming samples as one envelope. Leave
  safety off so it does not alter the level calculation.

The separate peak warning LED turns red when the mic's internal preamp clips.
If that happens, lower the dial, move the mic farther from the source, or enable
the -20 dB pad.

### Filter button

A short press cycles the high-pass filter through 75 Hz, 150 Hz, and off. Hold
the button for about three seconds to toggle high-frequency boost.

Use these starting points:

| Setting | Use it for | Tradeoff |
| --- | --- | --- |
| Filter off | Music where bass response matters and wind/rumble is controlled | Bass, handling, and wind can dominate the envelope |
| 75 Hz | Default for this bench; general music, speech, and outdoor use | Removes deep rumble with modest effect on useful content |
| 150 Hz | Speech, claps, or high percussion in wind/rumble | Rejects more low end and makes bass-driven response weaker |
| HF boost off | Default for the controller's broadband RMS measurement | No extra presence or hiss |
| HF boost on | A clarity experiment when a windshield dulls speech | Can overemphasize hiss, claps, and sharp transients |

The foam/furry windshield is mechanically important outdoors. Use it before
trying to solve wind noise with electronics alone.

### Battery indicator

The power LED reports the mic's internal battery state: green, amber, red, then
blinking red as charge falls. It shows blue while charging from a computer and
returns to green when charged. Charge through the mic's USB-C port when needed;
the 3.5 mm analog cable is the audio connection, not its charger.

## Reading the CoreS3 display

The Audio app identifies the input and state:

```text
AUX INPUT  PUBLISHING
rms 343 floor 31 level  67%
```

- `AUX INPUT` means the external Module Audio LINE/MIC path is selected.
- `AMBIENT MIC` means the CoreS3 microphones are selected instead.
- `INPUT FAILED` means no audio input initialized.
- `PUBLISHING` or `PAUSED` is the audio-reactive stream state.
- `rms` is the current raw DC-removed sample magnitude. It is useful for comparing
  relative signal levels but is not calibrated sound-pressure level or dBFS.
- `floor` is the bridge's learned ambient-noise estimate.
- `level` is the adaptive 0-100% value sent as fixture brightness. It changes its
  ceiling over time, so 100% does not by itself mean the analog path is clipping.

The `live fixtures` rows show peers heard during the last five seconds. A unit
that is absent there cannot receive an addressed audio color. The screen shows
only three rows in the audio layout even if more fixtures are live.

## Tuning recipes

These dial ranges are starting points, not fixed calibration values. Distance,
source loudness, wind, and placement matter more than the printed number.

| Situation | Rode gain | Filter | Other settings |
| --- | ---: | --- | --- |
| Conversation or claps a few feet away | 10-13 | 75 or 150 Hz | Pad off, safety off, HF boost off |
| Moderate music or a nearby loudspeaker | 7-10 | 75 Hz or off | Pad off unless the Rode clips |
| Loud PA or close percussion | 3-8 | 75 Hz | Use the -20 dB pad if the red peak LED appears |
| Windy outdoor bench | Set from meter | 75 or 150 Hz | Windshield on; HF boost usually off |

If the lights are too calm but the Rode is not clipping, raise the dial. If the
lights are always bright, lower the dial, improve mic placement, or remove the
unwanted low-frequency noise with the 75/150 Hz filter. Recalibrate after a
major change.

## Fixture lifecycle and safe fallback

The bridge sends 10 Hz direct frames only to fixtures whose heartbeats it has
heard recently. Each frame grants a short in-RAM micro-lease; it does not store a
new lifecycle or program selection in the fixture.

If the bridge is paused, unplugged, or stops transmitting, a fixture treats the
direct color as stale after three seconds and returns to its autonomous program.
The micro-lease itself is 10 seconds, but the stricter three-second frame-stale
rule controls this failure case. Pausing sends one zero-level frame first, then
stops the stream.

Normal fixture lifecycle policy still determines whether LEDs may illuminate.
For a daylight bench test, send `N1` to each authorized fixture over its own USB
serial connection to force night **in RAM only**. Always send `N2` afterward to
restore automatic lifecycle. Neither command persists across reboot.

The three perimeter fixtures already used for the small-fleet acceptance test
are:

- `F3FD88`
- `F2BE80`
- `F2BFEC`

They were validated as channel-11 perimeter peers. Hardware should still be
identified from fixture ID rather than remembered COM port, because Windows port
numbers can change.

## Troubleshooting

### RMS/level moves, but fixtures remain in CA

An older CoreS3 Bridge OS can transmit direct frames while an explicit CA or
Contagion program lease still wins fixture arbitration. It can also show current
`fx-*` fixtures as live while an obsolete firmware-name filter omits them from
the Audio frame. On the T-Deck, open CA Studio and tap **Release**, then restart
Audio. Bridge OS `cores3-os-0.1.2-dev` and later both perform the one-shot,
RAM-only release automatically and recognize current immutable fixture artifact
names. If the fixtures return to CA while Audio is still publishing, stop the
other bridge/app that is sending a new program lease.

### Display says `MODULE TRS`, but RMS stays exactly 0

Power the entire stack off, move the Module Audio selector to B, and restart.
Selector A was the exact cause of this symptom on the accepted bench.

### RMS moves, but the level is weak

- Confirm the Rode is powered and charged.
- Confirm the cable is in the Module Audio `LINE/MIC` jack.
- Raise the rear gain dial gradually from 10.
- Leave the -20 dB pad off.
- Aim the front of the directional mic at the source and reduce the distance.
- Toggle bridge audio off/on and leave two seconds of normal ambience for a new
  calibration.

### Level stays near 100% or the Rode peak LED is red

Lower the rear gain. Move the mic farther away. If the peak LED still turns red
with the expected loudest source, enable the -20 dB pad. Use the high-pass filter
for wind/handling rumble, not as a substitute for correcting overload.

### The bridge reacts, but the fixtures do not

- Confirm each expected ID appears under `live fixtures`.
- Confirm bridge and fixtures use ESP-NOW channel 11.
- Confirm the fixtures are running firmware with direct-frame support.
- In daylight, use the temporary `N1` bench override on only the authorized
  fixtures, then restore each one with `N2`.
- Use the Audio app's on-screen **Look**, **Input**, and **Pause/Start** controls.

### The app reports `AMBIENT MIC` when Aux is wanted

Tap **Input** once: the Module build can retry a controller that was late at
boot. If the app stays on Ambient, fully power down, reseat the stack, confirm
selector B, and restart. Also confirm the Bridge OS image was built with
`--audio-module`.

### Unwanted sounds dominate the lights

Use the mic's directionality first: point it at the wanted source and away from
wind, structure-borne impacts, the generator, and the solarnoid. Add the
windshield, then try 75 Hz. Move to 150 Hz only when rejecting low-frequency
energy is more important than bass response.

## Build and flash reference

The complete commands, dependency pin, and ordinary-vs-Cambium artifact rules
live in [`firmware/cores3_bridge/README.md`](../../firmware/cores3_bridge/README.md).
Use the unified Bridge OS Module Audio form:

```sh
bash ./build.sh --audio-module --channel 11 \
  --build-path build/nc-cores3-audio-module-r1
```

The resulting ordinary image contains both Listener and Audio; only Cambium
remains a separate build mode. Build once into a named directory and flash that
exact artifact.

## Validated baseline

On 2026-08-06, selector B changed the Rode input from exact digital zeros to a
noise floor around RMS 30. Speech and claps peaked at RMS 3422.6 and a 0.987
normalized level. All three perimeter fixtures entered direct program 3, matched
their addressed frames, and returned to autonomous program 1 after the stream was
stopped. The bridge reported no ESP-NOW send failures or receive drops during the
acceptance run.

On 2026-08-25, the unified Bridge OS Module Audio artifact was USB-flashed to
exact CoreS3 `4D5DB0` (`80:45:6B:4D:5D:B0`). A fresh boot selected
`MODULE TRS`, reported the input ready, received channel-11 mesh traffic, and
remained paused with zero direct frames and zero queue drops. That pass validates
hardware detection and the safe launcher posture; the four looks and fixture
fallback behavior still rely on the 2026-08-06 acceptance until they are rerun
deliberately against named canaries.

The runtime-selector revision was then hardware-tested on the same CoreS3. With
Audio paused, the shared Input action completed Aux -> Ambient -> Aux. Every
state reported ready, the Module remained available, and `frames=0` plus
`readfail=0` held throughout. A USB-reset run also exposed a delayed-module boot
case; the accepted behavior keeps Ambient working and permits a later bounded
Aux retry instead of permanently disabling the Input control.

The final `cores3-os-0.1.2-dev` acceptance fixed both hidden blockers found in
the first standalone run: Audio now releases an explicit prior program lease on
start and recognizes ADR 0040 `fx-*` fixtures. Builtin Dual Mic calibrated and
published four fleet chunks per tick with zero send/read failures; a separate
T-Deck observed fresh fixtures enter program 3 Direct, and Ben confirmed the
spoken `test test test` response worked really well. The exact 1,168,352-byte
binary SHA-256 is
`FDDAC35CA9778D1698763F77FAABA88A5FBB56A8167C1D24EE6E0701F1742C65`.

## References

- [Rode VideoMic NTG product and user guide](https://rode.com/en-us/products/videomic-ntg)
- [M5Stack Module Audio documentation](https://docs.m5stack.com/en/module/Module-Audio)
- [CoreS3 bridge firmware notes](../../firmware/cores3_bridge/README.md)
- [2026-08-06 acceptance record](../../LOG.md#2026-08-06----ben--codex----channel-11-migration-and-cores3-audio-reactive-bridge)
