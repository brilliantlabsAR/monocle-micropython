/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
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

#include "nrf_gpio.h"
#include "nrfx_glue.h"
#include "nrfx_gpiote.h"
#include "nrfx_log.h"
#include "nrfx_saadc.h"
#include "nrfx_systick.h"

#include "driver/config.h"

#define ASSERT  NRFX_ASSERT

/*
 * Prepare GPIO pins before the various chips receives power.
 */

void nrfx_gpio_ov5640(void)
{
    // Set to 0V = hold camera in reset.
    nrf_gpio_pin_write(OV5640_RESETB_N_PIN, false);
    nrf_gpio_cfg_output(OV5640_RESETB_N_PIN);

    // Set to 0V = not asserted.
    nrf_gpio_pin_write(OV5640_PWDN_PIN, false);
    nrf_gpio_cfg_output(OV5640_PWDN_PIN);
}

void nrfx_gpio_ecx336cn(void)
{
    // Set to 0V on boot (datasheet p.11)
    nrf_gpio_pin_write(ECX336CN_XCLR_PIN, 0);
    nrf_gpio_cfg_output(ECX336CN_XCLR_PIN);
}

void nrf_gpio_flash(void)
{
    // Prepare the SPI_CS pin for the flash.
    nrf_gpio_pin_set(SPI_FLASH_CS_PIN);
    nrf_gpio_cfg_output(SPI_FLASH_CS_PIN);
}

void nrfx_gpio_fpga(void)
{
    // MODE1 set low for AUTOBOOT from FPGA internal flash
    nrf_gpio_pin_write(FPGA_MODE1_PIN, false);
    nrf_gpio_cfg(
        FPGA_MODE1_PIN,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );

    // Let the FPGA start as soon as it has the power on.
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, true);
    nrf_gpio_cfg(
        FPGA_RECONFIG_N_PIN,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void nrfx_init(void)
{
    DRIVER("NRFX");

    uint32_t err;

    // NRFX SysTick
    nrfx_systick_init();

    // NRFX GPIOTE
    nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);

    // NRFX SAADC
    err = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    ASSERT(err == NRFX_SUCCESS);

    // GPIO
    nrfx_gpio_ov5640();
    nrfx_gpio_ecx336cn();
    nrf_gpio_flash();
    nrfx_gpio_fpga();
}
