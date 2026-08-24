#!/usr/bin/env bash
# Native unit tests for src/core/ (platform-independent; no Arduino).
# Each test_*.cpp is its own binary with a main().
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
FLAGS="-std=gnu++17 -Wall -Wextra -Werror -I../src/core"
OUT="$(mktemp -d /tmp/fixture-tests.XXXXXX)"
trap 'rm -rf "$OUT"' EXIT

# Keep the local fast-build path and immutable fleet path from drifting while
# remaining compile-free until the native C++ suite starts below.
"$PWD/test_build_wrapper_contract.sh"

# core .cpp files (headers-only modules need nothing here).
CORE_SRCS=$(ls ../src/core/*.cpp ../src/core/choreo/*.cpp 2>/dev/null || true)

rc=0
for t in test_*.cpp; do
  bin="$OUT/${t%.cpp}"
  if ! $CXX $FLAGS "$t" $CORE_SRCS -o "$bin"; then
    echo "BUILD FAIL: $t"
    rc=1
    continue
  fi
  if ! "$bin"; then
    rc=1
  fi
done

if [[ $rc -eq 0 ]]; then echo "ALL TESTS PASSED"; else echo "TEST FAILURES"; fi
exit $rc
