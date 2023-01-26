/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrfx.h"
#include "SEGGER_RTT.h"

/**
 * @brief Monocle PCB pinout.
 */

#define ECX336CN_CS_N_PIN 6                    // P0.06 SPI Slave Select pin for the display.
#define ECX336CN_XCLR_PIN 15                   // P0.15
#define FLASH_CS_N_PIN 4                       // P0.04 SPI Slave Select pin for the flash.
#define FPGA_CS_N_PIN 8                        // P0.08 SPI Slave Select pin for the FPGA.
#define FPGA_INT_PIN 5                         // P0.05 FPGA Interrupt pin, to hold low in init.
#define FPGA_MODE1_PIN FPGA_CS_N_PIN           // Alias
#define FPGA_RECONFIG_N_PIN FPGA_INT_PIN       // Alias
#define PMIC_TOUCH_I2C_SCL_PIN 17              // P0.17 I2C clock GPIO pin.
#define PMIC_TOUCH_I2C_SDA_PIN 13              // P0.13 I2C data GPIO pin.
#define CAMERA_I2C_SCL_PIN 18                  // P0.18 I2C clock GPIO pin.
#define CAMERA_I2C_SDA_PIN 16                  // P0.16 I2C data GPIO pin.
#define TOUCH_INTERRUPT_PIN 2                  // P0.02 Interrupt pin.
#define BATTERY_LEVEL_PIN NRF_SAADC_INPUT_AIN1 // P0.02/AIN0 = VBATT_MEAS
#define MAX77654_IRQ_PIN 14                    // P0.14
#define OV5640_PWDN_PIN 1                      // P0.01
#define OV5640_RESETB_N_PIN 0                  // P0.00
#define RESET_L_PIN 21                         // P0.21
#define SPI2_MISO_PIN 10                       // P0.10 SPI Master In Slave Out pin.
#define SPI2_MOSI_PIN 9                        // P0.09 SPI Master Out Slave In pin.
#define SPI2_SCK_PIN 7                         // P0.07 SPI Clock pin

/**
 * @brief LED driver.
 */

typedef enum led_t
{
    GREEN_LED,
    RED_LED
} led_t;

void monocle_set_led(led_t led, bool enable);

/**
 * @brief Startup and PMIC initialization.
 */

void monocle_critical_startup(void);

/**
 * @brief Bootloader entry function.
 */

void monocle_enter_bootloader(void);

/**
 * @brief I2C addresses.
 */

#define PMIC_I2C_ADDRESS 0x48
#define TOUCH_I2C_ADDRESS 0x44
#define CAMERA_I2C_ADDRESS 0x3C

/**
 * @brief Generic I2C driver.
 */

typedef struct i2c_response_t
{
    bool fail;
    uint8_t value;
} i2c_response_t;

typedef union i2c_register_address
{
    uint8_t register_address_8bit;
    uint16_t register_address_16bit;
} i2c_register_address;

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint8_t register_address,
                        uint8_t register_mask);

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint8_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value);

/**
 * @brief Logging and error handling macros.
 */

#define log(format, ...) SEGGER_RTT_printf(0, "\r\n" format, ##__VA_ARGS__)

#define log_clear() SEGGER_RTT_printf(0, RTT_CTRL_CLEAR "\r");

#define app_err(eval)                                                          \
    do                                                                         \
    {                                                                          \
        nrfx_err_t err = (eval);                                               \
        if (0x0000FFFF & err)                                                  \
        {                                                                      \
            log("App error code: 0x%x at %s:%u\r\n", err, __FILE__, __LINE__); \
            if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)              \
            {                                                                  \
                __BKPT();                                                      \
            }                                                                  \
            NVIC_SystemReset();                                                \
        }                                                                      \
    } while (0)
