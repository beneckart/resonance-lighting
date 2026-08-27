# Audio reactivity sound-to-photon latency baseline

**Started:** 2026-08-26

**Status:** measurement method and firmware instrumentation implemented;
physical measurements pending an attached CoreS3 and latency sensor

**Primary CoreS3:** `4D5DB0` (`80:45:6B:4D:5D:B0`)

## Purpose

Measure the artistic response path from sound at the selected CoreS3 input to
light physically emitted by fixtures. Loop frequency, packet acknowledgements,
and on-screen motion are supporting diagnostics; none substitutes for a
sound-to-photon measurement.

Report these separately:

- first visible response;
- time to 90 percent of a sustained target;
- release after sound stops;
- median, p95, and maximum over at least 100 events;
- first-to-last fixture skew for a cohort.

## Current analytical baseline, not physical evidence

The installed `cores3-os-0.1.2-dev` intensity path uses a 512-sample, 16 kHz
capture every 100 ms and current fixtures latch LEDs every 100 ms.

| Stage | Expected contribution |
| --- | ---: |
| Sound -> completed CoreS3 analysis | about 50 ms average, up to 100 ms |
| ESP-NOW send and receiver queue | usually a few ms |
| Receiver -> next installed-fixture latch | about 50 ms average, up to 100 ms |
| Total | about 100 ms typical, about 200 ms worst-case |

This table is the hypothesis the physical bench must replace. It excludes
source-to-microphone travel, which is approximately 2.9 ms/m.

## Milestone A firmware instrumentation

The CoreS3 source now uses independent phase-locked analysis, publish, display,
and one-second status deadlines. A late lane advances from its prior deadline,
skips stale periods, and never emits a catch-up burst. This corrects the spectral
source's former 40 ms / 100 ms quantization from a permanent 120 ms interval
(8.3 Hz) to a 100 ms long-term publisher cadence.

The on-device Audio header reports achieved, not requested, analysis and publish
rates. USB status adds one `audio-timing` line per second:

```text
audio-timing cal_ms=... cal_obs=... capture_hz_x1000=... analysis_hz_x1000=... analysis_ms=min/max tx_hz_x1000=... tx_ms=min/max display_hz_x1000=... skip=analysis/tx/display late_max_ms=analysis/tx/display sendfail=... rxdrops=... max_us_capture_analysis_tx_display_loop=capture/analysis/tx/display/loop
```

Interpretation:

- rates are Hz x 1000 (`24980` = 24.980 Hz);
- `skip` counts whole periods intentionally discarded after an overrun;
- `late_max_ms` is the worst dispatch lateness since Audio was started;
- `sendfail` and `rxdrops` are cumulative bridge radio-send failures and receive-
  queue drops, so a visually smooth screen cannot hide mesh health regression;
- `max_us...` identifies whether capture, FFT, fleet packetization, screen
  transfer, or total loop work owns the worst blocking interval;
- calibration requires two seconds across contiguous successful capture
  completions; a capture gap longer than 200 ms restarts its observation clock.
- if capture remains stale for more than 200 ms, publishing stops and one black
  direct frame is sent so the decoupled publisher cannot replay a bright sample.

## Authoritative Aux measurement

Required equipment:

- Module Audio on CoreS3 with selector B;
- repeatable electrical click or gated-tone generator into LINE/MIC;
- photodiode or fast light sensor held against one named fixture;
- two-channel oscilloscope or logic analyzer with analog threshold inputs;
- one named awake fixture for the first pass, then the declared cohort.

Connections:

1. Channel 1 observes the electrical input edge sent to Module Audio.
2. Channel 2 observes the photodiode/light-sensor edge.
3. Both channels use the same acquisition clock.
4. Record the exact CoreS3 and fixture revisions, input gain, mode, live cohort,
   and distance from sensor to LEDs.

For each mode/source combination, capture at least 100 events. Export raw edges
or scope CSV without overwriting an earlier run. Use the immutable run naming
and metadata checklist in `ops/bench/data/audio_latency/README.md`.

## Ambient-microphone cross-check

Record a visible clap, CoreS3, and fixture in one 240 fps video frame. Keep the
clap close to the microphone, record the distance, and subtract 2.9 ms/m only
when comparing the firmware portion with Aux. Video at 240 fps has a 4.17 ms
frame quantum and is a cross-check, not the primary scope result.

For fleet skew, frame all participating fixtures or use one sensor per output
when available. The event edge is hand contact; the response edge is first
visible LED change.

## Source matrix

| Source | Purpose | Minimum events |
| --- | --- | ---: |
| Broadband electrical click | Intensity and onset | 100 |
| Visible clap near Ambient mic | Acoustic end-to-end cross-check | 100 |
| 80 Hz gated tone | Low-bass settling | 100 |
| 125 Hz gated tone | Bass band | 100 |
| 500 Hz gated tone | Low-mid band | 100 |
| 1 kHz gated tone | Mid band | 100 |
| 5 kHz gated tone | Treble band | 100 |

Run CLASSIC or EMBER for broadband intensity, PULSE for onset, BANDS RGB and
BANDS SPLIT for separation, and TIMBRE HUE for centroid movement. Record attack
and release parameters with every run.

## Cohort matrix

1. One named fixture, installed firmware.
2. Current 14-awake cohort, installed firmware.
3. One named HEX plus one named RGBW USB canary, only if the future fixture
   phase is explicitly authorized.
4. Five feature-capable canaries.
5. Mixed capable/legacy cohort.
6. 130-node projection and 150-node stress case after staged RF validation.

## Result table

Do not fill a row from visual impression or firmware timestamps alone.

| Date/run | Input | Mode/tone | Cohort | Bridge rev | Fixture rev | n | median ms | p95 ms | max ms | settle90 ms | release ms | skew p95 ms | Notes |
| --- | --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| pending | | | | | | | | | | | | | |

## Immediate acceptance gates

Milestone A timing gates:

- analysis 24-26 Hz over 60 seconds;
- legacy publisher 9.8-10.2 Hz over 60 seconds;
- no permanent 120 ms publish cadence;
- zero watchdog resets, audio read failures, send failures, and RX queue drops
  in a 30-minute control/UI run.

CoreS3-only artistic gate on the current 14-awake cohort:

- intensity median <= 75 ms;
- intensity p95 <= 140 ms;
- fleet-skew p95 <= 100 ms.

The later feature-capable fixture gates live in
`docs/projects/LOW_LATENCY_AUDIO_REACTIVITY_DEV_PLAN.md`.

## Evidence status on 2026-08-26

- Native CoreS3 tests: 167 checks, zero failures.
- CoreS3 Module Audio embedded build: fresh exclusive `r4` pass at 1,187,435
  sketch bytes (37 percent) and 100,320 bytes dynamic memory (30 percent).
  The 1,187,584-byte binary SHA-256 is
  `8FE579C3B83B8481776D0028635C133031D94EDD5B3A3C9E1077F8FBA65724FF`;
  `build.options.json` confirms `esp32:esp32:m5stack_cores3`, channel 11, and
  `CORES3_AUDIO_MODULE=1`.
- Attached USB hardware: exact CoreS3 `4D5DB0` was identified on `COM43` as
  full MAC `80:45:6B:4D:5D:B0`; its pre-flash boot reported
  `cores3-os-0.1.2-dev`, channel 11, and Module Audio ready. T-Deck `8EB508`
  remained on `COM152` and was not opened.
- Fixture firmware, flash, OTA, NVS, profile, lifecycle, and mesh output: not
  changed.
