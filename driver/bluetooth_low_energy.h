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
 * Bluetooth Low Energy driver, providing the Nordic Uart Service console
 * and the custom media transfer protocol.
 */

#include "ble_gatts.h"
#include "ring.h"

#define BLE_MAX_MTU_LENGTH 128

/**
 * @brief Holds the handles for the conenction and characteristics.
 * Convenient for use in interrupts, to get all service-specific data
 * we need to carry around.
 */
typedef struct
{
    uint16_t handle;
    ble_gatts_char_handles_t rx_characteristic;
    ble_gatts_char_handles_t tx_characteristic;
} ble_service_t;

extern uint16_t ble_negotiated_mtu;

extern ring_buf_t nus_rx;
extern ring_buf_t nus_tx;
extern uint16_t ble_conn_handle;
extern uint8_t ble_adv_handle;
extern void ble_on_connect(void);
extern void ble_on_disconnect(void);
extern ble_service_t ble_nus_service;
extern ble_service_t ble_raw_service;

void ble_nus_tx(char const *buf, size_t len);
void ble_raw_tx(uint8_t const *buf, uint16_t len);
int ble_nus_rx(void);
void ble_tx(ble_service_t *service, uint8_t const *buf, uint16_t len);
