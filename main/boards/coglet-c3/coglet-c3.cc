#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "button.h"
#include "config.h"
#include "xiao_serial_commands.h"
#include "coglet_controller.h"
#include "coglet_mcp_tools.h"
#include "coglet_voice.h"

#include <esp_log.h>

#define TAG "CogletC3"

// Coglet on a XIAO ESP32-C3: same six-role PCA9685 mechanism as the S3 profile,
// same external I2S audio, but no camera and no PSRAM. The C3 has a single I2C
// controller, which the servo bus owns outright.
class CogletC3Board : public WifiBoard {
private:
    Button boot_button_;

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
        boot_button_.OnLongPress([this]() {
            EnterWifiConfigMode();
        });
    }

    void InitializeSerialInput() {
        xTaskCreate(XiaoSerialInputTask, "serial_input", 4096, nullptr, 5, nullptr);
    }

    // Watches for Idle (Standby) state and reconnects automatically.

public:
    CogletC3Board() : boot_button_(BOOT_BUTTON_GPIO) {
        struct { const char* ssid; const char* pass; } known[] = WIFI_NETWORKS;
        auto& ssid_manager = SsidManager::GetInstance();
        for (int i = 0; known[i].ssid != nullptr; i++) {
            const auto& list = ssid_manager.GetSsidList();
            bool found = false;
            for (const auto& item : list) {
                if (item.ssid == known[i].ssid) { found = true; break; }
            }
            if (!found) {
                ssid_manager.AddSsid(known[i].ssid, known[i].pass);
            }
        }
        CogletController::Instance().Start({SERVO_I2C_PORT, SERVO_I2C_SDA_PIN,
                                            SERVO_I2C_SCL_PIN, SERVO_OE_PIN,
                                            SERVO_PCA9685_ADDR});
        RegisterCogletMcpTools();
        InitializeButtons();
        InitializeSerialInput();
        StartCogletVoiceLoop();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        static NoDisplay display;
        return &display;
    }

    virtual const char* GetFirmwareBoardId() const override {
        return "coglet-c3";
    }

    virtual const char* GetWebFlasherManifest() const override {
        return "manifest-coglet-c3.json";  // github.com/ricklon/coglet web-flasher
    }
};

DECLARE_BOARD(CogletC3Board);
