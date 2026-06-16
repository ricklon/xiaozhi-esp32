#ifndef _PCA9685_H_
#define _PCA9685_H_

#include "i2c_device.h"

// Driver for the NXP PCA9685 16-channel, 12-bit I2C PWM controller
// (e.g. the Adafruit 16-channel servo driver, product 815).
//
// It is configured for a 50 Hz frame so each channel can drive a standard
// hobby servo: callers set the high-time of the frame in microseconds, which
// for a typical servo ranges ~500 us (one end) to ~2500 us (the other).
//
// Servo power must come from the board's V+ terminal on a separate supply with
// a common ground; the ESP32 only provides logic power and the I2C bus.
class Pca9685 : public I2cDevice {
public:
    Pca9685(i2c_master_bus_handle_t i2c_bus, uint8_t addr = 0x40);

    // Set the high-time of one channel (0..15) in microseconds within the
    // 20000 us (50 Hz) frame. Values are clamped to a valid frame. A no-op if
    // the controller is not present (see Available()).
    void SetPulseUs(int channel, int pulse_us);

    // True once the controller acknowledged on the I2C bus and was configured.
    // When false (board absent, unpowered, or miswired) all writes are skipped
    // so the rest of the device still runs instead of panicking.
    bool Available() const { return available_; }

private:
    // Fault-tolerant register write: returns false on any I2C error instead of
    // aborting, so a missing/unreachable servo board does not brick the device.
    bool TryWriteReg(uint8_t reg, uint8_t value);
    void SetPwm(int channel, uint16_t on, uint16_t off);

    bool available_ = false;
};

#endif // _PCA9685_H_
