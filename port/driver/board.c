/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
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

/**
 * General board setups.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nrfx_gpiote.h"
#include "nrfx_log.h"
#include "nrfx_systick.h"
#include "nrfx_twi.h"

#include "driver/battery.h"
#include "driver/board.h"
#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/flash.h"
#include "driver/fpga.h"
#include "driver/i2c.h"
#include "driver/iqs620.h"
#include "driver/max77654.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/timer.h"

#define LOG     NRFX_LOG
#define ASSERT  NRFX_ASSERT

static bool board_halt_on_error;
static uint32_t board_errors;
static uint32_t board_test_num;

static inline void board_blink_num(uint8_t num)
{
    nrfx_systick_delay_ms(400);
    for (uint8_t i = 0; i < num; i++) {
        nrfx_systick_delay_ms(200);
        max77654_led_green(true);
        nrfx_systick_delay_ms(200);
        max77654_led_green(false);
    }
}

/**
 * If an error was detected on early init
 */
static void board_check_errors(void)
{
    if (board_errors) {
        max77654_led_red(true);
        for (uint8_t i = 0; i <= board_test_num; i++)
            if (board_errors & (1u << i))
                board_blink_num(i);
        ASSERT(!"hardware could not be entirely initialized");
    }
}

void board_assert_func(char const *file, int line, char const *func, char const *expr)
{
    LOG("%s:%d: (#%d) %s: %s", file, line, board_test_num, func, expr);

    if (board_halt_on_error) {
        __assert_func(file, line, func, expr);
    } else {
        board_errors |= 1u << board_test_num;
    }
}

void board_power_on(void)
{
    // Used by: fpga
    max77654_rail_1v2(true);
    nrfx_systick_delay_ms(20);

    // Used by: fpga, ov5640, nrf52832, ecx336cn, iqs620
    max77654_rail_1v8(true);
    nrfx_systick_delay_ms(20);

    // Used by: fpga, ov5640
    max77654_rail_2v7(true);
    nrfx_systick_delay_ms(20);

    // Used by: ecx336cn
    max77654_rail_10v(true);
    nrfx_systick_delay_ms(20);

    // Used by: led
    max77654_rail_vled(true);
    nrfx_systick_delay_ms(20);
}

static void board_power_off(void)
{
    max77654_rail_vled(false);
    nrfx_systick_delay_ms(10);
    max77654_rail_10v(false);
    nrfx_systick_delay_ms(10);
    max77654_rail_2v7(false);
    nrfx_systick_delay_ms(10);
    max77654_rail_1v8(false);
    nrfx_systick_delay_ms(10);
    max77654_rail_1v2(false);
}

/**
 * Initialises the hardware drivers and IO.
 */
void board_init(void)
{
    board_halt_on_error = true;
    board_test_num = 0;

    // Initialise SysTick ARM timer driver for nrfx_systick_delay_ms().
    nrfx_systick_init();

    // Initialise the GPIO driver with event support.
    nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);

    // Custom wrapper around I2C used by the other drivers.
    i2c_init();

    // Custom wrapper around SPI used by the other drivers.
    spi_init();

    // Periodic timer
    timer_init();

    // Initialise the battery level sensing with the ADC.
    battery_init();

    board_halt_on_error = false;
    board_test_num = 1;

    // I2C-controlled PMIC, also controlling the red/green LEDs over I2C
    // Needs: i2c
    max77654_init();
    max77654_led_green(true);

    // I2C calls to make sure all power rails are turned off.
    // Needs: max77654
    board_power_off();

    // Initialise GPIO before the chips are powered on.
    ecx336cn_prepare();
    fpga_prepare();
    ov5640_prepare();
    flash_prepare();

    // I2C calls to setup power rails of the MAX77654.
    // Needs: max77654
    board_power_on();

    board_test_num = 2;

    // Initialise the Capacitive Touch Button controller over I2C.
    // Needs: i2c, gpiote
    iqs620_init();

    board_test_num = 3;

    // Initialise the FPGA: providing the clock for the display and screen.
    // Needs: power, spi
    fpga_init();
    board_test_num = 4;

    // Initialise the Screen
    // Needs: power, spi, fpga
    ecx336cn_init();
    ecx336cn_set_luminance(ECX336CN_DIM);

    board_test_num = 5;

    // Initialise the Camera: startup sequence then I2C config.
    // Needs: power, i2c, fpga
    ov5640_init();

    board_test_num = 6;

    // Initialise the SPI conection to the flash.
    // Needs: power
    flash_init();

    LOG("ready errors=0x%02X test_num=%d", board_errors, board_test_num);

    max77654_led_green(false);
    //board_check_errors();
}

void board_deinit(void)
{
    // Call deinit() hook of each driver.
    ecx336cn_deinit();
    fpga_deinit();
    ov5640_deinit();

    // Turn the power rails off.
    board_power_off();
}
