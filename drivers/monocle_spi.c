/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Wrapper library over Nordic SPI NRFX drivers.
 * @file monocle_spi.c
 * @author Nathan Ashelman
 * @author Shreyas Hemachandra
 */

#include "monocle_spi.h"
#include "monocle_config.h"
#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define CHECK(err) check(__func__, err)

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline void check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
}

// SPI instance
static const nrfx_spim_t m_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);

// Indicate that SPI completed the transfer from the interrupt handler to main loop.
static volatile bool m_xfer_done = true;

static inline void spi_chip_select(uint8_t cs_pin)
{
    nrf_gpio_pin_clear(cs_pin);
}

static inline void spi_chip_deselect(uint8_t cs_pin)
{
    nrf_gpio_pin_set(cs_pin);
}

/**
 * SPI event handler
 */
void spim_event_handler(nrfx_spim_evt_t const * p_event, void *p_context)
{
    // NOTE: there is only one event type: NRFX_SPIM_EVENT_DONE, so no need for case statement
    m_xfer_done = true;
}

/**
 * Configure the SPI peripheral.
 */
void spi_init(void)
{
    nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
        SPIM0_SCK_PIN,
        SPIM0_MOSI_PIN,
        SPIM0_MISO_PIN,
        NRFX_SPIM_PIN_NOT_USED
    );
    config.frequency      = NRF_SPIM_FREQ_1M;
    config.mode           = NRF_SPIM_MODE_3;
    config.bit_order      = NRF_SPIM_BIT_ORDER_LSB_FIRST;
    config.miso_pull      = 0;
    CHECK(nrfx_spim_init(&m_spi, &config, spim_event_handler, NULL));

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
    assert(SPI_INSTANCE == 2);
    *(volatile uint32_t *)0x40023FFC = 0;
    *(volatile uint32_t *)0x40023FFC;
    *(volatile uint32_t *)0x40023FFC = 1;
    // NOTE: this did not make a measurable difference
}


static void spi_xfer(nrfx_spim_xfer_desc_t *xfer)
{
    // wait for any pending SPI operation to complete
    while (!m_xfer_done)
        __WFE();

    // Start the transaction and wait for the interrupt handler to warn us it is done.
    m_xfer_done = false;
    CHECK(nrfx_spim_xfer(&m_spi, xfer, 0));
    while (!m_xfer_done)
        __WFE();
}

static inline void spi_tx_buffer(uint8_t *tx_buf, uint16_t tx_len)
{
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(tx_buf, tx_len);
    spi_xfer(&xfer);
}

static inline void spi_rx_buffer(uint8_t *rx_buf, uint16_t rx_len)
{
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(rx_buf, rx_len);
    spi_xfer(&xfer);
}

static inline void spi_tx_byte(uint8_t tx_byte)
{
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&tx_byte, 1);
    spi_xfer(&xfer);
}

static inline uint8_t spi_rx_byte(void)
{
    uint8_t rx_byte;
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(&rx_byte, 1);
    spi_xfer(&xfer);
    return rx_byte;
}

/**
 * Write a burst of multiple bytes over SPI, prefixed by an address field.
 * @param cs_pin GPIO pin number.
 * @param addr First byte of the SPI transaction, indicating an address.
 * @param buf Data buffer to send.
 * @param len Length of the buf buffer.
 */
void spi_write_buffer(uint8_t cs_pin, uint8_t addr, uint8_t *buf, uint16_t len)
{
    spi_chip_select(cs_pin);
    spi_tx_byte(addr);
    spi_tx_buffer(buf, len);
    spi_chip_deselect(cs_pin);
}

/**
 * Write a single byte over SPI
 * @param cs_pin GPIO pin number.
 * @param addr Address sent before the byte.
 * @param byte Byte to send after the address.
 */
void spi_write_register(uint8_t cs_pin, uint8_t addr, uint8_t data)
{
    spi_write_buffer(cs_pin, addr, &data, 1);
}

/**
 * Read a burst of multiple bytes at an address.
 * @param cs_pin GPIO pin number.
 * @param addr Address sent before the data is read.
 * @param len Number of bytes to read.
 * @return Pointer to data, but data will be overwritten by the next SPI read or write
 * so caller must guarantee processing of the data before the next SPI transaction.
 */
void spi_read_buffer(uint8_t cs_pin, uint8_t addr, uint8_t *rx_buf, uint16_t rx_len)
{
    // set RD_ON register to 1
    spi_chip_select(cs_pin);
    spi_tx_byte(0x80);
    spi_tx_byte(0x01);
    spi_chip_deselect(cs_pin);

    // write address of target register to read continuously
    spi_chip_select(cs_pin);
    spi_tx_byte(0x81);
    spi_tx_byte(addr);
    spi_chip_deselect(cs_pin);

    // read the data at that burst register
    spi_chip_select(cs_pin);
    spi_tx_byte(0x81);
    spi_rx_buffer(rx_buf, rx_len);
    spi_chip_deselect(cs_pin);
}

/*
 * Read a single byte over SPI at the given address.
 * @param cs_pin GPIO pin number.
 * @param addr Byte sent before the data is read.
 * @return The byte read after the address is sent.
 */
uint8_t spi_read_register(uint8_t cs_pin, uint8_t addr)
{
    uint8_t rx_byte;
    spi_read_buffer(cs_pin, addr, &rx_byte, 1);
    return rx_byte;
}
