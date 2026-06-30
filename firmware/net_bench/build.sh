#!/usr/bin/env bash
# Build (and optionally flash) the net-bench ESP-NOW feasibility firmware.
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 (V2 MAX17260 fuel gauge; #error guard).
#
# Usage:
#   ./build.sh --role master --channel 6 --port /dev/ttyACM0     # USB flash a master
#   ./build.sh --role peer   --channel 6 --port /dev/ttyACM1     # USB flash a peer
#   ./build.sh --role peer   --channel 6 --ota 192.168.4.61      # OTA flash (maintenance mode)
#   ./build.sh --role peer   --channel 6 --maint-ap              # emergency single-board AP OTA only
#   ./build.sh                                                    # compile only
#
# The bench AP MUST be configured to the same fixed channel as --channel.
# This script uses a unique Arduino build path per run. Do not remove that: parallel
# compiles against Arduino's default sketch cache have collided/corrupted artifacts.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ROLE=""; CHANNEL=""; FRAME_HZ=""; HB_HZ=""; JITTER=""; WDT_S=""; WDT_HANG=""
MAINT_TIMEOUT=""; START_MAINT=""; AUTOSLEEP=""; BUDGET=""; WAKE=""; LOWPOWER=""
CHEM=""; CAP=""; CHARGE=""; CHARGE_MA=""; MAINTAIN=""; PORT=""; OTA_IP=""
SERIAL_BRIDGE=""; SCAN_REPORT=""; SCAN_S=""; SCAN_MAX=""; SLEEP_CYCLE=""; SLEEP_S=""; WAKE_LISTEN_MS=""; BATT_NTC=""; MAINT_AP=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --role) ROLE="$2"; shift 2;;
    --channel) CHANNEL="$2"; shift 2;;
    --frame-hz) FRAME_HZ="$2"; shift 2;;
    --hb-hz) HB_HZ="$2"; shift 2;;
    --jitter-pct) JITTER="$2"; shift 2;;
    --wdt-s) WDT_S="$2"; shift 2;;
    --wdt-hangtest) WDT_HANG="1"; shift;;
    --maint-timeout) MAINT_TIMEOUT="$2"; shift 2;;
    --start-maint) START_MAINT="1"; shift;;
    --autosleep) AUTOSLEEP="1"; shift;;
    --budget-mah) BUDGET="$2"; shift 2;;
    --wake-s) WAKE="$2"; shift 2;;
    --wifi-lowpower) LOWPOWER="1"; shift;;
    --serial-bridge) SERIAL_BRIDGE="1"; shift;;     # desk bridge: relay nb-* to USB serial (master, no WiFi)
    --maint-ap) MAINT_AP="1"; shift;;               # emergency only: not fleet-scalable
    --scan-report) SCAN_REPORT="1"; shift;;         # field peer: 2.4 GHz scan-report over ESP-NOW
    --scan-s) SCAN_S="$2"; shift 2;;
    --scan-max) SCAN_MAX="$2"; shift 2;;
    --sleep-cycle) SLEEP_CYCLE="1"; shift;;         # field peer: deep-sleep duty-cycled load measurement
    --sleep-s) SLEEP_S="$2"; shift 2;;
    --wake-listen-ms) WAKE_LISTEN_MS="$2"; shift 2;;
    --batt-ntc) BATT_NTC="1"; shift;;               # battery thermistor on charger TS -- ONLY with the NTC physically attached

    --chem) CHEM="$2"; shift 2;;
    --cap) CAP="$2"; shift 2;;
    --charge-ma) CHARGE_MA="$2"; shift 2;;
    --no-charge) CHARGE="0"; shift;;
    --maintain) MAINTAIN="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --ota) OTA_IP="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done
if [[ -n "${PORT}" && -n "${OTA_IP}" ]]; then echo "use --port OR --ota, not both" >&2; exit 2; fi

# Reuse known local WiFi creds if we don't have our own. This keeps the default
# maintenance path on shared WiFi, which is the only fleet-scalable OTA mode.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" ]]; then
  if [[ -f "${SKETCH_DIR}/../power_bench/wifi_secrets.h" ]]; then
    cp "${SKETCH_DIR}/../power_bench/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
    echo "copied wifi_secrets.h from ../power_bench"
  elif [[ -f "${SKETCH_DIR}/../led_studio/wifi_secrets.h" ]]; then
    cp "${SKETCH_DIR}/../led_studio/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
    echo "copied wifi_secrets.h from ../led_studio"
  fi
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1"
case "${ROLE}" in
  master) FLAGS+=" -DNB_ROLE_MASTER=1";;
  peer|"") FLAGS+=" -DNB_ROLE_PEER=1";;
  *) echo "unknown --role: ${ROLE}" >&2; exit 2;;
esac
[[ -n "${CHANNEL}" ]]       && FLAGS+=" -DNB_CHANNEL=${CHANNEL}"
[[ -n "${FRAME_HZ}" ]]      && FLAGS+=" -DNB_FRAME_HZ=${FRAME_HZ}"
[[ -n "${HB_HZ}" ]]         && FLAGS+=" -DNB_HB_HZ=${HB_HZ}"
[[ -n "${JITTER}" ]]        && FLAGS+=" -DNB_JITTER_PCT=${JITTER}"
[[ -n "${WDT_S}" ]]         && FLAGS+=" -DNB_WDT_S=${WDT_S}"
[[ -n "${WDT_HANG}" ]]      && FLAGS+=" -DNB_WDT_HANGTEST=1"
[[ -n "${MAINT_TIMEOUT}" ]] && FLAGS+=" -DNB_MAINT_TIMEOUT_S=${MAINT_TIMEOUT}"
[[ -n "${START_MAINT}" ]]   && FLAGS+=" -DNB_START_MAINT=1"
[[ -n "${AUTOSLEEP}" ]]     && FLAGS+=" -DNB_AUTOSLEEP=1"
[[ -n "${BUDGET}" ]]        && FLAGS+=" -DNB_BUDGET_MAH=${BUDGET}"
[[ -n "${WAKE}" ]]          && FLAGS+=" -DNB_WAKE_S=${WAKE}"
[[ -n "${LOWPOWER}" ]]      && FLAGS+=" -DNB_WIFI_LOWPOWER=1"
[[ -n "${SERIAL_BRIDGE}" ]] && FLAGS+=" -DNB_SERIAL_BRIDGE=1"
[[ -n "${MAINT_AP}" ]]      && FLAGS+=" -DNB_MAINT_AP=1"
[[ -n "${SCAN_REPORT}" ]]   && FLAGS+=" -DNB_SCAN_REPORT=1"
[[ -n "${SCAN_S}" ]]        && FLAGS+=" -DNB_SCAN_S=${SCAN_S}"
[[ -n "${SCAN_MAX}" ]]      && FLAGS+=" -DNB_SCAN_MAX=${SCAN_MAX}"
[[ -n "${SLEEP_CYCLE}" ]]   && FLAGS+=" -DNB_SLEEP_CYCLE=1"
[[ -n "${SLEEP_S}" ]]       && FLAGS+=" -DNB_SLEEP_S=${SLEEP_S}"
[[ -n "${WAKE_LISTEN_MS}" ]] && FLAGS+=" -DNB_WAKE_LISTEN_MS=${WAKE_LISTEN_MS}"
[[ -n "${BATT_NTC}" ]]      && FLAGS+=" -DNB_BATT_NTC=1"
[[ -n "${CAP}" ]]           && FLAGS+=" -DRES_PF_BATTERY_CAPACITY_MAH=${CAP}"
case "${CHEM}" in
  3v7) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_3V7";;
  lfp) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_LFP";;
  "") ;;
  *) echo "unknown --chem: ${CHEM}" >&2; exit 2;;
esac
[[ -n "${CHARGE}" ]]    && FLAGS+=" -DRES_PF_ENABLE_CHARGING=${CHARGE}"
[[ -n "${CHARGE_MA}" ]] && FLAGS+=" -DRES_PF_MAX_CHARGE_MA=${CHARGE_MA}"
[[ -n "${MAINTAIN}" ]]  && FLAGS+=" -DRES_PF_MAINTAIN_V=${MAINTAIN}"

BUILD_PATH="${ARDUINO_BUILD_PATH:-$(mktemp -d)}"
if [[ -z "${ARDUINO_BUILD_PATH:-}" ]]; then
  trap 'rm -rf "${BUILD_PATH}"' EXIT
fi

if [[ -n "${MAINT_AP}" ]]; then
  echo "WARNING: --maint-ap is deprecated/emergency one-board fallback only; use shared-WiFi parallel OTA by default." >&2
fi

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
  ARGS=(compile --fqbn "${FQBN}" --build-path "${BUILD_PATH}" --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}")
  arduino-cli "${ARGS[@]}"
  if [[ -n "${PORT}" ]]; then
    arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" --build-path "${BUILD_PATH}" "${SKETCH_DIR}"
    echo "flashed ${PORT}; open serial (115200) for the boot banner."
  fi
fi
