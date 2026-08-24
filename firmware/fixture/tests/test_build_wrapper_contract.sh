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
grep -Fq -- 'dev-local' <<< "$help" || fail "help omits development identity"

expect_rejected '--dev-cache cannot be combined with --ota' \
  --dev-cache --ota 192.0.2.1
expect_rejected '--dev-cache cannot be combined with --artifact-dir' \
  --dev-cache --artifact-dir build/contract-must-not-exist
expect_rejected '--dev-cache cannot be combined with --fw-rev' \
  --dev-cache --fw-rev fx-260824-0000000-t
expect_rejected 'bad --jobs/ARDUINO_JOBS' --dev-cache --jobs invalid

[[ ! -e build/contract-must-not-exist ]] ||
  fail "a rejected boundary check created an artifact directory"

echo "BUILD WRAPPER CONTRACT PASSED"
