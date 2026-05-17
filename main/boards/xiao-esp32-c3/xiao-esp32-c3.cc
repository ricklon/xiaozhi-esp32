#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "ssid_manager.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#define TAG "XiaoEsp32C3"

// Reads lines from USB serial (stdin) and forwards them to the chat pipeline.
// A line is terminated by '\n' or '\r'. Empty lines are ignored.
static void SerialInputTask(void* arg) {
    char buf[256];
    int pos = 0;
    while (true) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            fputc('\n', stdout);
            fflush(stdout);
            if (pos > 0) {
                buf[pos] = '\0';
                Application::GetInstance().SendTextChat(std::string(buf, pos));
                pos = 0;
            }
            continue;
        }
        if (pos < (int)(sizeof(buf) - 1)) {
            buf[pos++] = (char)ch;
            fputc(ch, stdout);
            fflush(stdout);
        }
    }
}

class XiaoEsp32C3Board : public WifiBoard {
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
        xTaskCreate(SerialInputTask, "serial_input", 4096, nullptr, 5, nullptr);
    }

    // Watches for Idle (Standby) state and reconnects automatically.
    // Runs forever so the button works after idle timeouts, not just at boot.
    void InitializeAutoConnect() {
        xTaskCreate([](void*) {
            auto& app = Application::GetInstance();
            DeviceState last_state = kDeviceStateUnknown;
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(500));
                DeviceState state = app.GetDeviceState();
                if (state == kDeviceStateIdle && last_state != kDeviceStateIdle) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    if (app.GetDeviceState() == kDeviceStateIdle) {
                        app.ToggleChatState();
                    }
                }
                last_state = state;
            }
        }, "auto_connect", 4096, nullptr, 3, nullptr);
    }

public:
    XiaoEsp32C3Board() : boot_button_(BOOT_BUTTON_GPIO) {
        auto& ssid_manager = SsidManager::GetInstance();
        if (ssid_manager.GetSsidList().empty()) {
            ssid_manager.AddSsid(WIFI_SSID, WIFI_PASSWORD);
        }
        InitializeButtons();
        InitializeSerialInput();
        InitializeAutoConnect();
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
};

DECLARE_BOARD(XiaoEsp32C3Board);
