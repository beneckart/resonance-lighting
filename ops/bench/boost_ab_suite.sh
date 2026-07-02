#!/usr/bin/env bash
# Boost A/B per-mount capture suite. Run once per hex mount (bare or boosted),
# then hot-swap and run again. Usage:
#   ./boost_ab_suite.sh <config> <runtag>     e.g. ./boost_ab_suite.sh bare r2
#
# Looks captured per mount (center anchor unless noted):
#   white-full   center px r=g=b=255 bri=255, 60 s   (the headline number)
#   red-full     center px r=255 only, 30 s          (per-channel color accuracy)
#   green-full   center px g=255 only, 30 s
#   blue-full    center px b=255 only, 30 s
#   ring1white-half  center+inner ring (7 px) white bri=128, 30 s (mild multi-px
#                    load -- starts probing the sag/goldening regime)
# Restores center white full at the end. W channel skipped: NeoHEX is RGB-only
# (verified dark 2026-07-02). The logger pokes a render before each capture, so
# post-swap blank-hex artifacts are covered.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CONFIG="${1:?usage: boost_ab_suite.sh <bare|boosted> <runtag>}"
TAG="${2:?usage: boost_ab_suite.sh <bare|boosted> <runtag>}"
STUDIO="${STUDIO:-http://ledstudio.local}"

set_look() { curl -s -m 8 "${STUDIO}/set?$1" > /dev/null; sleep 1.5; }
cap() { python3 boost_ab_log.py --label "${CONFIG}-$1-${TAG}" --duration "$2" \
        --notes "suite ${CONFIG}/${TAG}: $3"; }

echo "== suite ${CONFIG} ${TAG} =="
set_look "shape=0&r=255&g=255&b=255&w=0&bri=255"
cap white-full 60 "center px RGB white full"
set_look "r=255&g=0&b=0"
cap red-full 30 "center px red only"
set_look "r=0&g=255&b=0"
cap green-full 30 "center px green only"
set_look "r=0&g=0&b=255"
cap blue-full 30 "center px blue only"
set_look "shape=1&r=255&g=255&b=255&bri=128"
cap ring1white-half 30 "center+ring1 (7 px) white bri=128"
set_look "shape=0&r=255&g=255&b=255&bri=255"
echo "== done; look restored to center white full =="
