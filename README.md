# XiaoZhi ESP32: English Developer Fork

(English | [中文](README_zh.md) | [日本語](README_ja.md))

## Introduction

This repository is an English-friendly ESP32 firmware fork focused on practical board bring-up, browser flashing, serial diagnostics, camera validation, and MCP-based device control.

XiaoZhi ESP32 turns small ESP32 boards into voice AI devices. It streams audio to a backend for ASR, LLM, and TTS, and exposes device capabilities through MCP tools so the backend or companion apps can control hardware features such as audio, display, camera, diagnostics, firmware updates, and board-specific actions.

<img src="docs/mcp-based-graph.jpg" alt="Control everything via MCP" width="320">

## What's New In This Fork

This fork adds a supported-board workflow for event demos, local testing, and English-speaking developers:

- **MCP tools for microcontrollers** — a step-by-step guide to exposing board features (mute, standby, LEDs, sensors) to the assistant as callable MCP tools, with a complete end-to-end standby example. See [Building MCP Tools For A Board](#building-mcp-tools-for-a-board).
- Browser-based web flasher for selected boards, with generated firmware manifests and GitHub Pages/release packaging support.
- `uv`-managed web flasher serving for Python tooling, matching Astral `uv` conventions.
- Web Serial console improvements that avoid toggling USB serial control signals, reducing unwanted ESP32 USB resets in Chrome.
- Playwright regression coverage for the serial console connection flow.
- Shared serial commands for supported boards: `!status`, `!server`, `!hub-token`, `!wifi`, `!camera`, `!reboot`, and `!help`.
- Camera diagnostics for the XIAO ESP32-S3 Sense, including a one-command capture check.
- MCP capability metadata and user-only diagnostic tools so companion software can discover board capabilities and run lightweight checks.
- Supported firmware packaging for XIAO ESP32-C3, XIAO ESP32-C6, XIAO ESP32-S3 Sense, and Waveshare ESP32-S3 Touch AMOLED 1.8.

## Supported Boards In This Fork

The upstream project supports many boards. This fork currently focuses release and web-flasher automation on:

| Board | Board ID | Notes |
|------|----------|-------|
| Seeed XIAO ESP32-C3 | `c3` | Budget I2S audio target; wake word disabled due resource limits. |
| Seeed XIAO ESP32-C6 | `c6` | XIAO form factor with corrected I2S GPIO mapping. |
| Seeed XIAO ESP32-S3 Sense | `s3` | OV2640 camera supported and verified with serial `!camera`. |
| Waveshare ESP32-S3 Touch AMOLED 1.8 | `waveshare-s3-amoled18` | AMOLED, touch, audio, wake word, Wi-Fi, and MCP screen/audio tools. |

Use `just` for local builds:

```bash
just build xiao-esp32-s3-sense
just flash xiao-esp32-s3-sense /dev/ttyACM0
```

For Waveshare:

```bash
just build waveshare/esp32-s3-touch-amoled-1.8
just flash waveshare/esp32-s3-touch-amoled-1.8 /dev/ttyACM0
```

## Version Notes

The current v2 version is incompatible with the v1 partition table, so it is not possible to upgrade from v1 to v2 via OTA. For partition table details, see [partitions/v2/README.md](partitions/v2/README.md).

All hardware running v1 can be upgraded to v2 by manually flashing the firmware.

The stable version of v1 is 1.9.2. You can switch to v1 by running `git checkout v1`. The v1 branch will be maintained until February 2026.

### Features Implemented

- Wi-Fi / ML307 Cat.1 4G
- Offline voice wake-up [ESP-SR](https://github.com/espressif/esp-sr)
- Supports two communication protocols ([Websocket](docs/websocket.md) or MQTT+UDP)
- Uses OPUS audio codec
- Voice interaction based on streaming ASR + LLM + TTS architecture
- Speaker recognition, identifies the current speaker [3D Speaker](https://github.com/modelscope/3D-Speaker)
- OLED / LCD display, supports emoji display
- Battery display and power management
- Multi-language support (Chinese, English, Japanese)
- Supports ESP32-C3, ESP32-S3, ESP32-P4 chip platforms
- Device-side MCP for device control (Speaker, LED, Servo, GPIO, etc.)
- Cloud-side MCP to extend large model capabilities (smart home control, PC desktop operation, knowledge search, email, etc.)
- Customizable wake words, fonts, emojis, and chat backgrounds with online web-based editing ([Custom Assets Generator](https://github.com/78/xiaozhi-assets-generator))
- Device capability reporting through MCP initialization metadata
- User-only MCP diagnostics for system info, device status, speaker, display, and camera checks where supported
- Browser web flasher and serial console for the supported board set in this fork

## Hardware

### Breadboard DIY Practice

See the Feishu document tutorial:

👉 ["XiaoZhi AI Chatbot Encyclopedia"](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb?from=from_copylink)

Breadboard demo:

![Breadboard Demo](docs/v1/wiring2.jpg)

### Supports 70+ Open Source Hardware (Partial List)

- <a href="https://oshwhub.com/li-chuang-kai-fa-ban/li-chuang-shi-zhan-pai-esp32-s3-kai-fa-ban" target="_blank" title="LiChuang ESP32-S3 Development Board">LiChuang ESP32-S3 Development Board</a>
- <a href="https://github.com/espressif/esp-box" target="_blank" title="Espressif ESP32-S3-BOX3">Espressif ESP32-S3-BOX3</a>
- <a href="https://docs.m5stack.com/zh_CN/core/CoreS3" target="_blank" title="M5Stack CoreS3">M5Stack CoreS3</a>
- <a href="https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base" target="_blank" title="AtomS3R + Echo Base">M5Stack AtomS3R + Echo Base</a>
- <a href="https://gf.bilibili.com/item/detail/1108782064" target="_blank" title="Magic Button 2.4">Magic Button 2.4</a>
- <a href="https://www.waveshare.net/shop/ESP32-S3-Touch-AMOLED-1.8.htm" target="_blank" title="Waveshare ESP32-S3-Touch-AMOLED-1.8">Waveshare ESP32-S3-Touch-AMOLED-1.8</a>
- <a href="https://github.com/Xinyuan-LilyGO/T-Circle-S3" target="_blank" title="LILYGO T-Circle-S3">LILYGO T-Circle-S3</a>
- <a href="https://oshwhub.com/tenclass01/xmini_c3" target="_blank" title="XiaGe Mini C3">XiaGe Mini C3</a>
- <a href="https://oshwhub.com/movecall/cuican-ai-pendant-lights-up-y" target="_blank" title="Movecall CuiCan ESP32S3">CuiCan AI Pendant</a>
- <a href="https://github.com/WMnologo/xingzhi-ai" target="_blank" title="WMnologo-Xingzhi-1.54">WMnologo-Xingzhi-1.54TFT</a>
- <a href="https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html" target="_blank" title="SenseCAP Watcher">SenseCAP Watcher</a>
- <a href="https://www.bilibili.com/video/BV1BHJtz6E2S/" target="_blank" title="ESP-HI Low Cost Robot Dog">ESP-HI Low Cost Robot Dog</a>

<div style="display: flex; justify-content: space-between;">
  <a href="docs/v1/lichuang-s3.jpg" target="_blank" title="LiChuang ESP32-S3 Development Board">
    <img src="docs/v1/lichuang-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/espbox3.jpg" target="_blank" title="Espressif ESP32-S3-BOX3">
    <img src="docs/v1/espbox3.jpg" width="240" />
  </a>
  <a href="docs/v1/m5cores3.jpg" target="_blank" title="M5Stack CoreS3">
    <img src="docs/v1/m5cores3.jpg" width="240" />
  </a>
  <a href="docs/v1/atoms3r.jpg" target="_blank" title="AtomS3R + Echo Base">
    <img src="docs/v1/atoms3r.jpg" width="240" />
  </a>
  <a href="docs/v1/magiclick.jpg" target="_blank" title="Magic Button 2.4">
    <img src="docs/v1/magiclick.jpg" width="240" />
  </a>
  <a href="docs/v1/waveshare.jpg" target="_blank" title="Waveshare ESP32-S3-Touch-AMOLED-1.8">
    <img src="docs/v1/waveshare.jpg" width="240" />
  </a>
  <a href="docs/v1/lilygo-t-circle-s3.jpg" target="_blank" title="LILYGO T-Circle-S3">
    <img src="docs/v1/lilygo-t-circle-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/xmini-c3.jpg" target="_blank" title="XiaGe Mini C3">
    <img src="docs/v1/xmini-c3.jpg" width="240" />
  </a>
  <a href="docs/v1/movecall-cuican-esp32s3.jpg" target="_blank" title="CuiCan">
    <img src="docs/v1/movecall-cuican-esp32s3.jpg" width="240" />
  </a>
  <a href="docs/v1/wmnologo_xingzhi_1.54.jpg" target="_blank" title="WMnologo-Xingzhi-1.54">
    <img src="docs/v1/wmnologo_xingzhi_1.54.jpg" width="240" />
  </a>
  <a href="docs/v1/sensecap_watcher.jpg" target="_blank" title="SenseCAP Watcher">
    <img src="docs/v1/sensecap_watcher.jpg" width="240" />
  </a>
  <a href="docs/v1/esp-hi.jpg" target="_blank" title="ESP-HI Low Cost Robot Dog">
    <img src="docs/v1/esp-hi.jpg" width="240" />
  </a>
</div>

## Software

### Firmware Flashing

For beginners, use the browser web flasher when prebuilt firmware is available. It avoids a local ESP-IDF setup and provides a Web Serial console for configuration and diagnostics.

Local web flasher development:

```bash
cd web-flasher
uv run python serve.py
```

Then open the local URL printed by the server in Chrome or another browser with Web Serial support.

The serial console supports these commands:

```text
!status              show firmware, board, Wi-Fi, IP, OTA URL, hub token status, heap, camera
!server IP           set OTA/server URL to http://IP:8003/xiaozhi/ota/ and reboot
!server URL          set a full OTA/server URL and reboot
!hub-token TOKEN     set an optional agent-hub enrollment token
!hub-token           show whether an agent-hub enrollment token is configured
!hub-token clear     remove the agent-hub enrollment token
!wifi SSID PASSWORD  add a saved Wi-Fi network
!wifi list           list saved Wi-Fi networks
!wifi clear          remove saved Wi-Fi networks
!camera              capture one camera frame when the board has a camera
!reboot              reboot the device
!help                show command help
```

The firmware connects to the official [xiaozhi.me](https://xiaozhi.me) server by default unless you configure a local/self-hosted server. Personal users can register an account to use the Qwen real-time model for free.

For an agent-hub server, point the OTA/check-in URL at the permanent XiaoZhi-compatible alias:

```text
!server http://HOST:8003/xiaozhi/ota/
```

If the server requires enrollment, configure the optional enrollment token:

```text
!hub-token ENROLLMENT_TOKEN
```

On OTA check-in, the device sends the token as `X-Agent-Hub-Enrollment-Token`. The server should return the normal `websocket` object with `url` and `token` fields. The firmware already persists string fields from that object into the `websocket` NVS namespace; when opening `/xiaozhi/v1/`, it reads `websocket.token` and sends `Authorization: Bearer <token>`. If no enrollment token is configured, the check-in request is unchanged for unauthenticated LAN/dev servers.

👉 [Beginner's Firmware Flashing Guide](https://ccnphfhqs21z.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS)

### Development Environment

- Cursor or VSCode
- Install ESP-IDF plugin, select SDK version 5.4 or above
- ESP-IDF v5.5.x is used for current local validation
- Python web tooling uses [`uv`](https://docs.astral.sh/uv/) from Astral
- Linux is better than Windows for faster compilation and fewer driver issues
- This project uses Google C++ code style, please ensure compliance when submitting code

Common local commands:

```bash
# List supported release/web-flasher boards
python3 scripts/supported_boards.py

# Build a supported board with isolated per-board build output
just build xiao-esp32-c3

# Flash a board
just flash xiao-esp32-c3 /dev/ttyACM0

# Run the web serial regression test
cd web-flasher
npm test
```

### Developer Documentation

- [Custom Board Guide](docs/custom-board.md) - Learn how to create custom boards for XiaoZhi AI
- [MCP Protocol IoT Control Usage](docs/mcp-usage.md) - Learn how to control IoT devices via MCP protocol
- [MCP Protocol Interaction Flow](docs/mcp-protocol.md) - Device-side MCP protocol implementation
- [MQTT + UDP Hybrid Communication Protocol Document](docs/mqtt-udp.md)
- [A detailed WebSocket communication protocol document](docs/websocket.md)
- [Quick Start Guide](QUICKSTART.md) - Local setup notes for ESP32-S3/C3 development
- [LLM Connection Guide](LLM_CONNECTION_GUIDE.md) - Official, self-hosted, and custom backend options

## MCP And Diagnostics

MCP messages are carried over the project WebSocket or MQTT transport as `type: "mcp"` messages with JSON-RPC 2.0 payloads. The device acts as an MCP server. The backend initializes the session, lists tools with `tools/list`, and invokes device functions with `tools/call`.

This fork adds richer capability metadata to the MCP `initialize` response and exposes user-only diagnostic tools:

- `self.get_system_info`
- `self.diagnostics.get_checks`
- `self.diagnostics.run_check`
- `self.reboot`
- `self.upgrade_firmware`
- screen snapshot/preview tools on LVGL display boards
- camera capture tools on camera boards

Use regular tools for model-callable actions and user-only tools for companion-app or explicit user actions. See [docs/mcp-usage.md](docs/mcp-usage.md) and [docs/mcp-protocol.md](docs/mcp-protocol.md).

### Building MCP Tools For A Board

An "MCP feature" is any board capability you expose to the assistant (or a companion app) as a callable function — set volume, toggle mute, set LED color, enter standby, read a sensor, and so on. This is **C++**, not a C library: the framework is a small set of header classes in [`main/mcp_server.h`](main/mcp_server.h) (`McpServer`, `McpTool`, `Property`, `PropertyList`). cJSON (a C library) is used internally only to serialize the JSON-RPC payloads — you never touch it for a normal tool.

#### MCP tools vs. physical buttons

These are different layers and are easy to confuse:

- A **physical button** (standby, mute, boot) is wired in the board's `.cc` via the `Button` class and usually calls an `Application` API like `ToggleChatState()`. It only works when a human presses it.
- An **MCP tool** exposes the *same capability* to the LLM and to companion apps over the network, so the model can act on it ("mute yourself", "go to standby").

A good pattern for a feature like mute or standby is to write one handler method and wire **both** the button callback and the MCP tool to it.

#### Identifying candidate features

Anything stateful or controllable on the board is a candidate. The recurring shape is a **getter + setter** pair (read current state, change it) and persisting the setting with the `Settings` helper so it survives reboot. Existing in-tree examples to copy:

| Feature | Where | Tool names |
|---------|-------|-----------|
| Press-to-talk vs click-to-talk mode | [`main/boards/common/press_to_talk_mcp_tool.cc`](main/boards/common/press_to_talk_mcp_tool.cc) | `self.set_press_to_talk` |
| RGB LED strip | [`main/boards/df-k10/led_control.cc`](main/boards/df-k10/led_control.cc) | `self.led_strip.get_brightness`, `self.led_strip.set_brightness`, `self.led_strip.set_all_color`, … |
| Volume / screen / camera | [`main/mcp_server.cc`](main/mcp_server.cc) `AddCommonTools()` | `self.audio_speaker.set_volume`, `self.screen.set_brightness`, `self.camera.take_photo` |

#### Where tools are registered

- **Common tools** shared by all boards (volume, brightness, theme, camera, device status) live in `McpServer::AddCommonTools()` in `main/mcp_server.cc`. Do **not** add board-specific tools here.
- **Board-specific tools** go in an `InitializeTools()` method on your board class, called from the board constructor. `Application` later calls `AddCommonTools()` / `AddUserOnlyTools()` once during startup (see `main/application.cc`), prepending the common tools to your board's tools.

#### Anatomy of a tool

```cpp
McpServer::GetInstance().AddTool(
    "self.audio_speaker.set_mute",          // name: self.<domain>.<action>
    "Mute or unmute the speaker.",          // description the model reads to decide when to call it
    PropertyList({                          // input schema (empty PropertyList() = no args)
        Property("mute", kPropertyTypeBoolean)
    }),
    [this](const PropertyList& properties) -> ReturnValue {   // callback
        bool mute = properties["mute"].value<bool>();
        Board::GetInstance().GetAudioCodec()->EnableOutput(!mute);
        return true;                        // ReturnValue: bool | int | std::string | cJSON* | ImageContent*
    });
```

Property types and rules (from `mcp_server.h`):

- `kPropertyTypeBoolean`, `kPropertyTypeInteger`, `kPropertyTypeString`.
- A property **with no default is required**; one constructed with a default value is optional.
- Integers can take a range: `Property("level", kPropertyTypeInteger, 0, 8)` (min/max) or `Property("level", kPropertyTypeInteger, 4, 0, 8)` (default, min, max). Out-of-range values throw and are reported as an MCP error.
- Read args inside the callback with `properties["name"].value<T>()`.

#### Regular vs. user-only tools

- `AddTool(...)` — visible to the LLM; use for anything the assistant should be able to do on its own.
- `AddUserOnlyTool(...)` — annotated `audience: ["user"]`, hidden from the model and only callable by a companion app / explicit user action. Use for sensitive operations like `self.reboot`, `self.upgrade_firmware`, and diagnostics.

#### Worked example: a "standby" feature, end to end

This project already has a first-class idle/sleep mechanism — `PowerSaveTimer` ([`main/boards/common/power_save_timer.h`](main/boards/common/power_save_timer.h)). The example below wires standby up three ways — a physical button, an MCP tool, and an automatic idle timeout — all sharing **one** handler, so the LLM, a companion app, and a human all trigger identical behavior.

```cpp
#include "wifi_board.h"
#include "audio/audio_codec.h"
#include "boards/common/button.h"
#include "boards/common/power_save_timer.h"
#include "mcp_server.h"
#include "config.h"
#include <esp_log.h>

#define TAG "MyBoard"

class MyBoard : public WifiBoard {
private:
    Button standby_button_{STANDBY_BUTTON_GPIO};   // from your board's config.h
    PowerSaveTimer* power_save_timer_ = nullptr;
    bool in_standby_ = false;

    // ---- one handler, shared by button + MCP tool + idle timer ----
    void EnterStandby() {
        if (in_standby_) return;
        in_standby_ = true;
        ESP_LOGI(TAG, "Entering standby");
        GetAudioCodec()->EnableOutput(false);          // silence speaker
        if (auto* bl = GetBacklight()) {
            bl->SetBrightness(0, true);                 // screen off
        }
    }

    void ExitStandby() {
        if (!in_standby_) return;
        in_standby_ = false;
        ESP_LOGI(TAG, "Leaving standby");
        GetAudioCodec()->EnableOutput(true);
        if (auto* bl = GetBacklight()) {
            bl->RestoreBrightness();                    // back to user's level
        }
    }

    // ---- 1. automatic standby after idle ----
    void InitializePowerSaveTimer() {
        // cpu_max_freq, seconds_to_sleep, seconds_to_shutdown(-1 = never)
        power_save_timer_ = new PowerSaveTimer(-1, 60);
        power_save_timer_->OnEnterSleepMode([this]() { EnterStandby(); });
        power_save_timer_->OnExitSleepMode([this]() { ExitStandby(); });
        power_save_timer_->SetEnabled(true);
    }

    // ---- 2. physical button: long-press to standby, click wakes ----
    void InitializeButtons() {
        standby_button_.OnLongPress([this]() { EnterStandby(); });
        standby_button_.OnClick([this]() {
            power_save_timer_->WakeUp();   // resets idle timer + fires OnExitSleepMode
        });
    }

    // ---- 3. MCP tools: let the assistant / companion app do it ----
    void InitializeTools() {
        auto& mcp = McpServer::GetInstance();

        mcp.AddTool("self.system.enter_standby",
            "Put the device into low-power standby. The screen and speaker turn off. "
            "The device wakes on a button press.",
            PropertyList(),                            // no arguments
            [this](const PropertyList&) -> ReturnValue {
                EnterStandby();
                return true;
            });

        // A getter so the model can answer "are you in standby?"
        mcp.AddTool("self.system.get_standby",
            "Returns true if the device is currently in standby.",
            PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                return in_standby_;                    // ReturnValue accepts bool
            });
    }

public:
    MyBoard() {
        InitializePowerSaveTimer();
        InitializeButtons();
        InitializeTools();      // tools must be registered during construction
    }

    virtual AudioCodec* GetAudioCodec() override { /* ...your codec... */ }
    virtual Display* GetDisplay() override { /* ...your display... */ }
};

DECLARE_BOARD(MyBoard);
```

How the pieces connect:

- **`PowerSaveTimer(cpu_max_freq, seconds_to_sleep, seconds_to_shutdown)`** fires `OnEnterSleepMode` after `seconds_to_sleep` of no activity and `OnExitSleepMode` when woken. `WakeUp()` resets the countdown and exits sleep — call it on any user activity. The optional `seconds_to_shutdown` drives `OnShutdownRequest` for a full power-off path (left at `-1`/never here).
- **The button and the tool both call the same `EnterStandby()`** — the key pattern. The button works offline; the MCP tool lets the LLM ("go to standby") or a companion app trigger identical behavior over the network.
- **`EnableOutput(false)` + `SetBrightness(0)`** are the real, in-tree way boards quiet the speaker and screen. For true deep sleep, `esp-spot` ([`main/boards/esp-spot/esp_spot_board.cc`](main/boards/esp-spot/esp_spot_board.cc)) goes all the way to `esp_deep_sleep` with a GPIO/IMU wake source in its `EnterDeepSleep()` — follow that model if "screen + audio off" isn't enough.

Adapt `STANDBY_BUTTON_GPIO` to a pin in your board's `config.h`, and the `GetAudioCodec()`/`GetDisplay()`/`GetBacklight()` overrides to your hardware.

#### Testing

Build and flash your board, then drive the device over its WebSocket/MQTT transport from the backend — the server sends `tools/list` (your tool should appear) and `tools/call`. The JSON-RPC framing and the exact `initialize` / `tools/list` / `tools/call` flow are documented in [docs/mcp-protocol.md](docs/mcp-protocol.md). For camera tools specifically there is a local harness at `scripts/local_camera_mcp_harness.py`.

## Large Model Configuration

If you already have a XiaoZhi AI chatbot device and have connected to the official server, you can log in to the [xiaozhi.me](https://xiaozhi.me) console for configuration.

👉 [Backend Operation Video Tutorial (Old Interface)](https://www.bilibili.com/video/BV1jUCUY2EKM/)

## Related Open Source Projects

For server deployment on personal computers, refer to the following open-source projects:

- [xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) Python server
- [joey-zhou/xiaozhi-esp32-server-java](https://github.com/joey-zhou/xiaozhi-esp32-server-java) Java server
- [AnimeAIChat/xiaozhi-server-go](https://github.com/AnimeAIChat/xiaozhi-server-go) Golang server
- [hackers365/xiaozhi-esp32-server-golang](https://github.com/hackers365/xiaozhi-esp32-server-golang) Golang server

Other client projects using the XiaoZhi communication protocol:

- [huangjunsen0406/py-xiaozhi](https://github.com/huangjunsen0406/py-xiaozhi) Python client
- [TOM88812/xiaozhi-android-client](https://github.com/TOM88812/xiaozhi-android-client) Android client
- [100askTeam/xiaozhi-linux](http://github.com/100askTeam/xiaozhi-linux) Linux client by 100ask
- [78/xiaozhi-sf32](https://github.com/78/xiaozhi-sf32) Bluetooth chip firmware by Sichuan
- [QuecPython/solution-xiaozhiAI](https://github.com/QuecPython/solution-xiaozhiAI) QuecPython firmware by Quectel

Custom Assets Tools:

- [78/xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) Custom Assets Generator (Wake words, fonts, emojis, backgrounds)

## About the Project

This is an open-source ESP32 project, released under the MIT license, allowing anyone to use it for free, including for commercial purposes.

We hope this project helps everyone understand AI hardware development and apply rapidly evolving large language models to real hardware devices.

If you have any ideas or suggestions, please feel free to raise Issues or join our [Discord](https://discord.gg/C759fGMBcZ) or QQ group: 994694848

## Star History

<a href="https://star-history.com/#78/xiaozhi-esp32&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
 </picture>
</a>
