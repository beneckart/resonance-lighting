#!/usr/bin/env bash
# Build (and optionally USB-flash) the WiFi range/roaming diagnostic.
#
# This is a SERIAL/USB tool -- flash over USB, then read the wd-* stream over the
# native-USB serial monitor (115200) while you walk. No OTA path: you need the
# tethered laptop to read the output anyway. See wifi_diag.ino header +
# docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md (b).
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 (matches the other PF V2 sketches).
#
# Usage:
#   ./build.sh --port /dev/ttyACM0                 # USB flash, then open serial
#   ./build.sh --assoc-s 1 --scan-s 10 --port ...  # faster cadence for a walk
#   ./build.sh --tx-low --port ...                 # 8.5 dBm instead of MAX (compare)
#   ./build.sh                                     # compile only
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ASSOC_S=""; SCAN_S=""; TX_LOW=""; ROAM_MARGIN=""; PORT=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --assoc-s) ASSOC_S="$2"; shift 2;;
    --scan-s) SCAN_S="$2"; shift 2;;
    --roam-margin) ROAM_MARGIN="$2"; shift 2;;
    --tx-low) TX_LOW="1"; shift;;
    --port) PORT="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

# Reuse power-bench WiFi creds if we don't have our own.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" && -f "${SKETCH_DIR}/../power_bench/wifi_secrets.h" ]]; then
  cp "${SKETCH_DIR}/../power_bench/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
  echo "copied wifi_secrets.h from ../power_bench"
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1"
[[ -n "${ASSOC_S}" ]]     && FLAGS+=" -DWD_ASSOC_S=${ASSOC_S}"
[[ -n "${SCAN_S}" ]]      && FLAGS+=" -DWD_SCAN_S=${SCAN_S}"
[[ -n "${ROAM_MARGIN}" ]] && FLAGS+=" -DWD_ROAM_MARGIN_DB=${ROAM_MARGIN}"
[[ -n "${TX_LOW}" ]]      && FLAGS+=" -DWD_TX_LOW=1"

echo "FLAGS: ${FLAGS}"
arduino-cli compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}"
if [[ -n "${PORT}" ]]; then
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" "${SKETCH_DIR}"
  echo "flashed ${PORT}; open serial (115200) and grep for wd-roam / wd-event."
fi
