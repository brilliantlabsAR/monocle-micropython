/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stdbool.h>
#include "nrfx_twi.h"
/**
 * Driver for the hardware I2C of the board.
 *
 * @defgroup I2C
 * @{
 */

extern const nrfx_twi_t i2c0;
extern const nrfx_twi_t i2c1;
void i2c_init(void);
bool i2c_write(nrfx_twi_t twi, uint8_t addr, uint8_t *buf, uint8_t sz);
bool i2c_write_no_stop(nrfx_twi_t twi, uint8_t addr, uint8_t *buf, uint8_t sz);
bool i2c_read(nrfx_twi_t twi, uint8_t addr, uint8_t *readBuffer, uint8_t sz);
void i2c_scan(nrfx_twi_t twi);

/** @} */
#endif
