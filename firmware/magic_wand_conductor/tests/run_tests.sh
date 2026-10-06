#!/usr/bin/env bash
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$(cd "${TESTS_DIR}/.." && pwd)"
BUILD_DIR="$(mktemp -d /tmp/magic-wand-conductor-tests.XXXXXX)"
trap 'rm -rf "${BUILD_DIR}"' EXIT

CORE_SRCS="$(find "${SKETCH_DIR}/src/core" -name '*.cpp' | sort)"
fail=0
for test_src in "${TESTS_DIR}"/test_*.cpp; do
  name="$(basename "${test_src}" .cpp)"
  bin="${BUILD_DIR}/${name}"
  # shellcheck disable=SC2086
  g++ -std=gnu++17 -Wall -Wextra -Werror -I"${SKETCH_DIR}/src" \
    "${test_src}" ${CORE_SRCS} -o "${bin}"
  if "${bin}"; then
    echo "PASS ${name}"
  else
    echo "FAIL ${name}"
    fail=1
  fi
done
exit "${fail}"
