#!/usr/bin/env bash
# Build (and optionally flash) the LED Studio (merged HEX + RGBW) bench tool.
#
# Usage:
#   ./build.sh                       # compile only
#   ./build.sh --port /dev/ttyACM0   # compile + USB flash
#   ./build.sh --pin 16 --port /dev/ttyACM0   # override HEX data pin (default 10)
#   ./build.sh --sensor-triad --cap 6000 --build-name tn-f2bfa0-r1
#
# WiFi: the sketch #includes wifi_secrets.h. If it's missing, build.sh copies the
# one from ../power_bench (same SSID/password). With no secrets at all, the
# firmware falls back to a SoftAP "ResonanceLED" (pw resonance) at 192.168.4.1.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=""; OTA_IP=""; PIN=""; SENSOR_TRIAD=""; CAP=""; CHARGE_MA=""; MAINTAIN=""
BUILD_NAME=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2;;
    --ota) OTA_IP="$2"; shift 2;;
    --pin)  PIN="$2"; shift 2;;
    --sensor-triad) SENSOR_TRIAD="1"; shift;;
    --cap) CAP="$2"; shift 2;;
    --charge-ma) CHARGE_MA="$2"; shift 2;;
    --maintain) MAINTAIN="$2"; shift 2;;
    --build-name) BUILD_NAME="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

# Reuse the power-bench WiFi creds if we don't have our own.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" && -f "${SKETCH_DIR}/../power_bench/wifi_secrets.h" ]]; then
  cp "${SKETCH_DIR}/../power_bench/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
  echo "copied wifi_secrets.h from ../power_bench"
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1" # SDK targets the V2 gauge/charger (LFP-safe charge profile)
[[ -n "${PIN}" ]] && FLAGS+=" -DDATA_PIN=${PIN}"
[[ -n "${SENSOR_TRIAD}" ]] && FLAGS+=" -DSTUDIO_SENSOR_TRIAD=1"
[[ -n "${CAP}" ]] && FLAGS+=" -DSTUDIO_BATTERY_MAH=${CAP}"
[[ -n "${CHARGE_MA}" ]] && FLAGS+=" -DSTUDIO_CHARGE_MA=${CHARGE_MA}"
[[ -n "${MAINTAIN}" ]] && FLAGS+=" -DSTUDIO_MAINTAIN_V=${MAINTAIN}"

if [[ -z "${BUILD_NAME}" ]]; then
  BUILD_NAME="led-studio-$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
BUILD_PATH="${SKETCH_DIR}/build/${BUILD_NAME}"
ARGS=(compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" "${SKETCH_DIR}")
[[ -n "${FLAGS}" ]] && ARGS+=(--build-property "compiler.cpp.extra_flags=${FLAGS}")
echo "arduino-cli ${ARGS[*]}"
arduino-cli "${ARGS[@]}"
BIN="${BUILD_PATH}/led_studio.ino.bin"
test -s "${BIN}"
echo "artifact: ${BIN}"

if [[ -n "${PORT}" ]]; then
  echo "flashing to ${PORT}"
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" --input-dir "${BUILD_PATH}" "${SKETCH_DIR}"
  echo "done. open the serial monitor (115200) to see the IP."
fi

if [[ -n "${OTA_IP}" ]]; then
  echo "OTA -> http://${OTA_IP}/update"
  curl -fsS -H 'Expect:' --max-time 180 -F "firmware=@${BIN}" "http://${OTA_IP}/update"
fi
