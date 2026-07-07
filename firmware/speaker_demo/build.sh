#!/usr/bin/env bash
# Build (and optionally flash) the Speaker Demo (STEMMA speaker #3885 synth bench).
#
# Usage:
#   ./build.sh                       # compile only
#   ./build.sh --port /dev/ttyACM1   # compile + USB flash
#   ./build.sh --ota 192.168.4.xx    # compile + WiFi OTA to a running speaker_demo
#   ./build.sh --pin 16 --port ...   # override audio PWM pin (default 10 / A0)
#
# WiFi: the sketch #includes wifi_secrets.h. If it's missing, build.sh copies the
# one from ../sway_demo (falling back to ../led_studio, ../power_bench). With no
# secrets at all, the firmware serves a SoftAP "ResonanceSpeaker" (pw resonance)
# at 192.168.4.1.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=""; PIN=""; OTA_IP=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2;;
    --pin)  PIN="$2"; shift 2;;
    --ota)  OTA_IP="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" ]]; then
  for src in ../sway_demo ../led_studio ../power_bench; do
    if [[ -f "${SKETCH_DIR}/${src}/wifi_secrets.h" ]]; then
      cp "${SKETCH_DIR}/${src}/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
      echo "copied wifi_secrets.h from ${src}"
      break
    fi
  done
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1" # SDK targets the V2 gauge/charger
[[ -n "${PIN}" ]] && FLAGS+=" -DAUDIO_PIN=${PIN}"

ARGS=(compile --fqbn "${FQBN}" --export-binaries "${SKETCH_DIR}")
[[ -n "${FLAGS}" ]] && ARGS+=(--build-property "compiler.cpp.extra_flags=${FLAGS}")
echo "arduino-cli ${ARGS[*]}"
arduino-cli "${ARGS[@]}"

if [[ -n "${PORT}" ]]; then
  echo "flashing to ${PORT}"
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" "${SKETCH_DIR}"
  echo "done. open the serial monitor (115200) to see the IP, or http://speakerdemo.local/"
fi

if [[ -n "${OTA_IP}" ]]; then
  BIN="${SKETCH_DIR}/build/esp32.esp32.esp32s3_powerfeather/speaker_demo.ino.bin"
  echo "OTA to http://${OTA_IP}/update"
  curl -fsS -F "firmware=@${BIN}" "http://${OTA_IP}/update"
fi
