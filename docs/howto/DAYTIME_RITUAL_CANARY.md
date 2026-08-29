# Exact-target daytime ritual canary

Use this procedure to validate ADR 0071's scheduled cymbal ritual on one named
downlight without enabling an hourly test image across the fleet. The canary is
non-interactive: no ToF approach, button press, open lid, or bridge command is
part of the ritual.

## Safety contract

The test artifact is locked at compile time to both:

- one six-digit fixture short MAC; and
- one Unix UTC hour key (`utc_s / 3600`).

A target mismatch cannot participate or shorten sleep for the ritual. The
artifact cannot actuate in an adjacent or later hour. The build wrapper requires
variant `t`, field profile, an exact target and exact hour together, and refuses
`--solenoid-test`; the canary therefore cannot bypass the production strike
gate. Normal class, scheduled-day, valid-time, uncertainty, FULL-tier, measured
solar, authority, boot guard, rest-time, NVS marker, and mechanism vetoes remain
in force.

The latest mainline PROTECT fix is also required: power policy owns the
continuous recovery window and the independent daytime cadence cannot sleep a
PROTECT fixture. A fixture in PROTECT remains silent.

## Before building

1. Physically identify one battery-installed, cymbal-bearing downlight and
   record its exact short MAC, current `fw_rev`, binary SHA-256, profile,
   channel, class, sensor signature, battery state, and solenoid arm state.
2. Declare one OTA/NVS operator and the exact target. Do not send fleet
   maintenance, profile, or strike commands during the run.
3. Select a future scheduled-day UTC hour far enough away to build, inspect,
   OTA, pass pending verify, and establish fresh UTC plus energy readiness.
4. Convert the top-of-hour UTC timestamp to its integer Unix hour key. For
   example:

```bash
python -c "from datetime import datetime; print(int(datetime.fromisoformat('2026-08-29T19:00:00+00:00').timestamp()) // 3600)"
```

## Build once

Commit the clean source first. Then build one generated immutable artifact:

```bash
cd firmware/fixture
./build.sh --artifact-variant t \
  --wifi-profile-label party-in-the-woods-v1 \
  --profile field --channel 11 \
  --daytime-ritual-target ABCDEF \
  --daytime-ritual-hour 496700
```

The wrapper derives `fx-YYMMDD-<recipe7>-t`, writes the manifest and exact
binary SHA-256, and refuses direct upload. Inspect the manifest and use the
retained `.bin`; never rebuild between review and OTA.

Upload only with exact-target tooling, for example:

```bash
python ops/bench/field_cycle_ota.py ABCDEF \
  --bin firmware/fixture/build/<fw_rev>/fixture.ino.bin \
  --expect-fw <fw_rev> --discover-timeout 360 \
  --notes "one-target one-hour ADR 0071 cymbal canary"
```

Require a fresh exact-revision heartbeat and survival past pending verify.
Verify that NVS still reports field profile, channel 11, downlight class, the
expected sensors, an armed solenoid, no class mismatch, no BQ fault, recovery
zero, and sufficient measured renewable-side energy. Under ADR 0072 that final
gate is `supply_good` plus either at least 150 mA live input or at least 5.8 V
on the VDC/solarnoid reservoir, always with FULL battery tier.

## Expected hour

The target uses its ordinary daytime cadence until the final sleep that lands
at T-20 seconds. With acceptable energy and time it remains awake only through
T+47 seconds:

```text
T-20.0        wake/listen for fresh time quality
T+05.0        unison attempt (requires uncertainty <=500 ms)
T+12.0..35.5 deterministic 500 ms organic-roll slot
T+42.0..45.5 optional deterministic quarter-fleet after-ring
after T+47     ordinary daytime sleep cadence; no later ritual is eligible
```

An event more than 350 ms late is dropped. Uncertainty from 501 through 3,000
ms suppresses only the unison; weaker or invalid time suppresses the ritual.

## Evidence without lid access

Each act and the window boundaries force a full heartbeat. Updated CoreS3 and
T-Deck bridges emit these fields, and `net_bench_dashboard.py` plus
`net_bench_log.py` preserve them:

```text
ritf ritexp ritat ritfire ritref ritblk ritu rith ritcanh ritcantgt
```

Event masks are bit 0 unison (`1`), bit 1 roll (`2`), and bit 2 after-ring
(`4`). `ritf` flags are bit 0 canary build, bit 1 target match, bit 2 window
seen, and bit 3 window complete.

For the primary high-quality-time acceptance, require:

- `ritf == 15` after T+47;
- `ritcantgt` equals the declared short MAC;
- `rith == ritcanh` and both equal the declared Unix hour key;
- `ritu <= 500` ms;
- `ritexp` is `3` or `7` according to the deterministic after-ring assignment;
- `ritat == ritfire == ritexp`; and
- `ritref == 0` and `ritblk == 0`.

Also retain `supply_v`, `supply_ma`, `supply_good`, battery tier, charger
enable/phase/fault, and solenoid counters around the window. A charge-done
fixture may correctly report 0 mA while its VDC reservoir remains charged;
that is the ADR 0072 regression case, not missing-solar evidence.

The same fields are available by targeted maintenance `GET /telemetry` if the
installed bridge has not yet been updated for the heartbeat tail. Audible
timing is corroborating evidence; the machine-readable masks are the
acceptance record.

## Closeout

Restore the exact retained prior fleet binary to the same MAC. Require a fresh
exact-prior rejoin and pending-verify survival, then record both OTA ledgers,
the ritual JSONL, audible observations, target/hour, artifact manifest, and
both binary SHA-256 values. Do not widen the cohort until this one-target run
passes.
