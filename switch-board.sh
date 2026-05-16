#!/bin/bash
# XiaoZhi ESP32 Board Switcher

show_help() {
    echo "XiaoZhi ESP32 Board Configuration Switcher"
    echo ""
    echo "Usage: $0 [s3|c3|status]"
    echo ""
    echo "Commands:"
    echo "  s3      - Configure for ESP32-S3 (recommended, more RAM)"
    echo "  c3      - Configure for ESP32-C3 (budget option)"
    echo "  status  - Show current configuration"
    echo ""
    echo "Examples:"
    echo "  $0 s3     # Switch to ESP32-S3"
    echo "  $0 c3     # Switch to ESP32-C3"
}

check_esp32_connected() {
    if ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep -q ""; then
        echo "✓ ESP32 detected!"
        return 0
    else
        echo "✗ No ESP32 connected"
        echo "  Connect your ESP32 and try again"
        return 1
    fi
}

switch_to_s3() {
    echo "Switching to ESP32-S3..."
    echo ""
    
    # Activate ESP-IDF
    . ~/esp/esp-idf/export.sh > /dev/null 2>&1
    
    # Clean previous build
    if [ -d "build" ]; then
        echo "Cleaning previous build..."
        idf.py fullclean > /dev/null 2>&1
    fi
    
    # Set target
    echo "Setting target to ESP32-S3..."
    idf.py set-target esp32s3
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✓ ESP32-S3 configuration complete!"
        echo ""
        echo "Next steps:"
        echo "  1. Configure Wi-Fi: idf.py menuconfig"
        echo "  2. Build: idf.py build"
        echo "  3. Flash: idf.py flash"
    else
        echo "✗ Configuration failed"
        exit 1
    fi
}

switch_to_c3() {
    echo "Switching to ESP32-C3..."
    echo ""
    
    # Activate ESP-IDF
    . ~/esp/esp-idf/export.sh > /dev/null 2>&1
    
    # Clean previous build
    if [ -d "build" ]; then
        echo "Cleaning previous build..."
        idf.py fullclean > /dev/null 2>&1
    fi
    
    # Set target
    echo "Setting target to ESP32-C3..."
    idf.py set-target esp32c3
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✓ ESP32-C3 configuration complete!"
        echo ""
        echo "Next steps:"
        echo "  1. Configure Wi-Fi: idf.py menuconfig"
        echo "  2. Build: idf.py build"
        echo "  3. Flash: idf.py flash"
    else
        echo "✗ Configuration failed"
        exit 1
    fi
}

show_status() {
    echo "Current XiaoZhi ESP32 Configuration"
    echo "===================================="
    echo ""
    
    # Check if in project directory
    if [ ! -f "CMakeLists.txt" ] || [ ! -d "main" ]; then
        echo "✗ Not in xiaozhi-esp32 project directory"
        echo "  Run from: ~/esp-projects/xiaozhi-esp32"
        exit 1
    fi
    
    # Check build directory
    if [ -d "build" ]; then
        if [ -f "build/config/sdkconfig.h" ]; then
            TARGET=$(grep "CONFIG_IDF_TARGET" build/config/sdkconfig.h 2>/dev/null | head -1 | cut -d'"' -f2)
            if [ -n "$TARGET" ]; then
                echo "✓ Current target: $TARGET"
            else
                echo "? Target not detected"
            fi
        else
            echo "? Build directory exists but no configuration"
        fi
    else
        echo "✗ No build directory (project not configured)"
    fi
    
    echo ""
    echo "ESP32 Connection:"
    if ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep -q ""; then
        ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | while read port; do
            echo "  ✓ $port"
        done
    else
        echo "  ✗ No ESP32 connected"
    fi
}

# Main
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

case "$1" in
    s3)
        switch_to_s3
        ;;
    c3)
        switch_to_c3
        ;;
    status)
        show_status
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
