#!/usr/bin/env bash
# Build and optionally USB-flash the PUCA performance-audio bridge PoC.
# A unique build directory is used by default to avoid Arduino cache collisions
# (never resume a killed build directory — retry with a fresh one).
#
# UNVERIFIED PoC: this firmware has never run on hardware. Flash only the PUCA
# DSP **Original Edition** (ESP32-PICO-D4); it is not a CoreS3/S3 binary and
# Original/Strawberry edition binaries are not interchangeable (ADR 0035).
set -euo pipefail

# PUCA DSP Original Edition is an ESP32-PICO-D4 (4 MB flash). The stock pico32
# profile matches; the 8 MB PSRAM is unused by this PoC.
FQBN="esp32:esp32:pico32"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_ROOT="$(cd "${SKETCH_DIR}/.." && pwd)"
PORT=""
BUILD_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2;;
    --build-path) BUILD_PATH="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

if [[ -z "${BUILD_PATH}" ]]; then
  BUILD_PATH="${SKETCH_DIR}/build/puca-bridge-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "${BUILD_PATH}"

# -I to the firmware root so the wire contract is included as
# "fixture/src/core/packet.h" (one contract, one file — never forked). The
# shared envelope tracker rides a relative include of
# ../cores3_bridge/audio_reactive.h from the sketch itself.
FLAGS="-I${FIRMWARE_ROOT}"

echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"

arduino-cli compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}"

if [[ -n "${PORT}" ]]; then
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" \
    --build-path "${BUILD_PATH}" "${SKETCH_DIR}"
  echo "flashed ${PORT}; open at 115200 baud (keys: t M A I)"
fi

echo "artifact: ${BUILD_PATH}/puca_bridge.ino.bin"
