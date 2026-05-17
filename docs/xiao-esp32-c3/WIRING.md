# XIAO ESP32-C3 Voice Assistant — Wiring & Code Reference

## Hardware

| Component | Purpose |
|---|---|
| Seeed XIAO ESP32-C3 | Main MCU, Wi-Fi, USB serial |
| MAX98357A | I2S amplifier → speaker output |
| INMP441 | I2S MEMS microphone → voice input |

---

## Wiring Diagram

```
                        ┌─────────────────────────────┐
                        │      XIAO ESP32-C3           │
                        │                              │
                        │  3.3V ──────────────────┬───┼─── VIN  ┐
                        │                         │   │         │ MAX98357A
                        │  GND  ──────────────┬───┼───┼─── GND  │
                        │                     │   │   │         │
                        │  GPIO2 (DOUT) ──────┼───┼───┼─── DIN  │
                        │                     │   └───┼─── SD   │  (tie to 3.3V)
                        │  GPIO3 (BCLK) ──┬───┼───────┼─── BCLK │
                        │                 │   │       │         │
                        │  GPIO4 (WS)   ──┼─┬─┼───────┼─── LRC  ┘
                        │                 │ │ │       │
                        │  GPIO5 (DIN)  ──┼─┼─┼──┐    │
                        │                 │ │ │  │    │  INMP441
                        │  3.3V ──────────┼─┼─┼──┼────┼─── VDD  │
                        │                 │ │ │  │    │         │
                        │  GND  ──────────┼─┼─┼──┼────┼─── GND  │
                        │                 │ │ │  │    │   L/R ──┘  (tie to GND)
                        │                 └─┼─┼──┼────┼─── SCK  │
                        │                   └─┼──┼────┼─── WS   │
                        │                     │  └────┼─── SD   ┘
                        │                     │       │
                        │  GPIO9 (BOOT btn) ──┘       │
                        │  [onboard]                  │
                        │                              │
                        │  USB-C ── PC terminal        │
                        └─────────────────────────────┘

  Speaker (+) ──── MAX98357A OUT+
  Speaker (-) ──── MAX98357A OUT-
```

> **Key point:** GPIO3 (BCLK) and GPIO4 (WS) are shared between MAX98357A and INMP441 — both devices receive the same I2S clock. The ESP32 drives TX (to amp) and RX (from mic) on the same I2S peripheral simultaneously.

---

## Wiring

### MAX98357A (Speaker Amplifier)

| MAX98357A Pin | XIAO ESP32-C3 Pin | Notes |
|---|---|---|
| BCLK | GPIO3 | Shared with INMP441 SCK |
| LRC | GPIO4 | Shared with INMP441 WS |
| DIN | GPIO2 | I2S data out from ESP32 |
| GND | GND | |
| VIN | 3.3V | |
| SD | 3.3V | **Must be tied HIGH** — floating = shutdown/clicks only |

### INMP441 (Microphone)

| INMP441 Pin | XIAO ESP32-C3 Pin | Notes |
|---|---|---|
| SCK | GPIO3 | Shared with MAX98357A BCLK |
| WS | GPIO4 | Shared with MAX98357A LRC |
| SD | GPIO5 | I2S data in to ESP32 |
| L/R | GND | Selects left channel (address = 0) |
| GND | GND | |
| VDD | 3.3V | |

> **Note:** Both devices share the I2S clock lines (BCLK/SCK and LRC/WS). This is duplex I2S — one I2S peripheral drives both TX (speaker) and RX (mic) simultaneously. Both must run at the same sample rate (16 kHz).

### Boot Button

| | |
|---|---|
| XIAO onboard BOOT button | GPIO9 (built-in) |

---

## GPIO Summary

| GPIO | Function |
|---|---|
| GPIO2 | I2S DOUT → MAX98357A DIN |
| GPIO3 | I2S BCLK (shared TX/RX clock) |
| GPIO4 | I2S WS/LRC (shared word select) |
| GPIO5 | I2S DIN ← INMP441 SD |
| GPIO9 | Boot button (onboard) |

---

## Software Architecture

```
Serial terminal (USB)
        │  SerialInputTask (FreeRTOS)
        ▼
Application::SendTextChat()
        │
        ▼
Protocol::SendWakeWordDetected(text)   ←── Boot button → VAD → ASR (voice path)
        │
        ▼
WebSocket → xiaozhi-esp32-server
        │
        ├── LLM: OpenRouter / Gemma-4
        └── TTS: EdgeTTS (en-US-AriaNeural)
                │
                ▼
        I2S audio stream → MAX98357A → Speaker
```

---

## Key Source Files

### `config.h`
GPIO and sample rate definitions.

```c
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_3
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_2   // → MAX98357A DIN
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_5   // ← INMP441 SD

#define BOOT_BUTTON_GPIO      GPIO_NUM_9
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
```

### `xiao-esp32-c3.cc`
Board class. Key pieces:

- **`SerialInputTask`** — FreeRTOS task that reads lines from USB serial stdin and calls `Application::SendTextChat()`. Characters are echoed back as you type. Press Enter to send.
- **`InitializeButtons`** — Boot button single-click toggles voice chat; long press enters Wi-Fi config mode.
- **`GetAudioCodec`** — Returns `NoAudioCodecDuplex` with shared I2S port for TX (speaker) and RX (mic).
- **`SsidManager::AddSsid`** — Saves Wi-Fi credentials to NVS on first boot; device connects automatically on subsequent boots.

### `application.cc` — `SendTextChat()`
Bypasses ASR entirely. Opens the WebSocket audio channel if needed, then sends a `listen/detect` event with the typed text directly to the server LLM pipeline.

```cpp
void Application::SendTextChat(const std::string& text) {
    Schedule([this, text]() {
        if (!protocol_) return;
        if (!protocol_->IsAudioChannelOpened()) {
            SetDeviceState(kDeviceStateConnecting);
            if (!protocol_->OpenAudioChannel()) {
                SetDeviceState(kDeviceStateIdle);
                return;
            }
        }
        if (GetDeviceState() == kDeviceStateSpeaking)
            AbortSpeaking(kAbortReasonNone);
        protocol_->SendWakeWordDetected(text);
        SetDeviceState(kDeviceStateListening);
    });
}
```

### `config.json`
Build configuration for the `idf.py` build system:

```json
{
    "target": "esp32c3",
    "builds": [{
        "name": "xiao-esp32-c3",
        "sdkconfig_append": [
            "CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y",
            "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/4m.csv\"",
            "CONFIG_LANGUAGE_EN_US=y",
            "CONFIG_OTA_URL=\"http://<server-ip>:8003/xiaozhi/ota/\"",
            "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y"
        ]
    }]
}
```

---

## Server Configuration

File: `xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml`

```yaml
server:
  websocket: ws://<server-ip>:8000/xiaozhi/v1/

TTS:
  EdgeTTS:
    type: edge
    voice: en-US-AriaNeural

selected_module:
  LLM: OpenRouterLLM
  Intent: function_call

LLM:
  OpenRouterLLM:
    type: openai
    base_url: https://openrouter.ai/api/v1
    api_key: <your-api-key>
    model_name: google/gemma-4-31b-it
    temperature: 0.7
    max_tokens: 1024

prompt: |
  You are XiaoZhi, a friendly and concise voice assistant on a smart device.
  Always respond in English, regardless of what language the user writes in.
  Keep all responses short (1-3 sentences) and conversational.
  Be helpful, warm, and direct.
  Current time: {time}
```

---

## Build & Flash

```bash
# Set up ESP-IDF environment
source ~/esp/esp-idf/export.sh

# Build
cd /home/ra/esp-projects/xiaozhi-esp32
idf.py build

# Flash (replace port as needed)
idf.py -p /dev/ttyACM0 flash

# Monitor serial output
idf.py -p /dev/ttyACM0 monitor
```

If you need to rebuild from scratch after config changes:

```bash
idf.py fullclean
idf.py set-target esp32c3
idf.py build
```

---

## Usage

### Voice input
Press the BOOT button once. Speak your query. The device uses VAD (voice activity detection) to detect when you stop, then sends audio to the server for transcription and response.

### Text input
Open a serial monitor (e.g., `idf.py monitor` or `pio device monitor`). Type a message and press Enter. The text goes directly to the LLM, bypassing ASR. Typed characters are echoed back to the terminal.

### Wi-Fi provisioning
On first boot, credentials from `config.h` (`WIFI_SSID` / `WIFI_PASSWORD`) are saved to NVS. To re-provision, long-press the BOOT button to enter Wi-Fi config mode.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Speaker clicks but no audio | MAX98357A SD pin floating | Tie SD pin to 3.3V |
| No mic input | INMP441 L/R pin floating | Tie L/R to GND |
| Device won't connect to Wi-Fi | Credentials not in NVS | Erase flash, reflash; credentials saved on first boot |
| Port busy when flashing | Serial monitor holding port | Close monitor (Ctrl+]) then flash |
| Build corruption after interrupt | Ninja state invalid | Run `idf.py fullclean` then rebuild |
| "Unknown message type: listen" | Server sends listen control messages; harmless | Informational only; no action needed |
