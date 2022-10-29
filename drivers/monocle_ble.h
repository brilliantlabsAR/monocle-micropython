/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef MONOCLE_BLE_H
#define MONOCLE_BLE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/**
 * Bluetooth Low Energy driver, providing the Nordic Uart Service console.
 *
 * @defgroup ble
 * @{
 */

void ble_init(void);
void ble_rfcomm_tx(char const *buf, size_t sz);
int ble_rfcomm_rx(void);
bool ble_rfcomm_is_rx_pending(void);

/** @} */
#endif
