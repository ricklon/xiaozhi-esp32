# XiaoZhi ESP32 - Quick Start Guide

## Your Setup

✅ **ESP-IDF v5.5.4** installed  
✅ **VS Code** configured  
✅ **ESP32-S3** and **ESP32-C3** boards available  

## Recommended: ESP32-S3

The **ESP32-S3** is the **better choice** for XiaoZhi because:
- More RAM (512KB vs 400KB) - critical for AI processing
- Better AI acceleration (vector instructions)
- Supports all features (display, audio, etc.)
- Faster wake-word detection

## Quick Start

### 1. Connect Your ESP32-S3

```bash
# Check if detected
lsusb | grep -iE "esp|silicon|ch340"
ls /dev/ttyUSB*
```

### 2. Open Project in VS Code

```bash
code ~/esp-projects/xiaozhi-esp32
```

### 3. Configure for ESP32-S3

**Option A: Use just**
```bash
just setup xiao-esp32-s3-sense
```

**Option B: Use VS Code Command Palette**
- Press `Ctrl+Shift+P`
- Type: "Run Task"
- Select: "XiaoZhi: Set Target ESP32-S3"

**Option C: Manual**
```bash
get_idf
idf.py set-target esp32s3
```

### 4. Configure Wi-Fi Credentials

```bash
idf.py menuconfig
```

Navigate to:
```
XiaoZhi AI Chatbot Configuration
  └── Wi-Fi Configuration
        ├── Wi-Fi SSID: [your-wifi-name]
        └── Wi-Fi Password: [your-wifi-password]
```

### 5. Build and Flash

```bash
# Build only
just build xiao-esp32-s3-sense

# Flash
just flash xiao-esp32-s3-sense /dev/ttyUSB0

# Monitor serial output
just monitor /dev/ttyUSB0

# Or use VS Code tasks:
# Ctrl+Shift+P -> "Run Task" -> "XiaoZhi: Full Deploy"
```

## For ESP32-C3 (Budget Option)

If you want to use the **ESP32-C3** instead:

```bash
just setup xiao-esp32-c3
```

Or in VS Code:
- `Ctrl+Shift+P` → "Run Task" → "XiaoZhi: Set Target ESP32-C3"

**Note:** C3 has less RAM, some features may be limited.

## Available VS Code Tasks

| Task | Command | Description |
|------|---------|-------------|
| Build | `Ctrl+Shift+B` | Compile the project |
| Flash | Task: Flash | Upload to ESP32 |
| Monitor | Task: Monitor | View serial output |
| Full Deploy | Task: Full Deploy | Build + Flash + Monitor |
| Menuconfig | Task: Menuconfig | SDK configuration |

## Hardware Requirements

### Minimum Setup:
- **ESP32-S3** DevKit
- **Microphone:** INMP441 or similar I2S MEMS mic
- **Speaker:** I2S DAC (MAX98357A) or PWM speaker
- **Wi-Fi:** 2.4GHz network

### Optional:
- **OLED Display:** SSD1306 or SH1106 (I2C)
- **Battery:** LiPo with charging circuit
- **Buttons:** Wake word, Reset

### Wiring — XIAO ESP32-C6

**IMPORTANT: C6 GPIO numbers differ from C3 — do not copy C3 wiring.**

| XIAO Pad | C3 GPIO | C6 GPIO | I2S Role       | INMP441 / MAX98357A |
|----------|---------|---------|----------------|---------------------|
| D0       | GPIO2   | GPIO0   | DOUT (speaker) | MAX98357A DIN       |
| D1       | GPIO3   | GPIO1   | BCLK           | BCLK / SCK          |
| D2       | GPIO4   | GPIO2   | WS / LRC       | WS / LRC            |
| D3       | GPIO5   | GPIO21  | DIN (mic)      | INMP441 SD          |

```
XIAO C6 Pad       INMP441 (Mic)
-----------       -------------
D3 (GPIO21) ----> SD
D2 (GPIO2)  ----> WS
D1 (GPIO1)  ----> SCK
3.3V        ----> VDD
GND         ----> GND, L/R (tie L/R to GND for left-channel)

XIAO C6 Pad       MAX98357A (Amp)
-----------       ---------------
D0 (GPIO0)  ----> DIN
D1 (GPIO1)  ----> BCLK
D2 (GPIO2)  ----> LRC
5V          ----> VIN
GND         ----> GND
```

### Wiring Example (ESP32-S3):

```
ESP32-S3          INMP441 (Mic)
--------          -------------
GPIO4  (WS)   --> WS
GPIO5  (SCK)  --> SCK
GPIO6  (SD)   --> SD
3.3V          --> VDD
GND           --> GND

ESP32-S3          MAX98357A (Audio Amp)
--------          --------------------
GPIO7  (DOUT) --> DIN
GPIO15 (BCLK) --> BCLK
GPIO16 (LRC)  --> LRC
5V            --> VIN
GND           --> GND

ESP32-S3          OLED Display (I2C)
--------          ------------------
GPIO8  (SDA)  --> SDA
GPIO9  (SCL)  --> SCL
3.3V          --> VCC
GND           --> GND
```

## Register Device

1. Flash the firmware
2. Connect to Wi-Fi
3. Device will show activation code on serial monitor
4. Go to https://xiaozhi.me
5. Register and add device with activation code
6. Free Qwen AI access for personal use!

## Use An Agent Hub Server

To use an agent-hub-compatible server while keeping the XiaoZhi protocol flow, configure the OTA/check-in URL to the permanent alias:

```text
!server http://HOST:8003/xiaozhi/ota/
```

If the server requires enrollment, set the optional enrollment token over serial:

```text
!hub-token ENROLLMENT_TOKEN
```

Check or clear it without printing the full secret:

```text
!hub-token
!hub-token clear
```

During check-in, the firmware sends the token as `X-Agent-Hub-Enrollment-Token`. The server should return `websocket.url` and `websocket.token`; the device stores both and uses `Authorization: Bearer <token>` when it opens `/xiaozhi/v1/`. Without a configured enrollment token, check-in remains compatible with unauthenticated LAN/dev servers.

## Troubleshooting

### Build Errors:
```bash
# Clean and rebuild
just clean xiao-esp32-s3-sense
just setup xiao-esp32-s3-sense
just build xiao-esp32-s3-sense
```

### Flash Errors:
```bash
# Hold BOOT button while flashing
# Or try lower baud rate
just flash xiao-esp32-s3-sense /dev/ttyUSB0
```

### Monitor Garbled Text:
```bash
# Check baud rate (should be 115200)
just monitor /dev/ttyUSB0
```

## Switch Between Boards

```bash
# Prepare S3
just setup xiao-esp32-s3-sense

# Prepare C3
just setup xiao-esp32-c3

# Check status
just status xiao-esp32-s3-sense
```

## Next Steps

1. ✅ Connect ESP32-S3
2. ✅ Run `just setup xiao-esp32-s3-sense`
3. ✅ Configure Wi-Fi in menuconfig
4. ✅ Build and flash
5. ✅ Register on xiaozhi.me
6. 🎉 Start chatting with AI!

## Documentation

- [Custom Board Guide](docs/custom-board.md)
- [MCP Protocol](docs/mcp-protocol.md)
- [WebSocket Protocol](docs/websocket.md)
- [Feishu Tutorial (Chinese)](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb)

## Support

- GitHub Issues: https://github.com/78/xiaozhi-esp32/issues
- Discord: https://discord.gg/C759fGMBcZ
- QQ Group: 994694848
