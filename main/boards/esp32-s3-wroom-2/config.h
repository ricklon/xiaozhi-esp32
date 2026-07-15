#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// External I2S audio: INMP441 mic + MAX98357A amp (NoAudioCodecDuplex).
//
// !!! CONFIRM THESE AGAINST THE ACTUAL WIRING BEFORE TRUSTING THE MIC !!!
// Wrong DIN/BCLK/WS is exactly what makes the INMP441 read MIC peak=0.
// These are safe default GPIOs for the ESP32-S3-WROOM-2: they avoid the
// octal flash/PSRAM pins (GPIO26-37), the strapping pins (0/45/46), the
// UART0 pins (43/44) and the native-USB pins (19/20).
//
//   MAX98357A DIN  <- DOUT (GPIO7)
//   MAX98357A BCLK <- BCLK (GPIO5)
//   MAX98357A LRC  <- WS   (GPIO6)
//   INMP441   SD   -> DIN  (GPIO4)
//   INMP441   SCK  <- BCLK (GPIO5)   shared
//   INMP441   WS   <- WS   (GPIO6)   shared
//   INMP441   L/R  -> GND  (LEFT slot, which the firmware reads)
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_6
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_5
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_7
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_4

// Boot button (S3 strapping/boot button, active-low)
#define BOOT_BUTTON_GPIO      GPIO_NUM_0

// Status LED. GPIO48 is the usual addressable-RGB pad on S3 dev boards; if
// this board has a plain LED on another pin, change it here.
#define BUILTIN_LED_GPIO      GPIO_NUM_48

// Known Wi-Fi networks — stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
// Keep the sentinel {nullptr, nullptr} at the end.
#define WIFI_NETWORKS { \
    {nullptr, nullptr} \
}

#endif // _BOARD_CONFIG_H_
