/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon (name@email.com)
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * Wrapper over the NRFX driver to include a leading address sent first.
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

