#!/usr/bin/env bash
# Fast, compile-free checks for the local-vs-fleet build wrapper contract.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
FIXTURE_DIR="$(cd "$TESTS_DIR/.." && pwd -P)"
cd "$FIXTURE_DIR"

fail() {
  echo "BUILD WRAPPER CONTRACT FAIL: $*" >&2
  exit 1
}

expect_rejected() {
  local expected="$1"
  shift
  local output rc
  set +e
  output="$(./build.sh "$@" 2>&1)"
  rc=$?
  set -e
  [[ $rc -eq 2 ]] || fail "expected rc=2 for $*; got $rc"
  grep -Fq -- "$expected" <<< "$output" ||
    fail "missing rejection '$expected' for $*"
}

bash -n build.sh
help="$(./build.sh --help)"
grep -Fq -- '--dev-cache --profile commission --channel 11' <<< "$help" ||
  fail "help omits the recommended local command"
grep -Fq -- 'Shared/fleet artifacts must omit --dev-cache' <<< "$help" ||
  fail "help omits the immutable fleet boundary"
grep -Fq -- '--artifact-variant p|b|t' <<< "$help" ||
  fail "help omits automatic artifact identity"
grep -Fq -- '--wifi-profile-label LABEL' <<< "$help" ||
  fail "help omits the non-secret credential label"
grep -Fq -- 'dev-local' <<< "$help" || fail "help omits development identity"
grep -Fq -- '--day-sleep-s N' <<< "$help" || fail "help omits day sleep cadence"
grep -Fq -- '--wake-listen-ms N' <<< "$help" || fail "help omits wake listen cadence"
grep -Fq -- '--msa-trace-target MAC' <<< "$help" || fail "help omits exact-target motion trace"
grep -Fq -- '--presence-sentinel' <<< "$help" || fail "help omits presence sentinel"
grep -Fq -- '--presence-distant-range' <<< "$help" || fail "help omits distant-range sentinel"
grep -Fq -- '--sentinel-trace-target MAC' <<< "$help" ||
  fail "help omits exact-target sentinel trace"
grep -Fq -- '--sentinel-trace-smoke' <<< "$help" ||
  fail "help omits sentinel persistence smoke gate"

expect_rejected '--dev-cache cannot be combined with --ota' \
  --dev-cache --ota 192.0.2.1
expect_rejected '--dev-cache cannot be combined with --artifact-variant' \
  --dev-cache --artifact-variant b
expect_rejected 'manual --artifact-dir/--fw-rev is disabled' \
  --artifact-dir build/contract-must-not-exist
expect_rejected 'manual --artifact-dir/--fw-rev is disabled' \
  --fw-rev fx-260824-0000000-t
expect_rejected '--artifact-variant requires --wifi-profile-label' \
  --artifact-variant b --profile field --channel 11
expect_rejected 'bad --artifact-variant' --artifact-variant x
expect_rejected '--artifact-variant requires explicit --profile' \
  --artifact-variant b --wifi-profile-label test-v1 --channel 11
expect_rejected '--artifact-variant requires explicit --channel' \
  --artifact-variant b --wifi-profile-label test-v1 --profile field
expect_rejected 'artifact builds never flash directly' \
  --artifact-variant b --wifi-profile-label test-v1 --profile field --channel 11 --port COM1
expect_rejected 'bad --jobs/ARDUINO_JOBS' --dev-cache --jobs invalid
expect_rejected 'bad --day-sleep-s' --day-sleep-s 29
expect_rejected 'bad --day-sleep-s' --day-sleep-s invalid
expect_rejected 'bad --wake-listen-ms' --wake-listen-ms 999
expect_rejected 'bad --wake-listen-ms' --wake-listen-ms invalid
expect_rejected 'bad --msa-trace-target' --msa-trace-target invalid
expect_rejected '--msa-trace-target requires --artifact-variant t' \
  --msa-trace-target F2BE0C --artifact-variant p \
  --wifi-profile-label test-v1 --profile field --channel 11
expect_rejected '--presence-sentinel requires --msa-trace-target' \
  --presence-sentinel
expect_rejected '--presence-distant-range requires --msa-trace-target' \
  --presence-distant-range
expect_rejected '--presence-distant-range requires --presence-sentinel' \
  --presence-distant-range --msa-trace-target F2BE0C
expect_rejected 'bad --sentinel-trace-target' --sentinel-trace-target invalid
expect_rejected '--sentinel-trace-target requires --artifact-variant t' \
  --sentinel-trace-target A1B2C3 --artifact-variant p \
  --wifi-profile-label test-v1 --profile field --channel 11
expect_rejected '--sentinel-trace-target cannot be combined with --msa-trace-target' \
  --sentinel-trace-target A1B2C3 --msa-trace-target F2BE0C
expect_rejected '--sentinel-trace-smoke requires --sentinel-trace-target' \
  --sentinel-trace-smoke

[[ ! -e build/contract-must-not-exist ]] ||
  fail "a rejected boundary check created an artifact directory"

python tests/test_artifact_recipe.py

echo "BUILD WRAPPER CONTRACT PASSED"
