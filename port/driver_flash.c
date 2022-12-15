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
#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#include "driver_board.h"
#include "driver_config.h"
#include "driver_flash.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define ASSERT BOARD_ASSERT

#define FLASH_CMD_PROGRAM_PAGE      0x02
#define FLASH_CMD_READ              0x03
#define FLASH_CMD_ENABLE_WRITE      0x06
#define FLASH_CMD_STATUS            0x05
#define FLASH_CMD_CHIP_ERASE        0xC7
#define FLASH_CMD_JEDEC_ID          0x9F

#define FLASH_STATUS_BUSY_MASK      0x01

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline void check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
}

static const nrfx_spim_t m_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);
static volatile bool m_xfer_done;

/**
 * SPI event handler
 */
static void flash_event_handler(nrfx_spim_evt_t const * p_event, void *p_context)
{
    // There is only one event type: NRFX_SPIM_EVENT_DONE, so no need for case statement
    (void)p_event;
    (void)p_context;

    m_xfer_done = true;
}

/**
 * Prepare the GPIO setup for the SPI flash interface.
 */
void flash_prepare(void)
{
    // Prepare the SPI_CS pin for the flash.
    nrf_gpio_pin_set(SPIM0_FLASH_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_FLASH_CS_PIN);
}

/**
 * Configure the SPI peripheral.
 */
void flash_init(void)
{
    uint32_t err;
    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
        SPIM0_SCK_PIN,
        SPIM0_MOSI_PIN,
        SPIM0_MISO_PIN,
        NRFX_SPIM_PIN_NOT_USED
    );

    config.frequency    = NRF_SPIM_FREQ_125K;
    config.mode         = NRF_SPIM_MODE_0;
    config.bit_order    = NRF_SPIM_BIT_ORDER_MSB_FIRST;
    err = nrfx_spim_init(&m_spi, &config, &flash_event_handler, NULL);
    ASSERT(err == NRFX_SUCCESS);

    LOG("ready");
}

/**
 * Write a chunk of max 16 bytes (EASYDMA limitation).
 * @param buf Buffer with data to send.
 * @param len Buffer length.
 */
static void flash_spi_write_chunk(const uint8_t *buf, size_t len)
{
    uint32_t err;
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, len);

    // Reset rx buffer and transfer done flag
    m_xfer_done = false;

    // Submit the buffer for transfer.
    err = nrfx_spim_xfer(&m_spi, &xfer, 0);
    ASSERT(err == NRFX_SUCCESS);

    // Wait until the event handler tells us that the transfer is over.
    while (!m_xfer_done)
        __WFE();
}

/**
 * Write a buffer to the flash over SPI.
 * @param buf Buffer with data to send.
 * @param len Buffer length.
 */
static void flash_spi_write(const uint8_t *buf, size_t len)
{
    for (size_t n; len > 0; len -= n, buf += n) {
        n = len;
        if (n > 16)
            n = 16;
        flash_spi_write_chunk(buf, n);
    }
}

/**
 * Read a chunk of max 16 bytes (EASYDMA limitation).
 * @param buf Buffer receiving the data.
 * @param len Buffer length.
 */
static void flash_spi_read_chunk(uint8_t *buf, size_t len)
{
    uint32_t err;
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(buf, len);

    // Reset rx buffer and transfer done flag
    m_xfer_done = false;

    // Submit the buffer for transfer.
    err = nrfx_spim_xfer(&m_spi, &xfer, 0);
    ASSERT(err == NRFX_SUCCESS);

    // Wait until the event handler tells us that the transfer is over.
    while (!m_xfer_done)
        __WFE();
}

/**
 * Read a buffer from the flash over SPI.
 * @param buf Buffer data is read to.
 * @param len Buffer length.
 */
static void flash_spi_read(uint8_t *buf, size_t len)
{
    for (size_t n; len > 0; len -= n, buf += n) {
        n = len;
        if (n > 16)
            n = 16;
        flash_spi_read_chunk(buf, n);
    }
}

/**
 * Set the CS pin configured with #define SPIM0_FLASH_CS_PIN
 */
static inline void flash_chip_select(void)
{
    nrfx_systick_delay_us(10);
    nrf_gpio_pin_clear(SPIM0_FLASH_CS_PIN);
    nrfx_systick_delay_us(10);
}

/**
 * Clear the CS pin configured with #define SPIM0_FLASH_CS_PIN
 */
static inline void flash_chip_deselect(void)
{
    nrfx_systick_delay_us(10);
    nrf_gpio_pin_set(SPIM0_FLASH_CS_PIN);
    nrfx_systick_delay_us(10);
}

/**
 * Read the manufacturer ID.
 * @return The combined manufacturer ID and device ID.
 */
uint32_t flash_get_id(void)
{
    uint8_t const cmds[] = { FLASH_CMD_JEDEC_ID };
    uint8_t buf[3] = {0};

    flash_chip_select();
    flash_spi_write(cmds, sizeof cmds);
    flash_spi_read(buf, sizeof buf);
    flash_chip_deselect();

    return buf[0] << 16 | buf[1] << 8 | buf[2] << 0;
}

/**
 * Wait FLASH operation completion by polling the BUSY register.
 */
static void flash_wait_completion(void)
{
    uint8_t const cmds[] = { FLASH_CMD_STATUS };
    uint8_t status = 0;

    do {
        flash_chip_select();
        flash_spi_write(cmds, sizeof cmds);
        flash_spi_read(&status, sizeof status);
        flash_chip_deselect();
    } while (status & FLASH_STATUS_BUSY_MASK);
}

static void flash_enable_write(void)
{
    uint8_t const cmds[] = { FLASH_CMD_ENABLE_WRITE };

    flash_chip_select();
    flash_spi_write(cmds, sizeof cmds);
    flash_chip_deselect();
}

/**
 * Program a page of the flash chip at the given address.
 * @param addr The address at which the data is written.
 * @param page The buffer holding the data to be sent to the flash chip, of size @ref FLASH_PAGE_SIZE.
 */
void flash_program_page(uint32_t addr, uint8_t const page[FLASH_PAGE_SIZE])
{
    uint8_t const cmds[] = { FLASH_CMD_PROGRAM_PAGE, addr >> 16, addr >> 8, addr >> 0 };

    ASSERT(addr % FLASH_PAGE_SIZE == 0);

    flash_enable_write();

    flash_chip_select();
    flash_spi_write(cmds, sizeof cmds);
    flash_spi_write(page, FLASH_PAGE_SIZE);
    flash_chip_deselect();

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
    uint8_t const cmds[] = { FLASH_CMD_READ, addr >> 16, addr >> 8, addr >> 0 };

    flash_chip_select();
    flash_spi_write(cmds, sizeof cmds);
    flash_spi_read(buf, len);
    flash_chip_deselect();
}

/**
 * Send a command to erase the whole chip.
 */
void flash_erase_chip(void)
{
    uint8_t const cmds[] = { FLASH_CMD_CHIP_ERASE };

    flash_chip_select();
    flash_spi_write(cmds, sizeof cmds);
    flash_chip_deselect();

    flash_wait_completion();
}
