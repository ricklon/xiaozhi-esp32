#include "wifi_board.h"
#include "k10_audio_codec.h"
#include "display/lcd_display.h"
#include "display/lvgl_display/lvgl_theme.h"
#include "esp_lcd_ili9341.h"
#include "led_control.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "esp32_camera.h"
#include "xiao_serial_commands.h"

#include "led/circular_strip.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#include <atomic>
#include <cstdio>
#include <exception>

#include "esp_io_expander_tca95xx_16bit.h"

#define TAG "DF-K10"

// Colours for the transcriber-specific UI (independent of the WeChat theme).
// Navigation = blue (button A), action = amber (button B) — matches the
// deck-asset-k10 companion's badge language.
static const lv_color_t kOnlineGreen  = lv_color_hex(0x25B04A);
static const lv_color_t kOfflineGrey  = lv_color_hex(0x808080);
static const lv_color_t kRecRed       = lv_color_hex(0xC01818);
static const lv_color_t kWhite        = lv_color_hex(0xFFFFFF);
static const lv_color_t kNavBlue      = lv_color_hex(0x2F6FEB);
static const lv_color_t kActionAmber  = lv_color_hex(0xE8930C);

// Hold button B this long to take a photo. The hold itself steadies the
// device; capture fires at the end of the hold while still pressed.
static const uint32_t kPhotoHoldMs = 1000;  // must match btn_b long_press_time

class AgentHubDisplay : public SpiLcdDisplay {
private:
    lv_obj_t* footer_ = nullptr;
    lv_obj_t* btn_b_verb_ = nullptr;   // "START"/"STOP" label inside the B chip
    lv_obj_t* online_dot_ = nullptr;   // header status light: green = hub online
    lv_obj_t* rec_label_ = nullptr;    // "REC MM:SS" banner, shown while recording
    lv_obj_t* flash_overlay_ = nullptr; // white full-screen shutter flash
    lv_obj_t* hold_bar_ = nullptr;      // hold-to-capture progress track
    lv_obj_t* hold_bar_fill_ = nullptr; // its animated amber fill
    lv_timer_t* ui_timer_ = nullptr;
    lv_color_t header_bg_ = {};

    bool recording_shown_ = false;
    bool hold_active_ = false;
    int last_online_ = -1;             // -1 = unknown, forces first paint
    int64_t rec_start_us_ = 0;

    static void UiTimerCb(lv_timer_t* t) {
        static_cast<AgentHubDisplay*>(lv_timer_get_user_data(t))->UiTick();
    }
    static void HideFlashCb(lv_timer_t* t) {
        auto self = static_cast<AgentHubDisplay*>(lv_timer_get_user_data(t));
        lv_obj_add_flag(self->flash_overlay_, LV_OBJ_FLAG_HIDDEN);
    }

    const char* IdleMessage(bool online) const {
        return online
            ? "\xE2\x97\x8F ONLINE\nReady to record\n\nPress  B  to start"
            : "Connecting to Agent Hub\xE2\x80\xA6\n\nCheck Wi-Fi if this persists";
    }

    // 1 Hz-ish paint of the recorder-specific chrome. Runs in the LVGL task
    // context (lock already held).
    void UiTick() {
        auto& app = Application::GetInstance();
        bool rec = app.IsTranscribing();
        int online = app.IsAgentHubOnline() ? 1 : 0;

        if (online != last_online_) {
            last_online_ = online;
            if (online_dot_ != nullptr) {
                lv_obj_set_style_bg_color(online_dot_, online ? kOnlineGreen : kOfflineGrey, 0);
            }
            if (!rec && GetDeviceState() == kDeviceStateIdle && !app.IsListeningPaused()) {
                SetChatMessage("system", IdleMessage(online));
            }
        }

        if (rec && !recording_shown_) {
            recording_shown_ = true;
            rec_start_us_ = esp_timer_get_time();
            lv_obj_set_style_bg_color(top_bar_, kRecRed, 0);
            lv_obj_set_style_bg_opa(top_bar_, LV_OPA_COVER, 0);
            lv_obj_remove_flag(rec_label_, LV_OBJ_FLAG_HIDDEN);
            if (btn_b_verb_ != nullptr) lv_label_set_text(btn_b_verb_, "STOP\nhold: photo");
        } else if (!rec && recording_shown_) {
            recording_shown_ = false;
            lv_obj_set_style_bg_color(top_bar_, header_bg_, 0);
            lv_obj_set_style_bg_opa(top_bar_, LV_OPA_COVER, 0);
            lv_obj_add_flag(rec_label_, LV_OBJ_FLAG_HIDDEN);
            if (btn_b_verb_ != nullptr) lv_label_set_text(btn_b_verb_, "START\nhold: photo");
        }

        if (rec && rec_label_ != nullptr) {
            int s = (int)((esp_timer_get_time() - rec_start_us_) / 1000000);
            char buf[24];
            snprintf(buf, sizeof(buf), "\xE2\x97\x8F REC   %02d:%02d", s / 60, s % 60);
            lv_label_set_text(rec_label_, buf);
        }
    }

    DeviceState GetDeviceState() const { return Application::GetInstance().GetDeviceState(); }

public:
    AgentHubDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                    int width, int height, int offset_x, int offset_y,
                    bool mirror_x, bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                        mirror_x, mirror_y, swap_xy) {
    }

    // Brief white "shutter" flash to confirm a photo was taken. Safe to call
    // from any task.
    void FlashShutter() {
        DisplayLockGuard lock(this);
        if (flash_overlay_ == nullptr) return;
        lv_obj_remove_flag(flash_overlay_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(flash_overlay_);
        lv_timer_t* t = lv_timer_create(HideFlashCb, 150, this);
        lv_timer_set_repeat_count(t, 1);
    }

    static void HoldAnimCb(void* obj, int32_t v) {
        lv_obj_set_width(static_cast<lv_obj_t*>(obj), v);
    }

    // Show the hold-to-capture progress track and fill it over kPhotoHoldMs.
    // Safe to call from any task.
    void BeginPhotoHold() {
        DisplayLockGuard lock(this);
        if (hold_bar_ == nullptr) return;
        hold_active_ = true;
        lv_obj_set_width(hold_bar_fill_, 0);
        lv_obj_remove_flag(hold_bar_, LV_OBJ_FLAG_HIDDEN);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, hold_bar_fill_);
        lv_anim_set_values(&a, 0, LV_HOR_RES);
        lv_anim_set_duration(&a, kPhotoHoldMs);
        lv_anim_set_exec_cb(&a, HoldAnimCb);
        lv_anim_start(&a);
    }

    void CancelPhotoHold() {
        DisplayLockGuard lock(this);
        if (hold_bar_ == nullptr || !hold_active_) return;
        hold_active_ = false;
        lv_anim_delete(hold_bar_fill_, HoldAnimCb);
        lv_obj_add_flag(hold_bar_, LV_OBJ_FLAG_HIDDEN);
    }

    void SetupUI() override {
        if (setup_ui_called_) {
            return;
        }
        SpiLcdDisplay::SetupUI();

        {
            DisplayLockGuard lock(this);
            auto theme = static_cast<LvglTheme*>(current_theme_);
            header_bg_ = theme->background_color();

            // Keep both the static AI glyph and animated emotions inside the
            // header. The base WeChat layout attaches them to the screen,
            // which lets a 128px K10 emotion obscure the transcript.
            auto header_left = lv_obj_create(top_bar_);
            lv_obj_set_size(header_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(header_left, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(header_left, 0, 0);
            lv_obj_set_style_pad_all(header_left, 0, 0);
            lv_obj_set_style_pad_column(header_left, theme->spacing(2), 0);
            lv_obj_set_flex_flow(header_left, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(header_left, LV_FLEX_ALIGN_START,
                                  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            // Hub online/offline status light.
            online_dot_ = lv_obj_create(header_left);
            lv_obj_set_size(online_dot_, 12, 12);
            lv_obj_set_style_radius(online_dot_, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(online_dot_, 0, 0);
            lv_obj_set_style_bg_color(online_dot_, kOfflineGrey, 0);
            lv_obj_set_scrollbar_mode(online_dot_, LV_SCROLLBAR_MODE_OFF);

            lv_obj_set_parent(network_label_, header_left);

            auto emotion_badge = lv_obj_create(header_left);
            lv_obj_set_size(emotion_badge, 26, 26);
            lv_obj_set_style_radius(emotion_badge, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_pad_all(emotion_badge, 0, 0);
            lv_obj_set_style_border_width(emotion_badge, 0, 0);
            lv_obj_set_style_bg_opa(emotion_badge, LV_OPA_TRANSP, 0);
            lv_obj_set_scrollbar_mode(emotion_badge, LV_SCROLLBAR_MODE_OFF);

            lv_obj_set_parent(emoji_label_, emotion_badge);
            lv_obj_set_style_text_font(emoji_label_, theme->icon_font()->font(), 0);
            lv_obj_center(emoji_label_);

            lv_obj_set_parent(emoji_image_, emotion_badge);
            // DF-K10 uses the 128px Noto collection; 48/256 scales it to 24px.
            lv_image_set_scale(emoji_image_, 48);
            lv_obj_center(emoji_image_);

            // Reparenting the network label leaves the right-icons group as
            // the first child. Restore left/right header order for flex layout.
            lv_obj_move_to_index(header_left, 0);

            // "REC MM:SS" banner: a red bar directly under the header, hidden
            // until a transcription session is streaming.
            rec_label_ = lv_label_create(container_);
            lv_obj_set_width(rec_label_, LV_HOR_RES);
            lv_obj_set_style_bg_color(rec_label_, kRecRed, 0);
            lv_obj_set_style_bg_opa(rec_label_, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(rec_label_, kWhite, 0);
            lv_obj_set_style_text_align(rec_label_, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_pad_ver(rec_label_, theme->spacing(2), 0);
            lv_obj_set_style_radius(rec_label_, 0, 0);
            lv_label_set_text(rec_label_, "\xE2\x97\x8F REC   00:00");
            lv_obj_add_flag(rec_label_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(rec_label_, 1);  // right after top_bar_

            footer_ = lv_obj_create(container_);
            lv_obj_set_size(footer_, LV_HOR_RES, 54);
            lv_obj_set_style_radius(footer_, 0, 0);
            lv_obj_set_style_pad_all(footer_, theme->spacing(2), 0);
            lv_obj_set_style_border_width(footer_, 1, 0);
            lv_obj_set_style_border_side(footer_, LV_BORDER_SIDE_TOP, 0);
            lv_obj_set_style_border_color(footer_, theme->border_color(), 0);
            lv_obj_set_style_bg_color(footer_, theme->background_color(), 0);
            lv_obj_set_flex_flow(footer_, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(footer_, LV_FLEX_ALIGN_SPACE_EVENLY,
                                  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_scrollbar_mode(footer_, LV_SCROLLBAR_MODE_OFF);

            // Footer control chips: a coloured [A]/[B] badge + verb + hold hint.
            auto add_button_chip = [this, theme](const char* letter, lv_color_t badge,
                                                 const char* text) -> lv_obj_t* {
                auto chip = lv_obj_create(footer_);
                lv_obj_set_size(chip, LV_HOR_RES / 2 - theme->spacing(4), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(chip, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(chip, 0, 0);
                lv_obj_set_style_pad_all(chip, 0, 0);
                lv_obj_set_style_pad_column(chip, theme->spacing(2), 0);
                lv_obj_set_flex_flow(chip, LV_FLEX_FLOW_ROW);
                lv_obj_set_flex_align(chip, LV_FLEX_ALIGN_CENTER,
                                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                lv_obj_set_scrollbar_mode(chip, LV_SCROLLBAR_MODE_OFF);

                auto badge_obj = lv_label_create(chip);
                lv_obj_set_style_radius(badge_obj, 4, 0);
                lv_obj_set_style_bg_color(badge_obj, badge, 0);
                lv_obj_set_style_bg_opa(badge_obj, LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(badge_obj, kWhite, 0);
                lv_obj_set_style_pad_hor(badge_obj, theme->spacing(2), 0);
                lv_obj_set_style_pad_ver(badge_obj, theme->spacing(1), 0);
                lv_label_set_text(badge_obj, letter);

                auto verb = lv_label_create(chip);
                lv_label_set_long_mode(verb, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_color(verb, theme->text_color(), 0);
                lv_label_set_text(verb, text);
                return verb;
            };
            add_button_chip("A", kNavBlue, "PAUSE\nhold: vol-");
            btn_b_verb_ = add_button_chip("B", kActionAmber, "START\nhold: photo");

            // Hold-to-capture progress track: pinned to the top edge of the
            // footer, hidden until button B is held.
            hold_bar_ = lv_obj_create(footer_);
            lv_obj_set_size(hold_bar_, LV_HOR_RES, 4);
            lv_obj_add_flag(hold_bar_, LV_OBJ_FLAG_IGNORE_LAYOUT);
            lv_obj_align(hold_bar_, LV_ALIGN_TOP_MID, 0, -theme->spacing(2));
            lv_obj_set_style_radius(hold_bar_, 0, 0);
            lv_obj_set_style_border_width(hold_bar_, 0, 0);
            lv_obj_set_style_pad_all(hold_bar_, 0, 0);
            lv_obj_set_style_bg_color(hold_bar_, theme->border_color(), 0);
            lv_obj_set_scrollbar_mode(hold_bar_, LV_SCROLLBAR_MODE_OFF);
            hold_bar_fill_ = lv_obj_create(hold_bar_);
            lv_obj_set_size(hold_bar_fill_, 0, 4);
            lv_obj_set_pos(hold_bar_fill_, 0, 0);
            lv_obj_set_style_radius(hold_bar_fill_, 0, 0);
            lv_obj_set_style_border_width(hold_bar_fill_, 0, 0);
            lv_obj_set_style_bg_color(hold_bar_fill_, kActionAmber, 0);
            lv_obj_add_flag(hold_bar_, LV_OBJ_FLAG_HIDDEN);

            // Full-screen white shutter flash, on top of everything, hidden.
            flash_overlay_ = lv_obj_create(lv_obj_get_screen(container_));
            lv_obj_set_size(flash_overlay_, LV_HOR_RES, LV_VER_RES);
            lv_obj_set_pos(flash_overlay_, 0, 0);
            lv_obj_set_style_bg_color(flash_overlay_, kWhite, 0);
            lv_obj_set_style_bg_opa(flash_overlay_, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(flash_overlay_, 0, 0);
            lv_obj_add_flag(flash_overlay_, LV_OBJ_FLAG_IGNORE_LAYOUT);
            lv_obj_add_flag(flash_overlay_, LV_OBJ_FLAG_HIDDEN);

            ui_timer_ = lv_timer_create(UiTimerCb, 500, this);
        }

        SetChatMessage("system", IdleMessage(false));
    }

    void ClearChatMessages() override {
        SpiLcdDisplay::ClearChatMessages();
        SetChatMessage("system",
                       IdleMessage(Application::GetInstance().IsAgentHubOnline()));
    }
};

class Df_K10Board : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    esp_io_expander_handle_t io_expander_ = nullptr;
    LcdDisplay* display_ = nullptr;
    button_handle_t btn_a_ = nullptr;
    button_handle_t btn_b_ = nullptr;
    Esp32Camera* camera_ = nullptr;
    std::atomic<bool> photo_capture_in_progress_{false};

    button_driver_t* btn_a_driver_ = nullptr;
    button_driver_t* btn_b_driver_ = nullptr;

    CircularStrip* led_strip_;

    static Df_K10Board* instance_;

    static void CaptureTranscriptPhotoTask(void* arg) {
        auto self = static_cast<Df_K10Board*>(arg);
        auto display = self->GetDisplay();
        auto agent_display = static_cast<AgentHubDisplay*>(self->display_);

        try {
            if (!self->camera_ || !self->camera_->Capture()) {
                display->ShowNotification("Photo capture failed");
            } else {
                if (agent_display != nullptr) agent_display->FlashShutter();
                self->camera_->UploadTranscriptSnapshot();
                display->ShowNotification("Photo added to transcript", 3000);
            }
        } catch (const std::exception& error) {
            ESP_LOGE(TAG, "Transcript photo failed: %s", error.what());
            display->ShowNotification("Photo upload failed");
        }

        self->photo_capture_in_progress_.store(false);
        vTaskDelete(nullptr);
    }

    void CaptureTranscriptPhoto() {
        if (photo_capture_in_progress_.exchange(true)) {
            GetDisplay()->ShowNotification("Photo already in progress");
            return;
        }

        BaseType_t created = xTaskCreate(CaptureTranscriptPhotoTask,
                                         "transcript_photo", 8192, this, 1, nullptr);
        if (created != pdPASS) {
            photo_capture_in_progress_.store(false);
            ESP_LOGE(TAG, "Failed to create transcript photo task");
            GetDisplay()->ShowNotification("Unable to start camera");
        }
    }

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
        iot_button_register_cb(btn_b_, BUTTON_DOUBLE_CLICK, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                self->EnterWifiConfigMode();
                return;
            }
            self->CaptureTranscriptPhoto();  // quick alternative to hold-to-capture
        }, this);
        // Hold B to take a photo: the press-and-hold steadies the device, a
        // progress bar fills over kPhotoHoldMs, and the capture fires at the end
        // of the hold while still pressed.
        iot_button_register_cb(btn_b_, BUTTON_PRESS_DOWN, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting || app.IsListeningPaused()) {
                return;
            }
            static_cast<AgentHubDisplay*>(self->display_)->BeginPhotoHold();
        }, this);
        iot_button_register_cb(btn_b_, BUTTON_LONG_PRESS_START, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting || app.IsListeningPaused()) {
                return;
            }
            self->CaptureTranscriptPhoto();
        }, this);
        iot_button_register_cb(btn_b_, BUTTON_PRESS_UP, nullptr, [](void* button_handle, void* usr_data) {
            auto self = static_cast<Df_K10Board*>(usr_data);
            static_cast<AgentHubDisplay*>(self->display_)->CancelPhotoHold();
        }, this);
    }

    void InitializeCamera() {
        camera_config_t camera_config = {};
        camera_config.pin_pwdn = CAMERA_PIN_PWDN;
        camera_config.pin_reset = CAMERA_PIN_RESET;
        camera_config.pin_xclk = CAMERA_PIN_XCLK;
        camera_config.pin_sccb_sda = -1;
        camera_config.pin_sccb_scl = -1;
        camera_config.pin_d7 = CAMERA_PIN_D9;
        camera_config.pin_d6 = CAMERA_PIN_D8;
        camera_config.pin_d5 = CAMERA_PIN_D7;
        camera_config.pin_d4 = CAMERA_PIN_D6;
        camera_config.pin_d3 = CAMERA_PIN_D5;
        camera_config.pin_d2 = CAMERA_PIN_D4;
        camera_config.pin_d1 = CAMERA_PIN_D3;
        camera_config.pin_d0 = CAMERA_PIN_D2;
        camera_config.pin_vsync = CAMERA_PIN_VSYNC;
        camera_config.pin_href = CAMERA_PIN_HREF;
        camera_config.pin_pclk = CAMERA_PIN_PCLK;
        camera_config.xclk_freq_hz = XCLK_FREQ_HZ;
        camera_config.ledc_timer = LEDC_TIMER_0;
        camera_config.ledc_channel = LEDC_CHANNEL_0;
        camera_config.pixel_format = PIXFORMAT_RGB565;
        camera_config.frame_size = FRAMESIZE_VGA;
        camera_config.jpeg_quality = 12;
        camera_config.fb_count = 1;
        camera_config.fb_location = CAMERA_FB_IN_PSRAM;
        camera_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        camera_config.sccb_i2c_port = 1;
        camera_ = new Esp32Camera(camera_config);
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
