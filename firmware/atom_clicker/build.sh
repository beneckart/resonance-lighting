#!/usr/bin/env bash
# Build and optionally USB-flash the reduced-access Atom Matrix clicker.
set -euo pipefail

FQBN="esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHANNEL=11
TARGET="9E5B8C"
PULSE_MS=40
PORT=""
BUILD_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --channel) CHANNEL="$2"; shift 2;;
    --target) TARGET="${2^^}"; shift 2;;
    --pulse-ms) PULSE_MS="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --build-path) BUILD_PATH="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

[[ "$CHANNEL" =~ ^[0-9]+$ ]] && (( CHANNEL >= 1 && CHANNEL <= 14 )) || {
  echo "channel must be 1..14" >&2; exit 2;
}
[[ "$TARGET" =~ ^[0-9A-F]{6}$ ]] || {
  echo "target must be exactly six hex digits" >&2; exit 2;
}
[[ "$PULSE_MS" =~ ^[0-9]+$ ]] && (( PULSE_MS >= 5 && PULSE_MS <= 300 )) || {
  echo "pulse-ms must be 5..300" >&2; exit 2;
}

if [[ -z "$BUILD_PATH" ]]; then
  BUILD_PATH="${SKETCH_DIR}/build/atom-clicker-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "$BUILD_PATH"

T0="${TARGET:0:2}"
T1="${TARGET:2:2}"
T2="${TARGET:4:2}"
FLAGS="-DNB_CHANNEL=${CHANNEL} -DRES_CLICKER_TARGET_0=0x${T0} -DRES_CLICKER_TARGET_1=0x${T1} -DRES_CLICKER_TARGET_2=0x${T2} -DRES_CLICKER_PULSE_MS=${PULSE_MS}"

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

echo "artifact: ${BUILD_PATH}/atom_clicker.ino.bin"
