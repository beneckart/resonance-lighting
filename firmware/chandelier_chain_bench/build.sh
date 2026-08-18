#!/usr/bin/env bash
# Build/USB-flash the PowerFeather chandelier pixel-chain diagnostic.
set -euo pipefail
cd "$(dirname "$0")"

FQBN="esp32:esp32:esp32s3_powerfeather"
PORT=""
TYPE="rgbw"
MODE="diagnostic"
PIXELS="4"
BUDGET_MA="800"
BRIGHTNESS="64"
CAPACITY_MAH="6000"
BUILD_NAME=""
FW_REV=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    --type) TYPE="$2"; shift 2 ;;
    --mode) MODE="$2"; shift 2 ;;
    --pixels) PIXELS="$2"; shift 2 ;;
    --budget-ma) BUDGET_MA="$2"; shift 2 ;;
    --brightness) BRIGHTNESS="$2"; shift 2 ;;
    --cap) CAPACITY_MAH="$2"; shift 2 ;;
    --build-name) BUILD_NAME="$2"; shift 2 ;;
    --fw-rev) FW_REV="$2"; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

[[ "$PIXELS" =~ ^[0-9]+$ ]] && (( PIXELS >= 1 && PIXELS <= 24 )) || {
  echo "--pixels must be 1..24" >&2; exit 2;
}
[[ "$BUDGET_MA" =~ ^[0-9]+$ ]] && (( BUDGET_MA >= 100 && BUDGET_MA <= 900 )) || {
  echo "--budget-ma must be 100..900" >&2; exit 2;
}
[[ "$BRIGHTNESS" =~ ^[0-9]+$ ]] && (( BRIGHTNESS >= 1 && BRIGHTNESS <= 255 )) || {
  echo "--brightness must be 1..255" >&2; exit 2;
}
case "$TYPE" in
  rgbw) RGBW=1 ;;
  rgb) RGBW=0 ;;
  *) echo "--type must be rgbw or rgb" >&2; exit 2 ;;
esac
case "$MODE" in
  diagnostic) DEMO_MODE=0 ;;
  demo) DEMO_MODE=1 ;;
  *) echo "--mode must be diagnostic or demo" >&2; exit 2 ;;
esac
if [[ -n "$FW_REV" ]]; then
  [[ "$FW_REV" =~ ^fx-[0-9]{6}-[0-9a-f]{7}-[pbt]$ ]] || {
    echo "bad --fw-rev: $FW_REV (expected fx-YYMMDD-recipe7-class)" >&2
    exit 2
  }
fi
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  [[ -n "$FW_REV" && "$FW_REV" == *-t ]] || {
    echo "dirty source requires an explicit targeted-test (-t) --fw-rev" >&2
    exit 2
  }
fi

[[ -n "$BUILD_NAME" ]] || BUILD_NAME="chain-$(date -u +%Y%m%dT%H%M%SZ)-$$"
BUILD_PATH="build/$BUILD_NAME"
[[ ! -e "$BUILD_PATH" ]] || { echo "refusing existing build path: $BUILD_PATH" >&2; exit 2; }
mkdir -p "$BUILD_PATH"
FLAGS="-DPOWERFEATHER_BOARD_V2=1 -DCHAIN_PIXEL_RGBW=$RGBW -DCHAIN_DEMO_MODE=$DEMO_MODE -DCHAIN_START_PIXELS=$PIXELS -DCHAIN_BUDGET_MA=$BUDGET_MA -DCHAIN_START_BRIGHTNESS=$BRIGHTNESS -DCHAIN_BATTERY_MAH=$CAPACITY_MAH"
if [[ -n "$FW_REV" ]]; then
  IDENTITY_HEADER="$BUILD_PATH/chandelier_build_identity.h"
  printf '#pragma once\n#define CHAIN_VERSION "%s"\n' "$FW_REV" > "$IDENTITY_HEADER"
  IDENTITY_INCLUDE="$(cd "$(dirname "$IDENTITY_HEADER")" && pwd)/$(basename "$IDENTITY_HEADER")"
  if command -v cygpath >/dev/null 2>&1; then
    IDENTITY_INCLUDE="$(cygpath -m "$IDENTITY_INCLUDE")"
  fi
  FLAGS+=" -include$IDENTITY_INCLUDE"
fi

echo "compiling $TYPE $MODE chain: pixels=$PIXELS brightness=$BRIGHTNESS budget=${BUDGET_MA}mA cap=${CAPACITY_MAH}mAh fw=${FW_REV:-legacy}"
arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_PATH" \
  --build-property "compiler.cpp.extra_flags=$FLAGS" .
BIN="$BUILD_PATH/chandelier_chain_bench.ino.bin"
test -s "$BIN"
echo "artifact: $BIN ($(stat -c%s "$BIN") bytes)"
sha256sum "$BIN"

if [[ -n "$PORT" ]]; then
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" --build-path "$BUILD_PATH" .
fi
