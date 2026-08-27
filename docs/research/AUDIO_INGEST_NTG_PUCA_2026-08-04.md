# Audio ingest -- VideoMic NTG -> PUCA DSP setup notes (2026-08-04)

Wired-first capture chain for audio-reactive show modes (DJ line feed, sound bath /
bowls, violin, singing). Architecture reminder: fixtures never see audio -- an ingest
node extracts compact features locally. ADR 0035 subsequently fixed the first
milestone at about 10 Hz using the existing `NB_DIRECT_FRAME` contract; the
30-60 Hz feature-packet idea below is future work, not the current wire contract.
This note records the hardware bought 2026-08-04 and the setup/bench items so
they are not rediscovered on playa.

Hardware on hand / inbound:

- 2x PUCA DSP (Crowd Supply, $107.68 the pair) -- ESP32-PICO-D4 + WM8978 codec,
  stereo 3.5 mm line-in with 3 dB-step PGA, dual Knowles MEMS mics (differential,
  beamformable). NOTE: classic ESP32, not S3 -- a second build target next to the
  PowerFeather fleet.
- RODE VideoMic NTG + WS11 furry windshield (Amazon, $265.14 + $71.08) + RODE
  Tripod 2 (RODE direct, $66.44) -- performer capture. Chosen over cheaper battery
  shotguns for the continuously variable output (time > money; adjustable gain
  de-risks the unknown line-in pairing).
- UHF wireless lav kit: DEFERRED until a roaming performer materializes. If bought,
  it must be UHF (~550-600 MHz), never a 2.4 GHz system (Rode Wireless GO / DJI Mic
  class) -- 2.4 GHz is the ESP-NOW control-plane band and is saturated at BRC anyway.
  If a performer sings over bowls: headset, not lapel.

## NTG settings + bench checks -- the three that matter

1. **Engage the NTG's high-pass filter (75 or 150 Hz).** Wind and distant art-car
   sub-bass are low-frequency energy the feature extractor cannot distinguish from
   music -- unfiltered, the lights pulse to the weather. The HPF is analog rejection
   *before* the ADC; it stacks with the WS11 furry and with any firmware band
   weighting, it does not replace them.
2. **Gain-stage at the mic, not the codec.** Run the NTG's variable output knob hot
   (toward headphone/line level) and keep the WM8978 input PGA low. Gain added at
   the mic where the signal is clean beats gain added at the codec input. Use a
   plain TRS-TRS cable -- not a TRRS/phone cable (the NTG's output auto-senses and
   a TRRS cable puts it in smartphone mode).
3. **Bench-verify NTG auto-power into the bias-less line-in.** The NTG auto-powers
   by sensing plug-in power from a camera/phone; the PUCA line-in supplies none.
   Expected fine (the mic has manual power too), but confirm it powers on AND stays
   on feeding the PUCA jack before it ships to playa. Find this quirk on the bench,
   not at 2 AM in front of a violinist.

## Placement + field notes

- **Proximity is the isolation, not the polar pattern.** Supercardioid rejects
  off-axis mids/highs but pattern control collapses at low frequencies -- exactly
  where playa noise lives. Boom the mic 30-50 cm from bowls/violin; at 2 m it loses
  to the neighbor camp's subs regardless of pattern.
- **Tether/sandbag the Tripod 2** -- a light tripod wearing a furry windjammer is a
  sail.
- **PUCA enclosure (print farm):** foam over the MEMS mic ports (same wind-as-bass
  problem as above). Onboard MEMS pair = zero-setup ambient fallback mode.
- **NTG doubles as a USB-C audio interface** -- plugs into the build-week laptop as
  the high-fidelity/experimental feature-publisher tier, no extra hardware.
- Feature message format should carry a `source` field so fixtures can blend
  multiple publishers (e.g. bowls spectral node + close vocal/violin node).
- Bowls firmware note: sound-bath mode wants longer FFT windows / slow envelopes,
  not beat detection (strong 100-800 Hz fundamentals, slow beating partials).
