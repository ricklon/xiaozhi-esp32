#include "wifi_board.h"
#include "k10_audio_codec.h"
#include "display/lcd_display.h"
#include "display/lvgl_display/lvgl_theme.h"
#include "esp_lcd_ili9341.h"
#include "led_control.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "esp_video.h"
#include "xiao_serial_commands.h"

#include "led/circular_strip.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#include "esp_io_expander_tca95xx_16bit.h"

#define TAG "DF-K10"

class AgentHubDisplay : public SpiLcdDisplay {
public:
    AgentHubDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                    int width, int height, int offset_x, int offset_y,
                    bool mirror_x, bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                        mirror_x, mirror_y, swap_xy) {
    }

    void SetupUI() override {
        if (setup_ui_called_) {
            return;
        }
        SpiLcdDisplay::SetupUI();

        {
            DisplayLockGuard lock(this);
            auto theme = static_cast<LvglTheme*>(current_theme_);

            auto footer = lv_obj_create(container_);
            lv_obj_set_size(footer, LV_HOR_RES, 54);
            lv_obj_set_style_radius(footer, 0, 0);
            lv_obj_set_style_pad_all(footer, theme->spacing(2), 0);
            lv_obj_set_style_border_width(footer, 1, 0);
            lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
            lv_obj_set_style_border_color(footer, theme->border_color(), 0);
            lv_obj_set_style_bg_color(footer, theme->background_color(), 0);
            lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_EVENLY,
                                  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

            auto add_button_hint = [footer, theme](const char* text) {
                auto label = lv_label_create(footer);
                lv_obj_set_width(label, LV_HOR_RES / 2 - theme->spacing(6));
                lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_set_style_text_color(label, theme->text_color(), 0);
                lv_label_set_text(label, text);
            };
            add_button_hint("A  PAUSE\nRESUME");
            add_button_hint("B  TRANSCRIBE\nSTOP");
        }

        SetChatMessage("system",
                       "AGENT HUB\nSay \"Computer\" to begin.\nYour conversation appears here.");
    }

    void ClearChatMessages() override {
        SpiLcdDisplay::ClearChatMessages();
        SetChatMessage("system",
                       "AGENT HUB\nSay \"Computer\" to begin.\nYour conversation appears here.");
    }
};

class Df_K10Board : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    esp_io_expander_handle_t io_expander_ = nullptr;
    LcdDisplay* display_ = nullptr;
    button_handle_t btn_a_ = nullptr;
    button_handle_t btn_b_ = nullptr;
    EspVideo* camera_ = nullptr;

    button_driver_t* btn_a_driver_ = nullptr;
    button_driver_t* btn_b_driver_ = nullptr;

    CircularStrip* led_strip_;

    static Df_K10Board* instance_;

    void InitializeI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
                .i2c_port = (i2c_port_t)1,
                .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
                .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .intr_priority = 0,
                .trans_queue_depth = 0,
                .flags = {
                                .enable_internal_pullup = 1,
                        },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = GPIO_NUM_21;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = GPIO_NUM_12;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    uint8_t IoExpanderGetLevel(uint16_t pin_mask) {
        uint32_t pin_val = 0;
        ESP_ERROR_CHECK(esp_io_expander_get_level(io_expander_, DRV_IO_EXP_INPUT_MASK, &pin_val));
        pin_mask &= DRV_IO_EXP_INPUT_MASK;
        return (uint8_t)((pin_val & pin_mask) ? 1 : 0);
    }

    void InitializeIoExpander() {
        ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca95xx_16bit(
                i2c_bus_, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, &io_expander_));

        esp_err_t ret;
        ret = esp_io_expander_print_state(io_expander_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Print state failed: %s", esp_err_to_name(ret));
        }

        ret = esp_io_expander_set_dir(io_expander_, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Set direction failed: %s", esp_err_to_name(ret));
        }
        ret = esp_io_expander_set_level(io_expander_, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Set level failed: %s", esp_err_to_name(ret));
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ret = esp_io_expander_set_level(io_expander_, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Set level failed: %s", esp_err_to_name(ret));
        }
        ret = esp_io_expander_set_dir(
                io_expander_, DRV_IO_EXP_INPUT_MASK,
                IO_EXPANDER_INPUT);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Set direction failed: %s", esp_err_to_name(ret));
        }
    }

    void InitializeButtons() {
        instance_ = this;

        // Button A
        button_config_t btn_a_config = {
            .long_press_time = 1000,
            .short_press_time = 0
        };
        btn_a_driver_ = (button_driver_t*)calloc(1, sizeof(button_driver_t));
        btn_a_driver_->enable_power_save = false;
        btn_a_driver_->get_key_level = [](button_driver_t *button_driver) -> uint8_t {
            return !instance_->IoExpanderGetLevel(IO_EXPANDER_PIN_NUM_12);
        };
        ESP_ERROR_CHECK(iot_button_create(&btn_a_config, btn_a_driver_, &btn_a_));
        iot_button_register_cb(btn_a_, BUTTON_SINGLE_CLICK, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                self->EnterWifiConfigMode();
                return;
            }
            app.ToggleListeningPaused();
        }, this);
        iot_button_register_cb(btn_a_, BUTTON_LONG_PRESS_START, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto codec = self->GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            self->GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        }, this);

        // Button B
        button_config_t btn_b_config = {
            .long_press_time = 1000,
            .short_press_time = 0
        };
        btn_b_driver_ = (button_driver_t*)calloc(1, sizeof(button_driver_t));
        btn_b_driver_->enable_power_save = false;
        btn_b_driver_->get_key_level = [](button_driver_t *button_driver) -> uint8_t {
            return !instance_->IoExpanderGetLevel(IO_EXPANDER_PIN_NUM_2);
        };
        ESP_ERROR_CHECK(iot_button_create(&btn_b_config, btn_b_driver_, &btn_b_));
        iot_button_register_cb(btn_b_, BUTTON_SINGLE_CLICK, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                self->EnterWifiConfigMode();
                return;
            }
            if (app.IsListeningPaused()) {
                self->GetDisplay()->ShowNotification("Press A to resume listening");
                return;
            }
            app.ToggleContinuousTranscription();
        }, this);
        iot_button_register_cb(btn_b_, BUTTON_LONG_PRESS_START, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto codec = self->GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            self->GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        }, this);
    }

    void InitializeCamera() {
        static esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
            .data_width = CAM_CTLR_DATA_WIDTH_8,
            .data_io = {
                [0] = CAMERA_PIN_D2,
                [1] = CAMERA_PIN_D3,
                [2] = CAMERA_PIN_D4,
                [3] = CAMERA_PIN_D5,
                [4] = CAMERA_PIN_D6,
                [5] = CAMERA_PIN_D7,
                [6] = CAMERA_PIN_D8,
                [7] = CAMERA_PIN_D9,
            },
            .vsync_io = CAMERA_PIN_VSYNC,
            .de_io = CAMERA_PIN_HREF,
            .pclk_io = CAMERA_PIN_PCLK,
            .xclk_io = CAMERA_PIN_XCLK,
        };

        esp_video_init_sccb_config_t sccb_config = {
            .init_sccb = false,
            .i2c_handle = i2c_bus_,
            .freq = 100000,
        };

        esp_video_init_dvp_config_t dvp_config = {
            .sccb_config = sccb_config,
            .reset_pin = CAMERA_PIN_RESET,
            .pwdn_pin = CAMERA_PIN_PWDN,
            .dvp_pin = dvp_pin_config,
            .xclk_freq = XCLK_FREQ_HZ,
        };

        esp_video_init_config_t video_config = {
            .dvp = &dvp_config,
        };

        camera_ = new EspVideo(video_config);
    }

    void InitializeIli9341Display() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = GPIO_NUM_14;
        io_config.dc_gpio_num = GPIO_NUM_13;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.bits_per_pixel = 16;
        panel_config.color_space = ESP_LCD_COLOR_SPACE_BGR;

        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(panel_io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, DISPLAY_BACKLIGHT_OUTPUT_INVERT));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

        display_ = new AgentHubDisplay(panel_io, panel,
                                DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeIot() {
        led_strip_ = new CircularStrip(BUILTIN_LED_GPIO, 3);
        new LedStripControl(led_strip_);
    }

    void InitializeSerialInput() {
        xTaskCreate(XiaoSerialInputTask, "serial_input", 4096, nullptr, 5, nullptr);
    }

    uint8_t DetectEs7243eAddress() {
        constexpr uint8_t addresses[] = {
            AUDIO_CODEC_ES7243E_ADDR_PRIMARY,
            AUDIO_CODEC_ES7243E_ADDR_SECONDARY,
        };
        for (uint8_t address : addresses) {
            if (i2c_master_probe(i2c_bus_, address, 100) == ESP_OK) {
                ESP_LOGI(TAG, "Detected ES7243E at 0x%02x", address);
                return address;
            }
        }
        ESP_LOGW(TAG, "ES7243E probe failed; trying primary address 0x%02x",
                 AUDIO_CODEC_ES7243E_ADDR_PRIMARY);
        return AUDIO_CODEC_ES7243E_ADDR_PRIMARY;
    }

public:
    Df_K10Board() {
        InitializeI2c();
        InitializeIoExpander();
        InitializeSpi();
        InitializeIli9341Display();
        InitializeButtons();
        InitializeIot();
        InitializeCamera();
        InitializeSerialInput();
    }

    virtual Led* GetLed() override {
        return led_strip_;
    }

    virtual AudioCodec *GetAudioCodec() override {
        static K10AudioCodec audio_codec(
                    i2c_bus_,
                    AUDIO_INPUT_SAMPLE_RATE,
                    AUDIO_OUTPUT_SAMPLE_RATE,
                    AUDIO_I2S_GPIO_MCLK,
                    AUDIO_I2S_GPIO_BCLK,
                    AUDIO_I2S_GPIO_WS,
                    AUDIO_I2S_GPIO_DOUT,
                    AUDIO_I2S_GPIO_DIN,
                    DetectEs7243eAddress(),
                    AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }

    virtual Camera* GetCamera() override {
        return camera_;
    }

    virtual Display *GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(Df_K10Board);

Df_K10Board* Df_K10Board::instance_ = nullptr;
