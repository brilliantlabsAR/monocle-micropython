/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#include "monocle.h"
#include "py/mphal.h"
#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrfx_twim.h"

void monocle_set_led(led_t led, bool enable)
{
    if (led == RED_LED)
    {
        if (enable)
        {
            app_err(i2c_write(PMIC_I2C_ADDRESS, 0x11, 0x2D, 0x00).fail);
        }
        else
        {
            app_err(i2c_write(PMIC_I2C_ADDRESS, 0x11, 0x2D, 0x08).fail);
        }
    }
    if (led == GREEN_LED)
    {
        if (enable)
        {
            app_err(i2c_write(PMIC_I2C_ADDRESS, 0x12, 0x2D, 0x00).fail);
        }
        else
        {
            app_err(i2c_write(PMIC_I2C_ADDRESS, 0x12, 0x2D, 0x08).fail);
        }
    }
}

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);
static const nrfx_spim_t spi_bus_2 = NRFX_SPIM_INSTANCE(2);

bool not_real_hardware_flag = false;

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint16_t register_address,
                        uint8_t register_mask)
{
    if (not_real_hardware_flag)
    {
        return (i2c_response_t){.fail = false, .value = 0x00};
    }

    // Populate the default response in case of failure
    i2c_response_t i2c_response = {
        .fail = true,
        .value = 0x00,
    };

    // Create the tx payload, bus handle and transfer descriptors
    uint8_t tx_payload[2] = {(uint8_t)(register_address), 0};

    nrfx_twim_t i2c_handle = i2c_bus_0;

    nrfx_twim_xfer_desc_t i2c_tx = NRFX_TWIM_XFER_DESC_TX(device_address_7bit,
                                                          tx_payload,
                                                          1);

    // Switch bus and use 16-bit addressing if the camera is requested
    if (device_address_7bit == CAMERA_I2C_ADDRESS)
    {
        i2c_handle = i2c_bus_1;
        tx_payload[0] = (uint8_t)(register_address >> 8);
        tx_payload[1] = (uint8_t)register_address;
        i2c_tx.primary_length = 2;
    }

    nrfx_twim_xfer_desc_t i2c_rx = NRFX_TWIM_XFER_DESC_RX(device_address_7bit,
                                                          &i2c_response.value,
                                                          1);

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_err_t tx_err = nrfx_twim_xfer(&i2c_handle, &i2c_tx, 0);

        if (tx_err == NRFX_ERROR_BUSY ||
            tx_err == NRFX_ERROR_NOT_SUPPORTED ||
            tx_err == NRFX_ERROR_INTERNAL ||
            tx_err == NRFX_ERROR_INVALID_ADDR ||
            tx_err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(tx_err);
        }

        nrfx_err_t rx_err = nrfx_twim_xfer(&i2c_handle, &i2c_rx, 0);

        if (rx_err == NRFX_ERROR_BUSY ||
            rx_err == NRFX_ERROR_NOT_SUPPORTED ||
            rx_err == NRFX_ERROR_INTERNAL ||
            rx_err == NRFX_ERROR_INVALID_ADDR ||
            rx_err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(rx_err);
        }

        if (tx_err == NRFX_SUCCESS && rx_err == NRFX_SUCCESS)
        {
            i2c_response.fail = false;
            break;
        }
    }

    i2c_response.value &= register_mask;

    return i2c_response;
}

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint16_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value)
{
    i2c_response_t resp = {.fail = false, .value = 0x00};

    if (not_real_hardware_flag)
    {
        return resp;
    }

    if (register_mask != 0xFF)
    {
        resp = i2c_read(device_address_7bit, register_address, 0xFF);

        if (resp.fail)
        {
            return resp;
        }
    }

    // Create a combined value with the existing data and the new value
    uint8_t updated_value = (resp.value & ~register_mask) |
                            (set_value & register_mask);

    // Create the tx payload, bus handle and transfer descriptor
    uint8_t tx_payload[3] = {(uint8_t)register_address, updated_value, 0};

    nrfx_twim_t i2c_handle = i2c_bus_0;

    nrfx_twim_xfer_desc_t i2c_tx = NRFX_TWIM_XFER_DESC_TX(device_address_7bit,
                                                          tx_payload,
                                                          2);

    // Switch bus and use 16-bit addressing if the camera is requested
    if (device_address_7bit == CAMERA_I2C_ADDRESS)
    {
        i2c_handle = i2c_bus_1;
        tx_payload[0] = (uint8_t)(register_address >> 8);
        tx_payload[1] = (uint8_t)register_address;
        tx_payload[2] = updated_value;
        i2c_tx.primary_length = 3;
    }

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_err_t err = nrfx_twim_xfer(&i2c_handle, &i2c_tx, 0);

        if (err == NRFX_ERROR_BUSY ||
            err == NRFX_ERROR_NOT_SUPPORTED ||
            err == NRFX_ERROR_INTERNAL ||
            err == NRFX_ERROR_INVALID_ADDR ||
            err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(err);
        }

        if (err == NRFX_SUCCESS)
        {
            break;
        }

        // If the last try failed. Don't continue
        if (i == 2)
        {
            resp.fail = true;
            NRFX_LOG_ERROR("failed to write 0x%02X = 0x%02X", register_address, set_value);
            return resp;
        }
    }

    return resp;
}

void spi_read(spi_device_t spi_device, uint8_t *data, size_t length)
{
    uint8_t cs_pin;

    switch (spi_device)
    {
    case DISPLAY:
        cs_pin = DISPLAY_CS_PIN;
        break;
    case FPGA:
        cs_pin = FPGA_CS_MODE_PIN;
        break;
    case FLASH:
        cs_pin = FLASH_CS_PIN;
        break;
    }

    nrf_gpio_pin_clear(cs_pin);

    // TODO prevent blocking here, and add a mutex
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(data, length);
    app_err(nrfx_spim_xfer(&spi_bus_2, &xfer, 0));

    nrf_gpio_pin_set(cs_pin);
}

void spi_write(spi_device_t spi_device, uint8_t *data, size_t length,
               bool hold_down_cs)
{
    uint8_t cs_pin;

    switch (spi_device)
    {
    case DISPLAY:
        cs_pin = DISPLAY_CS_PIN;
        break;
    case FPGA:
        cs_pin = FPGA_CS_MODE_PIN;
        break;
    case FLASH:
        cs_pin = FLASH_CS_PIN;
        break;
    }

    nrf_gpio_pin_clear(cs_pin);

    // TODO prevent blocking here, and add a mutex
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(data, length);
    app_err(nrfx_spim_xfer(&spi_bus_2, &xfer, 0));

    if (!hold_down_cs)
    {
        nrf_gpio_pin_set(cs_pin);
    }
}

void spi_release(void)
{
    nrf_gpio_cfg_input(FLASH_CS_PIN, NRF_GPIO_PIN_NOPULL);

    nrf_gpio_cfg(FPGA_FLASH_SPI_SCK_PIN,
                 NRF_GPIO_PIN_DIR_INPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(FPGA_FLASH_SPI_SDO_PIN,
                 NRF_GPIO_PIN_DIR_INPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);
}

void spi_acquire(void)
{
    nrf_gpio_cfg_output(FLASH_CS_PIN);

    nrf_gpio_cfg(FPGA_FLASH_SPI_SCK_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(FPGA_FLASH_SPI_SDO_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);
}

uint8_t bit_reverse(uint8_t byte)
{
    byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
    byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
    byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
    return byte;
}