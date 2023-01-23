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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/config.h"
#include "driver/i2c.h"
#include "nrfx_twim.h"
#include "app_err.h"

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);

static bool not_real_hardware = false;

void i2c_unused_callback(nrfx_twim_evt_t const *p_event, void *p_context)
{
    // (void)p_event;
    // (void)p_context;
    return;
}

void i2c_init(void)
{
    nrfx_twim_config_t bus_0_config = NRFX_TWIM_DEFAULT_CONFIG(PMIC_TOUCH_I2C_SCL_PIN,
                                                               PMIC_TOUCH_I2C_SDA_PIN);
    bus_0_config.frequency = NRF_TWIM_FREQ_400K;

    nrfx_twim_config_t bus_1_config = NRFX_TWIM_DEFAULT_CONFIG(CAMERA_I2C_SCL_PIN,
                                                               CAMERA_I2C_SDA_PIN);
    bus_1_config.frequency = NRF_TWIM_FREQ_400K;

    app_err(nrfx_twim_init(&i2c_bus_0, &bus_0_config, i2c_unused_callback, NULL));
    app_err(nrfx_twim_init(&i2c_bus_1, &bus_1_config, i2c_unused_callback, NULL));

    nrfx_twim_enable(&i2c_bus_0);
    nrfx_twim_enable(&i2c_bus_1);
}

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint8_t register_address,
                        uint8_t register_mask)
{
    if (not_real_hardware)
    {
        return (i2c_response_t){.fail = false, .value = 0x00};
    }

    i2c_response_t i2c_response = {
        .fail = true, // Default if failure
        .value = 0x00,
    };

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_twim_xfer_desc_t i2c_xfer = NRFX_TWIM_XFER_DESC_TXRX(device_address_7bit,
                                                                  &register_address, 1,
                                                                  &i2c_response.value, 1);

        nrfx_twim_t i2c_handle = i2c_bus_0;
        if (device_address_7bit == OV5640_ADDR)
        {
            i2c_handle = i2c_bus_1;
        }

        nrfx_err_t err = nrfx_twim_xfer(&i2c_handle, &i2c_xfer, 0);

        while (nrfx_twim_is_busy(&i2c_handle))
        {
        }

        if (err == NRFX_SUCCESS)
        {
            i2c_response.fail = false;
            break;
        }

        // Catch any errors except ESP_FAIL
        if (err == NRFX_ERROR_BUSY ||
            err == NRFX_ERROR_NOT_SUPPORTED ||
            err == NRFX_ERROR_INTERNAL ||
            err == NRFX_ERROR_INVALID_ADDR ||
            err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            assert(0); // TODO put something meaningful here
        }
    }

    i2c_response.value &= register_mask;

    return i2c_response;
}

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint8_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value)
{
    if (not_real_hardware)
    {
        return (i2c_response_t){.fail = false, .value = 0x00};
    }

    i2c_response_t resp = i2c_read(device_address_7bit, register_address, 0xFF);

    if (resp.fail)
    {
        return resp;
    }

    // Create a combined value with the existing data and the new value
    uint8_t updated_value = (resp.value & ~register_mask) |
                            (set_value & register_mask);

    uint8_t payload[] = {register_address, updated_value};

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_twim_xfer_desc_t i2c_xfer = NRFX_TWIM_XFER_DESC_TX(device_address_7bit,
                                                                payload,
                                                                2);

        nrfx_twim_t i2c_handle = i2c_bus_0;
        if (device_address_7bit == OV5640_ADDR)
        {
            i2c_handle = i2c_bus_1;
        }

        nrfx_err_t err = nrfx_twim_xfer(&i2c_handle, &i2c_xfer, 0);

        if (err == NRFX_SUCCESS)
        {
            break;
        }

        // Catch any errors except ESP_FAIL
        if (err == NRFX_ERROR_BUSY ||
            err == NRFX_ERROR_NOT_SUPPORTED ||
            err == NRFX_ERROR_INTERNAL ||
            err == NRFX_ERROR_INVALID_ADDR ||
            err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            assert(0); // TODO put something meaningful here
        }

        // If the last try failed. Don't continue
        if (i == 2)
        {
            resp.fail = true;
            return resp;
        }
    }

    // Check that the register was correctly updated
    resp = i2c_read(device_address_7bit, register_address, 0xFF);

    if (resp.value != updated_value)
    {
        resp.fail = true;
    }

    return resp;
}
