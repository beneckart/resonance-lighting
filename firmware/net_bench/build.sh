#!/usr/bin/env bash
# Build (and optionally flash) the net-bench ESP-NOW feasibility firmware.
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 (V2 MAX17260 fuel gauge; #error guard).
#
# Usage:
#   ./build.sh --role master --channel 6 --port /dev/ttyACM0     # USB flash a master
#   ./build.sh --role peer   --channel 6 --port /dev/ttyACM1     # USB flash a peer
#   ./build.sh --role peer   --channel 6 --ota 192.168.4.61      # OTA flash (maintenance mode)
#   ./build.sh                                                    # compile only
#
# The bench AP MUST be configured to the same fixed channel as --channel.
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ROLE=""; CHANNEL=""; FRAME_HZ=""; HB_HZ=""; JITTER=""; WDT_S=""; WDT_HANG=""
MAINT_TIMEOUT=""; START_MAINT=""; AUTOSLEEP=""; BUDGET=""; WAKE=""; LOWPOWER=""
CHEM=""; CAP=""; CHARGE=""; CHARGE_MA=""; MAINTAIN=""; PORT=""; OTA_IP=""
SERIAL_BRIDGE=""; SCAN_REPORT=""; SCAN_S=""; SCAN_MAX=""
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
    --scan-report) SCAN_REPORT="1"; shift;;         # field peer: 2.4 GHz scan-report over ESP-NOW
    --scan-s) SCAN_S="$2"; shift 2;;
    --scan-max) SCAN_MAX="$2"; shift 2;;
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

# Reuse power-bench WiFi creds if we don't have our own.
if [[ ! -f "${SKETCH_DIR}/wifi_secrets.h" && -f "${SKETCH_DIR}/../power_bench/wifi_secrets.h" ]]; then
  cp "${SKETCH_DIR}/../power_bench/wifi_secrets.h" "${SKETCH_DIR}/wifi_secrets.h"
  echo "copied wifi_secrets.h from ../power_bench"
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
[[ -n "${SCAN_REPORT}" ]]   && FLAGS+=" -DNB_SCAN_REPORT=1"
[[ -n "${SCAN_S}" ]]        && FLAGS+=" -DNB_SCAN_S=${SCAN_S}"
[[ -n "${SCAN_MAX}" ]]      && FLAGS+=" -DNB_SCAN_MAX=${SCAN_MAX}"
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

echo "FLAGS: ${FLAGS}"
if [[ -n "${OTA_IP}" ]]; then
  OUT="$(mktemp -d)"
  arduino-cli compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}" \
    --output-dir "${OUT}" "${SKETCH_DIR}"
  BIN="${OUT}/$(basename "${SKETCH_DIR}").ino.bin"
  echo "OTA -> http://${OTA_IP}/update"
  curl -fsS -H 'Expect:' --max-time 180 -F "firmware=@${BIN}" "http://${OTA_IP}/update" || true
  echo
else
  ARGS=(compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}" "${SKETCH_DIR}")
  arduino-cli "${ARGS[@]}"
  if [[ -n "${PORT}" ]]; then
    arduino-cli upload --fqbn "${FQBN}" --port "${PORT}" "${SKETCH_DIR}"
    echo "flashed ${PORT}; open serial (115200) for the boot banner."
  fi
fi
