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
 * Wrapper library over Nordic NRFX I2C drivers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_twi.h"

#include "driver_i2c.h"
#include "nrf_gpio.h"
#include "driver_config.h"
#include "nrfx_log.h"

#include "driver_board.h"
#include "driver_config.h"
#include "driver_i2c.h"

#define LOG     NRFX_LOG_OFF
#define ASSERT  NRFX_ASSERT

const nrfx_twi_t i2c0 = NRFX_TWI_INSTANCE(0);
const nrfx_twi_t i2c1 = NRFX_TWI_INSTANCE(1);

/** TWI operation ended, may have been successful, may have been NACK. */
static volatile bool m_xfer_done = false;

/** NACK returned, operation was unsuccessful. */
static volatile bool m_xfer_nack = false;

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline bool i2c_filter_error(char const *func, nrfx_err_t err)
{
    switch (err) {
        case NRFX_SUCCESS:
            return true;
        case NRFX_ERROR_DRV_TWI_ERR_ANACK:
            return false;
        default:
            LOG("%s, %s", func, NRFX_LOG_ERROR_STRING_GET(err));
            return false;
    }
}

/**
 * Configure the hardware I2C instance as well as software-based I2C instance.
 */
// TODO: validate that 400kH speed works & increase to that
void i2c_init(void)
{
    uint32_t err;
    nrfx_twi_config_t config = {0};

    config.scl                = I2C0_SCL_PIN;
    config.sda                = I2C0_SDA_PIN;
    config.frequency          = NRF_TWI_FREQ_100K;
    config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;

    err = nrfx_twi_init(&i2c0, &config, NULL, NULL);
    ASSERT(err == NRFX_SUCCESS);

    nrfx_twi_enable(&i2c0);

    config.scl                = I2C1_SCL_PIN;
    config.sda                = I2C1_SDA_PIN;
    config.frequency          = NRF_TWI_FREQ_100K;
    config.interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY;

    err = nrfx_twi_init(&i2c1, &config, NULL, NULL);
    ASSERT(err == NRFX_SUCCESS);

    nrfx_twi_enable(&i2c1);

    LOG("ready nrfx=twi");
}

/**
 * Write a buffer over I2C (hardware-based instance).
 * @param addr The address at which write the data.
 * @param buf The buffer to write.
 * @param sz The length of that bufer.
 * @return True if no I2C errors were reported.
 */
bool i2c_write(nrfx_twi_t twi, uint8_t addr, uint8_t *buf, uint8_t sz)
{
    nrfx_twi_xfer_desc_t xfer = NRFX_TWI_XFER_DESC_TX(addr, buf, sz);
    return i2c_filter_error(__func__, nrfx_twi_xfer(&twi, &xfer, 0));
}

/**
 * Write a buffer over I2C without stop condition.
 * The I2C transaction will still be ongoing, permitting to more data to be written.
 * @param addr The address at which write the data.
 * @param buf The buffer containing the data to write.
 * @param sz The length of that bufer.
 * @return True if no I2C errors were reported.
 */
bool i2c_write_no_stop(nrfx_twi_t twi, uint8_t addr, uint8_t *buf, uint8_t sz)
{
    nrfx_twi_xfer_desc_t xfer = NRFX_TWI_XFER_DESC_TX(addr, buf, sz);
    return i2c_filter_error(__func__, nrfx_twi_xfer(&twi, &xfer, NRFX_TWI_FLAG_TX_NO_STOP));
}

/**
 * Read a buffer from I2C.
 * @param addr Address of the peripheral.
 * @param buf The buffer receiving the data.
 * @param sz The length of that bufer.
 * @return True if no I2C errors were reported.
 */
bool i2c_read(nrfx_twi_t twi, uint8_t addr, uint8_t *buf, uint8_t sz)
{
    nrfx_twi_xfer_desc_t xfer = NRFX_TWI_XFER_DESC_RX(addr, buf, sz);;
    return i2c_filter_error(__func__, nrfx_twi_xfer(&twi, &xfer, 0));
}

/**
 * For debugging, scan the twi and expect MBR3@55=0x37 & OV5640@60=0x3C.
 * @return True if all expected addresses were found.
 */
void i2c_scan(nrfx_twi_t twi)
{
    uint8_t addr;
    uint8_t sample_data;
    bool detected_device = false;

    for (addr = 1; addr <= 127; addr++) {
        if (i2c_read(twi, addr, &sample_data, sizeof(sample_data))) {
            detected_device = true;
            LOG("I2C device found: addr=0x%02X", addr);
        }
    }
    if (! detected_device)
        LOG("No I2C device found");
}
