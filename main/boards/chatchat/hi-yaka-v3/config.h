#ifndef _HI_YAKA_V3_CONFIG_H_
#define _HI_YAKA_V3_CONFIG_H_

#include <driver/gpio.h>
#include <driver/i2c_types.h>

// M1 uses a dummy codec. These rates are placeholders for the application
// interface and are not the final YaKa audio contract.
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// Shared I2C bus from the signed YaKa 3.0 schematic.
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

#endif  // _HI_YAKA_V3_CONFIG_H_
