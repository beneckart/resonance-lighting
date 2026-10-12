#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [[ $# != 2 || "$1" != --build-path ]]; then
  echo "usage: bash build.sh --build-path <fresh directory>" >&2; exit 2
fi
BUILD_PATH="$2"
[[ ! -e "$BUILD_PATH" ]] || { echo 'build path must be fresh' >&2; exit 2; }
REV="atom-decomp-$(git rev-parse --short=7 HEAD)"
[[ -z "$(git diff HEAD -- . ../fixture/src/core/packet.h)" ]] || {
  echo 'commit source before immutable build' >&2; exit 2;
}
mkdir -p "$BUILD_PATH"
IDENTITY_HEADER="$(cd "$BUILD_PATH" && pwd)/atom_build_identity.h"
printf '#pragma once\n#define ATOM_DECOMPRESSION_VERSION "%s"\n' "$REV" > "$IDENTITY_HEADER"
if command -v cygpath >/dev/null 2>&1; then
  IDENTITY_HEADER="$(cygpath -m "$IDENTITY_HEADER")"
fi
FLAGS="-DNB_CHANNEL=11 -include$IDENTITY_HEADER"
arduino-cli compile --fqbn esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs \
  --build-path "$BUILD_PATH" --build-property "compiler.cpp.extra_flags=$FLAGS" .
python - "$BUILD_PATH" "$REV" <<'PY'
import hashlib,json,pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1]); b=p/'atom_decompression.ino.bin'
manifest=dict(fw_rev=sys.argv[2],git_commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
  fqbn='esp32:esp32:m5stack_atom:PartitionScheme=min_spiffs',channel=11,
  targets=['9F2738','9F0E54','9F26B0','F40254','9F26D8','9F26BC','9F0E5C','F2BE0C','9F2638'],
  binary=dict(file=b.name,bytes=b.stat().st_size,sha256=hashlib.sha256(b.read_bytes()).hexdigest()))
(p/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
PY
