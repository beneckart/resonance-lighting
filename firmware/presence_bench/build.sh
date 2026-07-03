#!/usr/bin/env bash
# Build (and optionally flash) the presence bench firmware.
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 on the PowerFeather target (#error guard).
#
# Usage:
#   ./build.sh --port /dev/ttyACM0          # USB flash the PowerFeather
#   ./build.sh --ota 192.168.x.y            # OTA flash a running bench
#   ./build.sh --board metro --port ...     # Metro ESP32-S3 variant (plain Wire)
#   ./build.sh --i2c-hz 100000              # bus-speed fallback (degraded rates)
#   ./build.sh --no-mlx --no-xm             # exclude sensors from the build
#   ./build.sh                              # compile only
#
# This script uses a unique Arduino build path per run. Do not remove that: parallel
# compiles against Arduino's default sketch cache have collided/corrupted artifacts.
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BOARD="powerfeather"; PORT=""; OTA_IP=""; I2C_HZ=""
NO_MLX=""; NO_VL53=""; NO_TMF=""; NO_XM=""; NO_L1X=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --board) BOARD="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --ota) OTA_IP="$2"; shift 2;;
    --i2c-hz) I2C_HZ="$2"; shift 2;;
    --no-mlx) NO_MLX="1"; shift;;
    --no-vl53) NO_VL53="1"; shift;;
    --no-tmf) NO_TMF="1"; shift;;
    --no-xm) NO_XM="1"; shift;;
    --no-l1x) NO_L1X="1"; shift;;
    --no-breadcrumb) NO_BC="1"; shift;;
    --no-task) NO_TASK="1"; shift;;
    --task-no-sdk) TASK_NO_SDK="1"; shift;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done
if [[ -n "${PORT}" && -n "${OTA_IP}" ]]; then echo "use --port OR --ota, not both" >&2; exit 2; fi

case "${BOARD}" in
  powerfeather) FQBN="esp32:esp32:esp32s3_powerfeather"; FLAGS="-DPOWERFEATHER_BOARD_V2=1";;
  metro) FQBN="esp32:esp32:adafruit_metro_esp32s3:USBMode=hwcdc,CDCOnBoot=cdc"; FLAGS="-DPB_BOARD_METRO=1";;
  *) echo "unknown --board: ${BOARD} (powerfeather|metro)" >&2; exit 2;;
esac

# Reuse known local WiFi creds if we don't have our own.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" ]]; then
  for SRC in ../power_bench ../led_studio ../net_bench; do
    if [[ -f "${SKETCH_DIR}/${SRC}/wifi_secrets.h" ]]; then
      cp "${SKETCH_DIR}/${SRC}/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
      echo "copied wifi_secrets.h from ${SRC}"
      break
    fi
  done
fi

[[ -n "${I2C_HZ}" ]] && FLAGS+=" -DPB_I2C_HZ=${I2C_HZ}"
[[ -n "${NO_MLX}" ]] && FLAGS+=" -DPB_ENABLE_MLX=0"
[[ -n "${NO_VL53}" ]] && FLAGS+=" -DPB_ENABLE_VL53=0"
[[ -n "${NO_TMF}" ]] && FLAGS+=" -DPB_ENABLE_TMF=0"
[[ -n "${NO_XM}" ]] && FLAGS+=" -DPB_ENABLE_XM=0"
[[ -n "${NO_L1X}" ]] && FLAGS+=" -DPB_ENABLE_L1X=0"
[[ -n "${NO_BC:-}" ]] && FLAGS+=" -DPB_BREADCRUMB=0"
[[ -n "${NO_TASK:-}" ]] && FLAGS+=" -DPB_NO_TASK=1"
[[ -n "${TASK_NO_SDK:-}" ]] && FLAGS+=" -DPB_TASK_NO_SDK=1"

BUILD_PATH="${ARDUINO_BUILD_PATH:-$(mktemp -d)}"
if [[ -z "${ARDUINO_BUILD_PATH:-}" ]]; then
  trap 'rm -rf "${BUILD_PATH}"' EXIT
fi

echo "FQBN: ${FQBN}"
echo "FLAGS: ${FLAGS}"
echo "BUILD_PATH: ${BUILD_PATH}"
if [[ -n "${OTA_IP}" ]]; then
  OUT="$(mktemp -d)"
  arduino-cli compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" \
    --build-property "compiler.cpp.extra_flags=${FLAGS}" \
    --output-dir "${OUT}" "${SKETCH_DIR}"
  BIN="${OUT}/$(basename "${SKETCH_DIR}").ino.bin"
  echo "OTA -> http://${OTA_IP}/update"
  curl -fsS -H 'Expect:' --max-time 180 -F "firmware=@${BIN}" "http://${OTA_IP}/update" || true
  echo
else
  arduino-cli compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" \
    --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}"
  if [[ -n "${PORT}" ]]; then
    arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" --build-path "${BUILD_PATH}" "${SKETCH_DIR}"
    echo "flashed ${PORT}; open serial (115200) for the boot banner."
  fi
fi
