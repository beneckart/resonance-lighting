#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SKETCH_DIR="$ROOT_DIR/firmware/clacker_demo"
BUILD_PATH="$SKETCH_DIR/build/metro-esp32s3"
FQBN="esp32:esp32:adafruit_metro_esp32s3"

arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD_PATH" \
  "$SKETCH_DIR"

if [[ "${1:-}" != "" ]]; then
  arduino-cli upload \
    -p "$1" \
    --fqbn "$FQBN" \
    --input-dir "$BUILD_PATH" \
    "$SKETCH_DIR"
fi
