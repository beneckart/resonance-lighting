#!/usr/bin/env bash
# Build/flash the production fixture firmware (PowerFeather V2 / ESP32-S3).
#
#   ./build.sh                          # compile only (throwaway build dir)
#   ./build.sh --port /dev/ttyACM0      # compile + USB flash
#   ./build.sh --ota 10.0.0.200         # compile + OTA via POST /update
#   ./build.sh --artifact-dir out/r1    # stable dir for fleet_usb_bringup.py
#   ./build.sh --fw-rev fx-YYMMDD-xxxxxxx-b  # reported artifact identity
#   ./build.sh --profile commission     # default NVS profile when unset
#   ./build.sh --channel 11             # ESP-NOW/AP channel build default
#   ./build.sh --wifi-source <header>    # replace local gitignored credentials
#   ./build.sh --chem 3v7               # bench-only Li-ion build (default lfp)
#   ./build.sh --precharge-ma 300        # BQ low-VBAT recovery limit (10..310)
#   ./build.sh --canopy-solenoid         # deprecated no-op; now fleet default
#   ./build.sh --solenoid-test           # targeted rev-2 manual-control bring-up
#   ./build.sh --basic-listener          # steady red 128 when no bridge command
#   ./build.sh --ota-fail-selftest      # P4 rollback drill image
#   ./build.sh --wdt-hangtest           # arm serial 'x' watchdog hang test
#
# Nearly everything that was a net_bench build flag is runtime NVS now
# (capacity C, charge cap G, class O, profile F, solenoid arm, channel);
# one inspected artifact serves the whole fleet (ADR 0009).

set -euo pipefail
cd "$(dirname "$0")"

FQBN="esp32:esp32:esp32s3_powerfeather"
PORT=""
OTA_IP=""
ARTIFACT_DIR=""
CHANNEL=""
PROFILE=""
CHEM="lfp"
EXTRA_FLAGS=""
WIFI_SOURCE=""
FW_REV=""
PRECHARGE_MA="300"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    --ota) OTA_IP="$2"; shift 2 ;;
    --artifact-dir) ARTIFACT_DIR="$2"; shift 2 ;;
    --fw-rev) FW_REV="$2"; shift 2 ;;
    --channel) CHANNEL="$2"; shift 2 ;;
    --profile) PROFILE="$2"; shift 2 ;;
    --wifi-source) WIFI_SOURCE="$2"; shift 2 ;;
    --chem) CHEM="$2"; shift 2 ;;
    --precharge-ma) PRECHARGE_MA="$2"; shift 2 ;;
    --canopy-solenoid)
      echo "NOTICE: --canopy-solenoid is deprecated; solenoid capability is now the fleet default"
      shift
      ;;
    --solenoid-test) EXTRA_FLAGS+=" -DRES_SOLENOID_FORCE_ENABLED=1 -DRES_SOLENOID_TEST_OVERRIDE=1"; shift ;;
    --ota-fail-selftest) EXTRA_FLAGS+=" -DRES_OTA_FAIL_SELFTEST=1"; shift ;;
    --wdt-hangtest) EXTRA_FLAGS+=" -DRES_WDT_HANGTEST=1"; shift ;;
    --basic-listener|--quiet-autonomy) EXTRA_FLAGS+=" -DRES_BASIC_LISTENER=1"; shift ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

[[ "$PRECHARGE_MA" =~ ^[0-9]+$ ]] || {
  echo "bad --precharge-ma: $PRECHARGE_MA (expected 10..310 in 10 mA steps)" >&2
  exit 2
}
(( PRECHARGE_MA >= 10 && PRECHARGE_MA <= 310 && PRECHARGE_MA % 10 == 0 )) || {
  echo "bad --precharge-ma: $PRECHARGE_MA (expected 10..310 in 10 mA steps)" >&2
  exit 2
}

# An explicit source replaces stale local credentials before compilation. This
# is mainly for one-time USB recovery onto the portable-router OTA path.
if [[ -n "$WIFI_SOURCE" ]]; then
  [[ -f "$WIFI_SOURCE" ]] || { echo "wifi source not found: $WIFI_SOURCE" >&2; exit 2; }
  cp "$WIFI_SOURCE" wifi_secrets.h
  echo "copied wifi_secrets.h from $WIFI_SOURCE"
fi

# wifi_secrets.h (gitignored): copy from a sibling sketch when missing.
if [[ ! -f wifi_secrets.h ]]; then
  for src in ../net_bench/wifi_secrets.h ../power_bench/wifi_secrets.h ../led_studio/wifi_secrets.h; do
    if [[ -f "$src" ]]; then
      cp "$src" wifi_secrets.h
      echo "copied wifi_secrets.h from $src"
      break
    fi
  done
fi
[[ -f wifi_secrets.h ]] || echo "WARNING: no wifi_secrets.h -- maintenance OTA will refuse to start"

# Shared solar-guard header lives one level up (firmware/); the sketch tree is
# copied into the build path, so it must arrive via the include search path.
# arduino-cli is a Windows executable on this bench even when this wrapper runs
# under Git Bash, so translate /c/... to C:/... before passing it to the compiler.
SHARED_INCLUDE="$(cd .. && pwd)"
if command -v cygpath >/dev/null 2>&1; then
  SHARED_INCLUDE="$(cygpath -m "$SHARED_INCLUDE")"
fi
FLAGS="-DPOWERFEATHER_BOARD_V2=1 -I$SHARED_INCLUDE"
FLAGS+=" -DRES_PF_PRECHARGE_MA=$PRECHARGE_MA"
case "$CHEM" in
  lfp) ;; # production default lives in board_power.cpp
  3v7) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_3V7" ;;
  *) echo "unknown --chem: $CHEM (lfp|3v7)" >&2; exit 2 ;;
esac
[[ -n "$CHANNEL" ]] && FLAGS+=" -DRES_CHANNEL=$CHANNEL"
if [[ -n "$FW_REV" ]]; then
  [[ "$FW_REV" =~ ^fx-[0-9]{6}-[0-9a-f]{7}-[pbt]$ ]] || {
    echo "bad --fw-rev: $FW_REV (expected fx-YYMMDD-recipe7-class)" >&2
    exit 2
  }
fi
case "$PROFILE" in
  "") ;;
  dev|commission)  FLAGS+=" -DRES_PROFILE_DEFAULT=PROFILE_DEV" ;;
  prod|field) FLAGS+=" -DRES_PROFILE_DEFAULT=PROFILE_PROD" ;;
  *) echo "unknown --profile: $PROFILE (commission|field; dev|prod aliases)" >&2; exit 2 ;;
esac
FLAGS+="$EXTRA_FLAGS"

# Unique build path per run unless an artifact dir was requested: parallel
# compiles against Arduino's shared sketch cache corrupt artifacts.
if [[ -n "$ARTIFACT_DIR" ]]; then
  BUILD_PATH="$ARTIFACT_DIR"
  mkdir -p "$BUILD_PATH"
else
  BUILD_PATH="$(mktemp -d /tmp/fixture-build.XXXXXX)"
  trap 'rm -rf "$BUILD_PATH"' EXIT
fi

# Arduino CLI's Windows command-line reconstruction does not reliably preserve
# an escaped C string passed through compiler.cpp.extra_flags. Generate the
# derived revision macro inside the unique build directory and force-include it
# instead; this also leaves the exact reported identity beside build.options.
if [[ -n "$FW_REV" ]]; then
  IDENTITY_HEADER="$BUILD_PATH/fixture_build_identity.h"
  printf '#pragma once\n#define RES_FIXTURE_VERSION "%s"\n' "$FW_REV" > "$IDENTITY_HEADER"
  IDENTITY_INCLUDE="$(cd "$(dirname "$IDENTITY_HEADER")" && pwd)/$(basename "$IDENTITY_HEADER")"
  if command -v cygpath >/dev/null 2>&1; then
    IDENTITY_INCLUDE="$(cygpath -m "$IDENTITY_INCLUDE")"
  fi
  FLAGS+=" -include$IDENTITY_INCLUDE"
fi

echo "compiling (flags:$FLAGS)"
arduino-cli compile --fqbn "$FQBN" \
  --build-property "compiler.cpp.extra_flags=$FLAGS" \
  --build-path "$BUILD_PATH" \
  .

BIN="$BUILD_PATH/fixture.ino.bin"
echo "artifact: $BIN ($(stat -c%s "$BIN") bytes)"
sha256sum "$BIN"

if [[ -n "$PORT" ]]; then
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" --build-path "$BUILD_PATH" .
elif [[ -n "$OTA_IP" ]]; then
  echo "OTA -> http://$OTA_IP/update"
  curl -sS -F "firmware=@$BIN" "http://$OTA_IP/update"
  echo
fi
