/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef MONOCLE_CONFIG_H
#define MONOCLE_CONFIG_H

#include "nrf_gpio.h"

// Pin mapping

#define IO_N_CAM_RESET              20      // P0.00
#define IO_CAM_PWDN                 29      // P0.01
#define IO_DISP_XCLR                15      // P0.15
#define IO_TOUCHED_PIN              2       // P0.21 = touched_N
#define IO_TOUCHED_PIN_ACTIVE       0       // active low
#define IO_TOUCHED_PIN_PULL         NRF_GPIO_PIN_PULLUP
#define IO_ADC_VBATT                NRF_SAADC_INPUT_AIN1 // P0.02/AIN0 = vbatt_meas
#define SPIM0_SCK_PIN               7       // P0.07 SPI Clock pin
#define SPIM0_MOSI_PIN              9       // P0.09 SPI Master Out Slave In pin.
#define SPIM0_MISO_PIN              10      // P0.10 SPI Master In Slave Out pin.
#define SPIM0_DISP_CS_PIN           6       // P0.06 SPI Slave Select pin for the display.
#define SPIM0_FPGA_CS_PIN           8       // P0.08 SPI Slave Select pin for the FPGA.
#define FPGA_INT_PIN                5       // P0.05 FPGA Interrupt pin, to hold low in init.
#define SPIM0_FLASH_CS_PIN          4       // P0.04 SPI Slave Select pin for the flash.
#define I2C0_SCL_PIN                17      // P0.17 I2C clock GPIO pin.
#define I2C0_SDA_PIN                13      // P0.13 I2C data GPIO pin.
#define I2C1_SCL_PIN                18      // P0.18 I2C clock GPIO pin.
#define I2C1_SDA_PIN                16      // P0.16 I2C data GPIO pin.
#define PMIC_IRQ_PIN                14      // P0.14
#define RESET_L_PIN                 21      // P0.21

// I2C

#define IQS620_I2C                  I2C0
#define IQS620_ADDR                 0x44

#define MAX77654_I2C                I2C0
#define MAX77654_ADDR               0x48

#define OV5640_I2C                  I2C1
#define OV5640_ADDR                 0x3C

// SPI

#define SPI_INSTANCE                2

#endif
