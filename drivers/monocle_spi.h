/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stdbool.h>
#include "nrfx_spim.h"
/**
 * Wrapper over the NRFX driver to include a leading address sent first.
 * @defgroup SPI
 * @{
 */

// Current FPGA FIFO max length is 1024
#define SPI_MAX_XFER_LEN 254

void spi_init(void);
void spi_uninit(void);
void spi_read_buffer(uint8_t cs_pin, uint8_t addr, uint8_t *buf, uint16_t len);
void spi_write_buffer(uint8_t cs_pin, uint8_t addr, uint8_t *data, uint16_t len);
void spi_write_register(uint8_t cs_pin, uint8_t addr, uint8_t byte);
uint8_t spi_read_register(uint8_t cs_pin, uint8_t addr);

/** @} */
#endif
