# 0061 -- CoreS3 multirate spectral audio stays inside the Audio app

**Date:** 2026-08-26

**Status:** Accepted for hardware experiment

**Owner:** Ben

## Context

CoreS3 Bridge OS already has one Audio app that owns microphone selection,
noise-floor calibration, the RAM-only program-lease handoff, direct-frame
publishing, and safe pause/exit behavior. Its first looks use only one broadband
RMS envelope. Ben wants to explore short-window frequency analysis, a fast local
spectrogram, and looks driven by bass, mid, and treble bands.

A second spectral app would duplicate the same audio-device and fleet-ownership
lifecycle. It would also create a new transition where two app surfaces could
appear to own one I2S input. The fixture fleet does not need raw audio or an FFT
wire format: it already consumes final RGBW values in `NB_DIRECT_FRAME`.

The existing implementation ties sampling, output, and screen feedback to the
fixture render cap of 10 Hz. That rate is appropriate for the fleet but too slow
for a useful spectrogram.

## Decision

1. Spectral analysis is an additional engine and view inside the existing Audio
   app, not a third Bridge OS app.
2. Capture and analysis use a 512-sample Hann-windowed radix-2 FFT at a nominal
   25 Hz. The built-in microphones are analyzed at 16 kHz. Module Audio retains
   its 44.1 kHz codec setting and its interleaved stereo input is collapsed to
   mono before analysis.
3. The screen keeps about three seconds of 24-row log-frequency history and
   refreshes at the analysis cadence while Audio is active. Bass, mid, and high
   meters remain visible beside the scrolling spectrogram.
4. Fleet `NB_DIRECT_FRAME` output remains capped at the proven 10 Hz. No audio,
   FFT bins, band values, new packet type, fixture OTA, or fixture NVS change is
   introduced.
5. The initial common bands are 60-250 Hz, 250-2000 Hz, and 2000-8000 Hz. Each
   band has an independent adaptive floor, peak memory, attack, and release.
6. Existing CLASSIC, EMBER, HUECYCLE, and PULSE looks remain. The experiment adds:
   - BANDS RGB: bass, mid, and treble drive shared red, green, and blue;
   - BANDS SPLIT: stable fixture-ID thirds follow bass, mid, or treble;
   - TIMBRE HUE: spectral centroid selects hue while energy selects brightness.
7. Start, pause, app exit, input handoff, lease release, target selection, and
   three-second fixture fallback retain their existing contracts.

## Consequences

- One Audio surface can compare envelope and FFT looks without stopping capture
  or recalibrating between modes.
- The local interface can be fluid without increasing fleet airtime.
- Frequency resolution is about 31 Hz on the built-in 16 kHz path and about
  86 Hz on the 44.1 kHz Aux path. This is sufficient for broad musical bands,
  not pitch transcription.
- A 512-point FFT, fixed work buffers, and spectrogram history add several
  kilobytes of static RAM but no heap churn in the real-time loop.
- More advanced onset, tempo, section, or drop analysis can build on the same
  spectral engine later without changing the fixture wire contract.

## Validation required

1. Confirm Ambient and Aux both show plausible quiet-room and spoken/music
   spectra without a false high-frequency rail from stereo interleaving.
2. Measure achieved analysis/display cadence and confirm the radio receive queue,
   watchdog, touch controls, and 10 Hz fleet output remain healthy.
3. Feed or play bass-, mid-, and treble-dominant material and confirm all three
   new looks separate the intended bands on a named awake cohort.
4. Confirm mode changes do not recalibrate or interrupt publishing, while Input
   changes still send black, pause capture, and recalibrate the new source.
5. Pause and leave Audio, then confirm the roughly three-second autonomous
   fallback and no persistent fixture mutation.

## References

- `firmware/cores3_bridge/audio_reactive.h`
- `firmware/cores3_bridge/cores3_bridge.ino`
- `docs/decisions/0054-cores3-wireless-two-app-bridge-os.md`
- `docs/decisions/0057-cores3-runtime-audio-input-selection.md`
- `docs/decisions/0058-cores3-audio-releases-prior-program-lease.md`
