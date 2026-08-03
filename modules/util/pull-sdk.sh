#!/usr/bin/env bash
set -euo pipefail

SDK_VERSION="${1:-2.6.2}"
RACK_SDK_DIR="../Rack-SDK"

# Platform detection
case "$(uname)" in
    Linux)  PLAT="lin-x64" ;;
    Darwin) PLAT="mac-x64" ;;
    *)      echo "Unsupported platform: $(uname)"; exit 1 ;;
esac

ZIP="Rack-SDK-${SDK_VERSION}-${PLAT}.zip"
URL="https://vcvrack.com/downloads/${ZIP}"

if [ -f "$RACK_SDK_DIR/plugin.mk" ]; then
    echo "SDK already exists at $RACK_SDK_DIR — nothing to do."
    echo "  Remove it first if you want to re-download."
    exit 0
fi

TMPDIR="$(mktemp -d)"
echo "Downloading ${URL}..."
curl -fSL -o "${TMPDIR}/${ZIP}" "$URL"

echo "Extracting..."
unzip -qo "${TMPDIR}/${ZIP}" -d "$TMPDIR"

# The zip contains a single directory like Rack-SDK-2.6.2-lin-x64/
SRC_DIR="$TMPDIR/Rack-SDK" 
if [ ! -d "$SRC_DIR" ]; then
    # Fallback: try the bare SDK directory name
    SRC_DIR="$TMPDIR/Rack-SDK-${SDK_VERSION}"
fi

if [ -d "$RACK_SDK_DIR" ]; then
    rm -rf "$RACK_SDK_DIR"
fi
mv "$SRC_DIR" "$RACK_SDK_DIR"

rm -rf "$TMPDIR"
echo "Rack SDK ${SDK_VERSION} installed at ${RACK_SDK_DIR}"
