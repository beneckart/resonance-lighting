#!/usr/bin/env bash
# Fast, compile-free checks for the T-Deck local-vs-retained build contract.
set -euo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
TDECK_DIR="$(cd "$TESTS_DIR/.." && pwd -P)"
cd "$TDECK_DIR"

fail() {
  echo "BUILD WRAPPER CONTRACT FAIL: $*" >&2
  exit 1
}

expect_rejected() {
  local expected="$1"
  shift
  local output rc
  set +e
  output="$(./build.sh "$@" 2>&1)"
  rc=$?
  set -e
  [[ $rc -eq 2 ]] || fail "expected rc=2 for $*; got $rc"
  grep -Fq -- "$expected" <<< "$output" ||
    fail "missing rejection '$expected' for $*"
}

bash -n build.sh
help="$(./build.sh --help)"
grep -Fq -- './build.sh --dev-cache' <<< "$help" ||
  fail "help omits the recommended local command"
grep -Fq -- 'tdeck-dev-local' <<< "$help" ||
  fail "help omits the mutable development identity"
grep -Fq -- 'Retained field artifacts must' <<< "$help" ||
  fail "help omits the retained-artifact boundary"

dev_identity="$(
  printf '#include "core/version.h"\nTDECK_FW_VERSION\n' |
    g++ -E -P -DTDECK_DEV_BUILD=1 -I"$TDECK_DIR/src" -x c++ -
)"
grep -Fq -- '"dev-local"' <<< "$dev_identity" ||
  fail "TDECK_DEV_BUILD does not select the dev-local identity"

expect_rejected '--dev-cache cannot be combined with --build-path' \
  --dev-cache --build-path build/contract-must-not-exist
expect_rejected 'bad --jobs/ARDUINO_JOBS' --dev-cache --jobs invalid
expect_rejected 'cache maintenance actions must be used alone' \
  --clean-dev-cache --port COM0
expect_rejected '--build-path must be new or empty' \
  --build-path tests

[[ ! -e build/contract-must-not-exist ]] ||
  fail "a rejected boundary check created an artifact directory"

# Reproduce Arduino's source-graph cleanup without invoking a real compiler.
# The fake first compile erases every entry in Arduino's build path, including
# dotfiles, then leaves one reusable library object. Wrapper state must survive
# outside that path so the second invocation is a real cache hit.
tmp_root="$(mktemp -d /tmp/tdeck-cache-contract.XXXXXX)"
trap 'rm -rf -- "$tmp_root"' EXIT
test_bridge="$tmp_root/firmware/tdeck_bridge"
fake_bin="$tmp_root/fake-bin"
mkdir -p "$test_bridge" "$fake_bin"
cp build.sh "$test_bridge/build.sh"
chmod +x "$test_bridge/build.sh"

cat > "$fake_bin/arduino-cli" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

case "${1:-}" in
  version)
    echo "arduino-cli Version: contract-test"
    ;;
  core)
    [[ "${2:-}" == "list" ]]
    printf 'ID Installed Latest Name\nesp32:esp32 3.3.0 3.3.0 esp32\n'
    ;;
  lib)
    [[ "${2:-}" == "list" ]]
    printf 'Name Installed Available Location Description\nlvgl 9.3.0 - user -\nLovyanGFX 1.2.7 - user -\n'
    ;;
  compile)
    shift
    build_path=""
    while (( $# )); do
      case "$1" in
        --build-path) build_path="$2"; shift 2 ;;
        *) shift ;;
      esac
    done
    [[ -n "$build_path" ]]
    mkdir -p "$build_path"
    count=0
    [[ ! -f "$FAKE_ARDUINO_STATE" ]] || count="$(<"$FAKE_ARDUINO_STATE")"
    count=$((count + 1))
    printf '%s\n' "$count" > "$FAKE_ARDUINO_STATE"
    if (( count == 1 )); then
      find "$build_path" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
      echo 'Compiling library "lvgl"'
      mkdir -p "$build_path/libraries/lvgl"
      printf 'cached object\n' > "$build_path/libraries/lvgl/cache.o"
    elif [[ -s "$build_path/libraries/lvgl/cache.o" ]]; then
      echo 'Using cached library "lvgl"'
    else
      echo 'Compiling library "lvgl"'
    fi
    printf 'fake binary\n' > "$build_path/tdeck_bridge.ino.bin"
    printf '{}\n' > "$build_path/build.options.json"
    ;;
  *)
    echo "unexpected fake arduino-cli command: $*" >&2
    exit 64
    ;;
esac
EOF
chmod +x "$fake_bin/arduino-cli"

export FAKE_ARDUINO_STATE="$tmp_root/compile-count"

# Schema migration must not silently resume a cache interrupted under schema 1.
mkdir -p "$test_bridge/build/dev-cache"
printf 'legacy interrupted build\n' > "$test_bridge/build/dev-cache/.build-in-progress"
set +e
legacy_output="$(PATH="$fake_bin:$PATH" "$test_bridge/build.sh" --dev-cache 2>&1)"
legacy_rc=$?
set -e
[[ $legacy_rc -eq 2 ]] ||
  fail "legacy interruption marker was not rejected"
grep -Fq -- 'DEV_CACHE_INTERRUPTED marker=' <<< "$legacy_output" ||
  fail "legacy interruption rejection omitted the marker"
grep -Fq -- 'recover-dev-cache' <<< "$legacy_output" ||
  fail "legacy interruption rejection omitted recovery guidance"
[[ ! -e "$test_bridge/build/dev-cache.lock.d" ]] ||
  fail "legacy interruption rejection left the lock owned"
PATH="$fake_bin:$PATH" "$test_bridge/build.sh" --recover-dev-cache >/dev/null

first_output="$(PATH="$fake_bin:$PATH" "$test_bridge/build.sh" --dev-cache 2>&1)"
grep -Fq -- 'DEV_CACHE_RESET reason=missing' <<< "$first_output" ||
  fail "first simulated build did not seed the cache"
[[ -s "$test_bridge/build/dev-cache.state/recipe.sha256" ]] ||
  fail "recipe hash was not stored outside Arduino's build path"
[[ -s "$test_bridge/build/dev-cache.state/recipe.txt" ]] ||
  fail "recipe detail was not stored outside Arduino's build path"
[[ ! -e "$test_bridge/build/dev-cache/.dev-cache-recipe.sha256" ]] ||
  fail "recipe hash leaked back into Arduino's disposable build path"

second_output="$(PATH="$fake_bin:$PATH" "$test_bridge/build.sh" --dev-cache 2>&1)"
grep -Fq -- 'DEV_CACHE_HIT' <<< "$second_output" ||
  fail "cache state did not survive simulated Arduino cleanup"
grep -Fq -- 'Using cached library "lvgl"' <<< "$second_output" ||
  fail "second simulated build did not reuse the library object"
if grep -Fq -- 'Compiling library "lvgl"' <<< "$second_output"; then
  fail "second simulated build rebuilt the library"
fi
[[ "$(<"$FAKE_ARDUINO_STATE")" == "2" ]] ||
  fail "simulated compile count was not two"
[[ ! -e "$test_bridge/build/dev-cache.state/build-in-progress" ]] ||
  fail "successful simulated build left an interruption marker"

PATH="$fake_bin:$PATH" "$test_bridge/build.sh" --clean-dev-cache >/dev/null
[[ ! -e "$test_bridge/build/dev-cache" && ! -e "$test_bridge/build/dev-cache.state" ]] ||
  fail "clean did not remove both Arduino cache and wrapper state"

echo "BUILD WRAPPER CONTRACT PASSED"
