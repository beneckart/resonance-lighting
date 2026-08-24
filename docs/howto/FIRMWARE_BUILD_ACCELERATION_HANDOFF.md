# Firmware Build Acceleration — 80/20 Codex Handoff

**Status:** Implemented and host-adopted as an opt-in local path; job-count and
one-device hardware follow-ups deferred. See
`docs/tests/FIRMWARE_BUILD_ACCELERATION_SMOKE_2026-08-22.md`.
**Date:** 2026-08-22  
**Repository:** `beneckart/resonance-lighting`  
**Working branch:** `codex/build-acceleration-plan`  
**Base at handoff:** `main` at `dfb4c95`  
**Primary target:** `firmware/fixture/build.sh` on the Windows + Git Bash PowerFeather bench

## Objective

Reduce the normal edit → compile → flash iteration time without reintroducing the cache corruption and agent churn that previously occurred when multiple Arduino builds wrote the same cache concurrently.

The 80/20 hypothesis is:

> The largest avoidable cost is not the absence of `-j8`; it is that the normal wrapper intentionally creates and deletes a unique `--build-path` on every run, forcing a cold ESP32/Arduino build each time.

The proposed order of operations is therefore:

1. Prove that retaining one build directory produces a meaningful warm-build speedup, using the current wrapper and no code changes.
2. Only if the speedup is substantial, add one opt-in persistent development cache with exactly one writer.
3. Stress the lock, recipe invalidation, and interrupted-build recovery.
4. Adopt the new local build rule only if both performance and safety gates pass.
5. Keep release and fleet artifacts on the existing fresh, immutable artifact path.

This is deliberately not a build-system migration. Do not start with PlatformIO, ESP-IDF, CMake, Ninja, `ccache`, distributed compilation, or a repo-wide build rewrite.

---

## Repository facts and constraints

The current fixture wrapper does the safe but expensive thing for ordinary builds:

```bash
BUILD_PATH="$(mktemp -d /tmp/fixture-build.XXXXXX)"
trap 'rm -rf "$BUILD_PATH"' EXIT
arduino-cli compile --build-path "$BUILD_PATH" ...
```

When `--artifact-dir` is supplied, the wrapper instead reuses the requested path. That existing option gives us a low-risk way to measure incremental behavior before adding any new cache logic.

`AGENTS.md` records the reason for the present design:

- Concurrent Arduino compiles against the same cache can collide with errors such as `unlinkat ... directory is not empty` and can produce mixed/corrupt artifacts.
- A killed build directory may contain a partial `core/core.a` and later produce misleading linker failures such as `bad reloc symbol index`.
- An uncached PowerFeather/ESP32-S3 build on the Windows bench has historically taken roughly 2–3 minutes.

The artifact-handoff contract must remain intact:

- Shared/fleetable builds have an immutable `fw_rev` and exact binary SHA-256.
- One `fw_rev` maps to one binary.
- Fleet tooling uploads an inspected artifact rather than rebuilding it.
- A killed build directory is never resumed.

## Non-negotiable invariants

Any implementation must preserve all of the following:

1. **One writer per build directory.** Two compiler processes must never write the same build path concurrently.
2. **Interrupted means untrusted.** A development cache active during a hard kill, timeout, or abandoned compiler process is quarantined or deleted before reuse.
3. **Development is not release.** A cached local binary must report an unmistakably non-fleet identity such as `dev-local` and must not be offered to fleet OTA tooling.
4. **Release artifacts stay fresh and immutable.** `--artifact-dir build/<fw_rev> --fw-rev <fw_rev>` remains a separate path.
5. **Current behavior remains the rollback.** A normal invocation without the new option still receives a fresh temporary build path.
6. **No hardware during cache stress.** Concurrency, recipe-change, stale-lock, and interrupted-build tests are compile-only. At most one explicitly selected sacrificial USB fixture is flashed after host-side gates pass.

---

# Phase 0 — Prove the opportunity without changing code

Run this phase on the actual slow Windows/Git Bash bench. The point is to answer one question cheaply:

> Does reusing the existing Arduino build directory save enough time to justify lock and recovery machinery?

## Preconditions

From a clean checkout of the intended source:

```bash
cd firmware/fixture

git status --short
git rev-parse HEAD
arduino-cli version
arduino-cli core list

# Confirm another agent/operator is not compiling firmware.
ps -W | grep -Ei 'arduino-cli|xtensa|esptool' || true

./tests/run_tests.sh
```

Stop if native tests fail or another compiler is active.

Phase 0 is **compile-only**:

- Do not pass `--port`.
- Do not pass `--ota`.
- Do not pass `--fw-rev`.
- Use exactly the same profile, channel, chemistry, precharge setting, and other compile flags in every timed run.
- Only one process may touch `build/cache-proof`.

Avoiding `--fw-rev` matters because the current wrapper force-includes a generated identity header. Changing that header can invalidate a large fraction of translation units and would make the warm-path experiment pessimistic for reasons unrelated to ordinary source iteration.

## Timing helper

```bash
mkdir -p build/build-accel-smoke
rm -f build/build-accel-smoke/times.csv
printf 'label,seconds,rc\n' > build/build-accel-smoke/times.csv

run_timed() {
  local label="$1"
  shift
  local log="build/build-accel-smoke/${label}.log"
  local start=$SECONDS

  set +e
  "$@" 2>&1 | tee "$log"
  local rc=${PIPESTATUS[0]}
  set -e

  local elapsed=$((SECONDS - start))
  printf '%s,%s,%s\n' "$label" "$elapsed" "$rc" \
    | tee -a build/build-accel-smoke/times.csv
  return "$rc"
}
```

If the interactive shell is not already using `set -e`, the helper still records return codes; inspect them before continuing.

## A. Establish the current cold baseline

Run at least two ordinary builds. Each should receive and discard a fresh temporary build path.

```bash
run_timed cold-default-1 \
  ./build.sh --profile commission --channel 11

run_timed cold-default-2 \
  ./build.sh --profile commission --channel 11
```

Three trials are preferable if the machine is noisy.

## B. Measure retained-build behavior using the existing wrapper

```bash
rm -rf build/cache-proof

run_timed retained-cold \
  ./build.sh \
    --artifact-dir build/cache-proof \
    --profile commission \
    --channel 11

sha256sum build/cache-proof/fixture.ino.bin \
  | tee build/build-accel-smoke/sha-retained-cold.txt

run_timed retained-noop-1 \
  ./build.sh \
    --artifact-dir build/cache-proof \
    --profile commission \
    --channel 11

run_timed retained-noop-2 \
  ./build.sh \
    --artifact-dir build/cache-proof \
    --profile commission \
    --channel 11

sha256sum build/cache-proof/fixture.ino.bin \
  | tee build/build-accel-smoke/sha-retained-noop.txt

test -s build/cache-proof/fixture.ino.bin
test -s build/cache-proof/build.options.json
```

Record whether the second and third calls compile anything, relink only, or return almost immediately.

A no-op SHA difference is worth recording, but it is not automatically a cache failure: toolchains are not always reproducible across relinks. The correctness rule is still that a shared immutable revision is built once and never overwritten. This proof directory is not a shared artifact.

## C. Measure a representative leaf-source edit

A timestamp-only touch asks the dependency graph to rebuild one translation unit without changing tracked content:

```bash
touch src/core/choreo/prog_idle.cpp

run_timed retained-leaf-touch \
  ./build.sh \
    --artifact-dir build/cache-proof \
    --profile commission \
    --channel 11

sha256sum build/cache-proof/fixture.ino.bin \
  | tee build/build-accel-smoke/sha-retained-leaf.txt
```

This approximates the common case: edit one `.cpp`, compile the changed unit, then link.

## D. Optional high-fanout probe

This is context, not a success criterion. Touching a common protocol header should trigger a broad rebuild:

```bash
touch src/core/packet.h

run_timed retained-common-header \
  ./build.sh \
    --artifact-dir build/cache-proof \
    --profile commission \
    --channel 11
```

A common-header build near cold speed does not disprove the optimization. Incremental builds cannot avoid recompiling consumers of a changed header.

## E. Summarize the result

```bash
python - <<'PY'
import csv
from pathlib import Path
from statistics import median

rows = list(csv.DictReader(Path('build/build-accel-smoke/times.csv').open()))
values = {r['label']: int(r['seconds']) for r in rows if r['rc'] == '0'}

cold = median([values['cold-default-1'], values['cold-default-2']])
noop = median([values['retained-noop-1'], values['retained-noop-2']])
leaf = values['retained-leaf-touch']

print(f'cold median:       {cold:.1f}s')
print(f'warm no-op median: {noop:.1f}s  ({cold/noop:.2f}x faster; {noop/cold:.1%} of cold)')
print(f'leaf touch:        {leaf:.1f}s  ({cold/leaf:.2f}x faster; {leaf/cold:.1%} of cold)')
PY
```

## Phase 0 go/no-go gate

Proceed to implementation only if all of these are true:

- Every timed build returns zero.
- `fixture.ino.bin` and `build.options.json` exist and are non-empty.
- Warm no-op median is **≤ 50% of cold median** and saves **at least 45 seconds**.
- Leaf-touch time is **≤ 75% of cold median** and saves **at least 30 seconds**.
- Logs contain no `unlinkat`, `directory is not empty`, `bad reloc`, corrupt archive, missing object, or mixed-artifact symptom.
- The retained build’s compile output is consistent with dependency reuse rather than an unexplained skipped build.

If the gate fails, document the numbers and stop. Do not add cache coordination complexity for a marginal gain. The next low-cost investigation would be explicit `--jobs` benchmarking and verbose compile tracing.

After preserving the logs, delete the unsafe proof cache:

```bash
rm -rf build/cache-proof
```

---

# Phase 1 — Minimal implementation, only after Phase 0 passes

Limit the first implementation to `firmware/fixture/build.sh`, a small helper only if needed, one smoke script, and the development identity branch in `src/core/version.h`.

## Proposed command surface

Add:

```text
--dev-cache             use the persistent, locked local development build path
--jobs N                pass N to arduino-cli compile --jobs
--clean-dev-cache       safely remove a healthy, unlocked cache
--recover-dev-cache     quarantine a stale/interrupted cache after process checks
```

Examples:

```bash
# Fast compile-only local iteration
./build.sh --dev-cache --jobs 0 --profile commission --channel 11

# Fast compile + one explicitly selected USB bench fixture
./build.sh --dev-cache --jobs 0 --profile commission --channel 11 --port COM42

# Shared/fleet artifact: unchanged, fresh, named, immutable
./build.sh \
  --artifact-dir build/<fw_rev> \
  --fw-rev <fw_rev> \
  --profile commission \
  --channel 11
```

Hard incompatibilities for the first version:

- Reject `--dev-cache` with `--artifact-dir`.
- Reject `--dev-cache` with `--fw-rev`.
- Reject `--dev-cache` with `--ota`.
- Continue requiring a fresh test-class artifact for targeted safety bypasses.

The initial scope allows `--port` because local USB flashing is explicit and fast. OTA stays outside the development-cache path until the fleet boundary is proven in practice.

## One persistent cache, one lock

Use one ignored local cache:

```text
firmware/fixture/build/dev-cache/
```

Use a sibling lock acquired with atomic directory creation:

```text
firmware/fixture/build/dev-cache.lock.d/
```

The lock directory should contain:

```text
pid
hostname
started_epoch
command.txt
```

Required behavior:

1. Successful `mkdir` means the caller owns the cache.
2. If the lock exists and its same-host PID is alive, print a clear wait message and wait for bounded time rather than starting another compiler.
3. If the lock exists but the PID is dead or cannot be trusted, fail closed with instructions to use `--recover-dev-cache`.
4. `--recover-dev-cache` must first check for lingering `arduino-cli`, Xtensa compiler, linker, `esptool`, and child processes. If process state is uncertain, abort rather than deleting under a live compiler.
5. Recovery renames the cache and lock into a timestamped quarantine rather than resuming them.
6. Normal exits release the lock through a trap.

Emit grep-friendly state lines:

```text
DEV_CACHE_WAIT
DEV_CACHE_ACQUIRED
DEV_CACHE_HIT
DEV_CACHE_RESET
DEV_CACHE_INTERRUPTED
DEV_CACHE_QUARANTINED
DEV_CACHE_RELEASED
```

A single global cache intentionally serializes cached fixture recipes. This is the 80/20 choice: simple, inspectable, and safe. Do not begin with multiple cache shards.

## Interrupted-build marker

Create this immediately before launching Arduino CLI:

```text
build/dev-cache/.build-in-progress
```

The marker should include PID, hostname, start time, and recipe fingerprint.

Behavior:

- Remove it after Arduino CLI returns, including after an ordinary compiler error.
- If the wrapper is hard-killed before Arduino CLI returns, the marker remains.
- A future `--dev-cache` invocation that sees the marker fails closed and requires recovery.
- Recovery quarantines the entire cache; it never resumes a possibly partial `core/core.a`.

A successful cached compile still requires:

- Arduino CLI exit code zero.
- Non-empty `fixture.ino.bin`.
- Non-empty `build.options.json`.
- Printed binary SHA-256.

## Recipe fingerprint

Store a deterministic recipe fingerprint beside the cache. At minimum include:

- Cache schema version.
- Absolute sketch path.
- FQBN.
- Canonicalized compiler flags and selected compile posture.
- Arduino CLI version.
- Installed ESP32 platform version.
- PowerFeather SDK version.
- Firmware-affecting Arduino library versions.

Do **not** include:

- Git commit.
- Source file hashes.
- Source modification times.
- `--jobs`.
- Serial port.
- Log paths.

Source changes are what Arduino dependency files are meant to detect incrementally. Putting the commit or source hash in the cache key would make every commit cold again.

When the recipe fingerprint changes, reset the cache while holding the lock and log the reason. Never reuse objects across different FQBNs, flags, platform versions, or library versions.

## Development firmware identity

Cached local builds must not report the current fallback `fx-*` identity or a supplied immutable revision.

Add a build flag such as:

```text
-DRES_DEV_BUILD=1
```

and make `src/core/version.h` resolve it to:

```text
dev-local
```

Preserve explicit immutable identity precedence. Conceptually:

```cpp
#ifndef RES_FIXTURE_VERSION
  #ifdef RES_DEV_BUILD
    #define RES_FIXTURE_VERSION "dev-local"
  #else
    #define RES_FIXTURE_VERSION "<existing fallback>"
  #endif
#endif
```

This is a safety boundary, not cosmetic labeling. `dev-local` explicitly means that the same reported string can correspond to changing local bytes and therefore cannot be promoted as a fleet artifact.

## Compiler job control

Pass a validated value to Arduino CLI:

```bash
arduino-cli compile --jobs "$JOBS" ...
```

Support `ARDUINO_JOBS` as an environment default and `--jobs` as the command-line override.

Measure rather than assume:

- `--jobs 1`: serial reference.
- `--jobs 4`: conservative parallelism.
- `--jobs 0`: use available CPU cores.

Job parallelism is secondary to cache reuse. Do not launch several outer builds that each request all cores.

---

# Phase 2 — Smoke and adversarial safety checks

Add a host-only script such as:

```text
firmware/fixture/tests/smoke_build_cache.sh
```

It must not flash or OTA by default.

## 1. Static and native checks

```bash
bash -n firmware/fixture/build.sh
firmware/fixture/tests/run_tests.sh

cd firmware/fixture
rm -rf build/dev-cache build/dev-cache.lock.d build/dev-cache.quarantine.*

./build.sh --dev-cache --jobs 2 --profile commission --channel 11
./build.sh --dev-cache --jobs 2 --profile commission --channel 11
```

The second build must announce a matching recipe and warm-cache reuse.

## 2. Same-recipe concurrency simulation

Start two compile-only callers against the same cache. The second must wait; it must never enter Arduino CLI while the first owns the lock.

```bash
cd firmware/fixture
rm -rf build/dev-cache build/dev-cache.lock.d
rm -f /tmp/res-build-a.log /tmp/res-build-b.log \
      /tmp/res-build-a.rc /tmp/res-build-b.rc

(
  ./build.sh --dev-cache --jobs 2 --profile commission --channel 11 \
    > /tmp/res-build-a.log 2>&1
  printf '%s\n' "$?" > /tmp/res-build-a.rc
) &
a_pid=$!

sleep 2

(
  ./build.sh --dev-cache --jobs 2 --profile commission --channel 11 \
    > /tmp/res-build-b.log 2>&1
  printf '%s\n' "$?" > /tmp/res-build-b.rc
) &
b_pid=$!

wait "$a_pid"
wait "$b_pid"

cat /tmp/res-build-a.rc /tmp/res-build-b.rc
grep -H -E 'DEV_CACHE_(WAIT|ACQUIRED|HIT|RELEASED)' \
  /tmp/res-build-a.log /tmp/res-build-b.log

grep -H -Ei 'unlinkat|directory is not empty|bad reloc|corrupt|core\.a' \
  /tmp/res-build-a.log /tmp/res-build-b.log && exit 1 || true

test -s build/dev-cache/fixture.ino.bin
test -s build/dev-cache/build.options.json
```

Pass conditions:

- Both invocations return zero.
- Only one process owns the cache at a time.
- Caller B logs `DEV_CACHE_WAIT`, or acquires only after A logs release.
- B completes as a warm/no-op build rather than starting a second cold compiler.
- No corruption signature appears.

Repeat this pair **five times** before adoption.

## 3. Different-recipe serialization and invalidation

Launch commission and field recipes nearly together:

```bash
(
  ./build.sh --dev-cache --jobs 2 --profile commission --channel 11 \
    > /tmp/res-recipe-a.log 2>&1
) &
a_pid=$!

sleep 2

(
  ./build.sh --dev-cache --jobs 2 --profile field --channel 11 \
    > /tmp/res-recipe-b.log 2>&1
) &
b_pid=$!

wait "$a_pid"
wait "$b_pid"

grep -H -E 'DEV_CACHE_(WAIT|ACQUIRED|RESET|RELEASED)' \
  /tmp/res-recipe-a.log /tmp/res-recipe-b.log
```

Pass conditions:

- The recipes serialize.
- The second caller detects a recipe mismatch after acquiring the lock.
- It resets the cache before compiling.
- Both builds succeed without mixed objects.

## 4. Hard-interruption and recovery simulation

Do not kill a real compiler merely to test recovery. Add a test-only hook that pauses after acquiring the lock and writing `.build-in-progress`, but before spawning Arduino CLI:

```text
RES_BUILD_TEST_PAUSE_AFTER_MARKER=<seconds>
```

The hook must be inert unless explicitly set.

Test:

```bash
rm -rf build/dev-cache build/dev-cache.lock.d build/dev-cache.quarantine.*

RES_BUILD_TEST_PAUSE_AFTER_MARKER=30 \
  ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
  > /tmp/res-interrupt.log 2>&1 &
interrupted_pid=$!

sleep 3
kill -9 "$interrupted_pid"
wait "$interrupted_pid" || true

# Ordinary reuse must fail closed.
if ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
     > /tmp/res-after-kill.log 2>&1; then
  echo 'ERROR: interrupted cache was reused' >&2
  exit 1
fi

grep -E 'DEV_CACHE_INTERRUPTED|recover-dev-cache' /tmp/res-after-kill.log

# Explicit recovery quarantines it after process checks.
./build.sh --recover-dev-cache

./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
  > /tmp/res-recovered.log 2>&1

grep -E 'DEV_CACHE_(QUARANTINED|ACQUIRED|RELEASED)' \
  /tmp/res-recovered.log build/dev-cache.quarantine.*/recovery.log 2>/dev/null || true

test -s build/dev-cache/fixture.ino.bin
```

Pass condition: the interrupted directory is never resumed; recovery produces a new cold build.

## 5. Stale-lock simulation

Create a fake same-host dead-PID lock:

```bash
rm -rf build/dev-cache.lock.d
mkdir -p build/dev-cache.lock.d
printf '99999999\n' > build/dev-cache.lock.d/pid
hostname > build/dev-cache.lock.d/hostname
printf '0\n' > build/dev-cache.lock.d/started_epoch

if ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
     > /tmp/res-stale-lock.log 2>&1; then
  echo 'ERROR: stale lock was silently bypassed' >&2
  exit 1
fi

grep -E 'stale|recover-dev-cache|DEV_CACHE_INTERRUPTED' \
  /tmp/res-stale-lock.log
```

Pass condition: the wrapper fails closed and requires explicit recovery. It must not infer that a dead wrapper implies a trustworthy cache.

## 6. Ordinary compiler-error behavior

Introduce a temporary, unmistakable syntax error in a leaf source, run the cached build, then restore the file with Git. Do this only in a disposable worktree.

Expected behavior:

- Compiler returns nonzero.
- The wrapper releases the lock.
- `.build-in-progress` is removed because Arduino CLI returned normally.
- The next corrected build may reuse valid prior objects.
- A compile error must not be confused with a hard interruption.

## 7. Development identity and fleet boundary

Before touching hardware:

- Confirm cached build options contain `RES_DEV_BUILD=1`.
- Confirm a normal fresh build does not contain it.
- Confirm these combinations fail before compilation/upload:

```bash
./build.sh --dev-cache --ota 10.0.0.200
./build.sh --dev-cache --artifact-dir build/nope
./build.sh --dev-cache --fw-rev fx-260822-0000000-t
```

After every host-only gate passes, flash at most one explicitly selected USB bench fixture:

```bash
./build.sh \
  --dev-cache \
  --jobs 0 \
  --profile commission \
  --channel 11 \
  --port <ONE_EXPLICIT_PORT>
```

Verify fresh serial/heartbeat evidence reports exactly:

```text
dev-local
```

Do not use the dev cache binary with fleet USB bring-up or OTA tooling.

## 8. Fresh/release-path regression

Verify the existing path still works unchanged:

```bash
./build.sh --profile commission --channel 11
```

It should announce a new temporary build path and remove it afterward.

Separately, if a release-path test is warranted, create a brand-new test-class revision according to `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`. Never reuse an existing `fw_rev`. Verify its directory, identity header, options, binary, and SHA are independent of `build/dev-cache`.

---

# Phase 3 — Before/after performance report

Use the same machine, source, recipe, and idle conditions as Phase 0.

Collect at least:

- 3 fresh builds using the normal temporary path.
- 3 warm no-op `--dev-cache` builds.
- 3 leaf-touch `--dev-cache` builds, touching the same leaf `.cpp` before each run.
- 1 common-header touch for context.
- Cold builds with `--jobs 1`, `--jobs 4`, and `--jobs 0`; repeat any close comparison.
- Five same-recipe concurrency pairs.

Write the evidence to:

```text
docs/tests/FIRMWARE_BUILD_ACCELERATION_SMOKE_2026-08-22.md
```

Include:

- Git commit and dirty state.
- Machine/CPU/RAM summary.
- Arduino CLI, ESP32 platform, PowerFeather SDK, and library versions.
- Exact commands.
- Raw elapsed times and medians.
- Relevant compiler output.
- Cache-state transitions.
- Binary SHA observations.
- Concurrency, interruption, stale-lock, and recipe-change results.
- Any quarantine directories created.

Use this table:

| Case | Trials | Median seconds | % of fresh median | Speedup | Gate |
|---|---:|---:|---:|---:|---|
| Fresh current path | 3 |  | 100% | 1.00× | reference |
| Dev cache no-op | 3 |  |  |  |  |
| Dev cache leaf touch | 3 |  |  |  |  |
| Dev cache common header | 1 |  |  |  | context |
| Fresh `jobs=1` | 2+ |  |  |  | context |
| Fresh `jobs=4` | 2+ |  |  |  | context |
| Fresh `jobs=0` | 2+ |  |  |  | context |

---

# Adoption gates

Adopt the new local rule only when **every safety gate** and **both performance gates** pass.

## Performance gates

- Warm no-op median is **≤ 40% of fresh median** and saves **at least 45 seconds**.
- Leaf-touch median is **≤ 65% of fresh median** and saves **at least 30 seconds**.
- The recommended job count is based on measured repeatable improvement and does not cause memory pressure or machine-wide oversubscription.

Cache reuse may be adopted even if `--jobs 0` adds little. Avoiding work is the primary win.

## Safety gates

- Five same-recipe concurrent pairs pass with one writer at a time.
- Different recipes serialize and invalidate incompatible state.
- A hard-interrupted cache is refused and quarantined before reuse.
- A stale lock fails closed.
- An ordinary compiler error releases the lock correctly.
- Native fixture tests pass.
- A normal fresh build still works unchanged.
- The immutable artifact path remains independent.
- Cached binaries report `dev-local`.
- `--dev-cache` cannot use OTA, `--fw-rev`, or `--artifact-dir`.
- No cache-corruption signature appears.

## No-go conditions

Do not adopt if any of these occurs:

- Warm no-op improvement is less than 2× or saves less than 45 seconds.
- Two ordinary shell invocations can both enter the cache.
- Recipe changes reuse incompatible objects.
- An interrupted cache can be resumed.
- A cached binary can be mistaken for a fleetable `fx-*` artifact.
- Release/fleet safety has to be weakened to make local caching work.

---

# Adoption action

If the gates pass:

1. Keep `--dev-cache` as the documented/recommended command for interactive local compile and one-device USB iteration.
2. Keep ordinary fresh builds available and keep all named fleet artifacts separate.
3. Update `AGENTS.md` to state:
   - use `--dev-cache` for iterative fixture work;
   - rely on its lock rather than starting another compile;
   - never point direct parallel Arduino CLI calls at the cache;
   - interrupted caches require explicit recovery;
   - shared/fleet artifacts remain fresh and immutable.
4. Update `firmware/fixture/README.md` with local-development versus release commands and the meaning of `dev-local`.
5. Commit the smoke script and dated evidence report.
6. Add a dated `LOG.md` entry with the measured before/after result and safety evidence.
7. Merge the implementation only with the evidence report and rollback path in the same branch.

Adoption does **not** mean promoting the development directory. A release is still a fresh named build from reviewed source.

# Rollback

Operational rollback is immediate:

```bash
cd firmware/fixture
./build.sh --clean-dev-cache || true
./build.sh --profile commission --channel 11
```

If the cache is interrupted:

```bash
./build.sh --recover-dev-cache
./build.sh --profile commission --channel 11
```

Code rollback removes the `--dev-cache`/lock/recovery plumbing and the `RES_DEV_BUILD` version branch. The current temporary-build behavior remains the known-safe fallback throughout the experiment.

---

# Codex execution checklist

- [ ] Rebase or fast-forward `codex/build-acceleration-plan` onto the latest intended `main` before implementation.
- [ ] Read `AGENTS.md`, ADR 0040, and `docs/howto/FIRMWARE_ARTIFACT_HANDOFF.md`.
- [ ] Run Phase 0 with the unmodified wrapper and record raw results.
- [ ] Stop if the Phase 0 performance gate fails.
- [ ] Implement only the narrow Phase 1 surface.
- [ ] Add the host-only smoke script.
- [ ] Run static and native tests.
- [ ] Run five same-recipe concurrency pairs.
- [ ] Run different-recipe, hard-interruption, stale-lock, and ordinary-error tests.
- [ ] Verify development identity and fleet boundaries.
- [ ] Flash only one explicitly named USB bench fixture after host gates pass.
- [ ] Run the Phase 3 before/after comparison.
- [ ] Write `docs/tests/FIRMWARE_BUILD_ACCELERATION_SMOKE_2026-08-22.md`.
- [ ] Update build rules and documentation only if adoption gates pass.
- [ ] Do not merge implementation without evidence and rollback.

# Deferred follow-up

Only after the fixture cache is measured and stable, consider the next low-risk bottleneck: `firmware/fixture/tests/run_tests.sh` currently recompiles all core `.cpp` files for each `test_*.cpp` executable. Compiling core objects once and linking each test against them could materially shorten native tests.

Keep that as a separate change. Also defer `ccache`, other sketch wrappers, multiple cache shards, and build-system migration until measurements show they are worth the complexity.
