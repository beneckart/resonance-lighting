#!/usr/bin/env bash
# Build/flash the production fixture firmware (PowerFeather V2 / ESP32-S3).
#
#   ./build.sh                          # compile only (throwaway build dir)
#   ./build.sh --port /dev/ttyACM0      # compile + USB flash
#   ./build.sh --ota 10.0.0.200         # compile + OTA via POST /update
#   ./build.sh --artifact-variant b --wifi-profile-label party-in-the-woods-v1
#                                       # immutable canary artifact + manifest
#   ./build.sh --profile commission     # default NVS profile when unset
#   ./build.sh --channel 11             # ESP-NOW/AP channel build default
#   ./build.sh --wifi-source <header>    # replace local gitignored credentials
#   ./build.sh --chem 3v7               # bench-only Li-ion build (default lfp)
#   ./build.sh --precharge-ma 300        # BQ low-VBAT recovery limit (10..310)
#   ./build.sh --deep-recovery-target F401DC  # target-locked low-VBAT test image
#   ./build.sh --msa-trace-target F2BE0C   # target-locked wind/ToF recorder
#   ./build.sh --presence-sentinel       # trace-only red -> RGB-white presence
#   ./build.sh --presence-distant-range  # trace-only raw 1000..<5000 mm presence
#   ./build.sh --sentinel-trace-target A1B2C3 # radio-off + VL53 power A/B/A
#   ./build.sh --canopy-solenoid         # deprecated no-op; now fleet default
#   ./build.sh --solenoid-test           # targeted rev-2 manual-control bring-up
#   ./build.sh --basic-listener          # class-aware listener when no command
#   ./build.sh --ota-fail-selftest      # P4 rollback drill image
#   ./build.sh --wdt-hangtest           # arm serial 'x' watchdog hang test
#   ./build.sh --dev-cache --jobs 0      # fast, locked local iteration
#   ./build.sh --clean-dev-cache         # remove a healthy local cache
#   ./build.sh --recover-dev-cache       # quarantine an interrupted cache
#   ./build.sh --help                    # local-vs-fleet build contract
#
# Nearly everything that was a net_bench build flag is runtime NVS now
# (capacity C, charge cap G, class O, profile F, solenoid arm, channel);
# one inspected artifact serves the whole fleet (ADR 0009).

set -euo pipefail
cd "$(dirname "$0")"

SKETCH_DIR="$(pwd -P)"
ORIGINAL_ARGS=("$@")

FQBN="esp32:esp32:esp32s3_powerfeather"
PORT=""
OTA_IP=""
ARTIFACT_DIR=""
ARTIFACT_VARIANT=""
WIFI_PROFILE_LABEL=""
CHANNEL=""
PROFILE=""
CHEM="lfp"
EXTRA_FLAGS=""
WIFI_SOURCE=""
FW_REV=""
PRECHARGE_MA="300"
DAY_SLEEP_S="300"
WAKE_LISTEN_MS="15000"
DEEP_RECOVERY_TARGET=""
MSA_TRACE_TARGET=""
PRESENCE_SENTINEL=0
PRESENCE_DISTANT_RANGE=0
SENTINEL_TRACE_TARGET=""
DEV_CACHE=0
CLEAN_DEV_CACHE=0
RECOVER_DEV_CACHE=0
JOBS="${ARDUINO_JOBS:-}"
DEV_CACHE_WAIT_SECONDS="${RES_DEV_CACHE_WAIT_SECONDS:-600}"
DEV_CACHE_SCHEMA=1
DEV_CACHE_PATH="$SKETCH_DIR/build/dev-cache"
DEV_LOCK_PATH="$SKETCH_DIR/build/dev-cache.lock.d"
DEV_LOCK_OWNED=0
TEMP_BUILD_PATH=""

fail() {
  echo "$*" >&2
  exit 2
}

usage() {
  cat <<'EOF'
Usage: ./build.sh [options]

Recommended local iteration (compile only):
  ./build.sh --dev-cache --profile commission --channel 11

One explicitly named USB development target may add:
  --port PORT

Shared/fleet artifacts must omit --dev-cache and use --artifact-variant. The
wrapper hashes the exact canonical recipe bytes, derives the revision/path,
and writes/verifies the manifest. Manual --artifact-dir/--fw-rev is refused.
The development cache always reports dev-local.

Development-cache options:
  --dev-cache                 use the persistent single-writer local cache
  --jobs N                    Arduino job count (default unchanged if omitted)
  --clean-dev-cache           remove a healthy unlocked cache; use alone
  --recover-dev-cache         quarantine interrupted/stale state; use alone

Common build options:
  --port PORT                 compile and upload over USB
  --ota IP                    fresh compile and HTTP OTA upload
  --artifact-variant p|b|t    create a fresh immutable artifact (ADR 0040)
  --wifi-profile-label LABEL  required non-secret credential-set label
  --profile commission|field  default profile when NVS is unset
  --channel N                 ESP-NOW/AP channel build default
  --wifi-source PATH          install a local gitignored credentials header
  --chem lfp|3v7              battery chemistry (production default: lfp)
  --precharge-ma N            BQ precharge limit, 10..310 in 10 mA steps
  --day-sleep-s N             field DAY_CHARGE timer sleep, 30..3600 s (default 300)
  --wake-listen-ms N          timer-wake listen grace, 1000..60000 ms (default 15000)
  --msa-trace-target MAC      exact-target MSA/TMF flight recorder; requires -t fw rev
  --presence-sentinel         exact trace target: red baseline -> RGB-white presence
  --presence-distant-range    sentinel uses any confident 1000..<5000 mm TMF zone
  --sentinel-trace-target MAC exact-target radio-off + perimeter-ToF A/B/A recorder
  -h, --help                  show this contract without compiling
EOF
}

safe_generated_path() {
  case "$1" in
    "$SKETCH_DIR/build/dev-cache"|"$SKETCH_DIR/build/dev-cache.lock.d"|"$SKETCH_DIR"/build/dev-cache.quarantine.*|/tmp/fixture-build.*) return 0 ;;
    *) fail "refusing unsafe generated path: $1" ;;
  esac
}

release_dev_lock() {
  if (( DEV_LOCK_OWNED )); then
    safe_generated_path "$DEV_LOCK_PATH"
    rm -rf -- "$DEV_LOCK_PATH"
    DEV_LOCK_OWNED=0
    echo "DEV_CACHE_RELEASED"
  fi
}

cleanup() {
  if [[ -n "$TEMP_BUILD_PATH" && -d "$TEMP_BUILD_PATH" ]]; then
    safe_generated_path "$TEMP_BUILD_PATH"
    rm -rf -- "$TEMP_BUILD_PATH"
  fi
  release_dev_lock
}

interrupted() {
  if (( DEV_LOCK_OWNED )); then
    echo "DEV_CACHE_INTERRUPTED signal=$1" >&2
  fi
  exit 130
}

trap cleanup EXIT
trap 'interrupted HUP' HUP
trap 'interrupted INT' INT
trap 'interrupted TERM' TERM

lock_pid() { [[ -f "$DEV_LOCK_PATH/pid" ]] && tr -d '\r\n' < "$DEV_LOCK_PATH/pid"; }
lock_host() { [[ -f "$DEV_LOCK_PATH/hostname" ]] && tr -d '\r\n' < "$DEV_LOCK_PATH/hostname"; }

lock_owner_alive() {
  local pid host current_host
  pid="$(lock_pid || true)"
  host="$(lock_host || true)"
  current_host="$(hostname | tr -d '\r\n')"
  [[ "$pid" =~ ^[0-9]+$ && "$host" == "$current_host" ]] || return 2
  kill -0 "$pid" 2>/dev/null
}

active_build_tools() {
  if command -v powershell.exe >/dev/null 2>&1; then
    powershell.exe -NoProfile -Command '
      Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $_.ProcessName -match "^(arduino-cli|xtensa-esp32s3-elf-(g\+\+|gcc|ld)|esptool)$" } |
        ForEach-Object { "{0} {1}" -f $_.Id,$_.ProcessName }
    ' 2>/dev/null | tr -d '\r'
  else
    ps -eo pid=,comm=,args= 2>/dev/null |
      grep -Ei '[a]rduino-cli|[x]tensa-esp32s3-elf-(g\+\+|gcc|ld)|[e]sptool' || true
  fi
}

acquire_dev_lock() {
  local start=$SECONDS waited=0 pid host
  mkdir -p "$SKETCH_DIR/build"
  while ! mkdir "$DEV_LOCK_PATH" 2>/dev/null; do
    [[ -d "$DEV_LOCK_PATH" ]] || continue
    if lock_owner_alive; then
      waited=$((SECONDS - start))
      if (( waited >= DEV_CACHE_WAIT_SECONDS )); then
        fail "DEV_CACHE_WAIT timeout=${DEV_CACHE_WAIT_SECONDS}s; cache still owned by pid $(lock_pid)"
      fi
      if (( waited == 0 || waited % 10 == 0 )); then
        echo "DEV_CACHE_WAIT owner_pid=$(lock_pid) elapsed=${waited}s"
      fi
      sleep 1
      continue
    fi
    pid="$(lock_pid || true)"
    host="$(lock_host || true)"
    echo "DEV_CACHE_INTERRUPTED stale-or-untrusted-lock pid=${pid:-unknown} host=${host:-unknown}" >&2
    fail "run ./build.sh --recover-dev-cache after confirming no compiler is active"
  done

  DEV_LOCK_OWNED=1
  printf '%s\n' "$$" > "$DEV_LOCK_PATH/pid"
  hostname > "$DEV_LOCK_PATH/hostname"
  date +%s > "$DEV_LOCK_PATH/started_epoch"
  printf '%q ' "$0" "${ORIGINAL_ARGS[@]}" > "$DEV_LOCK_PATH/command.txt"
  printf '\n' >> "$DEV_LOCK_PATH/command.txt"
  echo "DEV_CACHE_ACQUIRED pid=$$"

  if [[ -f "$DEV_CACHE_PATH/.build-in-progress" ]]; then
    echo "DEV_CACHE_INTERRUPTED marker=$DEV_CACHE_PATH/.build-in-progress" >&2
    fail "run ./build.sh --recover-dev-cache; interrupted caches are never resumed"
  fi
}

ensure_no_build_tools() {
  local active
  active="$(active_build_tools)"
  [[ -z "$active" ]] || fail "build tools are still active; refusing cache mutation:\n$active"
}

clean_dev_cache() {
  if [[ -d "$DEV_LOCK_PATH" ]]; then
    if lock_owner_alive; then
      fail "dev cache is active under pid $(lock_pid); not cleaning"
    fi
    fail "dev cache has a stale/untrusted lock; use --recover-dev-cache"
  fi
  if [[ -f "$DEV_CACHE_PATH/.build-in-progress" ]]; then
    fail "DEV_CACHE_INTERRUPTED marker present; use --recover-dev-cache"
  fi
  ensure_no_build_tools
  if [[ -d "$DEV_CACHE_PATH" ]]; then
    safe_generated_path "$DEV_CACHE_PATH"
    rm -rf -- "$DEV_CACHE_PATH"
  fi
  echo "DEV_CACHE_RESET reason=clean"
}

recover_dev_cache() {
  local quarantine stamp
  if [[ -d "$DEV_LOCK_PATH" ]] && lock_owner_alive; then
    fail "dev cache is active under pid $(lock_pid); not recovering"
  fi
  ensure_no_build_tools
  if [[ ! -e "$DEV_CACHE_PATH" && ! -e "$DEV_LOCK_PATH" ]]; then
    echo "DEV_CACHE_QUARANTINED none"
    return
  fi
  stamp="$(date -u +%Y%m%dT%H%M%SZ)-$$"
  quarantine="$SKETCH_DIR/build/dev-cache.quarantine.$stamp"
  safe_generated_path "$quarantine"
  mkdir -p "$quarantine"
  [[ ! -e "$DEV_CACHE_PATH" ]] || mv -- "$DEV_CACHE_PATH" "$quarantine/cache"
  [[ ! -e "$DEV_LOCK_PATH" ]] || mv -- "$DEV_LOCK_PATH" "$quarantine/lock"
  {
    echo "recovered_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "host=$(hostname)"
    echo "operator_pid=$$"
  } > "$quarantine/recovery.log"
  echo "DEV_CACHE_QUARANTINED path=$quarantine"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    --ota) OTA_IP="$2"; shift 2 ;;
    --artifact-dir) ARTIFACT_DIR="$2"; shift 2 ;;
    --artifact-variant) ARTIFACT_VARIANT="$2"; shift 2 ;;
    --wifi-profile-label) WIFI_PROFILE_LABEL="$2"; shift 2 ;;
    --fw-rev) FW_REV="$2"; shift 2 ;;
    --channel) CHANNEL="$2"; shift 2 ;;
    --profile) PROFILE="$2"; shift 2 ;;
    --wifi-source) WIFI_SOURCE="$2"; shift 2 ;;
    --chem) CHEM="$2"; shift 2 ;;
    --precharge-ma) PRECHARGE_MA="$2"; shift 2 ;;
    --day-sleep-s) DAY_SLEEP_S="$2"; shift 2 ;;
    --wake-listen-ms) WAKE_LISTEN_MS="$2"; shift 2 ;;
    --deep-recovery-target) DEEP_RECOVERY_TARGET="${2^^}"; shift 2 ;;
    --msa-trace-target) MSA_TRACE_TARGET="${2^^}"; shift 2 ;;
    --presence-sentinel) PRESENCE_SENTINEL=1; shift ;;
    --presence-distant-range) PRESENCE_DISTANT_RANGE=1; shift ;;
    --sentinel-trace-target) SENTINEL_TRACE_TARGET="${2^^}"; shift 2 ;;
    --dev-cache) DEV_CACHE=1; shift ;;
    --jobs) JOBS="$2"; shift 2 ;;
    --clean-dev-cache) CLEAN_DEV_CACHE=1; shift ;;
    --recover-dev-cache) RECOVER_DEV_CACHE=1; shift ;;
    -h|--help) usage; exit 0 ;;
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

[[ "$JOBS" =~ ^[0-9]+$ || -z "$JOBS" ]] || fail "bad --jobs/ARDUINO_JOBS: $JOBS (expected 0 or a positive integer)"
[[ "$DEV_CACHE_WAIT_SECONDS" =~ ^[0-9]+$ ]] || fail "bad RES_DEV_CACHE_WAIT_SECONDS: $DEV_CACHE_WAIT_SECONDS"
(( DEV_CACHE_WAIT_SECONDS >= 1 )) || fail "RES_DEV_CACHE_WAIT_SECONDS must be >=1"
(( CLEAN_DEV_CACHE + RECOVER_DEV_CACHE <= 1 )) || fail "choose only one cache maintenance action"
if (( CLEAN_DEV_CACHE || RECOVER_DEV_CACHE )); then
  (( ${#ORIGINAL_ARGS[@]} == 1 )) || fail "cache maintenance actions must be used alone"
  if (( CLEAN_DEV_CACHE )); then clean_dev_cache; else recover_dev_cache; fi
  exit 0
fi

if (( DEV_CACHE )); then
  [[ -z "$ARTIFACT_VARIANT" ]] || fail "--dev-cache cannot be combined with --artifact-variant"
  [[ -z "$OTA_IP" ]] || fail "--dev-cache cannot be combined with --ota"
fi

if [[ -n "$ARTIFACT_DIR" || -n "$FW_REV" ]]; then
  fail "manual --artifact-dir/--fw-rev is disabled; use --artifact-variant with --wifi-profile-label"
fi
if [[ -n "$ARTIFACT_VARIANT" ]]; then
  [[ "$ARTIFACT_VARIANT" =~ ^[pbt]$ ]] ||
    fail "bad --artifact-variant: $ARTIFACT_VARIANT (expected p|b|t)"
  [[ -n "$WIFI_PROFILE_LABEL" ]] ||
    fail "--artifact-variant requires --wifi-profile-label"
  [[ "$WIFI_PROFILE_LABEL" =~ ^[A-Za-z0-9._-]+$ ]] ||
    fail "bad --wifi-profile-label: use a non-secret ASCII label"
  [[ -n "$PROFILE" ]] || fail "--artifact-variant requires explicit --profile"
  [[ -n "$CHANNEL" ]] || fail "--artifact-variant requires explicit --channel"
  [[ -z "$PORT" && -z "$OTA_IP" ]] ||
    fail "artifact builds never flash directly; upload the retained artifact with exact-target tooling"
elif [[ -n "$WIFI_PROFILE_LABEL" ]]; then
  fail "--wifi-profile-label requires --artifact-variant"
fi

[[ "$PRECHARGE_MA" =~ ^[0-9]+$ ]] || {
  echo "bad --precharge-ma: $PRECHARGE_MA (expected 10..310 in 10 mA steps)" >&2
  exit 2
}
(( PRECHARGE_MA >= 10 && PRECHARGE_MA <= 310 && PRECHARGE_MA % 10 == 0 )) || {
  echo "bad --precharge-ma: $PRECHARGE_MA (expected 10..310 in 10 mA steps)" >&2
  exit 2
}
[[ "$DAY_SLEEP_S" =~ ^[0-9]+$ ]] || fail "bad --day-sleep-s: $DAY_SLEEP_S (expected 30..3600)"
(( DAY_SLEEP_S >= 30 && DAY_SLEEP_S <= 3600 )) || fail "bad --day-sleep-s: $DAY_SLEEP_S (expected 30..3600)"
[[ "$WAKE_LISTEN_MS" =~ ^[0-9]+$ ]] || fail "bad --wake-listen-ms: $WAKE_LISTEN_MS (expected 1000..60000)"
(( WAKE_LISTEN_MS >= 1000 && WAKE_LISTEN_MS <= 60000 )) || fail "bad --wake-listen-ms: $WAKE_LISTEN_MS (expected 1000..60000)"
if [[ -n "$CHANNEL" ]]; then
  [[ "$CHANNEL" =~ ^[0-9]+$ ]] || fail "bad --channel: $CHANNEL (expected 1..13)"
  (( CHANNEL >= 1 && CHANNEL <= 13 )) || fail "bad --channel: $CHANNEL (expected 1..13)"
fi
if [[ -n "$DEEP_RECOVERY_TARGET" ]]; then
  [[ "$DEEP_RECOVERY_TARGET" =~ ^[0-9A-F]{6}$ ]] || {
    echo "bad --deep-recovery-target: $DEEP_RECOVERY_TARGET (expected six hex digits)" >&2
    exit 2
  }
fi
if [[ -n "$MSA_TRACE_TARGET" ]]; then
  [[ "$MSA_TRACE_TARGET" =~ ^[0-9A-F]{6}$ ]] || {
    echo "bad --msa-trace-target: $MSA_TRACE_TARGET (expected six hex digits)" >&2
    exit 2
  }
  [[ -z "$DEEP_RECOVERY_TARGET" ]] ||
    fail "--msa-trace-target cannot be combined with --deep-recovery-target"
fi
if (( PRESENCE_SENTINEL )); then
  [[ -n "$MSA_TRACE_TARGET" ]] ||
    fail "--presence-sentinel requires --msa-trace-target"
fi
if (( PRESENCE_DISTANT_RANGE )); then
  [[ -n "$MSA_TRACE_TARGET" ]] ||
    fail "--presence-distant-range requires --msa-trace-target"
  (( PRESENCE_SENTINEL )) ||
    fail "--presence-distant-range requires --presence-sentinel"
fi
if [[ -n "$SENTINEL_TRACE_TARGET" ]]; then
  [[ "$SENTINEL_TRACE_TARGET" =~ ^[0-9A-F]{6}$ ]] || {
    echo "bad --sentinel-trace-target: $SENTINEL_TRACE_TARGET (expected six hex digits)" >&2
    exit 2
  }
  [[ -z "$DEEP_RECOVERY_TARGET" ]] ||
    fail "--sentinel-trace-target cannot be combined with --deep-recovery-target"
  [[ -z "$MSA_TRACE_TARGET" ]] ||
    fail "--sentinel-trace-target cannot be combined with --msa-trace-target"
fi

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
FLAGS+=" -DRES_DAY_SLEEP_S=$DAY_SLEEP_S -DRES_WAKE_LISTEN_MS=$WAKE_LISTEN_MS"
case "$CHEM" in
  lfp) ;; # production default lives in board_power.cpp
  3v7) FLAGS+=" -DRES_PF_BATTERY_TYPE=Mainboard::BatteryType::Generic_3V7" ;;
  *) echo "unknown --chem: $CHEM (lfp|3v7)" >&2; exit 2 ;;
esac
[[ -n "$CHANNEL" ]] && FLAGS+=" -DRES_CHANNEL=$CHANNEL"
if [[ -n "$DEEP_RECOVERY_TARGET" ]]; then
  [[ "$ARTIFACT_VARIANT" == "t" ]] || {
    echo "--deep-recovery-target requires --artifact-variant t" >&2
    exit 2
  }
  FLAGS+=" -DRES_DEEP_RECOVERY_TARGET=0x${DEEP_RECOVERY_TARGET}UL"
  FLAGS+=" -DRES_DEEP_RECOVERY_MAX_CHARGE_MA=100"
fi
if [[ -n "$MSA_TRACE_TARGET" ]]; then
  [[ "$ARTIFACT_VARIANT" == "t" ]] ||
    fail "--msa-trace-target requires --artifact-variant t"
  FLAGS+=" -DRES_MSA_TRACE_TARGET=0x${MSA_TRACE_TARGET}UL"
fi
if (( PRESENCE_SENTINEL )); then
  FLAGS+=" -DRES_CANOPY_PRESENCE_SENTINEL=1"
fi
if (( PRESENCE_DISTANT_RANGE )); then
  FLAGS+=" -DRES_CANOPY_PRESENCE_DISTANT_RANGE=1"
fi
if [[ -n "$SENTINEL_TRACE_TARGET" ]]; then
  [[ "$ARTIFACT_VARIANT" == "t" ]] ||
    fail "--sentinel-trace-target requires --artifact-variant t"
  FLAGS+=" -DRES_SENTINEL_TRACE_TARGET=0x${SENTINEL_TRACE_TARGET}UL"
fi
MANIFEST_PROFILE=""
case "$PROFILE" in
  "") ;;
  dev|commission)  FLAGS+=" -DRES_PROFILE_DEFAULT=PROFILE_DEV"; MANIFEST_PROFILE="commission" ;;
  prod|field) FLAGS+=" -DRES_PROFILE_DEFAULT=PROFILE_PROD"; MANIFEST_PROFILE="field" ;;
  *) echo "unknown --profile: $PROFILE (commission|field; dev|prod aliases)" >&2; exit 2 ;;
esac
FLAGS+="$EXTRA_FLAGS"

if (( DEV_CACHE )); then
  FLAGS+=" -DRES_DEV_BUILD=1"
fi

installed_library_version() {
  local name="$1" lines="$2" rest
  rest="$(printf '%s\n' "$lines" | awk -v name="$name" '
    index($0, name) == 1 {
      rest = substr($0, length(name) + 1)
      sub(/^[[:space:]]+/, "", rest)
      split(rest, fields, /[[:space:]]+/)
      print fields[1]
      exit
    }
  ')"
  printf '%s' "${rest:-missing}"
}

dev_recipe_content() {
  local cli_version core_version library_lines name key
  local libraries=(
    "Adafruit BusIO"
    "Adafruit MSA301"
    "Adafruit NeoPixel"
    "Adafruit Unified Sensor"
    "PowerFeather-SDK"
    "SparkFun Qwiic TMF882X Library"
  )
  cli_version="$(arduino-cli version | tr -d '\r')"
  core_version="$(arduino-cli core list | awk '$1 == "esp32:esp32" { print $2; exit }' | tr -d '\r')"
  library_lines="$(arduino-cli lib list | tr -d '\r')"
  printf 'schema=%s\n' "$DEV_CACHE_SCHEMA"
  printf 'sketch=%s\n' "$SKETCH_DIR"
  printf 'fqbn=%s\n' "$FQBN"
  printf 'flags=%s\n' "$FLAGS"
  printf 'arduino_cli=%s\n' "$cli_version"
  printf 'esp32_platform=%s\n' "${core_version:-missing}"
  for name in "${libraries[@]}"; do
    key="${name// /_}"
    printf 'library.%s=%s\n' "$key" "$(installed_library_version "$name" "$library_lines")"
  done
}

prepare_dev_cache() {
  local recipe_content recipe_sha old_sha reason
  acquire_dev_lock
  recipe_content="$(dev_recipe_content)"
  recipe_sha="$(printf '%s\n' "$recipe_content" | sha256sum | awk '{print $1}')"
  old_sha=""
  [[ ! -f "$DEV_CACHE_PATH/.dev-cache-recipe.sha256" ]] ||
    old_sha="$(tr -d '\r\n' < "$DEV_CACHE_PATH/.dev-cache-recipe.sha256")"

  if [[ -d "$DEV_CACHE_PATH" && "$old_sha" == "$recipe_sha" ]]; then
    echo "DEV_CACHE_HIT recipe=$recipe_sha"
  else
    reason="missing"
    [[ -z "$old_sha" ]] || reason="recipe-change old=$old_sha"
    if [[ -d "$DEV_CACHE_PATH" ]]; then
      safe_generated_path "$DEV_CACHE_PATH"
      rm -rf -- "$DEV_CACHE_PATH"
    fi
    mkdir -p "$DEV_CACHE_PATH"
    printf '%s\n' "$recipe_content" > "$DEV_CACHE_PATH/.dev-cache-recipe.txt"
    printf '%s\n' "$recipe_sha" > "$DEV_CACHE_PATH/.dev-cache-recipe.sha256"
    echo "DEV_CACHE_RESET reason=$reason new=$recipe_sha"
  fi
  BUILD_PATH="$DEV_CACHE_PATH"
  RECIPE_SHA="$recipe_sha"
}

# Unique build path per run unless an artifact dir was requested: parallel
# compiles against Arduino's shared sketch cache corrupt artifacts.
if (( DEV_CACHE )); then
  prepare_dev_cache
elif [[ -n "$ARTIFACT_VARIANT" ]]; then
  command -v python >/dev/null 2>&1 || fail "python is required for immutable artifact identity"
  REPO_ROOT="$(cd ../.. && pwd -P)"
  ARTIFACT_INFO="$(python artifact_recipe.py prepare \
    --repo-root "$REPO_ROOT" \
    --artifact-root "$SKETCH_DIR/build" \
    --variant "$ARTIFACT_VARIANT" \
    --flags "$FLAGS")"
  IFS=$'\t' read -r FW_REV BUILD_PATH RECIPE_SHA <<< "$ARTIFACT_INFO"
  [[ -n "$FW_REV" && -n "$BUILD_PATH" && -n "$RECIPE_SHA" ]] ||
    fail "artifact identity helper returned incomplete metadata"
  echo "ARTIFACT_PLAN fw_rev=$FW_REV recipe_sha256=$RECIPE_SHA path=$BUILD_PATH"
else
  BUILD_PATH="$(mktemp -d /tmp/fixture-build.XXXXXX)"
  TEMP_BUILD_PATH="$BUILD_PATH"
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
COMPILE_ARGS=(
  compile
  --fqbn "$FQBN"
  --build-property "compiler.cpp.extra_flags=$FLAGS"
  --build-path "$BUILD_PATH"
)
[[ -z "$JOBS" ]] || COMPILE_ARGS+=(--jobs "$JOBS")

if (( DEV_CACHE )); then
  {
    echo "pid=$$"
    echo "hostname=$(hostname)"
    echo "started_epoch=$(date +%s)"
    echo "recipe=$RECIPE_SHA"
  } > "$DEV_CACHE_PATH/.build-in-progress"
  if [[ -n "${RES_BUILD_TEST_PAUSE_AFTER_MARKER:-}" ]]; then
    [[ "$RES_BUILD_TEST_PAUSE_AFTER_MARKER" =~ ^[0-9]+$ ]] ||
      fail "RES_BUILD_TEST_PAUSE_AFTER_MARKER must be whole seconds"
    sleep "$RES_BUILD_TEST_PAUSE_AFTER_MARKER"
  fi
fi

set +e
arduino-cli "${COMPILE_ARGS[@]}" .
COMPILE_RC=$?
set -e

if (( DEV_CACHE )); then
  if (( COMPILE_RC >= 128 )); then
    echo "DEV_CACHE_INTERRUPTED compiler_exit=$COMPILE_RC" >&2
  else
    # Arduino returned normally. A regular source/compiler error does not make
    # already completed objects suspect; signals/timeouts leave the marker.
    rm -f -- "$DEV_CACHE_PATH/.build-in-progress"
  fi
fi
(( COMPILE_RC == 0 )) || exit "$COMPILE_RC"

BIN="$BUILD_PATH/fixture.ino.bin"
if [[ ! -s "$BIN" || ! -s "$BUILD_PATH/build.options.json" ]]; then
  if (( DEV_CACHE )); then
    echo "DEV_CACHE_INTERRUPTED invalid-build-output" >&2
    printf 'invalid build output after successful compiler return\n' > "$DEV_CACHE_PATH/.build-in-progress"
  fi
  exit 1
fi
echo "artifact: $BIN ($(stat -c%s "$BIN") bytes)"
sha256sum "$BIN"

if [[ -n "$ARTIFACT_VARIANT" ]]; then
  python artifact_recipe.py finalize \
    --repo-root "$REPO_ROOT" \
    --artifact-dir "$BUILD_PATH" \
    --channel-default "$CHANNEL" \
    --profile-default "$MANIFEST_PROFILE" \
    --wifi-profile "$WIFI_PROFILE_LABEL"
fi

if [[ -n "$PORT" ]]; then
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" --build-path "$BUILD_PATH" .
elif [[ -n "$OTA_IP" ]]; then
  echo "OTA -> http://$OTA_IP/update"
  curl -sS -F "firmware=@$BIN" "http://$OTA_IP/update"
  echo
fi
