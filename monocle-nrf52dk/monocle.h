/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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
#include "touch.h"

/**
 * @brief Monocle PCB pinout.
 */

#define BATTERY_LEVEL_PIN NRF_SAADC_INPUT_AIN1 //
#define I2C_SCL_PIN 27                         //
#define I2C_SDA_PIN 26                         //
#define CAMERA_RESET_PIN 1                     //
#define CAMERA_SLEEP_PIN 1                     //
#define DISPLAY_CS_PIN 1                       // Active low
#define DISPLAY_RESET_PIN 1                    // Active low
#define FLASH_CS_PIN 1                         // Active low
#define FPGA_CS_MODE_PIN 1                     // Active low
#define FPGA_FLASH_SPI_SCK_PIN 7               //
#define FPGA_FLASH_SPI_SDI_PIN 10              //
#define FPGA_FLASH_SPI_SDO_PIN 9               //
#define FPGA_RESET_INT_PIN 1                   //
#define PMIC_INTERRUPT_PIN 1                   //
#define TOUCH_INTERRUPT_PIN 1                  //
#define TOUCH_BUTTON_A_PIN BUTTON_1_PIN        //
#define TOUCH_BUTTON_B_PIN BUTTON_2_PIN        //
#define BUTTON_1_PIN 13                        //
#define BUTTON_2_PIN 14                        //
#define BUTTON_3_PIN 15                        //
#define BUTTON_4_PIN 16                        //
#define LED_1_PIN 17                           //
#define LED_2_PIN 18                           //
#define LED_3_PIN 19                           //
#define LED_4_PIN 20                           //

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
 * @brief Reboots Monocle with the safe mode flag enabled.
 */

void monocle_enter_safe_mode(void);

/**
 * @brief Checks if the safe mode flag is set or not.
 */

bool monocle_started_in_safe_mode(void);

/**
 * @brief Resets the FPGA, and either holds it in reset, or reboots.
 */

void monocle_fpga_reset(bool reboot);

/**
 * @brief Dev board mode flag. i.e. no PMIC, FPGA, display detected etc.
 */

extern bool not_real_hardware_flag;

/**
 * @brief Prevents sleeping when a 5V charging voltage is applied to Monocle.
 */

extern bool prevent_sleep_flag;

/**
 * @brief Forces sleep, as if Monocle was placed into the charging case.
 */

extern bool force_sleep_flag;

/**
 * @brief Low level I2C driver for accessing PMIC, camera and touch ICs.
 */

#define PMIC_I2C_ADDRESS 0x48
#define TOUCH_I2C_ADDRESS 0x44
#define CAMERA_I2C_ADDRESS 0x3C
#define EXPANDER_I2C_ADDRESS 0x00

typedef struct i2c_response_t
{
    bool fail;
    uint8_t value;
} i2c_response_t;

i2c_response_t monocle_i2c_read(uint8_t device_address_7bit,
                                uint16_t register_address,
                                uint8_t register_mask);

i2c_response_t monocle_i2c_write(uint8_t device_address_7bit,
                                 uint16_t register_address,
                                 uint8_t register_mask,
                                 uint8_t set_value);

/**
 * @brief Low level SPI driver for accessing FPGA, display and flash.
 */

typedef enum spi_device_t
{
    DISPLAY,
    FPGA,
    FLASH
} spi_device_t;

void monocle_spi_enable(bool enable);

void monocle_spi_read(spi_device_t spi_device, uint8_t *data, size_t length,
                      bool hold_down_cs);

void monocle_spi_write(spi_device_t spi_device, uint8_t *data, size_t length,
                       bool hold_down_cs);

/**
 * @brief High level SPI driver for accessing flash.
 */

void monocle_flash_read(uint8_t *buffer, size_t address, size_t length);

void monocle_flash_write(uint8_t *buffer, size_t address, size_t length);

void monocle_flash_page_erase(size_t address);

/**
 * @brief Error handling macro.
 */

#define app_err(eval)                                                      \
    do                                                                     \
    {                                                                      \
        nrfx_err_t err = (eval);                                           \
        if (0x0000FFFF & err)                                              \
        {                                                                  \
            NRFX_LOG("App error: 0x%x at %s:%u", err, __FILE__, __LINE__); \
            if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)          \
            {                                                              \
                __BKPT();                                                  \
            }                                                              \
            NVIC_SystemReset();                                            \
        }                                                                  \
    } while (0)
