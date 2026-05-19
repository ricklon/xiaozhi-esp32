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

#define TAG "XiaoEsp32C6"

static void HandleSerialLine(const char* buf) {
    auto& ssid_manager = SsidManager::GetInstance();
    if (strncmp(buf, "!wifi ", 6) == 0) {
        const char* args = buf + 6;
        if (strcmp(args, "list") == 0) {
            const auto& list = ssid_manager.GetSsidList();
            printf("Saved networks (%d):\r\n", (int)list.size());
            for (int i = 0; i < (int)list.size(); i++) {
                printf("  [%d] %s\r\n", i, list[i].ssid.c_str());
            }
        } else if (strcmp(args, "clear") == 0) {
            ssid_manager.Clear();
            printf("All networks cleared.\r\n");
        } else {
            char ssid[64] = {}, pass[64] = {};
            if (sscanf(args, "%63s %63s", ssid, pass) == 2) {
                ssid_manager.AddSsid(std::string(ssid), std::string(pass));
                printf("Added: %s\r\n", ssid);
            } else {
                printf("Usage: !wifi SSID PASSWORD\r\n");
            }
        }
        fflush(stdout);
        return;
    }
    Application::GetInstance().SendTextChat(std::string(buf));
}

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
                HandleSerialLine(buf);
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

class XiaoEsp32C6Board : public WifiBoard {
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
    XiaoEsp32C6Board() : boot_button_(BOOT_BUTTON_GPIO) {
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

DECLARE_BOARD(XiaoEsp32C6Board);
