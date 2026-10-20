#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio sample rates — duplex I2S shares one clock, both must match
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// External I2S audio (INMP441 mic + MAX98357A amp). Every Coglet is wired
// pad-for-pad like the S3 Coglet (ricklon/coglet hardware/WIRING.md); only the
// GPIO numbers differ. C3 pads: D0=GPIO2, D1=GPIO3, D2=GPIO4, D3=GPIO5,
// D4=GPIO6, D5=GPIO7, D10=GPIO10.
//   speaker = D0 (amp DIN) + D1 (BCLK) + D2 (WS)
//   mic     = D3 (mic SD)  + D1 (BCLK) + D2 (WS)
// Swapping D2 and D3 makes the amp run without an LRC (it hisses) and the mic
// sample a clock line instead of audio.
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_4   // D2, shared
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_3   // D1, shared
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_2   // D0 -> amp DIN
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_5   // D3 <- mic SD

// Boot button (XIAO C3 onboard button, active-low)
#define BOOT_BUTTON_GPIO      GPIO_NUM_9

// No user-addressable LED on the XIAO ESP32-C3.
#define BUILTIN_LED_GPIO      GPIO_NUM_NC

// Known Wi-Fi networks — stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
// Keep the sentinel {nullptr, nullptr} at the end.
#define WIFI_NETWORKS { \
    {nullptr, nullptr} \
}

// PCA9685 servo driver. The ESP32-C3 has a single I2C controller, so this is
// I2C_NUM_0 — unlike the S3 Coglet, which puts servos on controller 1 to keep
// controller 0 free for the camera SCCB bus. There is no camera here.
// SDA on D4 (GPIO6), SCL on D5 (GPIO7): the XIAO default I2C pads, and the
// same pads as the S3 Coglet. Swapped, the bus goes entirely silent: no address
// in 0x08-0x77 answers.
#define SERVO_I2C_PORT        I2C_NUM_0
#define SERVO_I2C_SDA_PIN     GPIO_NUM_6   // D4
#define SERVO_I2C_SCL_PIN     GPIO_NUM_7   // D5
// /OE on D10=GPIO10. GPIO8 and GPIO9 are strapping pins on the C3 (GPIO9 is
// also the boot button), so neither is usable as a reset-safe output enable.
#define SERVO_OE_PIN          GPIO_NUM_10
#define SERVO_PCA9685_ADDR    0x40

#endif // _BOARD_CONFIG_H_
