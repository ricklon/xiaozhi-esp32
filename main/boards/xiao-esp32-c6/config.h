#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// External I2S audio — XIAO ESP32-C6 GPIO mapping differs from C3!
// C6 pads: D0=GPIO0, D1=GPIO1, D2=GPIO2, D3=GPIO21
// Wire: D0(GPIO0)→DOUT(speaker), D1(GPIO1)→BCLK, D2(GPIO2)→WS/LRC, D3(GPIO21)→DIN(mic)
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_2
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_1
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_0
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_21

// Boot button (XIAO C6 onboard button)
#define BOOT_BUTTON_GPIO      GPIO_NUM_9

// User LED (XIAO C6 built-in)
#define BUILTIN_LED_GPIO      GPIO_NUM_15

// Known Wi-Fi networks — stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
#define WIFI_NETWORKS { \
    {"732-50-FUBAR", "aquaman13"}, \
    {nullptr, nullptr} \
}

#endif // _BOARD_CONFIG_H_
