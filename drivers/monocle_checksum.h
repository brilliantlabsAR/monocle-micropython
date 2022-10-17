/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef CHECKSUM_H
#define CHECKSUM_H

/**
 * Compute and verify a 32-bit checksum of a byte buffer.
 * @defgroup checksum
 * @ingroup firmware
 * @{
 */

uint32_t cal_checksum(uint8_t *buffer, uint32_t size);
uint32_t calnstr_checksum(uint8_t *buffer, uint32_t size);
bool verify_checksum(uint8_t *buffer, uint32_t size);

/** @} */
#endif