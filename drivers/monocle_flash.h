/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
#include <stdbool.h>
#include "nrfx_spim.h"
/**
 * SPI flash chip I/O library for reading/writing to/from the onboard flash chip.
 * for programming the FPGA.
 * @defgroup Flash
 * @{
 */

#define FLASH_PAGE_SIZE 256

void flash_prepare(void);
void flash_init(void);
uint32_t flash_get_id(void);
void flash_program_page(uint32_t addr, uint8_t const page[FLASH_PAGE_SIZE]);
void flash_read(uint32_t addr, uint8_t *buf, size_t len);
void flash_erase_chip(void);

/** @} */
#endif
