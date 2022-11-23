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
 * @defgroup BLE
 * @{
 */

void ble_init(void);
void ble_nus_tx(char const *buf, size_t sz);
int ble_nus_rx(void);
bool ble_nus_is_rx_pending(void);

/** @} */
#endif
