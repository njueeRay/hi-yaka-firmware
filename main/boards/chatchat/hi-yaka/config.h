#ifndef _HI_YAKA_CONFIG_H_
#define _HI_YAKA_CONFIG_H_

#include <driver/gpio.h>
#include <driver/i2c_types.h>
#include <esp_adc/adc_oneshot.h>

// M1 uses a dummy codec. These rates are placeholders for the application
// interface and are not the final HiYaKa audio contract.
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// Shared I2C bus from the signed HiYaKa schematic.
#define YAKA_I2C_PORT I2C_NUM_0
#define YAKA_I2C_SDA_GPIO GPIO_NUM_10
#define YAKA_I2C_SCL_GPIO GPIO_NUM_11
#define YAKA_I2C_FREQUENCY_HZ 100000

// Fixed 7-bit addresses on the shared bus.
#define YAKA_ES8311_I2C_ADDRESS 0x18
#define YAKA_PCA9557_I2C_ADDRESS 0x19
#define YAKA_PN532_I2C_ADDRESS 0x24
#define YAKA_ES7210_I2C_ADDRESS 0x41
#define YAKA_QMI8658A_I2C_ADDRESS 0x6A

// PCA9557 register contract. Configuration bit: 1=input, 0=output.
// Outputs in M1: IO0 LCD_BL, IO1 LCD_RST, IO2 MOTOR_CTRL,
// IO6 LCD_CS, IO7 LCD_DC. IO3/IO4/IO5 remain inputs.
// Safe values: backlight off, LCD held in reset, motor off, LCD CS inactive.
#define YAKA_PCA9557_SAFE_OUTPUT 0x40
#define YAKA_PCA9557_SAFE_CONFIGURATION 0x38

// PCA9557 bit contract. LCD CS is active-low; LCD reset is active-low.
#define YAKA_PCA9557_LCD_BL_BIT 0
#define YAKA_PCA9557_LCD_RST_BIT 1
#define YAKA_PCA9557_MOTOR_BIT 2
#define YAKA_PCA9557_LCD_CS_BIT 6
#define YAKA_PCA9557_LCD_DC_BIT 7

// ST7789 serial path uses PARLIO so SPI2 remains dedicated to the TF card.
// CS/DC/RST/BL are not direct MCU GPIOs and must only be driven through
// HiYaKaPca9557.
#define YAKA_LCD_MOSI_GPIO GPIO_NUM_6
#define YAKA_LCD_SCLK_GPIO GPIO_NUM_7
#define YAKA_LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)
#define YAKA_LCD_WIDTH 200
#define YAKA_LCD_HEIGHT 320
#define YAKA_LCD_OFFSET_X 0
#define YAKA_LCD_OFFSET_Y 0

// Direct input and feedback connections.
#define YAKA_DPAD_ADC_UNIT ADC_UNIT_1
#define YAKA_DPAD_ADC_CHANNEL ADC_CHANNEL_4
#define YAKA_KEY_A_GPIO GPIO_NUM_22
#define YAKA_KEY_B_GPIO GPIO_NUM_5
#define YAKA_STATUS_RGB_GPIO GPIO_NUM_8
#define YAKA_LED_ARRAY_GPIO GPIO_NUM_21
#define YAKA_LED_ARRAY_COUNT 5
#define YAKA_BATTERY_ADC_UNIT ADC_UNIT_1
#define YAKA_BATTERY_ADC_CHANNEL ADC_CHANNEL_2

// Audio and storage connections.
#define YAKA_AUDIO_MCLK_GPIO GPIO_NUM_0
#define YAKA_AUDIO_DOUT_GPIO GPIO_NUM_1
#define YAKA_AUDIO_BCLK_GPIO GPIO_NUM_3
#define YAKA_AUDIO_WS_GPIO GPIO_NUM_12
#define YAKA_AUDIO_DIN_GPIO GPIO_NUM_13
#define YAKA_AUDIO_PA_ENABLE_GPIO GPIO_NUM_23
#define YAKA_TF_CS_GPIO GPIO_NUM_15
#define YAKA_TF_MOSI_GPIO GPIO_NUM_18
#define YAKA_TF_SCLK_GPIO GPIO_NUM_19
#define YAKA_TF_MISO_GPIO GPIO_NUM_20

#endif  // _HI_YAKA_CONFIG_H_
