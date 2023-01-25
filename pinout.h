#pragma once
#include "nrf_gpio.h"

#define ECX336CN_CS_N_PIN 6                   // P0.06 SPI Slave Select pin for the display.
#define ECX336CN_XCLR_PIN 15                  // P0.15
#define FLASH_CS_N_PIN 4                      // P0.04 SPI Slave Select pin for the flash.
#define FPGA_CS_N_PIN 8                       // P0.08 SPI Slave Select pin for the FPGA.
#define FPGA_INT_PIN 5                        // P0.05 FPGA Interrupt pin, to hold low in init.
#define FPGA_MODE1_PIN FPGA_CS_N_PIN          // Alias
#define FPGA_RECONFIG_N_PIN FPGA_INT_PIN      // Alias
#define PMIC_TOUCH_I2C_SCL_PIN 17             // P0.17 I2C clock GPIO pin.
#define PMIC_TOUCH_I2C_SDA_PIN 13             // P0.13 I2C data GPIO pin.
#define CAMERA_I2C_SCL_PIN 18                 // P0.18 I2C clock GPIO pin.
#define CAMERA_I2C_SDA_PIN 16                 // P0.16 I2C data GPIO pin.
#define TOUCH_INTERRUPT_PIN 2                // P0.02 Interrupt pin.
#define MAX77654_ADC_PIN NRF_SAADC_INPUT_AIN1 // P0.02/AIN0 = VBATT_MEAS
#define MAX77654_IRQ_PIN 14                   // P0.14
#define OV5640_PWDN_PIN 1                     // P0.01
#define OV5640_RESETB_N_PIN 0                 // P0.00
#define RESET_L_PIN 21                        // P0.21
#define SPI2_MISO_PIN 10                      // P0.10 SPI Master In Slave Out pin.
#define SPI2_MOSI_PIN 9                       // P0.09 SPI Master Out Slave In pin.
#define SPI2_SCK_PIN 7                        // P0.07 SPI Clock pin