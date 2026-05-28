# MCP Implementation Audit

> Date: 2026-05-28
> Scope: xiaozhi-esp32 firmware MCP (Model Context Protocol) server implementation

## Architecture Overview

```
┌─────────────────┐    WebSocket/MQTT     ┌──────────────────────┐
│  ESP32 Device  │◄──────────────────────►│  Backend API       │
│  (MCP Server) │   MCP (JSON-RPC 2.0)  │  (MCP Client)     │
└─────────────────┘                         └──────────────────────┘
        │                                         │
        │ 1. Hello (features.mcp=true)              │
        │ 2. initialize ►───────────────────────┐ │
        │◄───────────────────────────────────────┘ │
        │ 3. tools/list ◄───────────────────────┐ │
        │◄───────────────────────────────────────┘ │
        │ 4. tools/call ◄───────────────────────┐ │
        │◄───────────────────────────────────────┘ │
        │ 5. notifications/... (device → backend)    │
        └─────────────────────────────────────────┘
```

**Key Point:** The ESP32 device acts as the MCP server. The backend (agent-hub) is the MCP client that discovers and invokes tools on the device.

---

## Core Classes (`main/mcp_server.h`)

### `Property`
Defines a single tool parameter with type, default value, and optional min/max range (integers only).

```cpp
Property(const std::string& name, PropertyType type);
Property(const std::string& name, PropertyType type, const T& default_value);
Property(const std::string& name, PropertyType type, int min, int max);
```

**PropertyType:** `kPropertyTypeBoolean`, `kPropertyTypeInteger`, `kPropertyTypeString`

### `PropertyList`
Container for multiple `Property` objects. Supports lookup by name and JSON schema generation.

```cpp
void AddProperty(const Property& property);
const Property& operator[](const std::string& name) const;
std::vector<std::string> GetRequired() const;  // returns names of properties without defaults
std::string to_json() const;              // generates JSON Schema fragment
```

### `McpTool`
Represents a single invokable tool with name, description, properties, and a callback.

```cpp
McpTool(const std::string& name,
        const std::string& description,
        const PropertyList& properties,
        std::function<ReturnValue(const PropertyList&)> callback);
```

**ReturnValue** is a `std::variant<bool, int, std::string, cJSON*, ImageContent*>`.

### `McpServer` (Singleton)
Central registry of all tools. Boards register their tools via this singleton.

```cpp
static McpServer& GetInstance();

void AddCommonTools();    // called by Application on startup
void AddUserOnlyTools(); // privileged tools (reboot, OTA, etc.)
void AddTool(McpTool* tool);
void AddTool(name, description, properties, callback);
void AddUserOnlyTool(...);  // hidden from normal tools/list

void ParseMessage(const std::string& message);  // entry point for incoming MCP JSON
```

---

## Protocol Flow (`docs/mcp-protocol.md`)

### 1. Transport Hello (device → backend)
The device advertises MCP support inside the transport-level hello:

```json
{
  "type": "hello",
  "version": 1,
  "features": { "mcp": true },
  "transport": "websocket",
  "session_id": "..."
}
```

### 2. Initialize (backend → device)
```json
{"jsonrpc":"2.0", "method":"initialize", "params":{"capabilities":{"vision":{"url":"...","token":"..."}}}, "id":1}
```

Device responds with `protocolVersion: "2024-11-05"` and its `serverInfo`.

### 3. Tools List (backend → device)
```json
{"jsonrpc":"2.0", "method":"tools/list", "params":{"cursor":"", "withUserTools":false}, "id":2}
```

Pagination: non-empty `nextCursor` triggers another `tools/list` with that cursor.

### 4. Tool Call (backend → device)
```json
{"jsonrpc":"2. 0", "method":"tools/call", "params":{"name":"self.audio_speaker.set_volume", "arguments":{"volume":50}}, "id":3}
```

### 5. Device Notifications (device → backend, no `id`)
```json
{"jsonrpc":"2.0", "method":"notifications/state_changed", "params":{"newState":"idle","oldState":"connecting"}}
```

---

## Built-in Tools (registered in `main/mcp_server.cc`)

### Common Tools (always visible, added first for prompt-cache efficiency)

| Tool Name | Description |
|------------|-------------|
| `self.get_device_status` | Returns real-time device status (audio, screen, battery, network) |
| `self.audio_speaker.set_volume` | Set audio output volume (0-100) |
| `self.screen.set_brightness` | Set screen brightness (0-100, only if backlight exists) |
| `self.screen.set_theme` | Set screen theme (`light` or `dark`, requires LVGL) |
| `self.camera.take_photo` | Capture photo and return base64 image + explanation |

### User-Only Tools (hidden by default, require `withUserTools: true`)

| Tool Name | Description |
|------------|-------------|
| `self.get_system_info` | Get system info (free heap, chip info, etc.) |
| `self.reboot` | Reboot the device |
| `self.ota.upgrade` | Trigger OTA firmware upgrade |
| `self.screen.get_screenshot` | Get base64 screenshot (LVGL displays only) |

---

## Board-Specific Tools

Boards register additional tools in their `InitializeTools()` method. Common patterns:

### Lamp Controller (`boards/common/lamp_controller.h`)
Reusable class that registers 3 tools for a GPIO-controlled lamp:

| Tool Name | Description |
|------------|-------------|
| `self.lamp.get_state` | Get lamp power state |
| `self.lamp.turn_on` | Turn lamp on |
| `self.lamp.turn_off` | Turn lamp off |

**Usage in board `.cc`:
```cpp
void InitializeTools() override {
    static LampController lamp(LAMP_GPIO);
}
```

Boards using `LampController`: `bread-compact-wifi`, `bread-compact-esp32`, `bread-compact-ml307`, `esp32-cgc`, `esp32-cgc-144`, `minsi-k08-dual`

### Press-to-Talk MCP Tool (`boards/common/press_to_talk_mcp_tool.h`)
Exposes the physical push-to-talk button as an MCP tool:

| Tool Name | Description |
|------------|-------------|
| `self.press_to_talk.start` | Simulate pressing the talk button |
| `self.press_to_talk.stop` | Simulate releasing the talk button |

### Camera Tools (board-specific)
- `sensecap-watcher`: Registers camera MCP tools via `sscma_camera.cc`
- `waveshare` ESP32-S3 eBook: `esp32-s3-ebook-1.54` registers camera tools

### LED Strip Controllers
Several boards register `self.led.set_color` / `self.led.set_brightness` tools via board-specific `led_control.cc` files:
- `kevin-c3`
- `waveshare/esp32-s3-lcd-0.85`
- `df-k10`

### Robot Controllers
- `otto-robot`: `OttoController` with movement/tools MCP tools
- `electron-bot`: `ElectronBotController`
- `licaoda-course-examples/eda-robot-pro`: `EdaRobotController`
- `licaoda-course-examples/eda-super-bear`: `EdaSuperBearController`
- `rymcu/bigsmart`: `RymcuBigSmartController`
- `movecall-moji2-esp32s3`: movement tools

### IR Filter Controller (`lilygo-t-cameraplus-s3`)
- `self.ir_filter.set_position`

### ESP32-CAM MCP (`zhengchen-cam`, `zhengchen-cam-ml307`)
- Registers camera controller tools

---

## How to Add a New MCP Tool

### Quick Pattern (lambda callback)
```cpp
void InitializeTools() override {
    auto& mcp_server = McpServer::GetInstance();
    mcp_server.AddTool(
        "self.my_feature.do_something",
        "Description of what this tool does. Use this tool when...",
        PropertyList({
            Property("param1", kPropertyTypeString),
            Property("param2", kPropertyTypeInteger, 0, 100),  // with range
            Property("param3", kPropertyTypeBoolean, false),  // with default
        }),
        [](const PropertyList& props) -> ReturnValue {
            std::string p1 = props["param1"].value<std::string>();
            int p2 = props["param2"].value<int>();
            bool p3 = props["param3"].value<bool>();
            // ... do work ...
            return "success";  // or true, or 42, or ImageContent
        });
}
```

### Reusable Controller Class Pattern
For hardware that multiple boards share:

**`boards/common/my_controller.h`:**
```cpp
class MyController {
public:
    MyController(gpio_num_t gpio_num) {
        // setup GPIO, I2C, etc.
        auto& mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.my.get_state", "...", PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                return GetState();
            });
        mcp_server.AddTool("self.my.set_state", "...", PropertyList({...}),
            [this](const PropertyList& props) -> ReturnValue {
                SetState(props[...]);
                return true;
            });
    }
};
```

**In board `.cc` `InitializeTools()`:**
```cpp
void InitializeTools() override {
    static MyController controller(MY_GPIO);
}
```

---

## Currently Known Issues / Gaps

### 1. MCP Server JSON-RPC Handling
- [ ] `mcp_server.cc` `ParseMessage()` — verify it correctly handles JSON-RPC `error` responses (not just requests)
- [ ] Pagination `nextCursor` logic — confirmed working via code review, but needs real-world test with >50 tools

### 2. ImageContent Support
- [ ] `ImageContent` uses base64 encoding via mbedtls — verify it works for large images (>50KB JPEG from camera)
- [ ] The backend `vision.url` capability — document expected upload endpoint behavior

### 3. User-Only Tools Discovery
- [ ] The `withUserTools` parameter is passed from backend — confirm agent-hub sends this correctly
- [ ] Companion app UX for user-only tools — not documented

### 4. Board Coverage
- [ ] `xiao-esp32-c6` — does NOT appear to register any MCP tools (no `InitializeTools()` found in `xiao-esp32-c6.cc`). Only has audio + WiFi.
- [ ] `xiao-esp32-s3-sense` — has camera; verify MCP camera tools are registered
- [ ] `lilygo-t-display-s3` — untested; MCP tools likely work if build succeeds

### 5. Documentation Gaps
- [ ] `docs/mcp-protocol.md` — marked "AI-assisted, cross-check against code" — needs manual verification
- [ ] `docs/mcp-usage.md` — documents how to register tools on device side; should be updated with `LampController` pattern
- [ ] No central list of ALL registered tool names across all boards (this audit is the first)

---

## Tool Naming Convention

All tool names follow the pattern:
```
self.<category>.<action>
```

**Categories used:**
- `device` — device-level info/control
- `audio_speaker` — audio output
- `audio_microphone` — audio input
- `screen` — display control
- `camera` — image capture
- `lamp` — GPIO lamp control
- `led` — addressable LED strips
- `press_to_talk` — PTT button control
- `system` — privileged system operations
- `ota` — firmware upgrade

---

## Testing Checklist

When testing MCP on a board:
- [ ] `tools/list` returns expected tools (with `withUserTools=false`)
- [ ] `tools/list` returns additional tools (with `withUserTools=true`)
- [ ] Tool `inputSchema` is valid JSON Schema
- [ ] Required vs optional parameters are correctly marked
- [ ] Integer range constraints (`minimum`/`maximum`) are present when applicable
- [ ] Tool call with valid arguments succeeds
- [ ] Tool call with invalid arguments returns proper JSON-RPC error
- [ ] `ImageContent` return value works (for camera tools)
- [ ] Device notification (`notifications/...`) is sent and received
- [ ] Pagination works (if > tools per page)

---

## Files to Reference

| File | Purpose |
|------|----------|
| `main/mcp_server.h` | MCP server class definitions |
| `main/mcp_server.cc` | Core tool registration (`AddCommonTools`, `AddUserOnlyTools`) |
| `main/application.cc` | Calls `McpServer::GetInstance().ParseMessage()` on incoming MCP data |
| `docs/mcp-protocol.md` | Protocol flow documentation |
| `docs/mcp-usage.md` | How to use MCP tools (device side) |
| `boards/common/lamp_controller.h` | Reusable lamp controller example |
| `boards/common/press_to_talk_mcp_tool.h` | PTT button MCP tool |
| `boards/<board>/<board>.cc` | Board-specific tool registration |
