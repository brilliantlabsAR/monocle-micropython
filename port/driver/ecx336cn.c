/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
 * Authored by: Shreyas Hemachandra <shreyas.hemachandran@gmail.com>
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

/**
 * Sony Microdisplay driver.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrfx_log.h"
#include "nrfx_systick.h"

#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/spi.h"
#include "driver/timer.h"

#define LEN(x)  (sizeof(x) / sizeof*(x))

uint8_t ecx336cn_config[] = {
    [0x00] = 0x9E, // [0]=0 -> enter power save mode
    [0x01] = 0x20,
    /* * */
    [0x03] = 0x20,  // 1125  
    [0x04] = 0x3F,
    [0x05] = 0xC8,  // 1125  DITHERON, LUMINANCE=0x00=2000cd/m2=medium (Datasheet 10.8)
    /* * */
    [0x07] = 0x40,
    [0x08] = 0x80,  // Luminance adjustment: OTPCALDAC_REGDIS=0 (preset mode per reg 5), white chromaticity: OTPDG_REGDIS=0 (preset mode, default)
    /* * */
    [0x0A] = 0x10,
    /* * */
    [0x0F] = 0x56,
    /* * */
    [0x20] = 0x01,
    /* * */
    [0x22] = 0x40,
    [0x23] = 0x40,
    [0x24] = 0x40,
    [0x25] = 0x80,
    [0x26] = 0x40,
    [0x27] = 0x40,
    [0x28] = 0x40,
    [0x29] = 0x0B,
    [0x2A] = 0xBE,    // CALDAC=190 (ignored, since OTPCALDAC_REGDIS=0)
    [0x2B] = 0x3C,
    [0x2C] = 0x02,
    [0x2D] = 0x7A,
    [0x2E] = 0x02,
    [0x2F] = 0xFA,
    [0x30] = 0x26,
    [0x31] = 0x01,
    [0x32] = 0xB6,
    /* * */
    [0x34] = 0x03,
    [0x35] = 0x60,    // 1125
    /* * */
    [0x37] = 0x76,
    [0x38] = 0x02,
    [0x39] = 0xFE,
    [0x3A] = 0x02,
    [0x3B] = 0x71,    // 1125
    /* * */
    [0x3D] = 0x1B,
    /* * */
    [0x3F] = 0x1C,
    [0x40] = 0x02,    // 1125
    [0x41] = 0x4D,    // 1125
    [0x42] = 0x02,    // 1125
    [0x43] = 0x4E,    // 1125
    [0x44] = 0x80,
    /* * */
    [0x47] = 0x2D,    // 1125
    [0x48] = 0x08,
    [0x49] = 0x01,    // 1125
    [0x4A] = 0x7E,    // 1125
    [0x4B] = 0x08,
    [0x4C] = 0x0A,    // 1125
    [0x4D] = 0x04,    // 1125
    /* * */
    [0x4F] = 0x3A,    // 1125
    [0x50] = 0x01,    // 1125
    [0x51] = 0x58,    // 1125
    [0x52] = 0x01,   
    [0x53] = 0x2D,
    [0x54] = 0x01,
    [0x55] = 0x15,    // 1125
    /* * */
    [0x57] = 0x2B,
    [0x58] = 0x11,    // 1125
    [0x59] = 0x02,
    [0x5A] = 0x11,    // 1125
    [0x5B] = 0x02,  
    [0x5C] = 0x25,
    [0x5D] = 0x04,    // 1125
    [0x5E] = 0x0B,    // 1125
    /* * */
    [0x60] = 0x23,
    [0x61] = 0x02,
    [0x62] = 0x1A,    // 1125
    /* * */
    [0x64] = 0x0A,    // 1125
    [0x65] = 0x01,    // 1125
    [0x66] = 0x8C,    // 1125
    [0x67] = 0x30,    // 1125
    /* * */
    [0x69] = 0x00,    // 1125
    /* * */
    [0x6D] = 0x00,    // 1125
    /* * */
    [0x6F] = 0x60,
    /* * */
    [0x79] = 0x68,
};

static inline const void ecx336cn_write_byte(uint8_t addr, uint8_t data)
{
    spi_chip_select(ECX336CN_CS_N_PIN);
    spi_write(&addr, 1);
    spi_write(&data, 1);
    spi_chip_deselect(ECX336CN_CS_N_PIN);
}

static inline uint8_t ecx336cn_read_byte(uint8_t addr)
{
    uint8_t data;

    ecx336cn_write_byte(0x80, 0x01);
    ecx336cn_write_byte(0x81, addr);

    spi_chip_select(ECX336CN_CS_N_PIN);
    spi_write(&addr, 1);
    spi_read(&data, 1);
    spi_chip_deselect(ECX336CN_CS_N_PIN);

    return data;
}

void ecx336cn_deinit(void)
{
    nrf_gpio_cfg_default(ECX336CN_CS_N_PIN);
    nrf_gpio_cfg_default(ECX336CN_XCLR_PIN);
}

/**
 * Configure the luminance level of the display.
 * @param level Predefined level of luminance.
 */
void ecx336cn_set_luminance(ecx336cn_luminance_t level)
{
    // maximum value value is 4
    assert(level <= 4);

    // LUMINANCE is register 0x05[3:0]; preserve other bits
    ecx336cn_write_byte(0x05, (ecx336cn_read_byte(0x05) & 0xF8) | level);
}

/**
 * Put the display to sleep power mode.
 */
void ecx336cn_sleep(void)
{
    ecx336cn_write_byte(0x00, 0x9E); // enter power saving mode (YUV)
    // it is now possible to turn off the 10V rail
}

/**
 * Power back on the dispay from sleep mode.
 */
void ecx336cn_awake(void)
{
    // the 10V power rail needs to be turned back on first
    ecx336cn_write_byte(0x00, 0x9F); // exit power saving mode (YUV)
}

/**
 * Configure each value of the screen over SPI.
 */
void ecx336cn_init(void)
{
    // set XCLR to high (1.8V to take it) to change to power-saving mode
    nrf_gpio_pin_set(ECX336CN_XCLR_PIN);

    // SONY ECX336CN register configuration, see Datasheet section 10.1
    ecx336cn_sleep();
    for (size_t i = 0; i < LEN(ecx336cn_config); i++)
        ecx336cn_write_byte(i, ecx336cn_config[i]);
    ecx336cn_awake();

    // check that 0x29 changed from default 0x0A to 0x0B
    // and that 0x2A has been restored
    LOG("0x29=0x%02X 0x2A=0x%02X", ecx336cn_read_byte(0x29), ecx336cn_read_byte(0x2A));
    // TODO: the SPI line cannot be read back!
    //assert(ecx336cn_read_byte(0x29) == 0x0B);
    //assert(ecx336cn_read_byte(0x2A) == 0xBE);
}
