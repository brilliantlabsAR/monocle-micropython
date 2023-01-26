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
 * Flash chip driver.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrfx_log.h"
#include "nrfx_systick.h"

#include "driver/config.h"
#include "driver/flash.h"
#include "driver/spi.h"
#include "driver/timer.h"
#include "monocle.h"

#define FLASH_CMD_PROGRAM_PAGE 0x02
#define FLASH_CMD_READ 0x03
#define FLASH_CMD_ENABLE_WRITE 0x06
#define FLASH_CMD_STATUS 0x05
#define FLASH_CMD_CHIP_ERASE 0xC7
#define FLASH_CMD_JEDEC_ID 0x9F
#define FLASH_CMD_DEVICE_ID 0x90

#define FLASH_STATUS_BUSY_MASK 0x01

static inline void flash_cmd_input(uint8_t cmd, uint8_t *buf, size_t len)
{
    spi_chip_select(FLASH_CS_N_PIN);
    spi_write(&cmd, 1);
    spi_read(buf, len);
    spi_chip_deselect(FLASH_CS_N_PIN);
}

static inline void flash_cmd_output(uint8_t cmd, uint8_t *buf, size_t len)
{
    spi_chip_select(FLASH_CS_N_PIN);
    spi_write(&cmd, 1);
    spi_write(buf, len);
    spi_chip_deselect(FLASH_CS_N_PIN);
}

/**
 * Read the manufacturer ID.
 * @return The combined manufacturer ID and device ID.
 */
uint32_t flash_get_jedec_id(void)
{
    uint8_t buf[3] = {0};

    flash_cmd_input(FLASH_CMD_JEDEC_ID, buf, sizeof buf);
    return buf[0] << 16 | buf[1] << 8 | buf[2] << 0;
}

/**
 * Wait FLASH operation completion by polling the BUSY register.
 */
static void flash_wait_completion(void)
{
    uint8_t status = 0;

    do
    {
        flash_cmd_input(FLASH_CMD_STATUS, &status, 1);
    } while (status & FLASH_STATUS_BUSY_MASK);
}

static void flash_enable_write(void)
{
    flash_cmd_output(FLASH_CMD_ENABLE_WRITE, NULL, 0);
}

/**
 * Program a page of the flash chip at the given address.
 * @param addr The address at which the data is written.
 * @param page The buffer holding the data to be sent to the flash chip, of size @ref FLASH_PAGE_SIZE.
 */
void flash_program_page(uint32_t addr, uint8_t page[FLASH_PAGE_SIZE])
{
    uint8_t cmds[] = {FLASH_CMD_PROGRAM_PAGE, addr >> 16, addr >> 8, addr >> 0};

    app_err(addr % FLASH_PAGE_SIZE != 0);

    flash_enable_write();

    spi_chip_select(FLASH_CS_N_PIN);
    spi_write(cmds, sizeof cmds);
    spi_read(page, FLASH_PAGE_SIZE);
    spi_chip_deselect(FLASH_CS_N_PIN);

    flash_wait_completion();
}

/**
 * Communicate to the chip over SPI and read multiple bytes at chosen address onto onto a buffer.
 * @param addr The address at which the data is read.
 * @param buf The buffer onto which the data read is stored.
 * @param len The size of ``buf``.
 */
void flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmds[] = {FLASH_CMD_READ, addr >> 16, addr >> 8, addr >> 0};

    spi_chip_select(FLASH_CS_N_PIN);
    spi_write(cmds, sizeof cmds);
    spi_read(buf, len);
    spi_chip_deselect(FLASH_CS_N_PIN);
}

/**
 * Send a command to erase the whole chip.
 */
void flash_erase_chip(void)
{
    flash_cmd_output(FLASH_CMD_CHIP_ERASE, NULL, 0);
    flash_wait_completion();
}

uint8_t flash_get_device_id(void)
{
    uint8_t id;

    flash_cmd_input(FLASH_CMD_DEVICE_ID, &id, 1);
    return id;
}

/**
 * Configure the SPI peripheral.
 */
void flash_init(void)
{
    log("flash_device_id=0x%02X", flash_get_device_id());
}
