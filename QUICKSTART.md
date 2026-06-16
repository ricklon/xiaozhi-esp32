# XiaoZhi ESP32 - Quick Start Guide

## Which Toolchain (and Who Installs It)

| Use case | Toolchain | Who installs it |
|----------|-----------|-----------------|
| Released / web-flasher firmware | `espressif/idf:v5.5.2` Docker image | **GitHub Actions** — automatic (`.github/workflows/build.yml`, `release-firmware.yml`). Nothing to install locally. |
| Local builds & flashing | ESP-IDF **v5.5.x** (min **5.4**) | **You**, once per machine (steps below). This bench uses **v5.5.4**. |

You only need a local ESP-IDF install if you build/flash yourself. If you just want
prebuilt firmware, use the [web flasher](README.md#firmware-flashing) — no toolchain required.

### Install ESP-IDF + chip toolchains (first time, Linux)

The supported XIAO boards span three chip targets, so install all three at once:

| Target | Toolchain pulled | Boards |
|--------|------------------|--------|
| `esp32c3` | `riscv32-esp-elf` | xiao-esp32-c3 |
| `esp32c6` | `riscv32-esp-elf` | xiao-esp32-c6, xiao-esp32-c6-eyes |
| `esp32s3` | `xtensa-esp-elf` | xiao-esp32-s3-sense, xiao-esp32-s3-eyes |

```bash
# 1. System prerequisites (Ubuntu/Debian)
sudo apt-get install -y git wget flex bison gperf python3 python3-venv python3-pip \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# 2. Clone ESP-IDF (v5.5.x; min 5.4). CI uses v5.5.2; this bench uses v5.5.4 — either works.
mkdir -p ~/esp && cd ~/esp
git clone -b v5.5.4 --recursive https://github.com/espressif/esp-idf.git

# 3. Install toolchains for every chip this project targets
cd ~/esp/esp-idf
./install.sh esp32c3,esp32c6,esp32s3

# 4. Activate in each new shell (puts idf.py on PATH)
. ~/esp/esp-idf/export.sh
```

After this, `./switch-board.sh <board> build` (preferred) or direct `idf.py` both work.

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
code ~/Projects/xiaozhi-esp32
```

### 3. Select Your Board

`switch-board.sh` sets the chip target automatically and keeps a per-board build
directory (`build-<board>/`). Use the full board name — run `./switch-board.sh list`
to see every board:

```bash
. ~/esp/esp-idf/export.sh        # source ESP-IDF (alias on this bench: get_idf)
./switch-board.sh xiao-esp32-s3-sense build
```

### 4. Configure Wi-Fi Credentials

Easiest: flash first, then use the `!wifi SSID PASSWORD` serial command (see the
[serial console commands](README.md#firmware-flashing)).

Or set it at build time via menuconfig, pointed at the board's build dir:

```bash
idf.py -B build-xiao-esp32-s3-sense menuconfig
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
# Build (auto-sets target, isolated build dir)
./switch-board.sh xiao-esp32-s3-sense build

# Build if needed, then flash (default port /dev/ttyACM0)
./switch-board.sh xiao-esp32-s3-sense flash

# Flash to an explicit port
./switch-board.sh xiao-esp32-s3-sense flash /dev/ttyACM1
```

Monitoring needs a TTY, so it isn't wrapped by `switch-board.sh` — use `idf.py` directly:

```bash
idf.py -B build-xiao-esp32-s3-sense -p /dev/ttyACM0 monitor
```

## For ESP32-C3 (Budget Option)

If you want to use the **ESP32-C3** instead:

```bash
./switch-board.sh xiao-esp32-c3 build
```

**Note:** C3 has less RAM, some features may be limited.

## Common Commands

| Action | Command |
|--------|---------|
| List boards & build state | `./switch-board.sh list` |
| Build | `./switch-board.sh <board> build` |
| Flash | `./switch-board.sh <board> flash [port]` |
| Monitor | `idf.py -B build-<board> -p /dev/ttyACM0 monitor` |
| Config (menuconfig) | `idf.py -B build-<board> menuconfig` |
| Show board config/state | `./switch-board.sh <board> status` |
| Clean board build | `./switch-board.sh <board> clean` |

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

## Troubleshooting

### Build Errors:
```bash
# Clean this board's build dir and rebuild (target is re-set automatically)
./switch-board.sh xiao-esp32-s3-sense clean
./switch-board.sh xiao-esp32-s3-sense build
```

### Flash Errors:
```bash
# Hold BOOT button while flashing
# Or try a lower baud rate (XIAO boards usually enumerate as /dev/ttyACM0)
idf.py -B build-xiao-esp32-s3-sense -p /dev/ttyACM0 -b 115200 flash
```

### Monitor Garbled Text:
```bash
# Check baud rate (should be 115200)
idf.py -B build-xiao-esp32-s3-sense -p /dev/ttyACM0 monitor
```

## Switch Between Boards

```bash
# Build the S3 Sense board
./switch-board.sh xiao-esp32-s3-sense build

# Build the C3 board
./switch-board.sh xiao-esp32-c3 build

# Show a board's config and build state
./switch-board.sh xiao-esp32-s3-sense status

# List every board and its build state
./switch-board.sh list
```

## Next Steps

1. ✅ Connect ESP32-S3
2. ✅ Run `./switch-board.sh xiao-esp32-s3-sense build`
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
