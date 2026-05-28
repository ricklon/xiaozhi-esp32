#!/bin/bash
# Board Testing Workflow Script
# Usage: ./test-board.sh <board-name>

BOARD=$1

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
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "main/boards/$BOARD/$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
    fi
done

# Check for .cc file
CC_FILE=$(ls main/boards/$BOARD/*.cc 2>/dev/null | head -1)
if [ -n "$CC_FILE" ]; then
    echo "✅ CC file exists: $(basename $CC_FILE)"
else
    echo "❌ No .cc file found"
fi

# Step 3: Try to build for this board
echo ""
echo "Attempting to build..."
./switch-board.sh $BOARD build 2>&1 | tee /tmp/board-test-$BOARD.log

if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo ""
    echo "✅ Build successful for $BOARD"
    echo "   You can now flash with: ./switch-board.sh $BOARD flash"
else
    echo ""
    echo "❌ Build failed for $BOARD"
    echo "   Check /tmp/board-test-$BOARD.log for details"
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
