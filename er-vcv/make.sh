#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

RACK_DIR="${RACK_DIR:-../Rack-SDK}"

echo "=== Building plugin ==="
make RACK_DIR="$RACK_DIR"

echo ""
echo "=== Packaging & installing ==="
make install RACK_DIR="$RACK_DIR"

echo ""
echo "=== Done ==="
echo "Installed to: ~/.local/share/Rack2/plugins-lin-x64/"
echo "Restart Rack to load the updated plugin."
