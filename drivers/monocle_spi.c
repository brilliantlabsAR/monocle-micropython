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

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define CHECK(err) check(__func__, err)

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline bool check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
    return err == NRFX_SUCCESS;
}

static const nrfx_spim_t m_spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static uint8_t m_spi_cs_pin; // keep track of current CS pin
static volatile bool m_spi_xfer_done = true;  /**< Flag used to indicate that SPI instance completed the transfer. */

static uint8_t m_tx_buf[SPI_MAX_BURST_LENGTH + 1];  /**< TX buffer. */
static uint8_t m_rx_buf[SPI_MAX_BURST_LENGTH + 1];  /**< RX buffer. */

/**
 * SPI event handler
 */
void spim_event_handler(nrfx_spim_evt_t const * p_event, void *p_context)
{
    // NOTE: there is only one event type: NRFX_SPIM_EVENT_DONE, so no need for case statement
    nrf_gpio_pin_set(m_spi_cs_pin);
    m_spi_xfer_done = true;
}

/**
 * Configure the SPI peripheral.
 */
void spi_init(void)
{
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG(SPIM0_SCK_PIN, SPIM0_MOSI_PIN, SPIM0_MISO_PIN, NRFX_SPIM_PIN_NOT_USED);
    spi_config.frequency      = NRF_SPIM_FREQ_1M;
    spi_config.mode           = NRF_SPIM_MODE_3;
    spi_config.bit_order      = NRF_SPIM_BIT_ORDER_LSB_FIRST;
    CHECK(nrfx_spim_init(&m_spi, &spi_config, spim_event_handler, NULL));

    // configure CS pins (for active low)
    nrf_gpio_pin_set(SPIM0_DISP_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_DISP_CS_PIN);
    nrf_gpio_pin_set(SPIM0_FPGA_CS_PIN);
    nrf_gpio_cfg_output(SPIM0_FPGA_CS_PIN);
    m_spi_cs_pin = SPIM0_DISP_CS_PIN; // default to this
    // initialze xfer state (needed for init/uninit cycles)
    m_spi_xfer_done = true;
}

/**
 * Reset the SPI peripheral.
 */
void spi_uninit(void)
{
    // return pins to default state (input, hi-z)
    nrf_gpio_cfg_default(SPIM0_FPGA_CS_PIN);
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

/**
 * Set the SPI CS GPIO pin associated with the SPI peripheral for manual control.
 * @param cs_pin GPIO pin number.
 */
void spi_set_cs_pin(uint8_t cs_pin)
{
    assert(cs_pin == SPIM0_DISP_CS_PIN || cs_pin == SPIM0_FPGA_CS_PIN);
    m_spi_cs_pin = cs_pin;
}

/**
 * Write a burst of multiple bytes over SPI, prefixed by an address field.
 * @param addr First byte of the SPI transaction, indicating an address.
 * @param data Data buffer to send.
 * @param length Length of the data buffer.
 */
void spi_write_burst(uint8_t addr, const uint8_t *data, uint16_t length)
{
    assert(length <= SPI_MAX_BURST_LENGTH);

    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buf, length + 1, m_rx_buf, length + 1);
    m_tx_buf[0]=addr;
    memcpy(&m_tx_buf[1], data, length + 1);

    // wait for any pending SPI operation to complete
    while (!m_spi_xfer_done) {
        __WFE();
    }

    // Reset rx buffer and transfer done flag
    memset(m_rx_buf, 0, length + 1);
    m_spi_xfer_done = false;

    // set the current CS pin low (will be set high by event handler)
    nrf_gpio_pin_clear(m_spi_cs_pin);

    // CS must remain asserted for both bytes (verified with scope)
    CHECK(nrfx_spim_xfer(&m_spi, &xfer_desc, 0));

    // Wait until SPI Master completes transfer data
    while (!m_spi_xfer_done) {
        __WFE();
    }
}

/**
 * Write a single byte over SPI
 * @param addr Address sent before the byte.
 * @param data Byte to send after the address.
 */
void spi_write_byte(uint8_t addr, uint8_t byte)
{
    spi_write_burst(addr, &byte, 1);
}

/**
 * Read a burst of multiple bytes at an address.
 * @param addr Address sent before the data is read.
 * @param length Number of bytes to read.
 * @return pointer to data, but data will be overwritten by the next SPI read or write
 *  so caller must guarantee processing of the data before the next SPI transaction.
 */
uint8_t *spi_read_burst(uint8_t addr, uint16_t length)
{
    assert(length > 0);
    assert(length <= SPI_MAX_BURST_LENGTH);

    // set RD_ON register to 1; CS should go high after
    spi_write_byte(0x80, 0x01);

    // write address of target register to RD_ADDR register; CS should go high after
    spi_write_byte(0x81, addr);

    // clear receive buffer in case there is anything left from earlier activity
    memset(m_rx_buf, 0, length + 1);

    // set TX buffer with don't cares (we will write 00) to write out as we read
    memset(m_tx_buf, 0, length + 1);

    // read data from target register by:
    // write RD_ADDR address to SI pin
    // write bytes, value don't care
    // SO pin will active during this second write, and data will go into Rx buffer
    spi_write_burst(0x81, m_tx_buf, length);

    // address contents should have been sent on MISO pin during second byte; read from Rx buffer
    return &m_rx_buf[1];
}

/*
 * Read a single byte over SPI at the given address.
 * @param addr Byte sent before the data is read.
 * @return The byte read after the address is sent.
 */
uint8_t spi_read_byte(uint8_t addr)
{
    uint8_t byte;

    // set RD_ON register to 1; CS should go high after
    spi_write_byte(0x80, 0x01);

    // write address of target register to RD_ADDR register; CS should go high after
    spi_write_byte(0x81, addr);

    // clear receive buffer in case there is anything left from earlier activity
    memset(m_rx_buf, 0, 2);

    // read data from target register by:
    // write RD_ADDR address to SI pin
    // write a second byte, value don't care (we will write 00)
    // SO pin will active during this second write, and data will go into Rx buffer
    spi_write_byte(0x81, 0x00);

    // address contents should have been sent on MISO pin during second byte; read from Rx buffer
    byte = m_rx_buf[1];

    LOG("SPI: read addr=%02X byte=%02X", addr, byte);

    return byte;
}

/**
 * Attempt multiple times to read and write bytes at the same address.
 * The read value is compared with the write value.
 * @pre Correct CS pin must be set (for OLED or FPGA).
 * @param addr Address being tseted.
 * @return True if all tests pass.
 */
bool spi_exercise_register(uint8_t addr)
{
    bool success = true;
    uint8_t original_value = spi_read_byte(addr);
    uint8_t write_value = 0x00;
    uint8_t read_value = 0x00;

    while (success && (write_value < 0xFF)) { // count up
        spi_write_byte(addr, write_value);
        read_value = spi_read_byte(addr);
        success = (read_value == write_value);
        if (success) {
            write_value++;
        } else {
            NRFX_LOG_ERROR("SPI ERROR: register(0x%x) = 0x%x, expected 0x%x", addr, read_value, write_value);
        }
    }
    if (success) { // count down
        //LOG("SPI test: register (0x%x) count up passed.", addr);
        while (success && (write_value > 0x00)) { // count up
            spi_write_byte(addr, write_value);
            read_value = spi_read_byte(addr);
            success = (read_value == write_value);
            if (success) {
                write_value--;
            } else {
                NRFX_LOG_ERROR("SPI ERROR: register(0x%x) = 0x%x, expected 0x%x", addr, read_value, write_value);
            }
        }
    }
    if (success) {
        //LOG("SPI test: register (0x%x) count down passed.", addr);
    }

    // attempt to restore original value, but this might fail if !success
    spi_write_byte(addr, original_value);

    return(success);
}
