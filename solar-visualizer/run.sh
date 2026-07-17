#!/usr/bin/env bash
# Serve this folder locally so the ES-module viewer loads (file:// blocks modules).
cd "$(dirname "$0")"
echo "Open http://127.0.0.1:8777/Resonance_Solar_3D.html"
exec python3 -m http.server 8777 --bind 127.0.0.1
