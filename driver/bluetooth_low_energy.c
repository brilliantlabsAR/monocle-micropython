/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Raj Nakarja - Silicon Witchery AB
 * Authored by: Josuah Demangeon - Panoramix Labs
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
 * Bluetooth Low Energy (BLE) driver with Nordic UART Service console.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "ble.h"
#include "nrf_clock.h"
#include "nrf_sdm.h"
#include "nrf_nvic.h"
#include "nrfx_log.h"
#include "monocle.h"

#include "driver/bluetooth_low_energy.h"

/** List of all services we might get a connection for. */
ble_service_t ble_nus_service;
ble_service_t ble_raw_service;

/** Identifier for the active connection with a single device. */
uint16_t ble_conn_handle = BLE_CONN_HANDLE_INVALID;

/** Advertising configured globally for all services. */
uint8_t ble_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

/** MTU length obtained by the negotiation with the currently connected peer. */
uint16_t ble_negotiated_mtu;

ring_buf_t nus_rx;
ring_buf_t nus_tx;

bool ring_full(ring_buf_t const *ring)
{
    uint16_t next = ring->tail + 1;
    if (next == sizeof(ring->buffer))
        next = 0;
    return next == ring->head;
}

bool ring_empty(ring_buf_t const *ring)
{
    return ring->head == ring->tail;
}

void ring_push(ring_buf_t *ring, uint8_t byte)
{
    ring->buffer[ring->tail++] = byte;
    if (ring->tail == sizeof(ring->buffer))
        ring->tail = 0;
}

uint8_t ring_pop(ring_buf_t *ring)
{
    uint8_t byte = ring->buffer[ring->head++];
    if (ring->head == sizeof(ring->buffer))
        ring->head = 0;
    return byte;
}

/**
 * Send a buffer out, retrying continuously until it goes to completion (with success or failure).
 */
static void ble_tx(ble_service_t *service, uint8_t const *buf, uint16_t len)
{
    nrfx_err_t err;
    ble_gatts_hvx_params_t hvx_params = {
        .handle = service->tx_characteristic.value_handle,
        .p_data = buf,
        .p_len = (uint16_t *)&len,
        .type = BLE_GATT_HVX_NOTIFICATION,
    };

    do
    {
        app_err(ble_conn_handle == BLE_CONN_HANDLE_INVALID);

        // Send the data
        err = sd_ble_gatts_hvx(ble_conn_handle, &hvx_params);

        // Retry if resources are unavailable.
    } while (err == NRF_ERROR_RESOURCES);

    // Ignore errors if not connected
    if (err == NRF_ERROR_INVALID_STATE || err == BLE_ERROR_INVALID_CONN_HANDLE)
        return;

    // Catch other errors
    app_err(err);
}

void ble_raw_tx(uint8_t const *buf, uint16_t len)
{
    ble_tx(&ble_raw_service, buf, len);
}

/**
 * Sends all buffered data in the tx ring buffer over BLE.
 */
static void ble_nus_flush_tx(void)
{
    // Local buffer for sending data
    uint8_t buf[BLE_MAX_MTU_LENGTH] = "";
    uint16_t len = 0;

    // If not connected, do not flush.
    if (ble_conn_handle == BLE_CONN_HANDLE_INVALID)
        return;

    // If there's no data to send, simply return
    if (ring_empty(&nus_tx))
        return;

    // For all the remaining characters, i.e until the heads come back together
    while (!ring_empty(&nus_tx))
    {
        // Copy over a character from the tail to the outgoing buffer
        buf[len++] = ring_pop(&nus_tx);

        // Break if we over-run the negotiated MTU size, send the rest later
        if (len >= ble_negotiated_mtu)
            break;
    }
    ble_tx(&ble_nus_service, buf, len);
}

int ble_nus_rx(void)
{
    while (ring_empty(&nus_rx))
    {
        // While waiting for incoming data, we can push outgoing data
        ble_nus_flush_tx();

        // If there's nothing to do
        if (ring_empty(&nus_tx) && ring_empty(&nus_rx))
            // Wait for events to save power
            sd_app_evt_wait();
    }

    // Return next character from the RX buffer.
    return ring_pop(&nus_rx);
}

void ble_nus_tx(char const *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        while (ring_full(&nus_tx))
            ble_nus_flush_tx();
        ring_push(&nus_tx, buf[i]);
    }
}

bool ble_nus_is_rx_pending(void)
{
    return ring_empty(&nus_rx);
}
