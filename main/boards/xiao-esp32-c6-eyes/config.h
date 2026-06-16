#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// External I2S audio (INMP441 mic + MAX98357A amp).
// XIAO ESP32-C6 GPIO mapping differs from C3! C6 pads: D0=GPIO0, D1=GPIO1, D2=GPIO2, D3=GPIO21.
// Wire: D0(GPIO0)->DOUT(speaker), D1(GPIO1)->BCLK, D2(GPIO2)->WS/LRC, D3(GPIO21)->DIN(mic)
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_2
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_1
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_0
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_21

// Servos are driven by a PCA9685 16-channel I2C PWM board (Adafruit 815), not
// by ESP32 GPIO. The XIAO only supplies logic power and the I2C bus; servo
// power must come from the PCA9685 V+ terminal on a separate 5V supply with a
// common ground. I2C uses the XIAO C6 default SDA/SCL pads: D4=GPIO22, D5=GPIO23.
#define SERVO_I2C_PORT             I2C_NUM_0
#define SERVO_I2C_SDA_PIN          GPIO_NUM_22  // D4
#define SERVO_I2C_SCL_PIN          GPIO_NUM_23  // D5
#define SERVO_PCA9685_ADDR         0x40

// PCA9685 output channel wired to each servo.
#define EYE_LEFT_SERVO_CHANNEL     0
#define EYE_RIGHT_SERVO_CHANNEL    1
#define EYELID_LEFT_SERVO_CHANNEL  2
#define EYELID_RIGHT_SERVO_CHANNEL 3

#define EYE_SERVO_CENTER_ANGLE    90
#define EYELID_OPEN_ANGLE         90
#define EYELID_CLOSED_ANGLE       20

// Boot button (XIAO C6 onboard button)
#define BOOT_BUTTON_GPIO          GPIO_NUM_9

// User LED (XIAO C6 built-in)
#define BUILTIN_LED_GPIO          GPIO_NUM_15

// Known Wi-Fi networks - stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
#define WIFI_NETWORKS { \
    {"732-50-FUBAR", "aquaman13"}, \
    {nullptr, nullptr} \
}

#endif // _BOARD_CONFIG_H_
