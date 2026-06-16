#ifndef _EYE_SERVO_CONTROLLER_H_
#define _EYE_SERVO_CONTROLLER_H_

#include <array>
#include <string>

#include "pca9685.h"

// Drives four hobby servos (two eyes, two eyelids) through a PCA9685 over I2C.
// The MCP eye/eyelid tools call SetEyes / SetEyelids / etc.; this class maps a
// 0-180 degree request to a servo pulse width and applies a persisted per-servo
// trim before writing the PCA9685 channel. The public API is backend-agnostic
// so the same tools work regardless of how the PWM is generated.
class EyeServoController {
public:
    EyeServoController(i2c_master_bus_handle_t i2c_bus, uint8_t pca_addr,
                       int eye_left_channel, int eye_right_channel,
                       int eyelid_left_channel, int eyelid_right_channel);

    void SetEyes(int left_angle, int right_angle);
    void SetEyelids(int left_angle, int right_angle);
    void Center();
    void OpenEyelids();
    void CloseEyelids();
    bool SetTrim(const std::string& servo_name, int trim);
    std::string GetStateJson() const;

private:
    enum ServoIndex {
        kEyeLeft = 0,
        kEyeRight,
        kEyelidLeft,
        kEyelidRight,
        kServoCount
    };

    void LoadTrims();
    void WriteServo(ServoIndex index, int angle);
    int ClampAngle(int angle) const;
    int IndexForName(const std::string& servo_name) const;
    const char* NameForIndex(ServoIndex index) const;

    Pca9685 pca_;
    std::array<int, kServoCount> channels_;
    std::array<int, kServoCount> angles_;
    std::array<int, kServoCount> trims_;
};

#endif // _EYE_SERVO_CONTROLLER_H_
