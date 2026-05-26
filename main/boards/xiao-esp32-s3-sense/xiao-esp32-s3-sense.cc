#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "led/gpio_led.h"
#include "button.h"
#include "config.h"
#include "esp32_camera.h"
#include "xiao_serial_commands.h"

#include <esp_log.h>

#define TAG "XiaoEsp32S3Sense"

class XiaoEsp32S3SenseBoard : public WifiBoard {
private:
    Button boot_button_;
    Esp32Camera* camera_;

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

    void InitializeCamera() {
        camera_config_t config = {};
        config.pin_pwdn    = CAMERA_PIN_PWDN;
        config.pin_reset   = CAMERA_PIN_RESET;
        config.pin_xclk    = CAMERA_PIN_XCLK;
        config.pin_sccb_sda = CAMERA_PIN_SIOD;
        config.pin_sccb_scl = CAMERA_PIN_SIOC;
        config.pin_d7      = CAMERA_PIN_D7;
        config.pin_d6      = CAMERA_PIN_D6;
        config.pin_d5      = CAMERA_PIN_D5;
        config.pin_d4      = CAMERA_PIN_D4;
        config.pin_d3      = CAMERA_PIN_D3;
        config.pin_d2      = CAMERA_PIN_D2;
        config.pin_d1      = CAMERA_PIN_D1;
        config.pin_d0      = CAMERA_PIN_D0;
        config.pin_vsync   = CAMERA_PIN_VSYNC;
        config.pin_href    = CAMERA_PIN_HREF;
        config.pin_pclk    = CAMERA_PIN_PCLK;
        config.xclk_freq_hz = XCLK_FREQ_HZ;
        config.ledc_timer   = LEDC_TIMER_0;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size   = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        config.sccb_i2c_port = 0;
        camera_ = new Esp32Camera(config);
    }

    // Watches for Idle state and reconnects automatically.
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
    XiaoEsp32S3SenseBoard() : boot_button_(BOOT_BUTTON_GPIO) {
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
        InitializeCamera();
        InitializeAutoConnect();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }

    virtual Led* GetLed() override {
        static GpioLed led(BUILTIN_LED_GPIO, 1);  // XIAO S3 user LED is active-low
        return &led;
    }

    virtual Display* GetDisplay() override {
        static NoDisplay display;
        return &display;
    }

    virtual Camera* GetCamera() override {
        return camera_;
    }
};

DECLARE_BOARD(XiaoEsp32S3SenseBoard);
