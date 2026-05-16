#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio sample rates — duplex I2S shares one clock, both must match
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// MAX98357A I2S amplifier — speaker only, no microphone
// Wire: BCLK→GPIO3, LRC→GPIO4, DIN→GPIO2
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_3
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_2
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_5

// Wi-Fi credentials (provisioned at first boot, stored in NVS)
#define WIFI_SSID     "dogeden-5g"
#define WIFI_PASSWORD "rumi1234mayim"

// Boot button (XIAO C3 onboard button)
#define BOOT_BUTTON_GPIO      GPIO_NUM_9

// No built-in LED (GPIO2 is used for WS2812 on XIAO C3 but not wired here)
#define BUILTIN_LED_GPIO      GPIO_NUM_NC

#endif // _BOARD_CONFIG_H_
