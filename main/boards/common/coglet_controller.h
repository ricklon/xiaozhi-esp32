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
        // v2 collapsed the lid_left/lid_right pair into one shared lids axis;
        // v3 covers the whole nine-servo mechanism, neck and ears included.
        uint32_t version = 3;
        std::array<Axis, 9> axes;
        float lid_trim = .85f;
        float upper_coeff = .8f;
    } cal_;
    std::mutex mutex_;
    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t device_ = nullptr;
    bool ready_ = false, released_ = true, builder_ = false;
    bool web_enabled_ = false;
    esp_err_t last_error_ = ESP_OK, release_error_ = ESP_OK;
    std::array<float, 9> commanded_;
    std::array<float, 3> pose_ = {.5f, .5f, NAN}, from_ = pose_;
    int animation_ = -1, frame_ = 0;
    // Endpoint hunting: the explored axis moves outside its calibrated window,
    // so it is deliberately not reflected in commanded_.
    int exploring_ = -1;
    float explore_angle_ = 90;
    int64_t frame_start_ = 0, next_blink_ = 0, blink_until_ = 0;
    esp_err_t Register(uint8_t reg, uint8_t value);
    esp_err_t Probe();
    esp_err_t InitHardware();
    esp_err_t Release();
    esp_err_t Fail(esp_err_t error);
    esp_err_t Write(int axis, float degrees);
    esp_err_t Pulse(int channel, float us);
    float Micros(const Axis& axis, float degrees) const;
    esp_err_t Apply(const std::array<float, 3>& pose);
    esp_err_t Tick(int64_t now);
    bool Valid(const Calibration& cal, bool complete) const;
    esp_err_t Save();
    void Load();
    cJSON* State();
    void Tools();
    void Web();
    static esp_err_t Http(httpd_req_t* req);
};
