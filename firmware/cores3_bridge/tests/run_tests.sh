#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
OUT="$(mktemp -d /tmp/cores3-bridge-tests.XXXXXX)"
trap 'rm -rf "$OUT"' EXIT

"$CXX" -std=gnu++17 -Wall -Wextra -Werror test_audio_reactive.cpp \
  -o "$OUT/test_audio_reactive"
"$OUT/test_audio_reactive"

"$CXX" -std=gnu++17 -Wall -Wextra -Werror test_audio_timing.cpp \
  -o "$OUT/test_audio_timing"
"$OUT/test_audio_timing"

"$CXX" -std=gnu++17 -Wall -Wextra -Werror test_app_model.cpp \
  -o "$OUT/test_app_model"
"$OUT/test_app_model"
