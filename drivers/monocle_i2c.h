/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * Driver for the hardware I2C of the board.
 * @defgroup i2c
 * @ingroup driver_nrf52
 * @{
 */
void i2c_init(void);
void i2c_uninit(void);
bool i2c_write(uint8_t addr, uint8_t *buf, uint8_t sz);
bool i2c_write_no_stop(uint8_t addr, uint8_t *buf, uint8_t sz);
bool i2c_read(uint8_t addr, uint8_t *readBuffer, uint8_t sz);
void i2c_scan(void);
/** @} */

#endif
