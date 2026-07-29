#!/bin/bash
# Board Testing Workflow Script
# Usage: ./test-board.sh <board-name>

set -u

BOARD="${1:-}"

if [ -z "$BOARD" ]; then
    echo "Usage: ./test-board.sh <board-name>"
    echo "Example: ./test-board.sh xiao-esp32-c6"
    exit 1
fi

echo "=========================================="
echo "Testing Board: $BOARD"
echo "=========================================="

# Step 1: Check if board directory exists
if [ ! -d "main/boards/$BOARD" ]; then
    echo "❌ Board directory not found: main/boards/$BOARD"
    exit 1
fi
echo "✅ Board directory exists"

# Step 2: Check required files
echo ""
echo "Checking required files..."
REQUIRED_FILES=("config.h" "config.json" "sdkconfig.defaults")
missing_files=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "main/boards/$BOARD/$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
        missing_files=1
    fi
done

# Check for .cc file
CC_FILE=$(find "main/boards/$BOARD" -maxdepth 1 -type f -name '*.cc' -print -quit)
if [ -n "$CC_FILE" ]; then
    echo "✅ CC file exists: $(basename "$CC_FILE")"
else
    echo "❌ No .cc file found"
    missing_files=1
fi

if [ "$missing_files" -ne 0 ]; then
    echo "❌ Required board files are missing"
    exit 1
fi

# Step 3: Try to build for this board
echo ""
echo "Attempting to build..."
log_file="/tmp/board-test-${BOARD//\//-}.log"
./switch-board.sh "$BOARD" build 2>&1 | tee "$log_file"
build_status=${PIPESTATUS[0]}

if [ "$build_status" -eq 0 ]; then
    echo ""
    echo "✅ Build successful for $BOARD"
    echo "   You can now flash with: ./switch-board.sh $BOARD flash"
else
    echo ""
    echo "❌ Build failed for $BOARD"
    echo "   Check $log_file for details"
    exit "$build_status"
fi

echo ""
echo "=========================================="
echo "Manual Testing Checklist:"
echo "=========================================="
echo "[ ] Flash the firmware"
echo "[ ] Connect to serial monitor"
echo "[ ] Test WiFi connection"
echo "[ ] Test audio input (microphone)"
echo "[ ] Test audio output (speaker)"
echo "[ ] Test wake word detection"
echo "[ ] Test display (if present)"
echo "[ ] Update BOARDS_STATUS.md with results"
