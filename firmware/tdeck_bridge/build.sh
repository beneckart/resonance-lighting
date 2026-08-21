#!/usr/bin/env bash
# Build and optionally USB-flash the Resonance Bridge OS (LilyGO T-Deck Plus).
# A unique build directory is used by default to avoid Arduino cache collisions
# (never resume a killed build directory — retry with a fresh one).
set -euo pipefail

# T-Deck Plus is ESP32-S3FN16R8: 16 MB QIO flash + 8 MB OPI PSRAM. A wrong
# PSRAM mode boot-loops, so these options are pinned here on purpose.
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
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
  BUILD_PATH="${SKETCH_DIR}/build/tdeck-bridge-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
mkdir -p "${BUILD_PATH}"

# -I to the firmware root so the wire contract is included as
# "fixture/src/core/packet.h" (one contract, one file — never forked).
# -I to the sketch dir + LV_CONF_INCLUDE_SIMPLE so the LVGL library sources
# (C files — hence compiler.c.extra_flags too) find our lv_conf.h.
# Version lives in the sketch #define; quoted -D strings through extra_flags
# are the known Windows-mangling trap the fixture build already hit.
# Windows flash bench: arduino-cli is a Windows executable even under Git
# Bash, so translate /c/... to C:/... for compiler flags (fixture/build.sh
# precedent). Deps there: arduino-cli lib install lvgl@9.5.0 LovyanGFX@1.2.24
INC_ROOT="${FIRMWARE_ROOT}"
INC_SKETCH="${SKETCH_DIR}"
if command -v cygpath >/dev/null 2>&1; then
  INC_ROOT="$(cygpath -m "${FIRMWARE_ROOT}")"
  INC_SKETCH="$(cygpath -m "${SKETCH_DIR}")"
fi
FLAGS="-I${INC_ROOT} -I${INC_SKETCH} -DLV_CONF_INCLUDE_SIMPLE"

echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"

arduino-cli compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" \
  --build-property "compiler.c.extra_flags=${FLAGS}" "${SKETCH_DIR}"

if [[ -n "${PORT}" ]]; then
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" \
    --build-path "${BUILD_PATH}" "${SKETCH_DIR}"
  echo "flashed ${PORT}; open at 115200 baud (type: help)"
fi

echo "artifact: ${BUILD_PATH}/tdeck_bridge.ino.bin"
