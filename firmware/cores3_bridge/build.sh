#!/usr/bin/env bash
# Build and optionally USB-flash the dedicated M5Stack CoreS3 desk bridge.
# A unique build directory is used by default to avoid Arduino cache collisions.
set -euo pipefail

FQBN="esp32:esp32:m5stack_cores3"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHANNEL=11
PORT=""
BUILD_PATH=""
CAMBIUM_MODE=0
AUDIO_ALIAS=0
AUDIO_MODULE=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --channel) CHANNEL="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --build-path) BUILD_PATH="$2"; shift 2;;
    --cambium) CAMBIUM_MODE=1; shift;;
    # Compatibility alias. The normal image now always contains both the
    # Listener and built-in-mic Audio apps.
    --audio) AUDIO_ALIAS=1; shift;;
    --audio-module) AUDIO_MODULE=1; shift;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

if [[ "${CAMBIUM_MODE}" == "1" &&
      ( "${AUDIO_ALIAS}" == "1" || "${AUDIO_MODULE}" == "1" ) ]]; then
  echo "--cambium and --audio/--audio-module are separate artifacts" >&2
  exit 2
fi

if [[ -z "${BUILD_PATH}" ]]; then
  BUILD_PATH="${SKETCH_DIR}/build/cores3-bridge-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "${BUILD_PATH}"

FLAGS="-DNB_CHANNEL=${CHANNEL}"
if [[ "${CAMBIUM_MODE}" == "1" ]]; then
  FLAGS="${FLAGS} -DCORES3_CAMBIUM_MODE=1"
fi
if [[ "${AUDIO_MODULE}" == "1" ]]; then
  FLAGS="${FLAGS} -DCORES3_AUDIO_MODULE=1"
fi
echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"

arduino-cli compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}"

if [[ -n "${PORT}" ]]; then
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" \
    --build-path "${BUILD_PATH}" "${SKETCH_DIR}"
  echo "flashed ${PORT}; open at 115200 baud or launch the dashboard"
fi

echo "artifact: ${BUILD_PATH}/cores3_bridge.ino.bin"
