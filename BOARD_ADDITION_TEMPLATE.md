# Board Addition Template

## Board Name: <board-name>

### Hardware Specifications
- **Chip**: ESP32-?? (S3/C3/C6/P4)
- **Microphone**: (e.g., INMP441, built-in)
- **Speaker**: (e.g., MAX98357A, built-in)
- **Display**: (e.g., OLED 128x64, LCD 1.54")
- **Other features**: (Camera, SD card, etc.)

### File Checklist
- [ ] `main/boards/<board-name>/config.h` - GPIO definitions
- [ ] `main/boards/<board-name>/config.json` - Target and build config
- [ ] `main/boards/<board-name>/sdkconfig.defaults` - Kconfig overrides
- [ ] `main/boards/<board-name>/<board-name>.cc` - Board class implementation

### config.h Template
```cpp
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// WiFi Networks
#define WIFI_NETWORKS \\\\
  X(WIFI_SSID_1, WIFI_PASSWORD_1) \\\\
  X(WIFI_SSID_2, WIFI_PASSWORD_2)

// Audio GPIO
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_I2S_GPIO_BCLK      ?
#define AUDIO_I2S_GPIO_WS        ?
#define AUDIO_I2S_GPIO_DOUT      ?
#define AUDIO_I2S_GPIO_DIN       ?

// Display (if present)
#define DISPLAY_WIDTH    ?
#define DISPLAY_HEIGHT   ?
#define DISPLAY_SDA      ?
#define DISPLAY_SCL      ?
#define DISPLAY_RST      ?

// Other GPIO
#define BUTTON_GPIO      ?
#define LED_GPIO         ?

#endif
```

### config.json Template
```json
{
  "manufacturer": "Manufacturer Name",
  "target": "esp32s3",
  "builds": [
    {
      "name": "<board-name>",
      "sdkconfig_append": [
        "CONFIG_BOARD_TYPE_<BOARD_TYPE>=y",
        "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y",
        "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/8m.csv\"",
        "CONFIG_LANGUAGE_EN_US=y",
        "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y"
      ]
    }
  ]
}
```

### sdkconfig.defaults Template
```
# Board-specific Kconfig overrides
CONFIG_BOARD_TYPE_<BOARD_TYPE>=y
CONFIG_SPIRAM_MODE_OCT=y  # If PSRAM present
```

### <board-name>.cc Template
```cpp
#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "config.h"

class <BoardName> : public WifiBoard {
public:
    <BoardName>() {
        // Add WiFi networks
        #ifdef WIFI_NETWORKS
        #define X(ssid, password) SsidManager::GetInstance().AddSsid(ssid, password);
        WIFI_NETWORKS
        #undef X
        #endif
    }

    ~<BoardName>() {}

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecDuplex codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                        AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
                                        AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
        return &codec;
    }

    virtual Display* GetDisplay() override {
        // Return display instance or nullptr if no display
        return nullptr;
    }
};

DECLARE_BOARD(<BoardName>);
```

### Common Pitfalls
1. **GPIO Mapping**: Check chip datasheet - GPIO numbers differ between ESP32-C3 and ESP32-C6
2. **I2S Pins**: Microphone (DIN) vs Speaker (DOUT) - don't swap them
3. **PSRAM**: Some boards need `CONFIG_SPIRAM=y` for display buffers
4. **Wake Word**: PSRAM required for `USE_AFE_WAKE_WORD`
5. **DECLARE_BOARD**: Must be at end of .cc file - defines `create_board()`

### Testing Steps
1. Build: `./switch-board.sh <board-name> build`
2. Flash: `./switch-board.sh <board-name> flash`
3. Monitor: Use serial monitor at 115200 baud
4. Test: Follow checklist in test-board.sh

### Reference Working Boards
- **xiao-esp32-c6**: Simple, no PSRAM, external audio codec
- **xiao-esp32-s3-sense**: Has PSRAM, display, camera
- **bread-compact-wifi**: Basic WiFi + audio
- **waveshare**: Touch display + audio

### Board Class Hierarchy
```
Board (singleton)
  └── WifiBoard
        └── <YourBoard> : public WifiBoard
```

Key virtual methods to implement:
- `GetAudioCodec()` - Return AudioCodec instance
- `GetDisplay()` - Return Display instance (or nullptr)
- Constructor - Add WiFi networks before `Start()`
