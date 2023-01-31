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
#include "nrfx_log.h"

/**
 * @brief Monocle PCB pinout.
 */

#define BATTERY_LEVEL_PIN NRF_SAADC_INPUT_AIN1 //
#define CAMERA_I2C_SCL_PIN 18                  //
#define CAMERA_I2C_SDA_PIN 16                  //
#define CAMERA_RESET_PIN 20                    //
#define CAMERA_SLEEP_PIN 29                    //
#define DISPLAY_CS_PIN 6                       // Active low
#define DISPLAY_RESET_PIN 15                   // Active low
#define FLASH_CS_PIN 4                         // Active low
#define FPGA_CS_PIN 8                          // Active low
#define FPGA_FLASH_SPI_SCK_PIN 7               //
#define FPGA_FLASH_SPI_SDI_PIN 10              //
#define FPGA_FLASH_SPI_SDO_PIN 9               //
#define FPGA_INTERRUPT_PIN 5                   //
#define PMIC_INTERRUPT_PIN 14                  //
#define PMIC_TOUCH_I2C_SCL_PIN 17              //
#define PMIC_TOUCH_I2C_SDA_PIN 13              //
#define TOUCH_INTERRUPT_PIN 2                  //

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

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint16_t register_address,
                        uint8_t register_mask);

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint16_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value);

/**
 * @brief Error handling macro.
 */

#define app_err(eval)                                                        \
    do                                                                       \
    {                                                                        \
        nrfx_err_t err = (eval);                                             \
        if (0x0000FFFF & err)                                                \
        {                                                                    \
            NRFX_LOG_ERROR("Error: 0x%x at %s:%u", err, __FILE__, __LINE__); \
            if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)            \
            {                                                                \
                __BKPT();                                                    \
            }                                                                \
            NVIC_SystemReset();                                              \
        }                                                                    \
    } while (0)
