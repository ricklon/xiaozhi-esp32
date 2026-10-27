#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "led/gpio_led.h"
#include "button.h"
#include "config.h"
#include "esp32_camera.h"
#include "xiao_serial_commands.h"
#include "coglet_controller.h"
#include "coglet_mcp_tools.h"
#include "coglet_voice.h"

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

    // Detects the sensor by default, so a module can be swapped without a
    // rebuild; a pinned model (menuconfig) refuses any other.
    void InitializeCamera() {
#if CONFIG_COGLET_CAMERA_NONE
        ESP_LOGI(TAG, "Camera: none configured");
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
        // The user LED owns LEDC timer 1, channel 0; the camera clock takes
        // timer 0, channel 1, so neither reconfigures the other.
        config.ledc_timer   = LEDC_TIMER_0;
        config.ledc_channel = LEDC_CHANNEL_1;
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size   = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        // With SCCB pins given, the driver takes its I2C port from
        // CONFIG_SCCB_HARDWARE_I2C_PORT0 (sdkconfig.defaults), not from here.
        config.sccb_i2c_port = 0;
        camera_ = new CogletCamera(config);
        auto* sensor = esp_camera_sensor_get();
        int pid = sensor ? sensor->id.PID : 0;
        const char* model = pid == OV2640_PID ? "OV2640" : pid == OV3660_PID ? "OV3660"
                          : pid == OV5640_PID ? "OV5640" : nullptr;
#if CONFIG_COGLET_CAMERA_OV2640
        bool accepted = pid == OV2640_PID;
#elif CONFIG_COGLET_CAMERA_OV3660
        bool accepted = pid == OV3660_PID;
#elif CONFIG_COGLET_CAMERA_OV5640
        bool accepted = pid == OV5640_PID;
#else
        bool accepted = model != nullptr;
#endif
        if (!accepted) {
            ESP_LOGE(TAG, "Camera: %s (PID 0x%x); camera off",
                     sensor ? (model ? "not the configured model" : "unsupported sensor") : "none answered", pid);
            delete camera_;
            camera_ = nullptr;
            return;
        }
        ESP_LOGI(TAG, "Camera: %s (PID 0x%x)", model, pid);
#endif
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
        CogletController::Instance().Start({SERVO_I2C_PORT, SERVO_I2C_SDA_PIN,
                                            SERVO_I2C_SCL_PIN, SERVO_OE_PIN,
                                            SERVO_PCA9685_ADDR});
        RegisterCogletMcpTools();
        InitializeButtons();
        InitializeSerialInput();
        InitializeCamera();
        StartCogletVoiceLoop();
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

    // The typed-phrase build (boards/coglet/phrase) is its own Web Programmer
    // entry, so it reports its own id and manifest.
    virtual const char* GetFirmwareBoardId() const override {
#if CONFIG_USE_CUSTOM_WAKE_WORD
        return "coglet-phrase";
#else
        return "coglet";
#endif
    }

    virtual const char* GetWebFlasherManifest() const override {
#if CONFIG_USE_CUSTOM_WAKE_WORD
        return "manifest-coglet-phrase.json";  // github.com/ricklon/coglet web-programmer
#else
        return "manifest-coglet.json";  // github.com/ricklon/coglet web-programmer
#endif
    }
};

DECLARE_BOARD(CogletBoard);
