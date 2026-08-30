#!/usr/bin/env bash
# Native tests for the tdeck_bridge pure-core modules (no Arduino includes).
# Same pattern as firmware/fixture/tests/run_tests.sh: one binary per
# test_*.cpp, plain g++, warnings are errors.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$(cd "${TESTS_DIR}/.." && pwd)"
FIRMWARE_ROOT="$(cd "${SKETCH_DIR}/.." && pwd)"
BUILD_DIR="$(mktemp -d /tmp/tdeck-tests.XXXXXX)"
trap 'rm -rf "${BUILD_DIR}"' EXIT

bash "${TESTS_DIR}/test_build_wrapper_contract.sh"

python "${SKETCH_DIR}/tools/generate_health_registry.py" \
  "${FIRMWARE_ROOT}/../ops/fleet/registry.csv" \
  "${FIRMWARE_ROOT}/../ops/fleet/callsigns.csv" \
  "${FIRMWARE_ROOT}/../ops/fleet/roster.csv" \
  > "${BUILD_DIR}/fleet_registry_generated.h"
diff -u "${SKETCH_DIR}/src/core/fleet_registry_generated.h" \
  "${BUILD_DIR}/fleet_registry_generated.h"

CORE_SRCS=$(find "${SKETCH_DIR}/src/core" -name '*.cpp' 2>/dev/null | sort || true)

fail=0
for test_src in "${TESTS_DIR}"/test_*.cpp; do
  name="$(basename "${test_src}" .cpp)"
  bin="${BUILD_DIR}/${name}"
  # shellcheck disable=SC2086
  g++ -std=gnu++17 -Wall -Wextra -Werror \
    -I"${SKETCH_DIR}/src" -I"${FIRMWARE_ROOT}" \
    "${test_src}" ${CORE_SRCS} -o "${bin}"
  if "${bin}"; then
    echo "PASS ${name}"
  else
    echo "FAIL ${name}"
    fail=1
  fi
done
exit "${fail}"
