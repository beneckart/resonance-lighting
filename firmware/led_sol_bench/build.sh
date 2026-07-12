#!/usr/bin/env bash
# Build (and optionally flash) the LED+Solenoid Bench (RGBW + strike, VBAT-direct).
#
# Usage:
#   ./build.sh                        # compile only
#   ./build.sh --port /dev/ttyACM1    # compile + USB flash
#   ./build.sh --ota 192.168.4.xx     # compile + WiFi OTA to a running led_sol_bench
#   ./build.sh --led-pin 11 --sol-pin 12 --px 1 ...   # pin/count overrides (defaults shown)
#
# WiFi: the sketch #includes wifi_secrets.h. If it's missing, build.sh copies the
# one from ../solenoid_demo (falling back to ../speaker_demo, ../sway_demo,
# ../led_studio, ../power_bench). With no secrets at all, the firmware serves a
# SoftAP "ResonanceLedSol" (pw resonance) at 192.168.4.1.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=""; LED_PIN=""; SOL_PIN=""; PX=""; OTA_IP=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)    PORT="$2"; shift 2;;
    --led-pin) LED_PIN="$2"; shift 2;;
    --sol-pin) SOL_PIN="$2"; shift 2;;
    --px)      PX="$2"; shift 2;;
    --ota)     OTA_IP="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" ]]; then
  for src in ../solenoid_demo ../speaker_demo ../sway_demo ../led_studio ../power_bench; do
    if [[ -f "${SKETCH_DIR}/${src}/wifi_secrets.h" ]]; then
      cp "${SKETCH_DIR}/${src}/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
      echo "copied wifi_secrets.h from ${src}"
      break
    fi
  done
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1" # SDK targets the V2 gauge/charger
[[ -n "${LED_PIN}" ]] && FLAGS+=" -DLED_PIN=${LED_PIN}"
[[ -n "${SOL_PIN}" ]] && FLAGS+=" -DSOLENOID_PIN=${SOL_PIN}"
[[ -n "${PX}" ]] && FLAGS+=" -DLED_COUNT=${PX}"

ARGS=(compile --fqbn "${FQBN}" --export-binaries "${SKETCH_DIR}")
[[ -n "${FLAGS}" ]] && ARGS+=(--build-property "compiler.cpp.extra_flags=${FLAGS}")
echo "arduino-cli ${ARGS[*]}"
arduino-cli "${ARGS[@]}"

if [[ -n "${PORT}" ]]; then
  echo "flashing to ${PORT}"
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" "${SKETCH_DIR}"
  echo "done. open the serial monitor (115200) to see the IP, or http://ledsol.local/"
fi

if [[ -n "${OTA_IP}" ]]; then
  BIN="${SKETCH_DIR}/build/esp32.esp32.esp32s3_powerfeather/led_sol_bench.ino.bin"
  echo "OTA to http://${OTA_IP}/update"
  curl -fsS -F "firmware=@${BIN}" "http://${OTA_IP}/update"
fi
