# Audio latency raw runs

Store each physical sound-to-photon run in a new directory named:

```text
YYYYMMDDTHHMMSSZ-<input>-<mode>-<cohort>
```

Examples:

```text
20260827T031500Z-aux-classic-4D5DB0_9E668C
20260827T032700Z-ambient-bands-rgb-14awake
```

Never reuse, replace, or append to an earlier run directory. If a run is
repeated, create a new UTC timestamp. Keep the instrument's original CSV, logic
capture, or video file alongside `metadata.txt` containing:

- CoreS3 full/short MAC and firmware revision;
- fixture short MACs and exact firmware revisions;
- input, Audio mode, tone/click source, gain, and filter settings;
- input-to-microphone and fixture-to-sensor distances;
- instrument model, sample rate, trigger thresholds, and channel mapping;
- event count and any exclusions with reasons;
- UTC start/end and operator.

Derived tables or plots must name the raw directory they came from. Do not
commit large raw video or scope captures blindly; preserve them locally first,
then decide whether Git LFS or an external immutable archive is appropriate.

The measurement protocol and result table are in
`docs/tests/AUDIO_REACTIVITY_LATENCY_BASELINE_2026-08.md`.
