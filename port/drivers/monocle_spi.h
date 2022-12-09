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
 * @defgroup spi
 */

// Current FPGA FIFO max length is 1024
#define SPI_MAX_XFER_LEN 254

void spi_init(void);
void spi_uninit(void);
void spi_chip_select(uint8_t cs_pin);
void spi_chip_deselect(uint8_t cs_pin);
void spi_xfer(uint8_t *buf, size_t len);

// compatibility wrappers

static uint8_t m_cs_pin;

static inline void spi_set_cs_pin(uint8_t cs_pin)
{
    m_cs_pin = cs_pin;
}

static inline uint8_t *spi_write_burst(uint8_t addr, uint8_t *buf, size_t len)
{
    buf[-1] = addr;
    spi_chip_select(m_cs_pin);
    spi_xfer(buf - 1, len + 1);
    spi_chip_deselect(m_cs_pin);
    return buf - 1;
}

#endif
