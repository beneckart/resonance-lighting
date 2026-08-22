#!/usr/bin/env bash
# Host-only acceptance checks for build.sh's persistent development cache.
# This script never flashes USB hardware and never invokes OTA.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
FIXTURE_DIR="$(cd "$TESTS_DIR/.." && pwd -P)"
cd "$FIXTURE_DIR"

JOBS=2
PAIRS=5
while [[ $# -gt 0 ]]; do
  case "$1" in
    --jobs) JOBS="$2"; shift 2 ;;
    --pairs) PAIRS="$2"; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done
[[ "$JOBS" =~ ^[0-9]+$ ]] || { echo "bad --jobs: $JOBS" >&2; exit 2; }
[[ "$PAIRS" =~ ^[1-9][0-9]*$ ]] || { echo "bad --pairs: $PAIRS" >&2; exit 2; }

STAMP="$(date -u +%Y%m%dT%H%M%SZ)-$$"
OUT="$FIXTURE_DIR/build/build-cache-smoke-$STAMP"
mkdir -p "$OUT"

fail() { echo "SMOKE FAIL: $*" >&2; exit 1; }
assert_clean_log() {
  local log="$1"
  if grep -Eiq 'unlinkat|directory is not empty|bad reloc|corrupt archive|missing object' "$log"; then
    fail "cache-corruption signature in $log"
  fi
}
run_logged() {
  local label="$1"
  shift
  local log="$OUT/$label.log" start=$SECONDS rc
  set +e
  "$@" > "$log" 2>&1
  rc=$?
  set -e
  printf '%s,%s,%s\n' "$label" "$((SECONDS - start))" "$rc" >> "$OUT/times.csv"
  cat "$log"
  assert_clean_log "$log"
  return "$rc"
}

printf 'label,seconds,rc\n' > "$OUT/times.csv"
bash -n build.sh
"$TESTS_DIR/run_tests.sh" | tee "$OUT/native-tests.log"

# Start only from a healthy or absent cache. Interrupted state requires a
# deliberate recovery outside this script so evidence is not destroyed.
if [[ -e build/dev-cache.lock.d || -f build/dev-cache/.build-in-progress ]]; then
  fail "pre-existing interrupted/locked cache; inspect and recover it first"
fi
./build.sh --clean-dev-cache | tee "$OUT/clean-start.log"

run_logged cold ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11
run_logged warm ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11
grep -q 'DEV_CACHE_HIT' "$OUT/warm.log" || fail "warm build did not report a cache hit"
grep -q 'RES_DEV_BUILD=1' build/dev-cache/build.options.json || fail "dev flag missing from build options"
grep -aq 'dev-local' build/dev-cache/fixture.ino.bin || fail "dev-local missing from cached binary"

boundary_index=0
for args in \
  '--dev-cache --ota 10.0.0.200' \
  '--dev-cache --artifact-dir build/nope' \
  '--dev-cache --fw-rev fx-260822-0000000-t'; do
  boundary_index=$((boundary_index + 1))
  # Intentional word splitting: these are fixed, non-secret test vectors.
  # shellcheck disable=SC2086
  if ./build.sh $args > "$OUT/boundary-$boundary_index.log" 2>&1; then
    fail "unsafe combination accepted: $args"
  fi
done

for ((i = 1; i <= PAIRS; ++i)); do
  a_log="$OUT/concurrency-$i-a.log"
  b_log="$OUT/concurrency-$i-b.log"
  (
    set +e
    ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11 > "$a_log" 2>&1
    printf '%s\n' "$?" > "$OUT/concurrency-$i-a.rc"
  ) &
  a_pid=$!
  sleep 2
  (
    set +e
    ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11 > "$b_log" 2>&1
    printf '%s\n' "$?" > "$OUT/concurrency-$i-b.rc"
  ) &
  b_pid=$!
  wait "$a_pid"
  wait "$b_pid"
  [[ "$(< "$OUT/concurrency-$i-a.rc")" == 0 ]] || fail "concurrency pair $i caller A failed"
  [[ "$(< "$OUT/concurrency-$i-b.rc")" == 0 ]] || fail "concurrency pair $i caller B failed"
  grep -q 'DEV_CACHE_WAIT' "$b_log" || fail "concurrency pair $i caller B did not wait"
  assert_clean_log "$a_log"
  assert_clean_log "$b_log"
done

# A different recipe must wait, acquire, then invalidate the cache.
(
  set +e
  ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11 > "$OUT/recipe-a.log" 2>&1
  printf '%s\n' "$?" > "$OUT/recipe-a.rc"
) &
a_pid=$!
sleep 2
(
  set +e
  ./build.sh --dev-cache --jobs "$JOBS" --profile field --channel 11 > "$OUT/recipe-b.log" 2>&1
  printf '%s\n' "$?" > "$OUT/recipe-b.rc"
) &
b_pid=$!
wait "$a_pid"
wait "$b_pid"
[[ "$(< "$OUT/recipe-a.rc")" == 0 && "$(< "$OUT/recipe-b.rc")" == 0 ]] ||
  fail "different-recipe callers failed"
grep -q 'DEV_CACHE_WAIT' "$OUT/recipe-b.log" || fail "different recipe did not wait"
grep -q 'DEV_CACHE_RESET reason=recipe-change' "$OUT/recipe-b.log" ||
  fail "different recipe did not reset the cache"
assert_clean_log "$OUT/recipe-a.log"
assert_clean_log "$OUT/recipe-b.log"

# Hard-kill only the test pause before Arduino starts. The marker and lock must
# survive, ordinary reuse must fail, and explicit recovery must quarantine.
RES_BUILD_TEST_PAUSE_AFTER_MARKER=30 \
  ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
  > "$OUT/interrupt.log" 2>&1 &
interrupted_pid=$!
sleep 3
kill -9 "$interrupted_pid"
wait "$interrupted_pid" 2>/dev/null || true
if ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
    > "$OUT/after-kill.log" 2>&1; then
  fail "interrupted cache was reused"
fi
grep -Eq 'DEV_CACHE_INTERRUPTED|recover-dev-cache' "$OUT/after-kill.log" ||
  fail "interrupted cache failure was not explicit"
./build.sh --recover-dev-cache | tee "$OUT/recover.log"
grep -q 'DEV_CACHE_QUARANTINED' "$OUT/recover.log" || fail "cache was not quarantined"

run_logged recovered ./build.sh --dev-cache --jobs "$JOBS" --profile commission --channel 11

# A dead same-host PID is never silently bypassed.
mkdir build/dev-cache.lock.d
printf '99999999\n' > build/dev-cache.lock.d/pid
hostname > build/dev-cache.lock.d/hostname
printf '0\n' > build/dev-cache.lock.d/started_epoch
if ./build.sh --dev-cache --jobs 1 --profile commission --channel 11 \
    > "$OUT/stale-lock.log" 2>&1; then
  fail "stale lock was silently bypassed"
fi
grep -Eq 'stale|recover-dev-cache|DEV_CACHE_INTERRUPTED' "$OUT/stale-lock.log" ||
  fail "stale lock failure was not explicit"
./build.sh --recover-dev-cache | tee "$OUT/stale-recover.log"

echo "BUILD CACHE SMOKE PASSED: $OUT"
