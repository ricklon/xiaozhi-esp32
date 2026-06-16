#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/display.h"
#include "led/gpio_led.h"
#include "button.h"
#include "config.h"
#include "eye_servo_controller.h"
#include "mcp_server.h"
#include "xiao_serial_commands.h"

#include <driver/i2c_master.h>
#include <esp_log.h>

#define TAG "XiaoEsp32C6Eyes"

class XiaoEsp32C6EyesBoard : public WifiBoard {
private:
    Button boot_button_;
    i2c_master_bus_handle_t servo_i2c_bus_ = nullptr;
    EyeServoController* eye_servos_ = nullptr;

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

    void InitializeServoI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = SERVO_I2C_PORT,
            .sda_io_num = SERVO_I2C_SDA_PIN,
            .scl_io_num = SERVO_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &servo_i2c_bus_));
    }

    void InitializeEyeServos() {
        eye_servos_ = new EyeServoController(
            servo_i2c_bus_, SERVO_PCA9685_ADDR,
            EYE_LEFT_SERVO_CHANNEL, EYE_RIGHT_SERVO_CHANNEL,
            EYELID_LEFT_SERVO_CHANNEL, EYELID_RIGHT_SERVO_CHANNEL);
    }

    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();

        mcp_server.AddTool(
            "self.eyes.set_position",
            "Set eye servo positions. left and right are angles from 0 to 180 degrees.",
            PropertyList({Property("left", kPropertyTypeInteger, EYE_SERVO_CENTER_ANGLE, 0, 180),
                          Property("right", kPropertyTypeInteger, EYE_SERVO_CENTER_ANGLE, 0, 180)}),
            [this](const PropertyList& properties) -> ReturnValue {
                eye_servos_->SetEyes(properties["left"].value<int>(),
                                     properties["right"].value<int>());
                return true;
            });

        mcp_server.AddTool("self.eyes.center",
                           "Center both eyes and open both eyelids.",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               eye_servos_->Center();
                               return true;
                           });

        mcp_server.AddTool("self.eyes.get_position",
                           "Return current eye and eyelid servo angles and trims as JSON.",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               return eye_servos_->GetStateJson();
                           });

        mcp_server.AddTool(
            "self.eyelids.set_position",
            "Set eyelid servo positions. left and right are angles from 0 to 180 degrees.",
            PropertyList({Property("left", kPropertyTypeInteger, EYELID_OPEN_ANGLE, 0, 180),
                          Property("right", kPropertyTypeInteger, EYELID_OPEN_ANGLE, 0, 180)}),
            [this](const PropertyList& properties) -> ReturnValue {
                eye_servos_->SetEyelids(properties["left"].value<int>(),
                                        properties["right"].value<int>());
                return true;
            });

        mcp_server.AddTool("self.eyelids.open",
                           "Open both eyelids.",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               eye_servos_->OpenEyelids();
                               return true;
                           });

        mcp_server.AddTool("self.eyelids.close",
                           "Close both eyelids.",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               eye_servos_->CloseEyelids();
                               return true;
                           });

        mcp_server.AddTool(
            "self.eyes.set_trim",
            "Persist a servo calibration trim. servo must be eye_left, eye_right, eyelid_left, "
            "or eyelid_right. trim is added to the requested angle and may be -30 to 30 degrees.",
            PropertyList({Property("servo", kPropertyTypeString, "eye_left"),
                          Property("trim", kPropertyTypeInteger, 0, -30, 30)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo = properties["servo"].value<std::string>();
                int trim = properties["trim"].value<int>();
                if (!eye_servos_->SetTrim(servo, trim)) {
                    return "Invalid servo. Use eye_left, eye_right, eyelid_left, or eyelid_right.";
                }
                return true;
            });
    }

public:
    XiaoEsp32C6EyesBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        struct { const char* ssid; const char* pass; } known[] = WIFI_NETWORKS;
        auto& ssid_manager = SsidManager::GetInstance();
        for (int i = 0; known[i].ssid != nullptr; i++) {
            const auto& list = ssid_manager.GetSsidList();
            bool found = false;
            for (const auto& item : list) {
                if (item.ssid == known[i].ssid) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ssid_manager.AddSsid(known[i].ssid, known[i].pass);
            }
        }
        InitializeButtons();
        InitializeSerialInput();
        InitializeServoI2c();
        InitializeEyeServos();
        InitializeTools();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }

    virtual Led* GetLed() override {
        static GpioLed led(BUILTIN_LED_GPIO, 1);
        return &led;
    }

    virtual Display* GetDisplay() override {
        static NoDisplay display;
        return &display;
    }

    virtual const char* GetFirmwareBoardId() const override {
        return "c6-eyes";
    }

    virtual const char* GetWebFlasherManifest() const override {
        return "manifest-c6-eyes.json";
    }
};

DECLARE_BOARD(XiaoEsp32C6EyesBoard);
