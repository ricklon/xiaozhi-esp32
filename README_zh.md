# An MCP-based Chatbot

（中文 | [English](README.md) | [日本語](README_ja.md)）

## 介绍

👉 [人类：给 AI 装摄像头 vs AI：当场发现主人三天没洗头【bilibili】](https://www.bilibili.com/video/BV1bpjgzKEhd/)

👉 [手工打造你的 AI 女友，新手入门教程【bilibili】](https://www.bilibili.com/video/BV1XnmFYLEJN/)

小智 AI 聊天机器人作为一个语音交互入口，利用 Qwen / DeepSeek 等大模型的 AI 能力，通过 MCP 协议实现多端控制。

<img src="docs/mcp-based-graph.jpg" alt="通过MCP控制万物" width="320">

### 版本说明

当前 v2 版本与 v1 版本分区表不兼容，所以无法从 v1 版本通过 OTA 升级到 v2 版本。分区表说明参见 [partitions/v2/README.md](partitions/v2/README.md)。

使用 v1 版本的所有硬件，可以通过手动烧录固件来升级到 v2 版本。

v1 的稳定版本为 1.9.2，可以通过 `git checkout v1` 来切换到 v1 版本，该分支会持续维护到 2026 年 2 月。

### 已实现功能

- Wi-Fi / ML307 Cat.1 4G
- 离线语音唤醒 [ESP-SR](https://github.com/espressif/esp-sr)
- 支持两种通信协议（[Websocket](docs/websocket_zh.md) 或 MQTT+UDP）
- 采用 OPUS 音频编解码
- 基于流式 ASR + LLM + TTS 架构的语音交互
- 声纹识别，识别当前说话人的身份 [3D Speaker](https://github.com/modelscope/3D-Speaker)
- OLED / LCD 显示屏，支持表情显示
- 电量显示与电源管理
- 支持多语言（中文、英文、日文）
- 支持 ESP32-C3、ESP32-S3、ESP32-P4 芯片平台
- 通过设备端 MCP 实现设备控制（音量、灯光、电机、GPIO 等）
- 通过云端 MCP 扩展大模型能力（智能家居控制、PC桌面操作、知识搜索、邮件收发等）
- 自定义唤醒词、字体、表情与聊天背景，支持网页端在线修改 ([自定义Assets生成器](https://github.com/78/xiaozhi-assets-generator))

## 硬件

### 面包板手工制作实践

详见飞书文档教程：

👉 [《小智 AI 聊天机器人百科全书》](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb?from=from_copylink)

面包板效果图如下：

![面包板效果图](docs/v1/wiring2.jpg)

### 支持 70 多个开源硬件（仅展示部分）

- <a href="https://oshwhub.com/li-chuang-kai-fa-ban/li-chuang-shi-zhan-pai-esp32-s3-kai-fa-ban" target="_blank" title="立创·实战派 ESP32-S3 开发板">立创·实战派 ESP32-S3 开发板</a>
- <a href="https://github.com/espressif/esp-box" target="_blank" title="乐鑫 ESP32-S3-BOX3">乐鑫 ESP32-S3-BOX3</a>
- <a href="https://docs.m5stack.com/zh_CN/core/CoreS3" target="_blank" title="M5Stack CoreS3">M5Stack CoreS3</a>
- <a href="https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base" target="_blank" title="AtomS3R + Echo Base">M5Stack AtomS3R + Echo Base</a>
- <a href="https://gf.bilibili.com/item/detail/1108782064" target="_blank" title="神奇按钮 2.4">神奇按钮 2.4</a>
- <a href="https://www.waveshare.net/shop/ESP32-S3-Touch-AMOLED-1.8.htm" target="_blank" title="微雪电子 ESP32-S3-Touch-AMOLED-1.8">微雪电子 ESP32-S3-Touch-AMOLED-1.8</a>
- <a href="https://github.com/Xinyuan-LilyGO/T-Circle-S3" target="_blank" title="LILYGO T-Circle-S3">LILYGO T-Circle-S3</a>
- <a href="https://oshwhub.com/tenclass01/xmini_c3" target="_blank" title="虾哥 Mini C3">虾哥 Mini C3</a>
- <a href="https://oshwhub.com/movecall/cuican-ai-pendant-lights-up-y" target="_blank" title="Movecall CuiCan ESP32S3">璀璨·AI 吊坠</a>
- <a href="https://github.com/WMnologo/xingzhi-ai" target="_blank" title="无名科技Nologo-星智-1.54">无名科技 Nologo-星智-1.54TFT</a>
- <a href="https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html" target="_blank" title="SenseCAP Watcher">SenseCAP Watcher</a>
- <a href="https://www.bilibili.com/video/BV1BHJtz6E2S/" target="_blank" title="ESP-HI 超低成本机器狗">ESP-HI 超低成本机器狗</a>

<div style="display: flex; justify-content: space-between;">
  <a href="docs/v1/lichuang-s3.jpg" target="_blank" title="立创·实战派 ESP32-S3 开发板">
    <img src="docs/v1/lichuang-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/espbox3.jpg" target="_blank" title="乐鑫 ESP32-S3-BOX3">
    <img src="docs/v1/espbox3.jpg" width="240" />
  </a>
  <a href="docs/v1/m5cores3.jpg" target="_blank" title="M5Stack CoreS3">
    <img src="docs/v1/m5cores3.jpg" width="240" />
  </a>
  <a href="docs/v1/atoms3r.jpg" target="_blank" title="AtomS3R + Echo Base">
    <img src="docs/v1/atoms3r.jpg" width="240" />
  </a>
  <a href="docs/v1/magiclick.jpg" target="_blank" title="神奇按钮 2.4">
    <img src="docs/v1/magiclick.jpg" width="240" />
  </a>
  <a href="docs/v1/waveshare.jpg" target="_blank" title="微雪电子 ESP32-S3-Touch-AMOLED-1.8">
    <img src="docs/v1/waveshare.jpg" width="240" />
  </a>
  <a href="docs/v1/lilygo-t-circle-s3.jpg" target="_blank" title="LILYGO T-Circle-S3">
    <img src="docs/v1/lilygo-t-circle-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/xmini-c3.jpg" target="_blank" title="虾哥 Mini C3">
    <img src="docs/v1/xmini-c3.jpg" width="240" />
  </a>
  <a href="docs/v1/movecall-cuican-esp32s3.jpg" target="_blank" title="CuiCan">
    <img src="docs/v1/movecall-cuican-esp32s3.jpg" width="240" />
  </a>
  <a href="docs/v1/wmnologo_xingzhi_1.54.jpg" target="_blank" title="无名科技Nologo-星智-1.54">
    <img src="docs/v1/wmnologo_xingzhi_1.54.jpg" width="240" />
  </a>
  <a href="docs/v1/sensecap_watcher.jpg" target="_blank" title="SenseCAP Watcher">
    <img src="docs/v1/sensecap_watcher.jpg" width="240" />
  </a>
  <a href="docs/v1/esp-hi.jpg" target="_blank" title="ESP-HI 超低成本机器狗">
    <img src="docs/v1/esp-hi.jpg" width="240" />
  </a>
</div>

## 软件

### 固件烧录

新手第一次操作建议先不要搭建开发环境，直接使用免开发环境烧录的固件。

固件默认接入 [xiaozhi.me](https://xiaozhi.me) 官方服务器，个人用户注册账号可以免费使用 Qwen 实时模型。

👉 [新手烧录固件教程](https://ccnphfhqs21z.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS)

### 开发环境

- Cursor 或 VSCode
- 安装 ESP-IDF 插件，选择 SDK 版本 5.4 或以上
- Linux 比 Windows 更好，编译速度快，也免去驱动问题的困扰
- 本项目使用 Google C++ 代码风格，提交代码时请确保符合规范

### 开发者文档

- [自定义开发板指南](docs/custom-board_zh.md) - 学习如何为小智 AI 创建自定义开发板
- [MCP 协议物联网控制用法说明](docs/mcp-usage_zh.md) - 了解如何通过 MCP 协议控制物联网设备
- [MCP 协议交互流程](docs/mcp-protocol_zh.md) - 设备端 MCP 协议的实现方式
- [MQTT + UDP 混合通信协议文档](docs/mqtt-udp_zh.md)
- [一份详细的 WebSocket 通信协议文档](docs/websocket_zh.md)

## MCP 与诊断

MCP 消息通过项目的 WebSocket 或 MQTT 传输层以 `type: "mcp"` 消息承载，使用 JSON-RPC 2.0 负载。设备充当 MCP 服务器。后端初始化会话，使用 `tools/list` 列出工具，并通过 `tools/call` 调用设备功能。

本分支为 MCP `initialize` 响应增加了更丰富的能力元数据，并暴露了仅供用户使用（user-only）的诊断工具：

- `self.get_system_info`
- `self.diagnostics.get_checks`
- `self.diagnostics.run_check`
- `self.reboot`
- `self.upgrade_firmware`
- LVGL 显示开发板上的屏幕截图/预览工具
- 摄像头开发板上的拍照工具

对模型可调用的动作使用普通工具，对配套 App 或显式用户操作使用 user-only 工具。参见 [docs/mcp-usage_zh.md](docs/mcp-usage_zh.md) 和 [docs/mcp-protocol_zh.md](docs/mcp-protocol_zh.md)。

### 为开发板构建 MCP 工具

“MCP 功能”是指你以可调用函数的形式暴露给助手（或配套 App）的任意开发板能力——设置音量、切换静音、设置 LED 颜色、进入待机、读取传感器等等。这是 **C++**，而不是 C 库：该框架是 [`main/mcp_server.h`](main/mcp_server.h) 中的一小组头文件类（`McpServer`、`McpTool`、`Property`、`PropertyList`）。cJSON（一个 C 库）仅在内部用于序列化 JSON-RPC 负载——普通工具开发中你完全不需要接触它。

#### MCP 工具 vs. 物理按键

这是两个不同的层级，容易混淆：

- **物理按键**（待机、静音、boot）通过 `Button` 类在开发板的 `.cc` 中接线，通常调用 `ToggleChatState()` 之类的 `Application` API。它只有在人按下时才起作用。
- **MCP 工具**把*同一能力*通过网络暴露给 LLM 和配套 App，因此模型可以执行它（“静音”、“进入待机”）。

对于静音或待机这类功能，一个好的模式是编写一个处理方法，并把按键回调和 MCP 工具**都**接到它上面。

#### 识别候选功能

开发板上任何有状态或可控制的东西都是候选。常见形态是一对 **getter + setter**（读取当前状态、修改它），并用 `Settings` 辅助类持久化设置，使其在重启后保留。可参考的现有树内示例：

| 功能 | 位置 | 工具名 |
|------|------|--------|
| 长按说话 vs 单击说话模式 | [`main/boards/common/press_to_talk_mcp_tool.cc`](main/boards/common/press_to_talk_mcp_tool.cc) | `self.set_press_to_talk` |
| RGB LED 灯带 | [`main/boards/df-k10/led_control.cc`](main/boards/df-k10/led_control.cc) | `self.led_strip.get_brightness`、`self.led_strip.set_brightness`、`self.led_strip.set_all_color` 等 |
| 音量 / 屏幕 / 摄像头 | [`main/mcp_server.cc`](main/mcp_server.cc) 的 `AddCommonTools()` | `self.audio_speaker.set_volume`、`self.screen.set_brightness`、`self.camera.take_photo` |

#### 工具在哪里注册

- 所有开发板共享的**通用工具**（音量、亮度、主题、摄像头、设备状态）位于 `main/mcp_server.cc` 的 `McpServer::AddCommonTools()` 中。**不要**在这里添加开发板专属工具。
- **开发板专属工具**放在你的开发板类的 `InitializeTools()` 方法中，并从开发板构造函数调用。`Application` 随后在启动时调用一次 `AddCommonTools()` / `AddUserOnlyTools()`（见 `main/application.cc`），把通用工具插到你的开发板工具前面。

#### 工具的结构

```cpp
McpServer::GetInstance().AddTool(
    "self.audio_speaker.set_mute",          // 名称：self.<域>.<动作>
    "Mute or unmute the speaker.",          // 描述，模型据此决定何时调用
    PropertyList({                          // 输入 schema（空的 PropertyList() = 无参数）
        Property("mute", kPropertyTypeBoolean)
    }),
    [this](const PropertyList& properties) -> ReturnValue {   // 回调
        bool mute = properties["mute"].value<bool>();
        Board::GetInstance().GetAudioCodec()->EnableOutput(!mute);
        return true;                        // ReturnValue: bool | int | std::string | cJSON* | ImageContent*
    });
```

属性类型与规则（来自 `mcp_server.h`）：

- `kPropertyTypeBoolean`、`kPropertyTypeInteger`、`kPropertyTypeString`。
- **没有默认值的属性是必填的**；用默认值构造的属性是可选的。
- 整数可以带范围：`Property("level", kPropertyTypeInteger, 0, 8)`（最小/最大）或 `Property("level", kPropertyTypeInteger, 4, 0, 8)`（默认、最小、最大）。超出范围的值会抛出异常并作为 MCP 错误返回。
- 在回调内用 `properties["name"].value<T>()` 读取参数。

#### 普通工具 vs. user-only 工具

- `AddTool(...)`——对 LLM 可见；用于助手可以自行完成的任何操作。
- `AddUserOnlyTool(...)`——标注 `audience: ["user"]`，对模型隐藏，仅可由配套 App / 显式用户操作调用。用于 `self.reboot`、`self.upgrade_firmware` 和诊断等敏感操作。

#### 完整示例：端到端的“待机”功能

本项目已经内置了一流的空闲/休眠机制——`PowerSaveTimer`（[`main/boards/common/power_save_timer.h`](main/boards/common/power_save_timer.h)）。下面的示例用三种方式接入待机——物理按键、MCP 工具和自动空闲超时——它们都共享**同一个**处理方法，因此 LLM、配套 App 和人都能触发完全相同的行为。

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
    Button standby_button_{STANDBY_BUTTON_GPIO};   // 来自你的开发板 config.h
    PowerSaveTimer* power_save_timer_ = nullptr;
    bool in_standby_ = false;

    // ---- 同一个处理方法，由按键 + MCP 工具 + 空闲定时器共享 ----
    void EnterStandby() {
        if (in_standby_) return;
        in_standby_ = true;
        ESP_LOGI(TAG, "Entering standby");
        GetAudioCodec()->EnableOutput(false);          // 关闭扬声器
        if (auto* bl = GetBacklight()) {
            bl->SetBrightness(0, true);                 // 关屏
        }
    }

    void ExitStandby() {
        if (!in_standby_) return;
        in_standby_ = false;
        ESP_LOGI(TAG, "Leaving standby");
        GetAudioCodec()->EnableOutput(true);
        if (auto* bl = GetBacklight()) {
            bl->RestoreBrightness();                    // 恢复到用户的亮度
        }
    }

    // ---- 1. 空闲后自动待机 ----
    void InitializePowerSaveTimer() {
        // cpu_max_freq, seconds_to_sleep, seconds_to_shutdown(-1 = 永不)
        power_save_timer_ = new PowerSaveTimer(-1, 60);
        power_save_timer_->OnEnterSleepMode([this]() { EnterStandby(); });
        power_save_timer_->OnExitSleepMode([this]() { ExitStandby(); });
        power_save_timer_->SetEnabled(true);
    }

    // ---- 2. 物理按键：长按进入待机，单击唤醒 ----
    void InitializeButtons() {
        standby_button_.OnLongPress([this]() { EnterStandby(); });
        standby_button_.OnClick([this]() {
            power_save_timer_->WakeUp();   // 重置空闲定时器并触发 OnExitSleepMode
        });
    }

    // ---- 3. MCP 工具：让助手 / 配套 App 来执行 ----
    void InitializeTools() {
        auto& mcp = McpServer::GetInstance();

        mcp.AddTool("self.system.enter_standby",
            "Put the device into low-power standby. The screen and speaker turn off. "
            "The device wakes on a button press.",
            PropertyList(),                            // 无参数
            [this](const PropertyList&) -> ReturnValue {
                EnterStandby();
                return true;
            });

        // 一个 getter，让模型能回答“你处于待机吗？”
        mcp.AddTool("self.system.get_standby",
            "Returns true if the device is currently in standby.",
            PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                return in_standby_;                    // ReturnValue 接受 bool
            });
    }

public:
    MyBoard() {
        InitializePowerSaveTimer();
        InitializeButtons();
        InitializeTools();      // 工具必须在构造期间注册
    }

    virtual AudioCodec* GetAudioCodec() override { /* ...你的 codec... */ }
    virtual Display* GetDisplay() override { /* ...你的 display... */ }
};

DECLARE_BOARD(MyBoard);
```

各部分如何衔接：

- **`PowerSaveTimer(cpu_max_freq, seconds_to_sleep, seconds_to_shutdown)`** 在 `seconds_to_sleep` 秒无活动后触发 `OnEnterSleepMode`，被唤醒时触发 `OnExitSleepMode`。`WakeUp()` 重置倒计时并退出休眠——在任何用户活动时调用它。可选的 `seconds_to_shutdown` 驱动 `OnShutdownRequest`，用于完全关机路径（此处设为 `-1`/永不）。
- **按键和工具都调用同一个 `EnterStandby()`**——这是关键模式。按键离线可用；MCP 工具让 LLM（“进入待机”）或配套 App 通过网络触发相同行为。
- **`EnableOutput(false)` + `SetBrightness(0)`** 是开发板让扬声器和屏幕静默的真实、树内做法。如需真正的深度休眠，`esp-spot`（[`main/boards/esp-spot/esp_spot_board.cc`](main/boards/esp-spot/esp_spot_board.cc)）在其 `EnterDeepSleep()` 中一路调用到带 GPIO/IMU 唤醒源的 `esp_deep_sleep`——如果“关屏 + 关音频”还不够，可参照该模式。

把 `STANDBY_BUTTON_GPIO` 改成你的开发板 `config.h` 中的引脚，并把 `GetAudioCodec()`/`GetDisplay()`/`GetBacklight()` 重写改成你的硬件。

#### 测试

构建并烧录你的开发板，然后从后端通过其 WebSocket/MQTT 传输驱动设备——服务器发送 `tools/list`（你的工具应当出现）和 `tools/call`。JSON-RPC 帧格式以及准确的 `initialize` / `tools/list` / `tools/call` 流程记录在 [docs/mcp-protocol_zh.md](docs/mcp-protocol_zh.md) 中。摄像头工具另有一个本地工具 `scripts/local_camera_mcp_harness.py`。

## 大模型配置

如果你已经拥有一个小智 AI 聊天机器人设备，并且已接入官方服务器，可以登录 [xiaozhi.me](https://xiaozhi.me) 控制台进行配置。

👉 [后台操作视频教程（旧版界面）](https://www.bilibili.com/video/BV1jUCUY2EKM/)

## 相关开源项目

在个人电脑上部署服务器，可以参考以下第三方开源的项目：

- [xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) Python 服务器
- [joey-zhou/xiaozhi-esp32-server-java](https://github.com/joey-zhou/xiaozhi-esp32-server-java) Java 服务器
- [AnimeAIChat/xiaozhi-server-go](https://github.com/AnimeAIChat/xiaozhi-server-go) Golang 服务器
- [hackers365/xiaozhi-esp32-server-golang](https://github.com/hackers365/xiaozhi-esp32-server-golang) Golang 服务器

使用小智通信协议的第三方客户端项目：

- [huangjunsen0406/py-xiaozhi](https://github.com/huangjunsen0406/py-xiaozhi) Python 客户端
- [TOM88812/xiaozhi-android-client](https://github.com/TOM88812/xiaozhi-android-client) Android 客户端
- [100askTeam/xiaozhi-linux](http://github.com/100askTeam/xiaozhi-linux) 百问科技提供的 Linux 客户端
- [78/xiaozhi-sf32](https://github.com/78/xiaozhi-sf32) 思澈科技的蓝牙芯片固件
- [QuecPython/solution-xiaozhiAI](https://github.com/QuecPython/solution-xiaozhiAI) 移远提供的 QuecPython 固件

## 关于项目

这是一个由虾哥开源的 ESP32 项目，以 MIT 许可证发布，允许任何人免费使用，修改或用于商业用途。

我们希望通过这个项目，能够帮助大家了解 AI 硬件开发，将当下飞速发展的大语言模型应用到实际的硬件设备中。

如果你有任何想法或建议，请随时提出 Issues 或加入 [Discord](https://discord.gg/C759fGMBcZ) 或 QQ 群：1011329060

## Star History

<a href="https://star-history.com/#78/xiaozhi-esp32&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
 </picture>
</a>
