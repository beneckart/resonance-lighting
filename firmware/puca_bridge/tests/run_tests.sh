#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
OUT="$(mktemp -d /tmp/puca-bridge-tests.XXXXXX)"
trap 'rm -rf "$OUT"' EXIT

"$CXX" -std=gnu++17 -Wall -Wextra -Werror test_puca_core.cpp \
  -o "$OUT/test_puca_core"
"$OUT/test_puca_core"
