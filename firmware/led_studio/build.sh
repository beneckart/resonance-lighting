#!/usr/bin/env bash
# Build (and optionally flash) the LED Studio (merged HEX + RGBW) bench tool.
#
# Usage:
#   ./build.sh                       # compile only
#   ./build.sh --port /dev/ttyACM0   # compile + USB flash
#   ./build.sh --pin 16 --port /dev/ttyACM0   # override HEX data pin (default 10)
#
# WiFi: the sketch #includes wifi_secrets.h. If it's missing, build.sh copies the
# one from ../power_bench (same SSID/password). With no secrets at all, the
# firmware falls back to a SoftAP "ResonanceLED" (pw resonance) at 192.168.4.1.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=""; PIN=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2;;
    --pin)  PIN="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

# Reuse the power-bench WiFi creds if we don't have our own.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" && -f "${SKETCH_DIR}/../power_bench/wifi_secrets.h" ]]; then
  cp "${SKETCH_DIR}/../power_bench/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
  echo "copied wifi_secrets.h from ../power_bench"
fi

FLAGS=""
[[ -n "${PIN}" ]] && FLAGS="-DDATA_PIN=${PIN}"

ARGS=(compile --fqbn "${FQBN}" "${SKETCH_DIR}")
[[ -n "${FLAGS}" ]] && ARGS+=(--build-property "compiler.cpp.extra_flags=${FLAGS}")
echo "arduino-cli ${ARGS[*]}"
arduino-cli "${ARGS[@]}"

if [[ -n "${PORT}" ]]; then
  echo "flashing to ${PORT}"
  arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" "${SKETCH_DIR}"
  echo "done. open the serial monitor (115200) to see the IP."
fi
