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

/**
 * FPGA Communication driver
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_systick.h"
#include "nrfx_spim.h"
#include "nrfx_log.h"

#include "driver/config.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/timer.h"

#define ASSERT      NRFX_ASSERT

void fpga_cmd_write(uint16_t cmd, const uint8_t *buf, size_t len)
{
    uint8_t cmd_buf[2] = { cmd >> 8, cmd >> 0 };

    spi_chip_select(FPGA_CS_N_PIN);
    spi_write(cmd_buf, sizeof cmd_buf);
    spi_write(buf, len);
    spi_chip_deselect(FPGA_CS_N_PIN);
}

void fpga_cmd_read(uint16_t cmd, uint8_t *buf, size_t len)
{
    uint8_t cmd_buf[2] = { (cmd >> 8) & 0xFF, (cmd >> 0) & 0xFF };

    spi_chip_select(FPGA_CS_N_PIN);
    spi_write(cmd_buf, sizeof cmd_buf);
    spi_read(buf, len);
    spi_chip_deselect(FPGA_CS_N_PIN);
}

void fpga_cmd(uint16_t cmd)
{
    fpga_cmd_write(cmd, NULL, 0);
}

/**
 * Initial configuration of the registers of the FPGA.
 */
void fpga_init(void)
{
    uint8_t version[3];

    // Reset the CS_N pin, changed as it is also MODE1.
    spi_chip_deselect(FPGA_CS_N_PIN);

    // enable 24mhz pixel clock to the ov5640, required for i²c configuration
    fpga_cmd(FPGA_CAMERA_ON);

    // report the FPGA bitstream version
    fpga_cmd_read(FPGA_SYSTEM_VERSION, version, sizeof version);
    LOG("version=%d.%d.%d", version[0], version[1], version[2]);
}
