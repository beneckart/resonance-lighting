#!/usr/bin/env bash
# Build/flash the ported PowerFeather demo (web telemetry app) for the V2 board.
# Always sets -DPOWERFEATHER_BOARD_V2=1 (V2 MAX17260 gauge). Optional --port to flash.
#
# Library stack (core-3.x compatible; install once):
#   arduino-cli lib install "Async TCP" "ESP Async WebServer" "ArduinoJson" "ESPUI"
#   (and: arduino-cli lib uninstall AsyncTCP   # remove the old core-2.x one)
#
# Usage: ./build_demo.sh [--port /dev/ttyACM0] [--cap MAH] [--chem 3v7|lfp]
set -euo pipefail
FQBN="esp32:esp32:esp32s3_powerfeather"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=""; CAP=""; CHEM=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2;;
    --cap) CAP="$2"; shift 2;;
    --chem) CHEM="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done
FLAGS="-DPOWERFEATHER_BOARD_V2=1"
[[ -n "${CAP}" ]] && FLAGS+=" -DBATTERY_CAPACITY=${CAP}"
case "${CHEM}" in
  3v7) FLAGS+=" -DDEMO_BATTERY_TYPE=Mainboard::BatteryType::Generic_3V7";;
  lfp) FLAGS+=" -DDEMO_BATTERY_TYPE=Mainboard::BatteryType::Generic_LFP";;
  "" ) ;;
  *) echo "unknown --chem: ${CHEM}" >&2; exit 2;;
esac
ARGS=(compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}")
[[ -n "${PORT}" ]] && ARGS+=(-u -p "${PORT}")
ARGS+=("${DIR}")
( set -x; arduino-cli "${ARGS[@]}" )
