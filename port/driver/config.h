/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Configuratin file for the drivers.
 * GPIO pins used, I2C addresses, peripheral instances in use...
 */

#include "nrf_gpio.h"

// Bluetooth params

#define BLE_DEVICE_NAME             "Monocle"

// Pin mapping

#define BATTERY_ADC_PIN NRF_SAADC_INPUT_AIN1 // P0.02/AIN0 = VBATT_MEAS
#define IQS620_TOUCH_RDY_PIN        2       // P0.02 Interrupt pin.              
#define SPI2_SCK_PIN                7       // P0.07 SPI Clock pin
#define SPI2_MOSI_PIN               9       // P0.09 SPI Master Out Slave In pin.
#define SPI2_MISO_PIN               10      // P0.10 SPI Master In Slave Out pin.
#define SPI_DISP_CS_PIN             6       // P0.06 SPI Slave Select pin for the display.
#define SPI_FPGA_CS_PIN             8       // P0.08 SPI Slave Select pin for the FPGA.
#define SPI_FLASH_CS_PIN            4       // P0.04 SPI Slave Select pin for the flash.
#define FPGA_MODE1_PIN              SPI_FPGA_CS_PIN // Alias
#define FPGA_INT_PIN                5       // P0.05 FPGA Interrupt pin, to hold low in init.
#define FPGA_RECONFIG_N_PIN         FPGA_INT_PIN // Alias
#define I2C0_SCL_PIN                17      // P0.17 I2C clock GPIO pin.
#define I2C0_SDA_PIN                13      // P0.13 I2C data GPIO pin.
#define I2C1_SCL_PIN                18      // P0.18 I2C clock GPIO pin.
#define I2C1_SDA_PIN                16      // P0.16 I2C data GPIO pin.
#define PMIC_IRQ_PIN                14      // P0.14
#define RESET_L_PIN                 21      // P0.21
#define OV5640_NRESETB_PIN          0       // P0.00
#define OV5640_PWDN_PIN             1       // P0.01
#define ECX336CN_XCLR_PIN           15      // P0.15

// TIMER

// 0 is reserved for SoftDevice
#define TIMER_INSTANCE              2
#define TIMER_MAX_HANDLERS          2

// I2C

#define IQS620_I2C                  i2c0
#define IQS620_ADDR                 0x44

#define MAX77654_I2C                i2c0
#define MAX77654_ADDR               0x48

#define OV5640_I2C                  i2c1
#define OV5640_ADDR                 0x3C

#define DRIVER(name) { \
    static bool ready = false; \
    if (ready) return; else ready = true; \
    PRINTF("DRIVER(%s)\r\n", name); \
}
