#!/usr/bin/env bash
# Build and optionally USB-flash Resonance Bridge OS for LilyGO T-Deck Plus.
#
#   ./build.sh                         # fresh retained build
#   ./build.sh --port COM152           # fresh build + explicit USB flash
#   ./build.sh --dev-cache             # fast, locked local iteration
#   ./build.sh --dev-cache --port COM152
#   ./build.sh --clean-dev-cache       # remove a healthy local cache
#   ./build.sh --recover-dev-cache     # quarantine an interrupted cache
#   ./build.sh --help                  # local-vs-retained build contract

set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
FIRMWARE_ROOT="$(cd "${SKETCH_DIR}/.." && pwd -P)"
ORIGINAL_ARGS=("$@")

# T-Deck Plus is ESP32-S3FN16R8: 16 MB QIO flash + 8 MB OPI PSRAM. A wrong
# PSRAM mode boot-loops, so these options are pinned here on purpose.
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
PORT=""
BUILD_PATH=""
DEV_CACHE=0
CLEAN_DEV_CACHE=0
RECOVER_DEV_CACHE=0
JOBS="${ARDUINO_JOBS:-}"
DEV_CACHE_WAIT_SECONDS="${RES_DEV_CACHE_WAIT_SECONDS:-600}"
DEV_CACHE_SCHEMA=1
DEV_CACHE_PATH="$SKETCH_DIR/build/dev-cache"
DEV_LOCK_PATH="$SKETCH_DIR/build/dev-cache.lock.d"
DEV_LOCK_OWNED=0

fail() {
  echo "$*" >&2
  exit 2
}

usage() {
  cat <<'EOF'
Usage: ./build.sh [options]

Recommended local iteration (compile only):
  ./build.sh --dev-cache

One explicitly named USB development target may add:
  --port PORT

The development cache is single-writer, recipe-pinned, fail-closed after an
interruption, and always reports tdeck-dev-local. Retained field artifacts must
omit --dev-cache and use a new --build-path or the fresh default build path.

Options:
  --dev-cache                 use the persistent single-writer local cache
  --jobs N                    Arduino job count (default unchanged if omitted)
  --clean-dev-cache           remove a healthy unlocked cache; use alone
  --recover-dev-cache         quarantine interrupted/stale state; use alone
  --build-path PATH           retain a fresh build at a new explicit path
  --port PORT                 upload the completed build over USB
  -h, --help                  show this contract without compiling
EOF
}

safe_generated_path() {
  case "$1" in
    "$DEV_CACHE_PATH"|"$DEV_LOCK_PATH"|"$SKETCH_DIR"/build/dev-cache.quarantine.*) return 0 ;;
    *) fail "refusing unsafe generated path: $1" ;;
  esac
}

lock_pid() {
  [[ -f "$DEV_LOCK_PATH/pid" ]] && tr -d '\r\n' < "$DEV_LOCK_PATH/pid"
}

lock_host() {
  [[ -f "$DEV_LOCK_PATH/hostname" ]] && tr -d '\r\n' < "$DEV_LOCK_PATH/hostname"
}

lock_owner_alive() {
  local pid host current_host
  pid="$(lock_pid || true)"
  host="$(lock_host || true)"
  current_host="$(hostname | tr -d '\r\n')"
  [[ "$pid" =~ ^[0-9]+$ && "$host" == "$current_host" ]] || return 2
  kill -0 "$pid" 2>/dev/null
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

ensure_no_build_tools() {
  local active
  active="$(active_build_tools)"
  [[ -z "$active" ]] || fail "build tools are still active; refusing cache mutation:\n$active"
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
    --build-path) BUILD_PATH="$2"; shift 2 ;;
    --dev-cache) DEV_CACHE=1; shift ;;
    --jobs) JOBS="$2"; shift 2 ;;
    --clean-dev-cache) CLEAN_DEV_CACHE=1; shift ;;
    --recover-dev-cache) RECOVER_DEV_CACHE=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) fail "unknown arg: $1" ;;
  esac
done

[[ "$JOBS" =~ ^[0-9]+$ || -z "$JOBS" ]] ||
  fail "bad --jobs/ARDUINO_JOBS: $JOBS (expected 0 or a positive integer)"
[[ "$DEV_CACHE_WAIT_SECONDS" =~ ^[0-9]+$ ]] ||
  fail "bad RES_DEV_CACHE_WAIT_SECONDS: $DEV_CACHE_WAIT_SECONDS"
(( DEV_CACHE_WAIT_SECONDS >= 1 )) || fail "RES_DEV_CACHE_WAIT_SECONDS must be >=1"
(( CLEAN_DEV_CACHE + RECOVER_DEV_CACHE <= 1 )) || fail "choose only one cache maintenance action"

if (( CLEAN_DEV_CACHE || RECOVER_DEV_CACHE )); then
  (( ${#ORIGINAL_ARGS[@]} == 1 )) || fail "cache maintenance actions must be used alone"
  if (( CLEAN_DEV_CACHE )); then clean_dev_cache; else recover_dev_cache; fi
  exit 0
fi

if (( DEV_CACHE )); then
  [[ -z "$BUILD_PATH" ]] || fail "--dev-cache cannot be combined with --build-path"
fi

# -I to the firmware root keeps packet.h as the one fleet wire contract. -I to
# the sketch plus LV_CONF_INCLUDE_SIMPLE lets both C and C++ LVGL sources find
# lv_conf.h. Translate paths for the Windows Arduino executable under Git Bash.
INC_ROOT="$FIRMWARE_ROOT"
INC_SKETCH="$SKETCH_DIR"
if command -v cygpath >/dev/null 2>&1; then
  INC_ROOT="$(cygpath -m "$FIRMWARE_ROOT")"
  INC_SKETCH="$(cygpath -m "$SKETCH_DIR")"
fi
FLAGS="-I${INC_ROOT} -I${INC_SKETCH} -DLV_CONF_INCLUDE_SIMPLE"
if (( DEV_CACHE )); then
  FLAGS+=" -DTDECK_DEV_BUILD=1"
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
  local libraries=(lvgl LovyanGFX)
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

if (( DEV_CACHE )); then
  prepare_dev_cache
elif [[ -z "$BUILD_PATH" ]]; then
  BUILD_PATH="$SKETCH_DIR/build/tdeck-bridge-$(date -u +%Y%m%dT%H%M%SZ)-$$"
  mkdir -p "$BUILD_PATH"
else
  if [[ -e "$BUILD_PATH" ]]; then
    [[ -d "$BUILD_PATH" ]] || fail "--build-path is not a directory: $BUILD_PATH"
    [[ -z "$(find "$BUILD_PATH" -mindepth 1 -maxdepth 1 -print -quit)" ]] ||
      fail "--build-path must be new or empty; never resume a prior build directory"
  fi
  mkdir -p "$BUILD_PATH"
fi

echo "FQBN: $FQBN"
echo "FLAGS: $FLAGS"
echo "BUILD_PATH: $BUILD_PATH"

COMPILE_ARGS=(
  compile
  --fqbn "$FQBN"
  --build-path "$BUILD_PATH"
  --build-property "compiler.cpp.extra_flags=$FLAGS"
  --build-property "compiler.c.extra_flags=$FLAGS"
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
arduino-cli "${COMPILE_ARGS[@]}" "$SKETCH_DIR"
COMPILE_RC=$?
set -e

if (( DEV_CACHE )); then
  if (( COMPILE_RC >= 128 )); then
    echo "DEV_CACHE_INTERRUPTED compiler_exit=$COMPILE_RC" >&2
  else
    # Normal compiler errors keep completed objects usable; signals and lost
    # wrappers leave this marker behind and require quarantine.
    rm -f -- "$DEV_CACHE_PATH/.build-in-progress"
  fi
fi
(( COMPILE_RC == 0 )) || exit "$COMPILE_RC"

BIN="$BUILD_PATH/tdeck_bridge.ino.bin"
if [[ ! -s "$BIN" || ! -s "$BUILD_PATH/build.options.json" ]]; then
  if (( DEV_CACHE )); then
    echo "DEV_CACHE_INTERRUPTED invalid-build-output" >&2
    printf 'invalid build output after successful compiler return\n' > "$DEV_CACHE_PATH/.build-in-progress"
  fi
  exit 1
fi

echo "artifact: $BIN ($(stat -c%s "$BIN") bytes)"
sha256sum "$BIN"

if [[ -n "$PORT" ]]; then
  arduino-cli upload --fqbn "$FQBN" --port "$PORT" \
    --build-path "$BUILD_PATH" "$SKETCH_DIR"
  echo "flashed $PORT; open at 115200 baud (type: help)"
fi
