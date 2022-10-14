/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef BOARD_H
#define BOARD_H

#include "nrf_gpio.h"

void board_init(void);
void board_uninit(void);
void board_check_recovery(void);
void board_pin_off(uint8_t pin);
void board_pin_on(uint8_t pin);
void board_aux_power_on(void); // only for MK9B, MK10?
void board_aux_power_off(void); // only for MK9B, MK10?

#endif
