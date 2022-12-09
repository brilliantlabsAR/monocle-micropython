/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Wrapper library over Nordic SPI NRFX drivers.
 * @file driver_spi.c
 * @author Nathan Ashelman
 * @author Shreyas Hemachandra
 */

#include "driver_spi.h"
#include "driver_config.h"
#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define ASSERT NRFX_ASSERT

// SPI instance
static const nrfx_spim_t m_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);

// Indicate that SPI completed the transfer from the interrupt handler to main loop.
static volatile bool m_xfer_done = true;

// Compatibility with the previous API.
static uint8_t m_cs_pin;

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

/**
 * Write a buffer over SPI, and read the result back to the same buffer.
 * @param cs_pin Pin to use as chip-select signal, must be configured as output.
 * @param buf Data buffer to send, starting one byte just before that pointer (compatibility hack).
 * @param len Length of the buffer (buf[-1] excluded).
 */
void spi_xfer(uint8_t *buf, size_t len)
{
    uint32_t err;
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TRX(buf, len, buf, len);

    // wait for any pending SPI operation to complete
    while (!m_xfer_done)
        __WFE();

    // Start the transaction and wait for the interrupt handler to warn us it is done.
    m_xfer_done = false;
    err = nrfx_spim_xfer(&m_spi, &xfer, 0);
    ASSERT(err == NRFX_SUCCESS);
    while (!m_xfer_done)
        __WFE();
}
