# ADR 0082: Locally timed signature for the decompression canary

Date: 2026-10-11
Status: Accepted for development on Ness (9F2738); not a fleet promotion.

## Context

Ben wants a small interactive chime installation for decompression at the Loom.
Lantern siting and the final roster are undecided. A USB-driven seven-hit
"shave and a haircut" sounded good once and uneven on a repeat. Both host
captures showed about 30 ms of inter-command timing error. Buffered serial
reports did not establish physical hit times. The good replacement enclosure
extension passed data/charging tests; rhythmic variation is not proof it failed.

## Decision

The first `--decompression` canary is a build option of the existing fixture
firmware, with the same power manager, identity, sensors, OTA and NVS history.
It is an immutable `b` artifact. It does not reinterpret the fleet packet
contract or require a new controller protocol for this first USB/USER proof.

One exact-target, newline-terminated USB command `!K9F2738:1` starts a local
signature. USER triggers the same signature. The fixed 40 ms hits have offsets
0, 500, 750, 1000, 1500, 2500 and 3000 ms. A short ESP timer callback processes
absolute deadlines independently of the sensor/power loop. It contains no
serial logging, allocation, I2C or NVS writes. The durable load marker is armed
before starting the phrase. The existing pulse timer and loop cutoff remain.

There is one active phrase, no queue and no restart-on-retrigger. A deadline
miss over 20 ms aborts rather than compressing or replaying late hits. Normal
single strikes cannot interleave with a phrase. Cancellation, maintenance,
sleep and power veto end the phrase and gate drive. A one-second post-phrase
cooldown limits repeated requests. Telemetry records the phrase result, hit
count, relative onset timestamps and worst onset lateness.

The first explicit successful signature trigger opens a 60-minute RAM-only
session. Repeated triggers do not extend that session. `!D9F2738:<minutes>`
explicitly sets 1..120 minutes; zero ends it. `!K9F2738:0` cancels only the
current phrase. These operations require complete, exact-target envelopes.

During a session the fixture stays reachable and shows its existing class
listener light. The decompression build suppresses the BRC schedule, automatic
field chimes and presence propagation. No session resumes automatically after
reset. Without a session it has a 15-second startup listening window and then
ordinary timer sleep; USER can wake it and request a new session/signature.
Existing independent PROTECT sleep and recovery policy remain authoritative.
New signature requests require a present battery and FULL/DIM allowance.

## Consequences and limits

- USB/radio arrival determines phrase start, not spacing between its hits.
  Multi-lantern synchronized start is a separate future contract.
- This is a button/USB canary, not the complete public installation controller.
  Explicit-roster remote triggering, presence rearm/cooldown, final session
  duration, siting and runtime qualification remain open.
- Preserve Ness's original full flash/NVS privately and its public identity
  and telemetry before flash. A build or successful upload is not acceptance;
  verify the actual device revision, timing, cancellation and stable operation.
- The VBAT-fed boost stays connected when firmware sleeps; the gauge does not
  measure that branch. No firmware mode substitutes for branch energy and
  low-battery cutoff qualification.
- ESP timer task dispatch can still be delayed by higher-priority work. The
  lateness abort makes that visible. Recorded GPIO timing is not an acoustic
  measurement; cap refill, solenoid travel and cymbal response need listening
  and physical measurement.

Reference: [ESP-IDF timer callback constraints](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/esp_timer.html).
