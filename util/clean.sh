#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../modules"

RACK_DIR="${RACK_DIR:-../Rack-SDK}"

echo "=== Cleaning plugin build artifacts ==="
make clean RACK_DIR="$RACK_DIR"

echo ""
echo "=== Done ==="
echo "Build artifacts removed (build/, plugin.so, etc.)."
echo "Sources and res/ are untouched."