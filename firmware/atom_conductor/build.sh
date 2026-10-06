#!/usr/bin/env bash
# Build and optionally USB-flash the Atom Matrix fleet conductor.
set -euo pipefail

FQBN="esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHANNEL=11
PULSE_MS=25
COLOR_VALUE=64
PORT=""
BUILD_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --channel) CHANNEL="$2"; shift 2;;
    --pulse-ms) PULSE_MS="$2"; shift 2;;
    --color-value) COLOR_VALUE="$2"; shift 2;;
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
[[ "$COLOR_VALUE" =~ ^[0-9]+$ ]] && \
  (( COLOR_VALUE >= 1 && COLOR_VALUE <= 255 )) || {
  echo "color-value must be 1..255" >&2; exit 2;
}

if [[ -z "$BUILD_PATH" ]]; then
  BUILD_PATH="${SKETCH_DIR}/build/atom-conductor-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "$BUILD_PATH"

FLAGS="-DNB_CHANNEL=${CHANNEL} -DRES_ATOM_PULSE_MS=${PULSE_MS} -DRES_ATOM_COLOR_VALUE=${COLOR_VALUE}"
echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"

arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_PATH" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" "$SKETCH_DIR"

if [[ -n "$PORT" ]]; then
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" \
    --build-path "$BUILD_PATH" "$SKETCH_DIR"
  echo "flashed ${PORT}"
fi

echo "artifact: ${BUILD_PATH}/atom_conductor.ino.bin"
