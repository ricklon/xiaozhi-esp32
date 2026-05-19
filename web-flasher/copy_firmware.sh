#!/usr/bin/env bash
# Usage: ./copy_firmware.sh <board-id> [--assets]
#   board-id: directory name under firmware/ (e.g. c3, c6, s3, waveshare-s3-amoled18)
#   --assets: also copy generated_assets.bin (for boards with displays / enough flash)
#
# Run this after building each board target to populate the web flasher.
set -euo pipefail

BOARD=${1:-""}
WITH_ASSETS=false
if [[ "${2:-}" == "--assets" ]]; then
  WITH_ASSETS=true
fi

if [ -z "$BOARD" ]; then
  echo "Usage: $0 <board-id> [--assets]"
  echo "  board-id examples: c3  c6  s3  waveshare-s3-amoled18"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
DEST="$SCRIPT_DIR/firmware/$BOARD"

if [ ! -f "$BUILD_DIR/xiaozhi.bin" ]; then
  echo "ERROR: $BUILD_DIR/xiaozhi.bin not found. Run idf.py build first."
  exit 1
fi

CHIP=$(python3 -c "import json; d=json.load(open('$BUILD_DIR/flasher_args.json')); print(d['extra_esptool_args']['chip'])" 2>/dev/null || echo "unknown")
echo "Detected chip: $CHIP"

mkdir -p "$DEST"
cp "$BUILD_DIR/bootloader/bootloader.bin"           "$DEST/bootloader.bin"
cp "$BUILD_DIR/partition_table/partition-table.bin" "$DEST/partition-table.bin"
cp "$BUILD_DIR/ota_data_initial.bin"                "$DEST/ota_data_initial.bin"
cp "$BUILD_DIR/xiaozhi.bin"                         "$DEST/xiaozhi.bin"

if $WITH_ASSETS && [ -f "$BUILD_DIR/generated_assets.bin" ]; then
  cp "$BUILD_DIR/generated_assets.bin" "$DEST/generated_assets.bin"
  echo "Copied assets."
fi

echo "Done — firmware/$BOARD/ updated:"
ls -lh "$DEST"
