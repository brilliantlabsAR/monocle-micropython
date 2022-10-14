#ifndef MONOCLE_BLE_H
#define MONOCLE_BLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void ble_init(void);
void ble_rfcomm_tx(char const *buf, size_t sz);
int ble_rfcomm_rx(void);
bool ble_rfcomm_is_rx_pending(void);

#endif
