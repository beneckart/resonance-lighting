# Firmware build acceleration smoke -- 2026-08-22 through 2026-08-24

## Result

Adopt the locked `--dev-cache` path for local fixture iteration. Keep shared and
fleet artifacts on the fresh immutable path. Cache reuse reduced a representative
fixture build from a 187.2-second fresh median to roughly 14 seconds, about 13x
faster. All host-side cache-safety gates passed. No fixture was flashed and no OTA
was attempted during this work.

Explicit Arduino job-count tuning is not adopted yet. A `--jobs 1` trial became
invalid when the laptop suspended with an orphaned compiler, and the final
`--jobs 0` trial was deliberately aborted. Both temporary/partial directories
were abandoned or quarantined; neither result is counted below. Avoiding work is
the proven improvement.

## Environment

- Phase 0 source: `e6f7d7546288f35336e9573cd9f93d9f81ae4bd7`
- Implementation: `4558a81` (`firmware: add locked fixture development cache`)
- Host: Windows 11 + Git Bash, Intel i7-11370H, 4 cores / 8 logical CPUs,
  31.8 GiB RAM
- Arduino CLI: 1.5.1
- ESP32 platform: 3.3.7
- PowerFeather-SDK: 2.1.0
- Libraries: Adafruit BusIO 1.17.4, Adafruit MSA301 1.1.4,
  Adafruit NeoPixel 1.15.5, Adafruit Unified Sensor 1.1.15,
  SparkFun Qwiic TMF882X Library 1.0.2
- Recipe: PowerFeather V2, LFP, 300 mA precharge, channel 11, commission
  profile

All fixture native tests passed before the experiment and again after the
implementation. `bash -n` passed for both the wrapper and smoke script.

## Phase 0 measurements -- unmodified wrapper

| Case | Seconds | RC | Binary SHA-256 prefix | Notes |
|---|---:|---:|---|---|
| Fresh default 1 | 187.153 | 0 | `2a04d5b8` | fresh `/tmp` path |
| Fresh default 2 | 217.748 | 0 | `a89a658f` | fresh `/tmp` path |
| Retained cold seed | 274.305 | 0 | `e29c14c3` | existing `--artifact-dir` proof only |
| Retained no-op 1 | 13.119 | 0 | `e29c14c3` | identical SHA to seed |
| Retained no-op 2 | 11.424 | 0 | `e29c14c3` | identical SHA to seed |
| Timestamp-only leaf touch | 11.442 | 0 | `e29c14c3` | Arduino detected unchanged content; no rebuild |
| Timestamp-only common-header touch | 11.390 | 0 | `e29c14c3` | not a valid high-fanout measurement |
| Harmless leaf content edit | 12.522 | 0 | `d7f4b2f8` | real source-content change |

The Phase 0 gate passed decisively. The two no-op trials used 6.1% of the
202.5-second two-trial cold median and saved about 190 seconds. The real leaf
content edit used 6.2% and saved about 190 seconds.

## Protected-cache measurements

The implementation adds fingerprint collection and lock/marker work, so its
warm overhead is slightly higher than the raw retained-directory proof:

- First locked cold seed, `--jobs 2`: 243.571 seconds, RC 0.
- Direct locked no-op: 14.252 seconds, RC 0, `DEV_CACHE_HIT`, identical SHA.
- Full-smoke cold seed: 285 seconds, RC 0.
- Full-smoke warm no-op: 18 seconds, RC 0, identical SHA.
- Post-quarantine cold recovery: 308 seconds, RC 0.
- Disposable-worktree ordinary compiler error: 10.821 seconds, RC 1.
- Same cache after correcting that source error: 16.089 seconds, RC 0 and
  `DEV_CACHE_HIT`.
- Fresh rollback-path regression after implementation: 177.266 seconds, RC 0,
  no `RES_DEV_BUILD` flag.

| Case | Trials | Median seconds | % of fresh median | Speedup | Gate |
|---|---:|---:|---:|---:|---|
| Fresh current path | 3 | 187.153 | 100% | 1.00x | reference |
| Warm retained/locked no-op | 4 | 13.686 | 7.3% | 13.68x | pass |
| Real leaf content/correction | 2 | 14.306 | 7.6% | 13.08x | pass on two trials; third deferred after further compiles were stopped |
| Common header | 0 valid | -- | -- | -- | timestamp-only probe did not rebuild |
| Fresh `jobs=1` | 0 valid | -- | -- | -- | invalid after host suspend; abandoned |
| Fresh `jobs=4` | 0 | -- | -- | -- | deferred |
| Fresh `jobs=0` | 0 valid | -- | -- | -- | user-aborted; quarantined |

The performance gates require warm no-op <=40% of fresh and a >=45-second
saving, plus leaf <=65% and a >=30-second saving. Observed warm and leaf builds
were both under 8% of fresh and saved roughly 173 seconds.

## Safety evidence

`firmware/fixture/tests/smoke_build_cache.sh --jobs 2 --pairs 5` passed:

- Five same-recipe contention pairs returned zero for both callers. Every caller
  B reported `DEV_CACHE_WAIT`; only one caller held the atomic directory lock.
- A concurrent commission -> field recipe change waited, acquired, reported
  `DEV_CACHE_RESET reason=recipe-change`, and cold-built without mixed objects.
- The test-only hard interruption occurred after the marker but before Arduino.
  Ordinary reuse failed closed. Recovery moved the entire cache and lock to
  `dev-cache.quarantine.20260822T171100Z-2023`; a new cold cache then built.
- A fabricated same-host dead PID lock failed closed and required recovery to
  `dev-cache.quarantine.20260822T171610Z-2123`.
- An intentional `#error` in a disposable worktree returned normally with RC 1,
  released the lock, removed the marker, and preserved valid prior objects. The
  corrected source rebuilt successfully from that cache.
- `--dev-cache` rejected `--ota`, `--artifact-dir`, and `--fw-rev` before compile.
- Cached build options contained `RES_DEV_BUILD=1`; binary string inspection
  found `dev-local` and no fleet identity promotion was attempted.
- A normal fresh build remained independent and succeeded.
- No `unlinkat`, `directory is not empty`, `bad reloc`, corrupt archive, or
  missing-object signature appeared.

The final user-aborted cache was also handled as designed: its exact wrapper,
Arduino, and child compiler PIDs were stopped; the live marker and lock were
left in place; `--recover-dev-cache` quarantined it as
`dev-cache.quarantine.20260824T080957Z-1862`. No partial cache was resumed.

## Cleanup incident and artifact recovery

During cleanup, `git clean -fdX -- firmware/fixture/build/cache-proof` removed
the entire ignored `firmware/fixture/build/` directory rather than only the
proof cache. The dry run had printed `Would remove firmware/fixture/build/` and
should have been treated as a stop condition. Tracked source was untouched, but
ignored smoke logs, quarantine directories, and local artifact copies were
deleted.

Twelve immutable artifact directories had exact surviving copies in the
untouched `resonance-tree-worktrees/basic-listener` worktree. Their contract
files (`fixture.ino.bin`, manifest, checksum, build options, and identity header)
were copied back to the expected path and every binary was re-hashed against its
manifest. This includes:

- `fx-260817-ec7f28d-b` ->
  `1598f5506e4541e4f5c6efdd8693a3959510c9ed1f3467db4bc8bf874b40f2b7`
- `fx-260818-05ed4b3-b` ->
  `2986a0294827ef6be970d2ffe50066c885f3107f139f8601d5054d797467e1db`
- `fx-260818-f80f315-b` ->
  `0f1119c6ba80f2280db2c04f478a59b6be0c407edf6c95c62248f89af90ad638`

The bench-only `fx-260819-7afe0a6-b` binary (recorded SHA-256
`95e8d74727089c9bc309ae66109c2f26c1cb7cb7888d84c8fe90158f8bc9fcbc`)
has no surviving filesystem copy found under the known worktrees, Documents,
Desktop, Downloads, or local Temp. Do not rebuild or reuse that revision. Its
exact 1,170,736 app bytes may be recoverable later by reading the deployed app
partition from prototype `9E5AF0` or `9E5AB8` and accepting the readback only if
the SHA matches.

## Adopted commands

Fast local compile-only iteration:

```bash
cd firmware/fixture
./build.sh --dev-cache --profile commission --channel 11
```

Interrupted/stale cache:

```bash
./build.sh --recover-dev-cache
```

Healthy cache cleanup:

```bash
./build.sh --clean-dev-cache
```

Shared/fleet artifacts remain fresh, named, immutable builds. Never copy or
promote `build/dev-cache/fixture.ino.bin`.

## Deferred

- Repeat controlled `--jobs 1`, `--jobs 4`, and `--jobs 0` cold trials during a
  stable awake session. Do not infer performance from either suspended run.
- Flash one explicitly named sacrificial USB fixture and verify fresh telemetry
  reports `dev-local`; no hardware was authorized or required for this host gate.
- Separately optimize native tests by compiling core objects once.
