#!/usr/bin/env bash
# Fast, compile-free checks for the T-Deck local-vs-retained build contract.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
TDECK_DIR="$(cd "$TESTS_DIR/.." && pwd -P)"
cd "$TDECK_DIR"

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
grep -Fq -- './build.sh --dev-cache' <<< "$help" ||
  fail "help omits the recommended local command"
grep -Fq -- 'tdeck-dev-local' <<< "$help" ||
  fail "help omits the mutable development identity"
grep -Fq -- 'Retained field artifacts must' <<< "$help" ||
  fail "help omits the retained-artifact boundary"

dev_identity="$(
  printf '#include "core/version.h"\nTDECK_FW_VERSION\n' |
    g++ -E -P -DTDECK_DEV_BUILD=1 -I"$TDECK_DIR/src" -x c++ -
)"
grep -Fq -- '"dev-local"' <<< "$dev_identity" ||
  fail "TDECK_DEV_BUILD does not select the dev-local identity"

expect_rejected '--dev-cache cannot be combined with --build-path' \
  --dev-cache --build-path build/contract-must-not-exist
expect_rejected 'bad --jobs/ARDUINO_JOBS' --dev-cache --jobs invalid
expect_rejected 'cache maintenance actions must be used alone' \
  --clean-dev-cache --port COM0
expect_rejected '--build-path must be new or empty' \
  --build-path tests

[[ ! -e build/contract-must-not-exist ]] ||
  fail "a rejected boundary check created an artifact directory"

echo "BUILD WRAPPER CONTRACT PASSED"
