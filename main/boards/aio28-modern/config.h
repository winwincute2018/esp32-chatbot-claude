#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

// ============================================================================
//  Board: AIDEEPEN ESP32-S3 2.8" LCD 240x320 Capacitive Touch (ES3C28P)
//  Nguon: tài liệu chính thức lcdwiki.com ES3C28P_ESP-IDF_Demo_Instructions
//  Chip: ES8311 + ILI9341V + FT6336G + SDMMC
// ============================================================================

#include <driver/gpio.h>

// ---- Audio ES8311 (I2S) ----
#define AUDIO_INPUT_SAMPLE_RATE   24000
#define AUDIO_OUTPUT_SAMPLE_RATE  24000

#define AUDIO_I2S_GPIO_MCLK  GPIO_NUM_4    // I2S_MCK
#define AUDIO_I2S_GPIO_BCLK  GPIO_NUM_5    // I2S_SCK
#define AUDIO_I2S_GPIO_WS    GPIO_NUM_7    // I2S_LRC
#define AUDIO_I2S_GPIO_DOUT  GPIO_NUM_8    // I2S_DO
#define AUDIO_I2S_GPIO_DIN   GPIO_NUM_6    // I2S_DI

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_1    // Audio_EN (LOW = enable)
#define AUDIO_CODEC_PA_INVERT    true          // PA bat khi LOW
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_16   // TP_SDA (dung chung voi touch)
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_15   // TP_SCL (dung chung voi touch)
#define AUDIO_CODEC_ES8311_ADDR  0x18          // IIC Slave address tu schematic

// ---- Man hinh ILI9341V (SPI) ----
#define DISPLAY_SPI_SCK_PIN   GPIO_NUM_12   // TFT_SCK
#define DISPLAY_SPI_MOSI_PIN  GPIO_NUM_11   // TFT_MOSI
#define DISPLAY_SPI_MISO_PIN  GPIO_NUM_13   // TFT_MISO
#define DISPLAY_DC_PIN        GPIO_NUM_46   // TFT_RS (Data/Command)
#define DISPLAY_SPI_CS_PIN    GPIO_NUM_10   // TFT_CS
#define DISPLAY_RST_PIN       GPIO_NUM_NC   // TFT_RST = CHIP_PU (reset chung)
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_45   // TFT_BL

#define DISPLAY_WIDTH               240
#define DISPLAY_HEIGHT              320
#define DISPLAY_SWAP_XY             false
#define DISPLAY_MIRROR_X            false
#define DISPLAY_MIRROR_Y            false
#define DISPLAY_OFFSET_X            0
#define DISPLAY_OFFSET_Y            0
#define DISPLAY_INVERT_COLOR        false
#define DISPLAY_RGB_ORDER_BGR       false     // ILI9341V: RGB
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false // BL HIGH = bat sang

// ---- Cam ung FT6336G (I2C, dung chung bus voi ES8311) ----
#define TOUCH_I2C_ADDR    0x38
#define TOUCH_RST_PIN     GPIO_NUM_18   // TP_RST
#define TOUCH_INT_PIN     GPIO_NUM_17   // TP_INT
// SDA/SCL dung chung voi codec: GPIO_NUM_16 / GPIO_NUM_15

// ---- SD Card (SDMMC 4-bit) ----
#define SD_CLK_PIN   GPIO_NUM_38
#define SD_CMD_PIN   GPIO_NUM_40
#define SD_D0_PIN    GPIO_NUM_39
#define SD_D1_PIN    GPIO_NUM_41
#define SD_D2_PIN    GPIO_NUM_48
#define SD_D3_PIN    GPIO_NUM_47

// ---- Nut bam ----
#define BOOT_BUTTON_GPIO  GPIO_NUM_0    // BOOT_KEY

// ---- RGB LED ----
#define RGB_LED_GPIO  GPIO_NUM_42       // RGB_INT (WS2812 / single-wire)

// ---- Pin (ADC) ----
#define BATTERY_ADC_PIN  GPIO_NUM_9     // BAT_ADC

// ---- USB ----
// GPIO_NUM_19 = USB_N, GPIO_NUM_20 = USB_P (USB OTG, khong can khai bao them)

#endif // _BOARD_CONFIG_H_
