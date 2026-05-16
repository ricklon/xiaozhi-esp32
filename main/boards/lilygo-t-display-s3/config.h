#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio sample rates
#define AUDIO_INPUT_SAMPLE_RATE   16000
#define AUDIO_OUTPUT_SAMPLE_RATE  24000

// External I2S microphone (INMP441 or similar)
// Wire: SCK→GPIO17, WS→GPIO18, SD→GPIO21
#define AUDIO_I2S_MIC_GPIO_SCK    GPIO_NUM_17
#define AUDIO_I2S_MIC_GPIO_WS     GPIO_NUM_18
#define AUDIO_I2S_MIC_GPIO_DIN    GPIO_NUM_21

// External I2S amplifier (MAX98357A or similar)
// Wire: BCLK→GPIO15, LRC→GPIO16, DIN→GPIO3
#define AUDIO_I2S_SPK_GPIO_BCLK   GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK   GPIO_NUM_16
#define AUDIO_I2S_SPK_GPIO_DOUT   GPIO_NUM_3

// Buttons
#define BOOT_BUTTON_GPIO          GPIO_NUM_0

// No built-in LED
#define BUILTIN_LED_GPIO          GPIO_NUM_NC

// ST7789V 1.9" 170x320 display — i8080 8-bit parallel bus
#define DISPLAY_WIDTH    170
#define DISPLAY_HEIGHT   320

#define LCD_D0  GPIO_NUM_39
#define LCD_D1  GPIO_NUM_40
#define LCD_D2  GPIO_NUM_41
#define LCD_D3  GPIO_NUM_42
#define LCD_D4  GPIO_NUM_45
#define LCD_D5  GPIO_NUM_46
#define LCD_D6  GPIO_NUM_47
#define LCD_D7  GPIO_NUM_48
#define LCD_WR  GPIO_NUM_8
#define LCD_RD  GPIO_NUM_9
#define LCD_DC  GPIO_NUM_7
#define LCD_CS  GPIO_NUM_6
#define LCD_RST GPIO_NUM_5
#define LCD_BL  GPIO_NUM_38

// ST7789V in a 170-wide panel has a 35-pixel offset inside the 240-column frame buffer
#define DISPLAY_OFFSET_X  35
#define DISPLAY_OFFSET_Y  0

#define DISPLAY_MIRROR_X      false
#define DISPLAY_MIRROR_Y      false
#define DISPLAY_SWAP_XY       false
#define DISPLAY_INVERT_COLOR  true

#define DISPLAY_BACKLIGHT_PIN            LCD_BL
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT  false

#endif // _BOARD_CONFIG_H_
