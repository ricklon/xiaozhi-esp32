#pragma once

#include <array>
#include <cmath>
#include <mutex>
#include <string>
#include "driver/i2c_master.h"
#include "esp_http_server.h"
#include "cJSON.h"

// All entry points and the device timer serialize through one mutex.
// No servo feedback: cached values are successful commands only.
class CogletController {
public:
    static CogletController& Instance();
    void Start();
    std::string Command(const std::string& text, bool builder = false);

private:
    struct Axis {
        int channel = -1;
        float low = 90, high = 90; // semantic endpoints; may run backwards
        int min_us = 1000, max_us = 2000, trim_us = 0;
        uint32_t confirmed = 0;
    };
    struct Calibration {
        uint32_t version = 1;
        std::array<Axis, 6> axes;
        float lid_trim = .85f;
        float upper_coeff = .8f;
    } cal_;
    std::mutex mutex_;
    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t device_ = nullptr;
    bool ready_ = false, released_ = true, builder_ = false;
    bool web_enabled_ = false;
    esp_err_t last_error_ = ESP_OK, release_error_ = ESP_OK;
    std::array<float, 6> commanded_;
    std::array<float, 4> pose_ = {.5f, .5f, NAN, NAN}, from_ = pose_;
    int animation_ = -1, frame_ = 0;
    int64_t frame_start_ = 0, next_blink_ = 0, blink_until_ = 0;
    esp_err_t Register(uint8_t reg, uint8_t value);
    esp_err_t Probe();
    esp_err_t InitHardware();
    esp_err_t Release();
    esp_err_t Fail(esp_err_t error);
    esp_err_t Write(int axis, float degrees);
    esp_err_t Apply(const std::array<float, 4>& pose);
    esp_err_t Tick(int64_t now);
    bool Valid(const Calibration& cal, bool complete) const;
    esp_err_t Save();
    void Load();
    cJSON* State();
    void Tools();
    void Web();
    static esp_err_t Http(httpd_req_t* req);
};
