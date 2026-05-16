#!/bin/bash
# XiaoZhi ESP32 Auto-Setup Script
# Automatically detects ESP32 and configures the project

echo "================================"
echo "XiaoZhi ESP32 Auto-Setup"
echo "================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Activate ESP-IDF
echo "Activating ESP-IDF..."
. ~/esp/esp-idf/export.sh > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to activate ESP-IDF${NC}"
    exit 1
fi
echo -e "${GREEN}✓ ESP-IDF activated${NC}"
echo ""

# Check for connected ESP32
echo "Detecting ESP32..."
PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1)

if [ -z "$PORT" ]; then
    echo -e "${RED}✗ No ESP32 detected!${NC}"
    echo ""
    echo "Please:"
    echo "  1. Connect your ESP32-S3 or ESP32-C3"
    echo "  2. Wait a few seconds"
    echo "  3. Run this script again"
    echo ""
    echo "To check manually:"
    echo "  ls /dev/ttyUSB* /dev/ttyACM*"
    exit 1
fi

echo -e "${GREEN}✓ Found ESP32 at: $PORT${NC}"
echo ""

# Try to detect chip type
echo "Identifying chip type..."
CHIP_INFO=$(python3 -m esptool --port $PORT chip_id 2>&1)

if echo "$CHIP_INFO" | grep -q "ESP32-S3"; then
    CHIP="esp32s3"
    echo -e "${GREEN}✓ Detected: ESP32-S3${NC}"
    echo ""
elif echo "$CHIP_INFO" | grep -q "ESP32-C3"; then
    CHIP="esp32c3"
    echo -e "${GREEN}✓ Detected: ESP32-C3${NC}"
    echo ""
elif echo "$CHIP_INFO" | grep -q "ESP32-C6"; then
    CHIP="esp32c6"
    echo -e "${GREEN}✓ Detected: ESP32-C6${NC}"
    echo ""
else
    echo -e "${YELLOW}? Could not auto-detect chip type${NC}"
    echo ""
    echo "Select your chip:"
    echo "  1) ESP32-S3 (recommended - more RAM)"
    echo "  2) ESP32-C3 (budget option)"
    echo "  3) ESP32-C6 (WiFi 6)"
    read -p "Enter choice [1-3]: " choice
    
    case $choice in
        1) CHIP="esp32s3" ;;
        2) CHIP="esp32c3" ;;
        3) CHIP="esp32c6" ;;
        *) echo "Invalid choice"; exit 1 ;;
    esac
fi

# Clean previous build if different target
if [ -f "build/config/sdkconfig.h" ]; then
    CURRENT_TARGET=$(grep "CONFIG_IDF_TARGET" build/config/sdkconfig.h 2>/dev/null | head -1 | cut -d'"' -f2)
    if [ "$CURRENT_TARGET" != "$CHIP" ]; then
        echo "Switching from $CURRENT_TARGET to $CHIP..."
        echo "Cleaning previous build..."
        idf.py fullclean > /dev/null 2>&1
    fi
fi

# Set target
echo "Configuring for $CHIP..."
idf.py set-target $CHIP

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Target configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Target set to $CHIP${NC}"
echo ""

# Check if Wi-Fi is configured
if [ -f "sdkconfig" ]; then
    WIFI_SSID=$(grep "CONFIG_WIFI_SSID" sdkconfig 2>/dev/null | cut -d'"' -f2)
    if [ -z "$WIFI_SSID" ] || [ "$WIFI_SSID" == "your-ssid" ]; then
        echo -e "${YELLOW}! Wi-Fi not configured${NC}"
        echo ""
        echo "Configure Wi-Fi:"
        echo "  idf.py menuconfig"
        echo ""
        read -p "Configure Wi-Fi now? [Y/n]: " configure_wifi
        if [[ $configure_wifi =~ ^[Yy]$ ]] || [ -z "$configure_wifi" ]; then
            idf.py menuconfig
        fi
    else
        echo -e "${GREEN}✓ Wi-Fi configured: $WIFI_SSID${NC}"
    fi
fi

echo ""
echo "================================"
echo -e "${GREEN}Setup Complete!${NC}"
echo "================================"
echo ""
echo "Next steps:"
echo "  1. Build:         idf.py build"
echo "  2. Flash:         idf.py flash"
echo "  3. Monitor:       idf.py monitor"
echo "  4. Or all-in-one: idf.py build flash monitor"
echo ""
echo "Or use VS Code:"
echo "  Ctrl+Shift+P -> 'Run Task' -> 'XiaoZhi: Full Deploy'"
echo ""
echo "Register device at: https://xiaozhi.me"
echo ""
