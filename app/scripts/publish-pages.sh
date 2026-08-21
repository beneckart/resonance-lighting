#!/usr/bin/env bash
# Publish the lighting controller to its live link:
#   https://resonanceart.github.io/resonance-lighting/
# Builds base-relocated, excludes the unreferenced 21 MB tree.glb, force-pushes
# the gh-pages branch of our fork. Run from app/:  npm run publish:pages
set -euo pipefail
cd "$(dirname "$0")/.."
npx vite build --base=/resonance-lighting/
TMP=$(mktemp -d)
rsync -a --exclude tree.glb dist/ "$TMP/"
touch "$TMP/.nojekyll"
SHA=$(git rev-parse --short HEAD)
git -C "$TMP" init -q -b gh-pages
git -C "$TMP" add -A
git -C "$TMP" -c user.name=resonanceart -c user.email=resonanceartcollective@gmail.com \
  commit -q -m "pages: lighting controller @ $SHA"
git -C "$TMP" push -q -f https://github.com/resonanceart/resonance-lighting.git gh-pages:gh-pages
rm -rf "$TMP"
echo "published: https://resonanceart.github.io/resonance-lighting/ (@ $SHA)"
