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
 * Wrapper library over Nordic SPI NRFX drivers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#include "driver/config.h"
#include "driver/spi.h"

#define ASSERT  NRFX_ASSERT

// SPI instance
const nrfx_spim_t spi2 = NRFX_SPIM_INSTANCE(2);

// Indicate that SPI completed the transfer from the interrupt handler to main loop.
static volatile bool m_xfer_done = true;

/**
 * SPI event handler
 */
void spim_event_handler(nrfx_spim_evt_t const *p_event, void *p_context)
{
    // NOTE: there is only one event type: NRFX_SPIM_EVENT_DONE
    // so no need for case statement
    m_xfer_done = true;
}

/**
 * Select the CS pin: software control.
 * @param cs_pin GPIO pin to use, must be configured as output.
 */
void spi_chip_select(uint8_t cs_pin)
{
    nrf_gpio_pin_clear(cs_pin);
}

/**
 * Deselect the CS pin: software control.
 * @param cs_pin GPIO pin to use, must be configured as output.
 */
void spi_chip_deselect(uint8_t cs_pin)
{
    nrf_gpio_pin_set(cs_pin);
}

static void spi_xfer_chunk(nrfx_spim_xfer_desc_t *xfer)
{
    uint32_t err;

    // wait for any pending SPI operation to complete
    while (!m_xfer_done)
        __WFE();

    // Start the transaction and wait for the interrupt handler to warn us it is done.
    m_xfer_done = false;
    err = nrfx_spim_xfer(&spi2, xfer, 0);
    ASSERT(err == NRFX_SUCCESS);
    while (!m_xfer_done)
        __WFE();
}

/**
 * @brief Read a buffer over SPI.
 *
 * @param buf Data buffer to send.
 * @param len Length of the buffer.
 */
void spi_read(uint8_t *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n) {
        n = MIN(SPI_MAX_XFER_LEN, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(buf, n);
        spi_xfer_chunk(&xfer);
    }
}

/**
 * @brief Write a buffer over SPI.
 *
 * @param buf Data buffer to receive.
 * @param len Length of the buffer (buf[-1] excluded).
 */
void spi_write(uint8_t const *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n) {
        n = MIN(SPI_MAX_XFER_LEN, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, n);
        spi_xfer_chunk(&xfer);
    }
}

/**
 * @brief Initialise an SPI master interface with defaults values.
 *
 * @param spi The instance to configure.
 * @param sck_pin SPI SCK pin used with it.
 * @param MOSI_pin SPI MOSI pin used with it.
 * @param MISO_pin SPI MISO pin used with it.
 */
void spi_init(nrfx_spim_t spi, uint8_t sck_pin, uint8_t mosi_pin, uint8_t miso_pin)
{
    uint32_t err;
    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
        sck_pin, mosi_pin, miso_pin, NRFX_SPIM_PIN_NOT_USED
    );

    config.frequency = NRF_SPIM_FREQ_4M;
    config.mode      = NRF_SPIM_MODE_3;
    config.bit_order = NRF_SPIM_BIT_ORDER_LSB_FIRST;

    err = nrfx_spim_init(&spi2, &config, spim_event_handler, NULL);
    ASSERT(err == NRFX_SUCCESS);
}
