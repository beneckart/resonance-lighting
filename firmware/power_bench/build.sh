#!/usr/bin/env bash
# Build (and optionally flash) the PowerFeather power-bench firmware.
#
# ALWAYS passes -DPOWERFEATHER_BOARD_V2=1 so the SDK selects the V2 MAX17260 fuel
# gauge (without it the SDK silently uses the V1 LC709204F and SOC/health/cycles
# fail). The sketch has a #error guard to enforce this.
#
# Usage:
#   ./build.sh [--led is31|neohex|rgbw1|none] [--cap MAH] [--chem 3v7|lfp]
#              [--charge-ma MA] [--no-charge] [--maintain V] [--port /dev/ttyACM0]
#              [-- <extra ardocl args>]
#
# Examples:
#   ./build.sh --led is31 --cap 4400 --port /dev/ttyACM0          # build + flash
#   ./build.sh --led neohex --cap 1500 --chem lfp --no-charge     # build only
set -euo pipefail

FQBN="esp32:esp32:esp32s3_powerfeather"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

LED=""; CAP=""; CHEM=""; CHARGE=""; CHARGE_MA=""; MAINTAIN=""; PORT=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --led) LED="$2"; shift 2;;
    --cap) CAP="$2"; shift 2;;
    --chem) CHEM="$2"; shift 2;;
    --charge-ma) CHARGE_MA="$2"; shift 2;;
    --no-charge) CHARGE="0"; shift;;
    --maintain) MAINTAIN="$2"; shift 2;;
    --port) PORT="$2"; shift 2;;
    --) shift; break;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

FLAGS="-DPOWERFEATHER_BOARD_V2=1"
case "${LED}" in
  is31)   FLAGS+=" -DRES_PF_LED_IS31=1";;
  neohex) FLAGS+=" -DRES_PF_LED_NEOHEX=1";;
  rgbw1)  FLAGS+=" -DRES_PF_LED_RGBW1=1";;
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

echo "flags: ${FLAGS}"
ARGS=(compile --fqbn "${FQBN}" --build-property "compiler.cpp.extra_flags=${FLAGS}")
[[ -n "${PORT}" ]] && ARGS+=(-u -p "${PORT}")
ARGS+=("$@" "${SKETCH_DIR}")

set -x
arduino-cli "${ARGS[@]}"
