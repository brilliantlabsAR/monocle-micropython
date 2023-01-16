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
#include "nrfx_log.h"

#include "driver/config.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/timer.h"

#define ASSERT      NRFX_ASSERT

static inline void fpga_cmd(uint8_t cmd1, uint8_t cmd2)
{
    spi_chip_select(SPI_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_chip_deselect(SPI_FPGA_CS_PIN);
}

static inline void fpga_cmd_write(uint8_t cmd1, uint8_t cmd2, uint8_t *buf, size_t len)
{
    LOG("cmd1=0x%02X cmd2=0x%02X buf[]={ 0x%02X, ... (x%d) }", cmd1, cmd2, buf[0], len);
    spi_chip_select(SPI_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_read(buf, len);
    spi_chip_deselect(SPI_FPGA_CS_PIN);
}

static inline void fpga_cmd_read(uint8_t cmd1, uint8_t cmd2, uint8_t *buf, size_t len)
{
    spi_chip_select(SPI_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_read(buf, len);
    spi_chip_deselect(SPI_FPGA_CS_PIN);
}

#define FPGA_CMD_SYSTEM 0x00

uint32_t fpga_system_id(void)
{
    uint8_t buf[] = { 0x00, 0x00 };

    fpga_cmd_read(FPGA_CMD_SYSTEM, 0x01, buf, sizeof buf);
    return buf[0] << 8 | buf[1] << 0;
}

uint32_t fpga_system_version(void)
{
    uint8_t buf[] = { 0x00, 0x00, 0x00 };

    fpga_cmd_read(FPGA_CMD_SYSTEM, 0x02, buf, sizeof buf);
    return buf[0] << 16 | buf[1] << 8 | buf[2] << 0;
}

#define FPGA_CMD_CAMERA 0x10

void fpga_camera_zoom(uint8_t zoom_level)
{
    fpga_cmd_write(FPGA_CMD_CAMERA, 0x02, &zoom_level, 1);
}

void fpga_camera_stop(void)
{
    fpga_cmd(FPGA_CMD_CAMERA, 0x04);
}

void fpga_camera_start(void)
{
    fpga_cmd(FPGA_CMD_CAMERA, 0x05);
}

void fpga_camera_capture(void)
{
    fpga_cmd(FPGA_CMD_CAMERA, 0x06);
}

void fpga_camera_off(void)
{
    fpga_cmd(FPGA_CMD_CAMERA, 0x08);
}

void fpga_camera_on(void)
{
    fpga_cmd(FPGA_CMD_CAMERA, 0x09);
}

#define FPGA_CMD_LIVE_VIDEO 0x30

void fpga_live_video_start(void)
{
    fpga_cmd(FPGA_CMD_LIVE_VIDEO, 0x05);
}

void fpga_live_video_stop(void)
{
    fpga_cmd(FPGA_CMD_LIVE_VIDEO, 0x04);
}

void fpga_live_video_replay(void)
{
    fpga_cmd(FPGA_CMD_LIVE_VIDEO, 0x07);
}

#define FPGA_CMD_GRAPHICS 0x44

void fpga_graphics_off(void)
{
    fpga_cmd(FPGA_CMD_GRAPHICS, 0x04);
}

void fpga_graphics_on(void)
{
    fpga_cmd(FPGA_CMD_GRAPHICS, 0x05);
}

void fpga_graphics_clear(void)
{
    fpga_cmd(FPGA_CMD_GRAPHICS, 0x06);
}

void fpga_graphics_swap_buffer(void)
{
    fpga_cmd(FPGA_CMD_GRAPHICS, 0x07);
}

void fpga_graphics_set_write_addr(uint32_t addr)
{
    uint8_t buf[] = { addr >> 24, addr >> 16, addr >> 8, addr >> 0 };

    fpga_cmd_write(FPGA_CMD_GRAPHICS, 0x10, buf, sizeof buf);
}

void fpga_graphics_write_data(uint8_t *buf, size_t len)
{
    assert(len % 128 == 0);
    for (size_t i = 0; i < len; i += 128) {
        fpga_cmd_write(FPGA_CMD_GRAPHICS, 0x11, buf + i, 128);
    }
}

#define FPGA_CMD_CAPTURE 0x50

uint16_t fpga_capture_get_status(void)
{
    uint8_t buf[] = { 0x00, 0x00 };

    fpga_cmd_read(FPGA_CMD_CAPTURE, 0x00, buf, sizeof buf);
    return (buf[0] << 8 | buf[1] << 0);
}

static void fpga_capture_get_data(uint8_t *buf, size_t len)
{
    fpga_cmd_read(FPGA_CMD_CAPTURE, 0x10, buf, len);
    for (size_t i = 0; i < len; i++)
        PRINTF("%02X", buf[i]);
    PRINTF("\r\n");
}

size_t fpga_capture_read(uint8_t *buf, size_t len)
{
    for (size_t n, i = 0; i < len; i++)
    {
        // the FPGA stores the length to read in a dedicated register
        n = fpga_capture_get_status() & 0x0FFF;
        LOG("len=%d n=%d", len, n);

        // if there is nothing more to read, stop now
        if (n == 0)
        {
            return i;
        }

        // finally read the prepared read length from the FPGA
        fpga_capture_get_data(buf + i, MIN(n, len));
    }
    return len;
}

/**
 * Initial configuration of the registers of the FPGA.
 */
void fpga_init(void)
{
    // Reset the CSN pin, changed as it is also MODE1.
    nrf_gpio_pin_write(SPI_FPGA_CS_PIN, true);
    nrfx_systick_delay_ms(1);

    // enable 24mhz pixel clock to the ov5640, required for i²c configuration
    fpga_camera_on();
}
