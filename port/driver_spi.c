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

#include "driver_spi.h"
#include "driver_config.h"
#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#define LOG     NRFX_LOG
#define ASSERT  NRFX_ASSERT

// SPI instance
static const nrfx_spim_t m_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);

// Indicate that SPI completed the transfer from the interrupt handler to main loop.
static volatile bool m_xfer_done = true;

/**
 * SPI event handler
 */
void spim_event_handler(nrfx_spim_evt_t const * p_event, void *p_context)
{
    // NOTE: there is only one event type: NRFX_SPIM_EVENT_DONE
    // so no need for case statement
    m_xfer_done = true;
}

/**
 * Configure the SPI peripheral.
 */
void spi_init(void)
{
    uint32_t err;
    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
        SPIM0_SCK_PIN, SPIM0_MOSI_PIN, SPIM0_MISO_PIN, NRFX_SPIM_PIN_NOT_USED
    );

    config.frequency = NRF_SPIM_FREQ_1M;
    config.mode      = NRF_SPIM_MODE_3;
    config.bit_order = NRF_SPIM_BIT_ORDER_LSB_FIRST;

    err = nrfx_spim_init(&m_spi, &config, spim_event_handler, NULL);
    ASSERT(err == NRFX_SUCCESS);

    // configure CS pin for the Display (for active low)
    nrf_gpio_pin_set(SPIM0_DISP_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_DISP_CS_PIN);

    // for now, pull high to disable external flash chip
    nrf_gpio_pin_set(SPIM0_FLASH_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_FLASH_CS_PIN);

    // for now, pull high to disable external flash chip
    nrf_gpio_pin_set(SPIM0_FPGA_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_FPGA_CS_PIN);

    // initialze xfer state (needed for init/uninit cycles)
    m_xfer_done = true;

    LOG("ready nrfx=spim");
}

/**
 * Reset the SPI peripheral.
 */
void spi_uninit(void)
{
    nrf_gpio_cfg_default(SPIM0_FLASH_CS_PIN);
    // return pins to default state (input, hi-z)
    nrf_gpio_cfg_default(SPIM0_DISP_CS_PIN);
    // uninitialize the SPIM driver instance
    nrfx_spim_uninit(&m_spi);

    // errata 89, see https://infocenter.nordicsemi.com/index.jsp?topic=%2Ferrata_nRF52832_Rev3%2FERR%2FnRF52832%2FRev3%2Flatest%2Fanomaly_832_89.html&cp=4_2_1_0_1_26
    ASSERT(SPI_INSTANCE == 2);
    *(volatile uint32_t *)0x40023FFC = 0;
    *(volatile uint32_t *)0x40023FFC;
    *(volatile uint32_t *)0x40023FFC = 1;
    // NOTE: this did not make a measurable difference
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

static void spi_xfer(nrfx_spim_xfer_desc_t *xfer)
{
    uint32_t err;

    // wait for any pending SPI operation to complete
    while (!m_xfer_done)
        __WFE();

    // Start the transaction and wait for the interrupt handler to warn us it is done.
    m_xfer_done = false;
    err = nrfx_spim_xfer(&m_spi, xfer, 0);
    ASSERT(err == NRFX_SUCCESS);
    while (!m_xfer_done)
        __WFE();
}

/**
 * Write a buffer over SPI, and read the result back to the same buffer.
 * @param buf Data buffer to send, starting one byte just before that pointer (compatibility hack).
 * @param len Length of the buffer (buf[-1] excluded).
 */
void spi_read(uint8_t *buf, size_t len)
{
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(buf, len);
    spi_xfer(&xfer);
}

/**
 * Write a buffer over SPI, and read the result back to the same buffer.
 * @param buf Data buffer to send, starting one byte just before that pointer (compatibility hack).
 * @param len Length of the buffer (buf[-1] excluded).
 */
void spi_write(uint8_t *buf, size_t len)
{
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, len);
    spi_xfer(&xfer);
}
