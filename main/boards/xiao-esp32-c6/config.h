#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// External I2S audio — same physical pad positions as XIAO ESP32-C3.
// XIAO C6 pads D0-D3 share the same GPIO numbers as C3: D0=GPIO2, D1=GPIO3, D2=GPIO4, D3=GPIO5
// Wire: D0(GPIO2)→DOUT(speaker), D1(GPIO3)→BCLK, D2(GPIO4)→WS/LRC, D3(GPIO5)→DIN(mic)
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_3
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_2
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_5

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
