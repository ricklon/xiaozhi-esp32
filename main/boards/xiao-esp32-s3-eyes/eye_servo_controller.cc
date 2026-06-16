#include "eye_servo_controller.h"

#include "config.h"
#include "settings.h"

#include <algorithm>

#include <esp_log.h>

#define TAG "EyeServoController"

namespace {
// Standard hobby-servo pulse range. 0 degrees -> 500 us, 180 degrees -> 2500 us.
constexpr int kServoMinPulseUs = 500;
constexpr int kServoMaxPulseUs = 2500;

int AngleToPulseUs(int angle) {
    return kServoMinPulseUs +
           (angle * (kServoMaxPulseUs - kServoMinPulseUs) / 180);
}
}  // namespace

EyeServoController::EyeServoController(i2c_master_bus_handle_t i2c_bus, uint8_t pca_addr,
                                       int eye_left_channel, int eye_right_channel,
                                       int eyelid_left_channel, int eyelid_right_channel)
    : pca_(i2c_bus, pca_addr),
      channels_({eye_left_channel, eye_right_channel,
                 eyelid_left_channel, eyelid_right_channel}),
      angles_({EYE_SERVO_CENTER_ANGLE, EYE_SERVO_CENTER_ANGLE,
               EYELID_OPEN_ANGLE, EYELID_OPEN_ANGLE}),
      trims_({0, 0, 0, 0}) {
    LoadTrims();
    Center();
}

void EyeServoController::LoadTrims() {
    Settings settings("eye_servos", false);
    trims_[kEyeLeft] = settings.GetInt("eye_left", 0);
    trims_[kEyeRight] = settings.GetInt("eye_right", 0);
    trims_[kEyelidLeft] = settings.GetInt("eyelid_left", 0);
    trims_[kEyelidRight] = settings.GetInt("eyelid_right", 0);
}

void EyeServoController::SetEyes(int left_angle, int right_angle) {
    WriteServo(kEyeLeft, left_angle);
    WriteServo(kEyeRight, right_angle);
}

void EyeServoController::SetEyelids(int left_angle, int right_angle) {
    WriteServo(kEyelidLeft, left_angle);
    WriteServo(kEyelidRight, right_angle);
}

void EyeServoController::Center() {
    SetEyes(EYE_SERVO_CENTER_ANGLE, EYE_SERVO_CENTER_ANGLE);
    OpenEyelids();
}

void EyeServoController::OpenEyelids() {
    SetEyelids(EYELID_OPEN_ANGLE, EYELID_OPEN_ANGLE);
}

void EyeServoController::CloseEyelids() {
    SetEyelids(EYELID_CLOSED_ANGLE, EYELID_CLOSED_ANGLE);
}

bool EyeServoController::SetTrim(const std::string& servo_name, int trim) {
    int index = IndexForName(servo_name);
    if (index < 0) {
        return false;
    }

    trims_[index] = trim;
    Settings settings("eye_servos", true);
    settings.SetInt(NameForIndex(static_cast<ServoIndex>(index)), trim);
    WriteServo(static_cast<ServoIndex>(index), angles_[index]);
    return true;
}

std::string EyeServoController::GetStateJson() const {
    return std::string("{\"eye_left\":") + std::to_string(angles_[kEyeLeft]) +
           ",\"eye_right\":" + std::to_string(angles_[kEyeRight]) +
           ",\"eyelid_left\":" + std::to_string(angles_[kEyelidLeft]) +
           ",\"eyelid_right\":" + std::to_string(angles_[kEyelidRight]) +
           ",\"trim_eye_left\":" + std::to_string(trims_[kEyeLeft]) +
           ",\"trim_eye_right\":" + std::to_string(trims_[kEyeRight]) +
           ",\"trim_eyelid_left\":" + std::to_string(trims_[kEyelidLeft]) +
           ",\"trim_eyelid_right\":" + std::to_string(trims_[kEyelidRight]) + "}";
}

void EyeServoController::WriteServo(ServoIndex index, int angle) {
    int clamped_angle = ClampAngle(angle);
    angles_[index] = clamped_angle;

    int output_angle = ClampAngle(clamped_angle + trims_[index]);
    pca_.SetPulseUs(channels_[index], AngleToPulseUs(output_angle));
    ESP_LOGD(TAG, "%s angle=%d trim=%d output=%d", NameForIndex(index), clamped_angle,
             trims_[index], output_angle);
}

int EyeServoController::ClampAngle(int angle) const {
    return std::min(std::max(angle, 0), 180);
}

int EyeServoController::IndexForName(const std::string& servo_name) const {
    for (int i = 0; i < kServoCount; ++i) {
        if (servo_name == NameForIndex(static_cast<ServoIndex>(i))) {
            return i;
        }
    }
    return -1;
}

const char* EyeServoController::NameForIndex(ServoIndex index) const {
    switch (index) {
        case kEyeLeft:
            return "eye_left";
        case kEyeRight:
            return "eye_right";
        case kEyelidLeft:
            return "eyelid_left";
        case kEyelidRight:
            return "eyelid_right";
        default:
            return "unknown";
    }
}
