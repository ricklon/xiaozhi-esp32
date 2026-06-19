# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses ESP-IDF v5.5.2 with `idf.py` (matches CI and release builds; minimum 5.4). Each board gets its own build directory so artifacts are never shared or lost when switching boards.

### switch-board.sh (preferred)

```bash
./switch-board.sh list                          # show all boards and build state
./switch-board.sh xiao-esp32-c6 build          # build (setup if first time)
./switch-board.sh xiao-esp32-c6 flash          # build if needed, then flash to /dev/ttyACM0
./switch-board.sh xiao-esp32-c6 flash /dev/ttyACM1  # explicit port
./switch-board.sh xiao-esp32-c6 status         # show sdkconfig key settings
./switch-board.sh xiao-esp32-c6 clean          # delete build-xiao-esp32-c6/
PORT=/dev/ttyUSB0 ./switch-board.sh xiao-esp32-c6 flash  # port via env var
```

Build directories are named `build-<board>/` (slashes in board paths become dashes, e.g. `waveshare/esp32-s3-touch-amoled-1.8` → `build-waveshare-esp32-s3-touch-amoled-1.8/`).

### Direct idf.py (when not using switch-board.sh)

```bash
# idf.py is not always on PATH — source first
. ~/esp/esp-idf/export.sh

idf.py -B build-xiao-esp32-c6 build
idf.py -B build-xiao-esp32-c6 -p /dev/ttyACM0 flash
```

### Serial monitoring (idf.py monitor requires a TTY)

```bash
python3 -c "
import serial, time, sys
s = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
start = time.time()
while time.time() - start < 30:
    line = s.readline().decode('utf-8', errors='replace').strip()
    if line: print(line, flush=True)
s.close()
"
```

### sdkconfig.defaults merge order (later wins)

1. `sdkconfig.defaults` — project-wide base
2. `sdkconfig.defaults.<target>` — target-level (e.g. `sdkconfig.defaults.esp32c6`)
3. `main/boards/<board>/sdkconfig.defaults` — board authority

`switch-board.sh` handles this merge automatically via `-DSDKCONFIG_DEFAULTS=`.

---

## Board Architecture

### Adding or modifying a board

Every board lives in `main/boards/<board>/` and requires:

| File | Purpose |
|------|---------|
| `config.h` | `#define` for all GPIO numbers and `WIFI_NETWORKS` |
| `config.json` | `target` chip, `builds` array with `sdkconfig_append` entries |
| `sdkconfig.defaults` | Board-specific Kconfig overrides |
| `<board>.cc` | C++ class inheriting `WifiBoard` (or `Board`) |

The `.cc` file must end with `DECLARE_BOARD(ClassName)` — this defines `create_board()` which the `Board` singleton factory calls.

### Board class hierarchy

```
Board (singleton, main/boards/common/board.h)
  └── WifiBoard (main/boards/common/wifi_board.h)
        └── <YourBoard> : public WifiBoard
```

Key virtual methods to implement: `GetAudioCodec()`, `GetDisplay()`. WiFi connection, NVS seeding of known networks, and OTA are handled by `WifiBoard`.

### NVS pre-seeding WiFi credentials

In the board constructor, iterate `WIFI_NETWORKS` and call `SsidManager::GetInstance().AddSsid()` before `Start()` is called. See `xiao-esp32-c6.cc` for the pattern.

---

## Audio Pipeline

### NoAudioCodecDuplex

Used for boards with external INMP441 mic + MAX98357A amp (no dedicated codec IC):

```cpp
#include "no_audio_codec.h"

auto codec = new NoAudioCodecDuplex(
    AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
    AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
    AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
```

### XIAO GPIO mapping — critical difference between C3 and C6

The pad numbers (D0–D3) map to **different GPIOs** on C3 vs C6:

| Pad | C3 GPIO | C6 GPIO |
|-----|---------|---------|
| D0  | GPIO2   | GPIO0   |
| D1  | GPIO3   | GPIO1   |
| D2  | GPIO4   | GPIO2   |
| D3  | GPIO5   | GPIO21  |

Using C3 values on a C6 board causes the INMP441 mic to read zero. Always check `main/boards/xiao-esp32-c6/config.h` for the authoritative C6 values.

---

## Application & State Machine

**`Application`** (singleton, `main/application.cc`) owns the main FreeRTOS event loop. Key methods callable from board code:
- `Application::GetInstance().SendTextChat(text)` — send text directly to LLM
- `Application::GetInstance().ToggleChatState()` — simulate button press

**`DeviceState`** enum (`main/device_state.h`): `Starting → Activating → Idle → Connecting → Listening → Speaking`. Transitions are enforced by `DeviceStateMachine`.

Boards do not manage state directly — they call `Application` APIs or post events.

---

## Serial Command Handler

`main/boards/common/xiao_serial_commands.h` provides a shared UART command interface. Boards launch it with:

```cpp
xTaskCreate(XiaoSerialInputTask, "serial_input", 4096, nullptr, 1, nullptr);
```

Commands at runtime (via serial terminal at 115200 baud):

| Command | Action |
|---------|--------|
| `!wifi SSID PASS` | Add WiFi network to NVS |
| `!wifi list` | List saved networks |
| `!wifi clear` | Remove all networks |
| `!server IP` | Set OTA URL to `http://IP:8003/xiaozhi/ota/` |
| `!server URL` | Set full OTA URL |
| `!status` | Firmware version, IP, OTA URL, free heap |
| `!camera` | Capture one camera frame (diagnostic) |
| `!mic [gain N\|mute\|unmute\|status]` | Test/configure the microphone |
| `!speaker [test\|vol 0-100\|status]` | Test the speaker — plays a built-in chime through the amp |
| `!stop` | Stop listening / close the active session |
| `!reboot` | Reboot device |
| `!help` | List all commands |
| Any other text | Sent to LLM via `SendTextChat()` |

On boards whose console is routed to USB-Serial-JTAG (e.g. the C6 — `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`), `XiaoSerialInputTask` installs the USB-Serial-JTAG VFS driver on startup so `fgetc(stdin)` actually receives host bytes; without it the commands are silently dead. No-op on UART0 consoles.

---

## Local Server

The companion server is **agent-hub** at `~/Projects/agent-hub` (separate repo) — a clean Python reimplementation that replaces the older `xiaozhi-esp32-server`. It serves:

- **Port 8000** — voice-session WebSocket (`/xiaozhi/v1/`) and vision/explain upload (`/xiaozhi/v1/image/`).
- **Port 8003** — OTA check-in (`/xiaozhi/ota/`, `/checkin/`) and the dashboard (`/dashboard/`).

Start the server:
```bash
cd ~/Projects/agent-hub
.venv/bin/python3 -m agent_hub.server   # or: just <recipe> — see agent-hub/justfile
```

### Driving a device's MCP tools without serial

agent-hub bridges device MCP tools, so you can exercise firmware through the dashboard once a device is connected. Device id is its MAC (e.g. `dc:da:0c:57:6f:94`):

```bash
DID=dc:da:0c:57:6f:94
curl -s http://127.0.0.1:8003/dashboard/agents/$DID/status    # MCP ready? tool list
curl -s -X POST http://127.0.0.1:8003/dashboard/agents/$DID/capture   # calls self.camera.take_photo over MCP
```

### Pointing a device at this server (verify, don't hardcode)

The server's address changes machine to machine — **never assume a fixed IP**. Derive
and verify it each time:

```bash
# 1. This host's LAN IP (the one on the same subnet as the device)
ip -4 addr show | grep -oP 'inet \K[\d.]+' | grep -v '^127\.'

# 2. Confirm agent-hub is actually listening before pointing a device at it
ss -ltn '( sport = :8000 or sport = :8003 )'   # both ports should be LISTEN

# 3. Sanity-check OTA reachability from this host (replace <ip> with step 1's value)
curl -fsS http://<ip>:8003/xiaozhi/ota/ >/dev/null && echo "OTA reachable"
```

Then point the device with `!server <ip>` (the IP from step 1, verified in steps 2–3).
The device must be on the **same subnet** as `<ip>` for it to connect.

---

## Git

Both `xiaozhi-esp32` and `xiaozhi-esp32-server` are forks owned by `ricklon`. In this clone, `origin` already points at the `ricklon/xiaozhi-esp32` fork (there is no separate `ricklon` remote), so push to `origin`:

```bash
git push origin main
```

Verify with `git remote -v` before pushing — if a clone instead has upstream as `origin` and the fork as a `ricklon` remote, push to `ricklon` instead.
