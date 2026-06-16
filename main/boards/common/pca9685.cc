#include "pca9685.h"

#include <algorithm>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "Pca9685"

namespace {
// PCA9685 register map (datasheet section 7.3).
constexpr uint8_t kRegMode1 = 0x00;
constexpr uint8_t kRegLed0OnL = 0x06;  // first of 4 bytes per channel
constexpr uint8_t kRegPreScale = 0xFE;

constexpr uint8_t kMode1Sleep = 0x10;     // oscillator off; required to set prescale
constexpr uint8_t kMode1AutoInc = 0x20;   // register auto-increment
constexpr uint8_t kMode1Restart = 0x80;   // restart PWM channels after wake

constexpr int kPwmResolution = 4096;      // 12-bit counts per frame
constexpr int kFrameUs = 20000;           // 50 Hz refresh
// prescale = round(25 MHz / (4096 * 50 Hz)) - 1
constexpr uint8_t kPreScale50Hz = 121;
}  // namespace

Pca9685::Pca9685(i2c_master_bus_handle_t i2c_bus, uint8_t addr)
    : I2cDevice(i2c_bus, addr) {
    // The prescaler can only be programmed while the oscillator is asleep.
    WriteReg(kRegMode1, kMode1Sleep);
    WriteReg(kRegPreScale, kPreScale50Hz);
    WriteReg(kRegMode1, kMode1AutoInc);
    vTaskDelay(pdMS_TO_TICKS(1));  // oscillator needs >= 500 us to stabilize
    WriteReg(kRegMode1, kMode1AutoInc | kMode1Restart);
    ESP_LOGI(TAG, "PCA9685 @0x%02X ready at 50 Hz", addr);
}

void Pca9685::SetPwm(int channel, uint16_t on, uint16_t off) {
    uint8_t base = kRegLed0OnL + 4 * channel;
    WriteReg(base + 0, on & 0xFF);
    WriteReg(base + 1, (on >> 8) & 0x0F);
    WriteReg(base + 2, off & 0xFF);
    WriteReg(base + 3, (off >> 8) & 0x0F);
}

void Pca9685::SetPulseUs(int channel, int pulse_us) {
    pulse_us = std::min(std::max(pulse_us, 0), kFrameUs);
    int off = pulse_us * kPwmResolution / kFrameUs;
    if (off >= kPwmResolution) {
        off = kPwmResolution - 1;
    }
    // Each frame starts high at count 0 and falls at 'off'.
    SetPwm(channel, 0, static_cast<uint16_t>(off));
}
