#!/usr/bin/env bash
# Build and install the RaCardinal DAW plugins (Linux only).
# Produces LV2 + VST3 + CLAP bundles in the user's plugin dirs.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CARDINAL_DIR="$ROOT/cardinal"

if [ ! -d "$CARDINAL_DIR" ]; then
    echo "error: $CARDINAL_DIR not found" >&2
    exit 1
fi

JOBS="${JOBS:-$(nproc)}"

echo "=== Building RaCardinal (${JOBS} jobs) ==="
make -C "$CARDINAL_DIR" all -j"$JOBS"

# Install dirs
LV2_DIR="${LV2_USER_DIR:-$HOME/.lv2}"
VST3_DIR="${VST3_USER_DIR:-$HOME/.vst3}"
CLAP_DIR="${CLAP_USER_DIR:-$HOME/.clap}"
mkdir -p "$LV2_DIR" "$VST3_DIR" "$CLAP_DIR"

echo ""
echo "=== Installing to user plugin dirs ==="

# LV2
rm -rf "$LV2_DIR/Cardinal.lv2" "$LV2_DIR/CardinalFX.lv2" "$LV2_DIR/CardinalSynth.lv2"
cp -r "$CARDINAL_DIR/bin/Cardinal.lv2"       "$LV2_DIR/"
cp -r "$CARDINAL_DIR/bin/CardinalFX.lv2"     "$LV2_DIR/"
cp -r "$CARDINAL_DIR/bin/CardinalSynth.lv2"  "$LV2_DIR/"
echo "LV2  -> $LV2_DIR"

# VST3 (bundle dirs; each variant is its own bundle)
rm -rf "$VST3_DIR/Cardinal.vst3" "$VST3_DIR/CardinalFX.vst3" "$VST3_DIR/CardinalSynth.vst3"
cp -r "$CARDINAL_DIR/bin/Cardinal.vst3"      "$VST3_DIR/"
cp -r "$CARDINAL_DIR/bin/CardinalFX.vst3"    "$VST3_DIR/"
cp -r "$CARDINAL_DIR/bin/CardinalSynth.vst3" "$VST3_DIR/"
echo "VST3 -> $VST3_DIR"

# CLAP (one bundle dir per variant, each carrying its own resources)
rm -rf "$CLAP_DIR/Cardinal.clap" "$CLAP_DIR/CardinalFX.clap" "$CLAP_DIR/CardinalSynth.clap"
cp -r "$CARDINAL_DIR/bin/Cardinal.clap" "$CLAP_DIR/Cardinal.clap"
rm -f "$CLAP_DIR/Cardinal.clap/CardinalFX.clap" "$CLAP_DIR/Cardinal.clap/CardinalSynth.clap"
mkdir -p "$CLAP_DIR/CardinalFX.clap" "$CLAP_DIR/CardinalSynth.clap"
cp "$CARDINAL_DIR/bin/Cardinal.clap/CardinalFX.clap"   "$CLAP_DIR/CardinalFX.clap/"
cp "$CARDINAL_DIR/bin/Cardinal.clap/CardinalSynth.clap" "$CLAP_DIR/CardinalSynth.clap/"
for d in "$CLAP_DIR/Cardinal.clap" "$CLAP_DIR/CardinalFX.clap" "$CLAP_DIR/CardinalSynth.clap"; do
    cp -r "$CARDINAL_DIR/bin/Cardinal.clap/resources" "$d/"
done
echo "CLAP -> $CLAP_DIR"

echo ""
echo "=== Done ==="
echo "Installed bundles:"
ls -1 "$LV2_DIR"/Cardinal*.lv2 "$VST3_DIR"/Cardinal*.vst3 "$CLAP_DIR"/Cardinal*.clap 2>/dev/null || true
echo "Restart your DAW to pick them up."
