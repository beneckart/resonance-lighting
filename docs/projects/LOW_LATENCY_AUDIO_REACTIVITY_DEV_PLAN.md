# Low-latency audio reactivity development plan

**Date:** 2026-08-26

**Owner:** Ben

**Status:** Milestone A source implemented; embedded and physical gates remain open

## Outcome

Sound-to-light latency is an artistic requirement, not merely a performance
metric. The target is a response that reads as attached to speech, percussion,
and musical transients across the awake fleet while preserving the existing
power vetoes, lease fallback, wire compatibility, and safe OTA process.

This plan deliberately separates two paths:

1. **CoreS3-only path:** improve capture, scheduling, and publishing without
   changing or flashing any fixture. This is the immediate bench path.
2. **Feature-packet fixture path:** add one compact broadcast of audio features
   and render it locally on updated fixtures. This is the scalable low-latency
   path for the approximately 130-fixture installation, but it is gated behind
   measurement, USB canaries, an ADR, and explicit OTA authorization.

Writing or executing this plan does not authorize a fixture flash or OTA.

## Definitions

- **Feature latency:** sound reaches the selected input -> the CoreS3 has a new
  usable envelope, onset, or band value.
- **Publish latency:** feature becomes usable -> the bridge transmits the state
  that contains it.
- **Sound-to-photon latency:** sound reaches the selected input -> a fixture's
  LEDs physically emit the corresponding new frame. This is the primary metric.
- **Fleet skew:** difference between the first and last responding fixtures for
  one event. A low average with a visibly smeared fleet is not a pass.
- **Settling:** time to reach 90 percent of a sustained target. Measure this
  separately from first visible response so smoothing cannot hide latency.
- **Release:** time for the look to decay after sound stops. Release is an
  artistic control, not input latency, and must be reported separately.

All latency reports use median, p95, and maximum over at least 100 events. Mean
alone is insufficient. Ambient-microphone reports record and subtract the known
2.9 ms/m acoustic travel time when comparing firmware paths.

## Current baseline and constraints

The accepted `cores3-os-0.1.2-dev` intensity build used last night captures 512
samples at 16 kHz every 100 ms. The capture window is 32 ms, the bridge publishes
at 10 Hz, and the fixture latches LEDs at 10 Hz. For the current 14-fixture awake
cohort, one `NB_DIRECT_FRAME` contains every addressed color.

That path has an expected sound-to-photon latency of about 100 ms typical and
about 200 ms worst-case. The fast envelope attacks to 65 percent on the first
update, so its first visible response is not deliberately delayed. Its slower
release can nevertheless make the look feel soft.

The unflashed spectral source analyzes at a nominal 25 Hz but retains a nominal
10 Hz direct-frame output. One scheduler issue must be fixed before hardware
testing: a 100 ms transmit deadline is polled on a 40 ms analysis grid, which
produces a 120 ms interval, or 8.3 Hz, in ideal timing.

The present fixture `renderTick()` has a fixed 100 ms latch interval. Sending
more often can reduce the age of the command waiting for that latch, but cannot
make the installed fixture image display more than 10 new frames per second.
Unsynchronized 10 Hz latch phases can also spread one fleet response across
nearly 100 ms.

At tree scale, individualized direct colors are the other limit. One direct
frame carries 18 fixtures, so 130 fixtures require eight packets per update.
Ten updates per second are about 80 broadcasts/s and roughly 10.3 KB/s of
payload before radio overhead. Raising this scheme to 50 Hz would require about
400 broadcasts/s. The scalable design must send features once, not pixels 130
times.

## Target architecture

```text
Ambient mic or Aux
        |
        v
short capture blocks -> common 16 kHz rolling sample buffer
        |                         |
        |                         +-> 512-sample FFT / bands at 25 Hz
        +-> envelope + onset at 125 Hz
                                  |
                 latest coherent feature snapshot
                    |                         |
                    +-> CoreS3 UI at 25 Hz    |
                                              v
                                  publisher chosen by fleet capability
                                  |                         |
                         legacy RGBW direct frames     one audio-feature packet
                                  |                         |
                         current 10 Hz fixture latch   local fixture mapper
                                                            |
                                                  event-driven / 50 Hz latch
```

The fast lane answers "when did something happen?" The spectral lane answers
"what part of the sound happened?" A transient may therefore create an immediate
brightness or pulse response while the latest stable spectrum determines color.
Frequency analysis does not sit in front of and delay the transient path.

CoreS3 owns input calibration, adaptive normalization, envelope/onset detection,
and band extraction. A future fixture owns deterministic mapping from compact
features to its local pixels and only enough time-based interpolation to avoid
packet-loss discontinuities. It must not add a second attack envelope.

## Development stages

### Stage 0 - Establish the physical latency bench

**Fixture firmware change:** none.

**Purpose:** create evidence that spans the microphone or Aux connector all the
way to emitted light.

Tasks:

1. Add CoreS3 diagnostic counters for achieved capture, analysis, display, and
   publish intervals; deadline misses; longest loop; send failures; and receive
   queue drops. Report rolling and maximum values without printing per sample.
2. Build a repeatable electrical path for Aux: inject a click or gated tone into
   Module Audio and observe a fixture with a photodiode or fast light sensor on
   a two-channel oscilloscope or logic analyzer. This is the authoritative
   sound-to-photon measurement because both edges share one clock.
3. Use a visible clap in 240 fps video as a useful Ambient-mic cross-check. Put
   the clap, CoreS3, and at least one fixture in the same frame and record the
   source-to-mic distance.
4. Record at least 100 events for broadband clicks and 80, 125, 500, 1000, and
   5000 Hz tone bursts. Measure first response, 90 percent settling, and release.
5. Measure one fixture and the 14-awake cohort. For the cohort, report fleet
   first-to-last skew as well as per-fixture latency.
6. Store the method and results in
   `docs/tests/AUDIO_REACTIVITY_LATENCY_BASELINE_2026-08.md`; store raw captures
   under a new, non-overwriting `ops/bench/data/audio_latency/` run directory.

Exit gate:

- A repeatable measurement varies by no more than 10 ms p95 between identical
  runs, or its larger measurement uncertainty is explicitly documented.
- Baseline median, p95, maximum, settling, release, and fleet skew exist for both
  inputs that will be compared.

### Stage 1 - Correct and isolate the CoreS3 schedulers

**Fixture firmware change:** none.

Tasks:

1. Replace `now + period` coupling with independent monotonic deadlines for
   capture/analysis, fixture publishing, display, and one-second status.
2. Advance deadlines by their period so a 100 ms publisher does not become a
   120 ms publisher on a 40 ms analysis grid. If a lane overruns, skip stale work
   and count it; never transmit a catch-up burst.
3. Publish the newest complete feature snapshot. UI drawing must never hold a
   mutex or critical section needed by capture or radio work.
4. Base the two-second input calibration on captured sample time, not a magic
   frame count, so later cadence changes do not alter its duration.
5. Keep pause, input handoff, black frame, lease release, app exit, and stale
   fallback behavior unchanged.
6. If the 25 Hz full-screen sprite prevents the timing gates, first reduce UI
   work per frame. Move capture/analysis to a dedicated FreeRTOS task only if the
   measured single-loop scheduler cannot meet the gate.

Exit gate:

- Analysis is 24-26 Hz over 60 seconds.
- The legacy publisher is 9.8-10.2 Hz over 60 seconds, with no 120 ms cadence.
- No watchdog reset, audio read failure, send failure, or receive-queue drop in
  a 30-minute Audio run while touching every control.

### Stage 2 - Add the fast transient lane and test the CoreS3-only ceiling

**Fixture firmware change:** none.

Tasks:

1. Capture 128-sample blocks at an effective 16 kHz, giving an 8 ms transient
   update opportunity, and maintain a rolling 512-sample buffer for the FFT.
2. Keep spectral analysis near 25 Hz with overlapping 32 ms windows. Do not wait
   for a new non-overlapping 512-sample block before updating the envelope.
3. Put Ambient and Aux into the same effective sample-rate domain. Prefer a clean
   16 kHz Module Audio codec mode if the library and hardware validate it;
   otherwise resample 44.1 kHz input into the common rolling buffer. This avoids
   Aux's current 86 Hz FFT bins and two-bin bass estimate.
4. Derive a fast onset strength independently from broadband level. Feed PULSE
   and any future impact accent from onset; feed sustained brightness from the
   envelope; feed color from the most recent band snapshot.
5. Bench direct-frame publishing at 10, 15, 20, and 25 Hz. Start with one packet
   at 14 fixtures. Then emulate or stage 36, 72, and 130 targets and measure
   send failures, downlink PDR, queue pressure, and fixture response. Do not
   select a fleet rate from packet arithmetic alone.
6. Keep 10 Hz as the safe automatic rate for an unknown or large legacy fleet
   until the rate matrix proves another bound. A labeled low-latency bench mode
   may use the faster proven rate for a small explicit cohort.

CoreS3 feature-latency gates:

- Broadband/onset p95 <= 20 ms from input edge to feature snapshot.
- Mid/high band p95 <= 45 ms from tone-burst edge to usable band snapshot.
- Achieved spectrogram cadence >= 24 Hz with no radio-health regression.

Current-fixture artistic gate on the 14-awake cohort:

- Intensity sound-to-photon median <= 75 ms and p95 <= 140 ms.
- Fleet skew p95 <= 100 ms; this is a documented current-firmware ceiling, not
  the final artistic target.

Decision gate A:

- If the CoreS3-only result is artistically convincing, keep the safe installed
  fixture image and defer the feature-packet rollout.
- If the 10 Hz fixture latch or fleet skew remains visibly detached from sound,
  continue to Stage 3. Do not try to solve it with unbounded direct-frame rate.

### Stage 3 - Specify one compact audio-feature packet

**Fixture firmware change:** source only; no hardware flash in this stage.

Create a new ADR before implementation. The recommended contract is a new
`NB_AUDIO_FEATURES` type 31 in the one canonical `packet.h`, consumed by a new
`PROG_AUDIO_FEATURES` program 6.

The first packet should remain near 24 bytes and contain only normalized,
fixed-point performance controls:

- RAM-only micro-lease and validity flags;
- look/mapping mode;
- broadband level;
- bass, mid, and treble levels;
- spectral centroid;
- onset strength;
- global brightness or artistic intensity.

`NbHeader` already supplies source identity, sequence, and source uptime. Do not
send raw audio, FFT bins, floats, per-fixture colors, NVS settings, or actuator
permission. Beat phase and tempo may be appended later after their algorithms
are validated; unused speculative fields do not belong in v1.

At 50 Hz, a roughly 24-byte feature packet is about 1.2 KB/s of payload and 50
broadcasts/s regardless of fleet size. This is approximately one ninth of the
payload of 130 individualized direct colors at 10 Hz.

Compatibility and authority rules:

1. Old fixtures ignore unknown type 31 and continue receiving direct frames.
2. Updated fixtures advertise feature-v1 support in currently unused bit 4 of
   `NbChoreoState.flags`; the CoreS3 adds type-18 receive/capability tracking.
3. During a mixed rollout, CoreS3 sends one feature stream for capable fixtures
   and 10 Hz direct entries for legacy fixtures. Unknown or expired capability
   is treated as legacy.
4. A fresh feature stream wins over direct frames on a capable fixture. An
   explicit Program Set lease still wins over both; entering Audio retains the
   existing one-shot program release.
5. The packet never writes NVS and never bypasses lifecycle, brightness, rail,
   battery, maintenance, or boot-safety vetoes.
6. A missing stream begins a local decay after 100 ms, reaches black by about
   300 ms, and returns to the configured autonomous program through the existing
   three-second stale fallback. A lost bridge must not freeze a bright audio
   frame for seconds.

Native exit gate:

- Golden size/offset tests pin the new struct and type value.
- Old packet fixtures ignore it safely.
- Sequence gaps, wraparound, duplicate packets, two competing sources, mixed
  direct/feature input, program precedence, stale decay, and three-second
  fallback all have native tests.

### Stage 4 - Implement local fixture audio rendering on USB canaries

**Fixture firmware change:** yes, but USB canaries only. No OTA.

Tasks:

1. Add pure `AudioFeatureState` and `ProgAudioFeatures` code under the native-
   tested choreography core. Radio receive only copies the newest validated
   feature state; it never renders from the ESP-NOW callback.
2. Map the existing artistic looks locally using fixture class, stable short-ID
   hashing, and pixel count: CLASSIC, EMBER, HUECYCLE, PULSE, BANDS RGB, BANDS
   SPLIT, and TIMBRE HUE.
3. Preserve one owner for smoothing. CoreS3 owns feature attack/release; the
   fixture interpolates only between feature snapshots and performs the missing-
   stream decay. A new sound edge must not wait through a second attack filter.
4. Keep ordinary programs at their present render cadence. Only an active fresh
   audio-feature lease may request an event-driven latch capped at 50 Hz. With a
   maximum 37-pixel strip, measure rather than assume RMT/show duration and loop
   headroom.
5. Advertise capability bit 4 and expose serial-only canary diagnostics for
   received feature rate, gaps, age, active source, renders, and overruns.
6. USB-flash one named HEX and one named RGBW fixture. Verify every power veto,
   lease precedence, black/stale behavior, and ordinary-program regression
   before adding canaries.

Steady-state latency gates, excluding an intentionally cold rail-on safety ramp:

- Aux electrical intensity: median <= 35 ms, p95 <= 55 ms.
- Ambient intensity, corrected for acoustic distance: median <= 45 ms,
  p95 <= 70 ms.
- Mid/high band response: median <= 55 ms, p95 <= 80 ms.
- Stable 60-125 Hz bass response: median <= 80 ms, p95 <= 120 ms.
- Two-fixture first-photon skew p95 <= 15 ms.
- No render overrun, watchdog reset, rail-safety regression, or control RX loss
  during a 60-minute mixed-look run.

Decision gate B:

- Promote beyond USB canaries only if the measured improvement is artistically
  material and the power/radio regressions are zero. A faster loop counter by
  itself is not grounds for fixture rollout.

### Stage 5 - Validate mixed fleet and tree-scale radio load

**Fixture firmware change:** USB canaries first; a later OTA cohort requires a
separate explicit operator decision.

Tasks:

1. Expand from two USB canaries to five, then the current 14-awake cohort with a
   deliberate mix of capable and legacy firmware.
2. Confirm capability aging and dual publication: capable fixtures follow one
   50 Hz feature stream; legacy fixtures continue receiving only their 10 Hz
   individualized direct entries; no fixture oscillates between programs.
3. Run 10/20/25/50 Hz feature rates and the expected worst mixed-rollout direct
   load. Record downlink PDR, RSSI, sequence gaps, bridge failures, fixture gaps,
   fleet skew, watchdog health, and power draw.
4. Re-run the established network projection at 130 nodes and retain 150 as the
   conservative stress case. Include normal heartbeats and choreography traffic,
   not only the new packet.
5. Run source-loss, bridge reboot, competing publisher, packet burst, sequence
   wrap, and three-second fallback tests.
6. Run at least a four-hour music/ambient soak on production-representative LFP
   power before considering fleet promotion.

Exit gate:

- Feature downlink PDR >= 99 percent at the selected rate on the staged fleet.
- No p95 latency or fleet-skew regression as cohort size grows.
- No old-fixture control regression during mixed publication.
- No unexplained reset, queue overflow, rail fault, or power-policy bypass.

### Stage 6 - Promote a fixture artifact only after an explicit go decision

**Fixture firmware change:** yes. This is the only fleet rollout stage.

1. Record the final architecture and measured rates in an accepted ADR.
2. Build one clean immutable `fx-YYMMDD-<recipe7>-p` artifact and manifest under
   ADR 0040. Build once; OTA that exact binary.
3. Install an LFP or separately proven stable supply on every OTA canary. Bare
   USB is not valid evidence through the pending-verify window.
4. Use the shared-WiFi/router OTA path, one declared operator, and explicit short
   MACs. Do not use the deprecated self-hosted maintenance AP.
5. Promote in gates: one USB rescue canary -> one OTA canary -> five mixed-role
   canaries -> 20 fixtures -> remainder. Stop on any latency, PDR, reset,
   rollback, class, sensor, rail, or power anomaly.
6. Completion requires a fresh post-job heartbeat, exact expected revision,
   and survival beyond the 20-second pending-verify window. Upload ACK is not
   completion.
7. Preserve the previous known-good artifact and rehearse rollback before the
   remainder batch.

## Artistic tuning after the transport is fast

Do not tune away transport latency with long smoothing. Once Stages 1-4 establish
the response budget, evaluate each look using the same source clips and controls:

- transient attack and accent strength;
- sustained envelope release;
- independent bass/mid/treble release;
- band crossover and Aux/Ambient parity;
- brightness floor and dynamic range;
- stable-ID spatial assignment versus class-based assignment;
- packet-loss interpolation;
- an optional operator-facing Fast / Balanced / Sculpted feel preset.

The preferred spectral look combines the fast onset lane with slower frequency
identity: onset controls the immediate accent, while bands or centroid control
its color. This preserves musical attachment without making 60 Hz bass pretend
to have the time resolution of a clap.

## Milestones and recommended order

| Milestone | Scope | Expected focused bench work | Fixture OTA? |
| --- | --- | ---: | --- |
| A | Stages 0-1: baseline, scheduler fix, honest telemetry | 1-2 sessions | No |
| B | Stage 2: rolling capture, onset lane, legacy-rate matrix | 1-2 sessions | No |
| C | Stage 3: ADR, packet, native compatibility tests | 1 session | No |
| D | Stage 4: HEX + RGBW USB canaries | 2-3 sessions | No |
| E | Stage 5: mixed fleet, RF/load/soak validation | 2-3 sessions | Only after a separate go |
| F | Stage 6: named artifact and gated fleet promotion | Dedicated rollout window | Yes |

The immediate next action is to finish Milestone A's embedded and physical
gates: build the corrected CoreS3-only image, reconnect exact CoreS3 `4D5DB0`,
record its 60-second/30-minute cadence evidence, then populate the Aux/Ambient
latency baseline without touching fixture firmware.

## Required deliverables

- `docs/tests/AUDIO_REACTIVITY_LATENCY_BASELINE_2026-08.md`
- repeatable non-overwriting latency capture tooling and raw run metadata
- achieved-cadence and overrun telemetry on CoreS3
- native tests for rolling capture, scheduler behavior, onset, spectral bands,
  packet layout, mixed compatibility, lease precedence, and stale decay
- an ADR before allocating type 31/program 6
- one measured HEX/RGBW USB-canary report
- one mixed-fleet/tree-scale RF report
- one ADR 0040 artifact manifest and rollback record if Stage 6 is authorized

## Explicit non-goals for the first low-latency release

- raw audio or full FFT transport over ESP-NOW;
- pitch transcription;
- persistent fixture audio settings;
- automatic fixture OTA from the Audio app;
- bypassing lifecycle or power policy to reduce latency;
- tempo/section/drop inference before onset and three broad bands meet their
  hardware gates;
- increasing every fixture program's render rate when only live Audio needs it.

## References

- `docs/decisions/0061-cores3-multirate-spectral-audio.md`
- `docs/howto/CORES3_AUDIO_REACTIVE.md`
- `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`
- `firmware/cores3_bridge/audio_reactive.h`
- `firmware/cores3_bridge/cores3_bridge.ino`
- `firmware/fixture/src/core/packet.h`
- `firmware/fixture/src/core/choreo/program.h`
- `firmware/fixture/fixture.ino`
