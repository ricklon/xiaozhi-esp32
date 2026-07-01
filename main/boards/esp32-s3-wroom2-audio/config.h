#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio sample rates
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  16000

// Espressif ESP32-S3-DevKitC-1 (ESP32-S3-WROOM-2 module) — external I2S mic
// (INMP441) + amp (MAX98357A), no camera. The DevKitC-1 silkscreen labels are
// real GPIO numbers, so these #defines map 1:1 to the header pads. GPIO4-7 are
// plain GPIOs on this board (not strapping/USB/UART, and clear of the octal
// flash+PSRAM pins GPIO33-37 that the WROOM-2 module consumes internally).
// Wiring on this board:
//   DOUT (GPIO7) -> amp DIN      (I2S data to speaker amp)
//   BCLK (GPIO5) -> bit clock / SCLK (shared mic + amp)
//   WS   (GPIO6) -> word select / LRCLK (shared mic + amp)
//   DIN  (GPIO4) <- mic DOUT     (I2S data from microphone)
// INMP441 note: tie the mic's L/R (SEL) pin to GND so it drives the left slot;
// left floating or tied to VDD gives MIC peak = 0.
#define AUDIO_I2S_GPIO_MCLK   GPIO_NUM_NC
#define AUDIO_I2S_GPIO_BCLK   GPIO_NUM_5
#define AUDIO_I2S_GPIO_WS     GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT   GPIO_NUM_7
#define AUDIO_I2S_GPIO_DIN    GPIO_NUM_4

// Boot button (S3 onboard button, active-low)
#define BOOT_BUTTON_GPIO      GPIO_NUM_0

// Known Wi-Fi networks — stored in NVS at boot.
// Add networks here or use the serial "!wifi SSID PASSWORD" command.
// Keep the sentinel {nullptr, nullptr} at the end.
#define WIFI_NETWORKS { \
    {nullptr, nullptr} \
}

#endif // _BOARD_CONFIG_H_
