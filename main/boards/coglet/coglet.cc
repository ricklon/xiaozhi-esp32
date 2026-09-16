#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "led/gpio_led.h"
#include "button.h"
#include "config.h"
#include "esp32_camera.h"
#include "xiao_serial_commands.h"
#include "coglet_controller.h"

#include <esp_log.h>

#define TAG "Coglet"

// The shared camera interface has no virtual destructor. Delete only this
// final concrete type when initialization or sensor verification fails.
class CogletCamera final : public Esp32Camera {
public:
    using Esp32Camera::Esp32Camera;
};

class CogletBoard : public WifiBoard {
private:
    Button boot_button_;
    CogletCamera* camera_ = nullptr;

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
#if CONFIG_COGLET_CAMERA_UNCONFIRMED
        ESP_LOGW(TAG, "Camera disabled: confirm fitted model in menuconfig");
        return;
#else
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
        camera_ = new CogletCamera(config);
        auto* sensor = esp_camera_sensor_get();
#if CONFIG_COGLET_CAMERA_OV2640
        constexpr int expected = OV2640_PID;
#elif CONFIG_COGLET_CAMERA_OV3660
        constexpr int expected = OV3660_PID;
#else
        constexpr int expected = OV5640_PID;
#endif
        if (!sensor || sensor->id.PID != expected) {
            ESP_LOGE(TAG, "Camera missing or PID differs from confirmed model");
            delete camera_;
            camera_ = nullptr;
        }
#endif
    }

    // Watches for Idle state and reconnects automatically.
    void InitializeAutoConnect() {
        xTaskCreate([](void*) {
            auto& app = Application::GetInstance();
            DeviceState last_state = kDeviceStateUnknown;
            bool quieted = false;
            while (true) {
                vTaskDelay(pdMS_TO_TICKS(500));
                // Calibration needs silence: stop talking, stop listening, and
                // stop reconnecting until the builder releases the servos.
                // Entering builder mode silences the voice agent, but leaving it
                // does NOT bring the voice back: the LLM can call
                // self.coglet.release over MCP, and an auto-resume let the robot
                // talk its own way out of a calibration session. Quiet ends only
                // when a builder says so, with !quiet off.
                if (CogletController::Instance().BuilderMode() && !quieted) {
                    quieted = true;
                    if (app.GetDeviceState() == kDeviceStateSpeaking) {
                        app.AbortSpeaking(kAbortReasonNone);
                    }
                    app.SetListeningPaused(true);
                    app.StopListening();
                    ESP_LOGI(TAG, "Builder mode: voice agent paused until !quiet off");
                } else if (quieted && !app.IsListeningPaused()) {
                    quieted = false; // !quiet off, or the button resumed listening
                    ESP_LOGI(TAG, "Voice agent resumed");
                }
                DeviceState state = app.GetDeviceState();
                if (app.IsListeningPaused()) { last_state = state; continue; }
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
    CogletBoard() : boot_button_(BOOT_BUTTON_GPIO) {
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
        CogletController::Instance().Start();
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

    virtual const char* GetFirmwareBoardId() const override {
        return "coglet";
    }

    virtual const char* GetWebFlasherManifest() const override {
        return nullptr;  // No published Coglet web-flasher manifest yet.
    }
};

DECLARE_BOARD(CogletBoard);
