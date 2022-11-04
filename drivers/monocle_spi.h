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

#define SPI_MAX_BURST_LENGTH 254 // maximum length of burst write or read (Bytes)
#if (SPI_MAX_BURST_LENGTH >= UINT16_MAX)
#error "Reduce SPI_MAX_BURST_LENGTH or redefine length type."
#elif (SPI_MAX_BURST_LENGTH > 1024)
#error "Current FPGA FIFO limit is 1024 bytes."
#endif

void spi_init(void);
void spi_uninit(void);
void spi_set_cs_pin(uint8_t cs_pin);
void spi_write_byte(uint8_t addr, uint8_t data);
void spi_write_burst(uint8_t addr, const uint8_t *data, uint16_t length);
uint8_t spi_read_byte(uint8_t addr);
uint8_t *spi_read_burst(uint8_t addr, uint16_t length);

/** @} */
#endif
