#!/usr/bin/env bash
# Fetch the RX480E-4 3D model used by the docked visualization (make_rx_viz.py).
# Not vendored into this repo: the upstream project ships no license file, and
# the STEP itself is credited to Alejandro Hurtado (GrabCAD). Fetch it locally,
# use it for visualization, don't redistribute it.
set -euo pipefail
cd "$(dirname "$0")"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
git clone --depth 1 https://github.com/besi/kicad-radio-receiver "$TMP/rx" >/dev/null 2>&1
mkdir -p viz
cp "$TMP/rx/radio-receiver.step" viz/radio-receiver.step
echo "fetched viz/radio-receiver.step"
