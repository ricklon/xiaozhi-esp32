#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio sample rates
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// Every Coglet is wired pad-for-pad the same; only the GPIO numbers differ per
// board. This S3 layout is the reference (ricklon/coglet hardware/WIRING.md):
//   D0 amp DIN, D1 BCLK, D2 WS, D3 mic SD, D4 PCA SDA, D5 PCA SCL, D10 /OE.
// XIAO S3 pads: D0=GPIO1, D1=GPIO2, D2=GPIO3, D3=GPIO4, D4=GPIO5, D5=GPIO6, D10=GPIO9
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_3   // D2, shared
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_2   // D1, shared
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_1   // D0 -> amp DIN
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_4   // D3 <- mic SD

// Boot button (XIAO S3 onboard button, active-low)
#define BOOT_BUTTON_GPIO      GPIO_NUM_0

// User LED on XIAO ESP32-S3
#define BUILTIN_LED_GPIO      GPIO_NUM_21

// Sense camera connector (sensor must be confirmed) — XIAO ESP32-S3 Sense built-in pinout
#define CAMERA_PIN_PWDN   GPIO_NUM_NC
#define CAMERA_PIN_RESET  GPIO_NUM_NC
#define CAMERA_PIN_XCLK   GPIO_NUM_10
#define CAMERA_PIN_SIOD   GPIO_NUM_40
#define CAMERA_PIN_SIOC   GPIO_NUM_39
#define CAMERA_PIN_D7     GPIO_NUM_48
#define CAMERA_PIN_D6     GPIO_NUM_11
#define CAMERA_PIN_D5     GPIO_NUM_12
#define CAMERA_PIN_D4     GPIO_NUM_14
#define CAMERA_PIN_D3     GPIO_NUM_16
#define CAMERA_PIN_D2     GPIO_NUM_18
#define CAMERA_PIN_D1     GPIO_NUM_17
#define CAMERA_PIN_D0     GPIO_NUM_15
#define CAMERA_PIN_VSYNC  GPIO_NUM_38
#define CAMERA_PIN_HREF   GPIO_NUM_47
#define CAMERA_PIN_PCLK   GPIO_NUM_13
#define XCLK_FREQ_HZ      20000000

// Known Wi-Fi networks — stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
// Keep the sentinel {nullptr, nullptr} at the end.
#define WIFI_NETWORKS { \
    {nullptr, nullptr} \
}

// Servo I2C is controller 1 so the camera's SCCB keeps controller 0.
#define SERVO_I2C_PORT I2C_NUM_1
#define SERVO_I2C_SDA_PIN GPIO_NUM_5   // D4
#define SERVO_I2C_SCL_PIN GPIO_NUM_6   // D5
#define SERVO_OE_PIN GPIO_NUM_9        // D10; not the camera's XCLK (GPIO10)
#define SERVO_PCA9685_ADDR 0x40

#endif // _BOARD_CONFIG_H_
