#!/bin/bash
# Generates compile_commands.json for clangd LSP support.
#
# Usage:
#   ./compile_commands.sh
#   RACK_DIR=/path/to/Rack-SDK ./compile_commands.sh
#
# Regenerate after changing the SDK path or Makefile includes.
set -euo pipefail
cd "$(dirname "$0")/.."
RACK_DIR="${RACK_DIR:-../Rack-SDK}"

rm -rf build

bear -- make RACK_DIR="$RACK_DIR" 2>&1

mv compile_commands.json .
