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

#include "driver/board.h"
#include "driver/fpga.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/config.h"

#define LOG NRFX_LOG_ERROR
#define ASSERT BOARD_ASSERT

void fpga_check_pins(char const *msg)
{
    static bool first = true;

    if (first) {
        LOG("| INT   |       | MODE1 |       |       |");
        LOG("| RECFG | SCK   | CSN   | MOSI  | MISO  |");
        LOG("+-------+-------+-------+-------+-------+");
        LOG("| P0.05 | P0.07 | P0.08 | P0.09 | P0.10 |");
        LOG("+=======+=======+=======+=======+=======+");
        first = false;
    }
    LOG("|  %3d  |  %3d  |  %3d  |  %3d  |  %3d  | %s",
        nrf_gpio_pin_read(5),
        nrf_gpio_pin_read(7),
        nrf_gpio_pin_read(8),
        nrf_gpio_pin_read(9),
        nrf_gpio_pin_read(10),
        msg
    );
}

/**
 * Preparations for GPIO pins before to power-on the FPGA.
 */
void fpga_prepare(void)
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

/**
 * Initial configuration of the registers of the FPGA.
 */
void fpga_init(void)
{
    // Set the FPGA to boot from its internal flash.
    nrf_gpio_pin_write(FPGA_MODE1_PIN, false);
    nrfx_systick_delay_ms(1);

    // Issue a "reconfig" pulse.
    // Datasheet UG290E: T_recfglw >= 70 us
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, false);
    nrfx_systick_delay_ms(100); // 1000 times more than needed
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, true);

    // Give the FPGA some time to boot.
    // Datasheet UG290E: T_recfgtdonel <=
    nrfx_systick_delay_ms(100);

    // Reset the CSN pin, changed as it is also MODE1.
    nrf_gpio_pin_write(SPIM0_FPGA_CS_PIN, true);

    // Give the FPGA some further time.
    nrfx_systick_delay_ms(100);

    LOG("ready model=GW1N-LV9MG100 id=0x%X version=0x%X", fpga_system_id(), fpga_system_version());
}

void fpga_deinit(void)
{
    nrf_gpio_cfg_default(FPGA_MODE1_PIN);
    nrf_gpio_cfg_default(FPGA_RECONFIG_N_PIN);
}

#define FPGA_CMD_SYSTEM 0x00

static inline void fpga_cmd(uint8_t cmd1, uint8_t cmd2)
{
    spi_chip_select(SPIM0_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);
}

static inline void fpga_cmd_write(uint8_t cmd1, uint8_t cmd2, uint8_t *buf, size_t len)
{
    spi_chip_select(SPIM0_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_read(buf, len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);
}

static inline void fpga_cmd_read(uint8_t cmd1, uint8_t cmd2, uint8_t *buf, size_t len)
{
    spi_chip_select(SPIM0_FPGA_CS_PIN);
    spi_write(&cmd1, 1);
    spi_write(&cmd2, 1);
    spi_read(buf, len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);
}

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

#define FPGA_CMD_GRAPHICS 0x40

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

void fpga_graphics_set_write_base(uint32_t base)
{
    uint8_t buf[] = {
        (base & 0xFF000000) >> 24,
        (base & 0x00FF0000) >> 16,
        (base & 0x0000FF00) >> 8,
        (base & 0x000000FF) >> 0,
    };

    fpga_cmd_write(FPGA_CMD_GRAPHICS, 0x10, buf, sizeof buf);
}

void fpga_graphics_write_data(uint8_t *buf, size_t len)
{
    fpga_cmd_write(FPGA_CMD_GRAPHICS, 0x11, buf, len);
}

#define FPGA_CMD_CAPTURE 0x50

uint16_t fpga_capture_read_status(void)
{
    uint8_t buf[] = { 0x00, 0x00 };

    fpga_cmd_read(FPGA_CMD_CAPTURE, 0x00, buf, sizeof buf);
    return (buf[0] & 0xFF00) >> 8 | (buf[1] & 0x00FF) >> 0;
}

void fpga_capture_read_data(uint8_t *buf, size_t len)
{
    fpga_cmd_read(FPGA_CMD_CAPTURE, 0x10, buf, len);
}
