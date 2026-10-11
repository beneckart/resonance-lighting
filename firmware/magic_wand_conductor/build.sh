#!/usr/bin/env bash
# Build and optionally USB-flash Steve's exact F40344 Magic Wand Conductor.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHANNEL=11
PULSE_MS=40
TREE_COLOR_VALUE=64
TREE_RGB_VALUE=21
PORT=""
BUILD_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --channel) CHANNEL="$2"; shift 2;;
    --pulse-ms) PULSE_MS="$2"; shift 2;;
    --tree-color-value) TREE_COLOR_VALUE="$2"; shift 2;;
    --tree-rgb-value) TREE_RGB_VALUE="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --build-path) BUILD_PATH="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

[[ "$CHANNEL" =~ ^[0-9]+$ ]] && (( CHANNEL >= 1 && CHANNEL <= 14 )) || {
  echo "channel must be 1..14" >&2; exit 2;
}
[[ "$PULSE_MS" =~ ^[0-9]+$ ]] && (( PULSE_MS >= 5 && PULSE_MS <= 300 )) || {
  echo "pulse-ms must be 5..300" >&2; exit 2;
}
[[ "$TREE_COLOR_VALUE" =~ ^[0-9]+$ ]] &&
  (( TREE_COLOR_VALUE >= 1 && TREE_COLOR_VALUE <= 255 )) || {
  echo "tree-color-value must be 1..255" >&2; exit 2;
}
[[ "$TREE_RGB_VALUE" =~ ^[0-9]+$ ]] &&
  (( TREE_RGB_VALUE >= 1 && TREE_RGB_VALUE <= 85 )) || {
  echo "tree-rgb-value must be 1..85" >&2; exit 2;
}

if [[ -z "$BUILD_PATH" ]]; then
  BUILD_PATH="${SKETCH_DIR}/build/wand-conductor-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "$BUILD_PATH"

FLAGS="-DPOWERFEATHER_BOARD_V2=1 -DNB_CHANNEL=${CHANNEL}"
FLAGS+=" -DRES_WAND_PULSE_MS=${PULSE_MS}"
FLAGS+=" -DRES_WAND_TREE_COLOR_VALUE=${TREE_COLOR_VALUE}"
FLAGS+=" -DRES_WAND_TREE_RGB_VALUE=${TREE_RGB_VALUE}"
echo "TARGET: exact Magic Wand 68:EE:8F:F4:03:44 only"
echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"

arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_PATH" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" "$SKETCH_DIR"

if [[ -n "$PORT" ]]; then
  echo "USB FLASH OWNER must verify ${PORT} is MAC 68:EE:8F:F4:03:44"
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" \
    --build-path "$BUILD_PATH" "$SKETCH_DIR"
  echo "flashed ${PORT}"
fi

echo "artifact: ${BUILD_PATH}/magic_wand_conductor.ino.bin"
