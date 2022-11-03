/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef MONOCLE_CHECKSUM_H
#define MONOCLE_CHECKSUM_H
/**
 * Compute and verify a 32-bit checksum of a byte buffer.
 * @defgroup Checksum
 * @{
 */

uint32_t cal_checksum(uint8_t *buffer, uint32_t size);
uint32_t calnstr_checksum(uint8_t *buffer, uint32_t size);
bool verify_checksum(uint8_t *buffer, uint32_t size);

/** @} */
#endif
