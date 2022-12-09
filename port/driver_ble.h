/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef DRIVER_BLE_H
#define DRIVER_BLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Bluetooth Low Energy driver, providing the Nordic Uart Service console.
 * @defgroup ble
 */

void ble_init(void);
void ble_nus_tx(char const *buf, size_t sz);
int ble_nus_rx(void);
bool ble_nus_is_rx_pending(void);

#endif
