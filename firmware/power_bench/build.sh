#!/usr/bin/env bash
# Build (and optionally flash) the PowerFeather power-bench firmware.
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 so the SDK selects the V2 MAX17260 fuel
# gauge (without it the SDK silently uses the V1 LC709204F and SOC/health/cycles
# fail). The sketch has a #error guard to enforce this.
#
# Usage:
#   ./build.sh [--led is31|neohex|rgbw1|neodriver|none] [--cap MAH] [--chem 3v7|lfp]
#              [--charge-ma MA] [--no-charge] [--maintain V]
#              [--port /dev/ttyACM0 | --ota <ip>] [-- <extra ardocl args>]
#
# --port  flashes over USB; --ota flashes wirelessly via the firmware's web
# /update endpoint (no USB needed -- use this for deployed/outdoor harnesses).
#
# Examples:
#   ./build.sh --led is31 --cap 4400 --port /dev/ttyACM0          # build + USB flash
#   ./build.sh --led is31 --cap 4400 --ota 192.168.4.185         # build + OTA flash
#   ./build.sh --led neohex --cap 1500 --chem lfp --no-charge     # build only
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

LED=""; CAP=""; CHEM=""; CHARGE=""; CHARGE_MA=""; MAINTAIN=""; PORT=""; OTA_IP=""; WIFI_LP=""; BATT_STRESS=""; BATT_STRESS_FULL=""; LOADGEN=""; LOADGEN_LED=""; LOADGEN_TXHEAVY=""; LOADGEN_SHED=""; LOADGEN_AUTOSLEEP=""; PIXEL_PIN=""; BUDGET_MAH=""; WAKE_S=""; BRIGHTNESS=""; BRIGHT_SWEEP=""; SWEEP_MAX=""; RGBW_WHITE=""; STEP_MS=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --led) LED="$2"; shift 2;;
    --pixel-pin) PIXEL_PIN="$2"; shift 2;;
    --cap) CAP="$2"; shift 2;;
    --chem) CHEM="$2"; shift 2;;
    --charge-ma) CHARGE_MA="$2"; shift 2;;
    --no-charge) CHARGE="0"; shift;;
    --wifi-lowpower) WIFI_LP="1"; shift;;
    --batt-stress) BATT_STRESS="1"; shift;;
    --batt-stress-full) BATT_STRESS="1"; BATT_STRESS_FULL="1"; shift;;
    --loadgen) LOADGEN="1"; shift;;
    --loadgen-led) LOADGEN="1"; LOADGEN_LED="1"; shift;;
    --tx-heavy) LOADGEN="1"; LOADGEN_TXHEAVY="1"; shift;;
    --loadgen-shed) LOADGEN="1"; LOADGEN_SHED="1"; shift;;
    --autosleep) LOADGEN="1"; LOADGEN_AUTOSLEEP="1"; shift;;
    --budget-mah) BUDGET_MAH="$2"; shift 2;;
    --wake-s) WAKE_S="$2"; shift 2;;
    --brightness) BRIGHTNESS="$2"; shift 2;;
    --bright-sweep) LOADGEN="1"; BRIGHT_SWEEP="1"; shift;;
    --sweep-max) SWEEP_MAX="$2"; shift 2;;
    --rgbw-white) RGBW_WHITE="1"; shift;;
    --step-ms) STEP_MS="$2"; shift 2;;
    --maintain) MAINTAIN="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --ota) OTA_IP="$2"; shift 2;;
    --) shift; break;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done
if [[ -n "${PORT}" && -n "${OTA_IP}" ]]; then
  echo "use either --port (USB) or --ota (WiFi), not both" >&2; exit 2
fi

FLAGS="-DPOWERFEATHER_BOARD_V2=1"
case "${LED}" in
  is31)      FLAGS+=" -DRES_PF_LED_IS31=1";;
  neohex)    FLAGS+=" -DRES_PF_LED_NEOHEX=1";;
  rgbw1)     FLAGS+=" -DRES_PF_LED_RGBW1=1";;
  neodriver) FLAGS+=" -DRES_PF_LED_NEODRIVER=1";;
  none|"" ) ;;
  *) echo "unknown --led: ${LED}" >&2; exit 2;;
esac
[[ -n "${CAP}" ]] && FLAGS+=" -DRES_PF_BATTERY_CAPACITY_MAH=${CAP}"
case "${CHEM}" in
  3v7) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_3V7";;
  lfp) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_LFP";;
  "" ) ;;
  *) echo "unknown --chem: ${CHEM}" >&2; exit 2;;
esac
[[ -n "${CHARGE}" ]] && FLAGS+=" -DRES_PF_ENABLE_CHARGING=${CHARGE}"
# No 'f' suffix: the firmware casts these with (float), and '1000f' is an invalid
# literal (parsed as a user-defined-literal operator).
[[ -n "${CHARGE_MA}" ]] && FLAGS+=" -DRES_PF_MAX_CHARGE_MA=${CHARGE_MA}"
[[ -n "${MAINTAIN}" ]] && FLAGS+=" -DRES_PF_MAINTAIN_V=${MAINTAIN}"
[[ -n "${WIFI_LP}" ]] && FLAGS+=" -DRES_WIFI_LOWPOWER=1"
[[ -n "${BATT_STRESS}" ]] && FLAGS+=" -DRES_BATT_STRESS=1"
[[ -n "${BATT_STRESS_FULL}" ]] && FLAGS+=" -DRES_BATT_STRESS_FULL=1"
[[ -n "${LOADGEN}" ]] && FLAGS+=" -DRES_LOADGEN=1"
[[ -n "${LOADGEN_LED}" ]] && FLAGS+=" -DRES_LOADGEN_LED=1"
[[ -n "${LOADGEN_TXHEAVY}" ]] && FLAGS+=" -DRES_LOADGEN_TXHEAVY=1"
[[ -n "${LOADGEN_SHED}" ]] && FLAGS+=" -DRES_LOADGEN_SHED=1"
[[ -n "${LOADGEN_AUTOSLEEP}" ]] && FLAGS+=" -DRES_LOADGEN_AUTOSLEEP=1"
[[ -n "${PIXEL_PIN}" ]] && FLAGS+=" -DRES_PIXEL_PIN=${PIXEL_PIN}"
[[ -n "${BUDGET_MAH}" ]] && FLAGS+=" -DRES_LOADGEN_BUDGET_MAH=${BUDGET_MAH}"
[[ -n "${WAKE_S}" ]] && FLAGS+=" -DLG_SLEEP_WAKE_S=${WAKE_S}"
[[ -n "${BRIGHTNESS}" ]] && FLAGS+=" -DRES_LED_BRIGHTNESS=${BRIGHTNESS}"
[[ -n "${BRIGHT_SWEEP}" ]] && FLAGS+=" -DRES_LOADGEN_BRIGHTSWEEP=1"
[[ -n "${SWEEP_MAX}" ]] && FLAGS+=" -DRES_SWEEP_MAX=${SWEEP_MAX}"
[[ -n "${RGBW_WHITE}" ]] && FLAGS+=" -DRES_RGBW_WHITE_ONLY=1"
[[ -n "${STEP_MS}" ]] && FLAGS+=" -DRES_SWEEP_STEP_MS=${STEP_MS}"

echo "flags: ${FLAGS}"

if [[ -n "${OTA_IP}" ]]; then
  # Compile to a temp dir, then POST the .bin to the firmware's /update endpoint.
  OUT="$(mktemp -d)"
  trap 'rm -rf "${OUT}"' EXIT
  ( set -x
    arduino-cli compile --fqbn "${FQBN}" \
      --build-property "compiler.cpp.extra_flags=${FLAGS}" \
      --output-dir "${OUT}" "$@" "${SKETCH_DIR}" )
  BIN="${OUT}/$(basename "${SKETCH_DIR}").ino.bin"
  [[ -f "${BIN}" ]] || { echo "no binary at ${BIN}" >&2; exit 1; }
  echo "OTA flashing ${BIN} -> http://${OTA_IP}/update"
  # -H 'Expect:' avoids the 100-continue some ESP servers dislike; board reboots
  # on success so the connection may close as it replies -- that's expected.
  curl -fsS -H 'Expect:' --max-time 180 \
    -F "firmware=@${BIN}" "http://${OTA_IP}/update" || true
  echo
  echo "uploaded; board reboots into the new firmware (re-joins WiFi in a few s)."
else
  ARGS=(compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}")
  [[ -n "${PORT}" ]] && ARGS+=(-u -p "${PORT}")
  ARGS+=("$@" "${SKETCH_DIR}")
  ( set -x; arduino-cli "${ARGS[@]}" )
fi
