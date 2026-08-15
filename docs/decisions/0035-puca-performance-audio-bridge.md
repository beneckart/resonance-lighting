# 0035 -- PUCA as the primary performance-audio bridge

**Date:** 2026-08-13

**Status:** Accepted hardware direction; firmware and field validation pending

**Owners:** Ben + Codex

## Context

Resonance needs a low-operations path from a clean performance source -- DJ
output, singing, violin, bowls, or ambient sound -- into the ESP-NOW light fleet.
Putting a microphone and gain problem in every lantern would add cost, noise,
calibration, and roughly 130 failure surfaces. The existing CoreS3 + M5Stack
Module Audio bridge has already proved audio-reactive `NB_DIRECT_FRAME` control
and the safe stale-frame fallback on three perimeter fixtures.

The CoreS3 remains a useful screen-equipped bridge and independent fallback, but
its external module is a less direct audio-ingest instrument. The purchased PUCA
DSP combines an ESP32, WM8978 codec, real stereo line input, onboard microphone
array, USB-C, and sufficient memory for FFT/onset work. Its Eurorack carrier adds
two knobs, a capacitive paw, protected CV/trigger inputs, and an enclosed powered
desktop form.

The complete PUCA Eurorack setup and RODE VideoMic NTG have arrived. Hardware
arrival is not firmware completion; the received factory image is a musical
oscillator/effect image, not a Resonance bridge.

## Decision

1. Use the **PUCA DSP Original Edition + Eurorack expansion** as the primary
   dedicated performance-audio source bridge.
2. Use the **RODE VideoMic NTG + WS11** as the default external source for
   non-DJ performers. PUCA's onboard microphones are the cable-free fallback.
3. Keep the already-owned **CoreS3 + Module Audio** stack as an independent
   fallback, visible diagnostic instrument, and implementation reference.
4. Preserve the decentralized fleet architecture. PUCA is an optional publisher,
   not a coordinator required for autonomous shows, timing, OTA, or safety.
5. First production milestone: perform audio analysis locally and publish the
   existing approximately 10 Hz `NB_DIRECT_FRAME` contract on fixed ESP-NOW
   channel 11. Preserve the fixture's three-second stale-frame fallback.
6. Do not stream raw audio. A future compact feature packet is allowed only after
   its semantics, rate, versioning, and fixture behavior are explicitly defined.
7. Put codec/I2S/Eurorack board support behind a PUCA-specific layer while reusing
   platform-independent audio feature code and the canonical fleet packet types.

## Consequences

- One calibrated source and one radio publisher replace per-fixture microphones.
- DJ line input, directional performer capture, and onboard-mic fallback live in
  one compact device.
- The Eurorack knobs/paw can expose the few controls that matter in the field
  without a laptop, but assignments must be made explicit in firmware.
- A separate original-ESP32 target and WM8978 driver are required; CoreS3 binaries
  and Strawberry-edition PUCA binaries are not interchangeable.
- The factory WiFi AP behavior must be removed from runtime firmware so ESP-NOW
  remains pinned to the production channel.
- The metal Pod20 enclosure and real placement require RF testing.

## Validation required

1. Complete the hardware, audio-input, knob, paw, and edition checks in
   `hardware/puca-audio-bridge/README.md`.
2. Demonstrate unclipped RODE and onboard-mic capture with visible/serial levels.
3. Demonstrate direct frames and stale fallback on one fixture, then a mixed
   HEX/RGBW group.
4. Measure range/PDR, DSP overruns, packet rate, resets, and multi-hour stability
   with the PUCA installed in the powered Pod20.
5. Record a known-good firmware artifact and a taped/marked field gain preset.

## References

- `hardware/puca-audio-bridge/README.md`
- `firmware/cores3_bridge/README.md`
- `firmware/fixture/src/core/packet.h`
- `docs/decisions/0004-mesh-esp-now.md`
- <https://github.com/ohmic-net/puca_dsp>
