/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 * Authored by: Shreyas Hemachandra <shreyas.hemachandran@gmail.com>
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/checksum.h"

/**
 * This function add each bytes of the buffer (passes) of size (passed).
 * @param Pointer to the buffer
 * @param Size of the buffer
 * @return The sum of each byte in the buffer.
 */
static uint32_t add_octets(uint8_t *buffer, uint32_t size)
{
    uint32_t sum = 0u;
    int i = 0;

    for (; i < size; i++)
    {
        sum += (uint32_t)buffer[i];
    }

    return sum;
}

/**
 * This function does the 2's Complement of the passed uint32_t value.
 * @param uint32_t, value
 * @return 2's Complement.
 */
static uint32_t do_2sComp(uint32_t val)
{
    val  = ~val;
    val += 1;
    return val;
}

/**
 * Calulates the checksum of a sized buffer.
 * @param buffer Pointer to the buffer
 * @param size Size of the buffer
 * @return The 32-bit checksum of the buffer.
 */
uint32_t cal_checksum(uint8_t *buffer, uint32_t size)
{
    uint32_t checksum = 0u;

    checksum = add_octets(buffer, size);
    checksum = do_2sComp(checksum);

    return checksum;
}

/**
 * This function calulates the checksum of the buffer (Passed) of
 *  size (Passed), stores the checksum in the last 32byte address 
 *  and returns the checksum
 * @param buffer Pointer to the buffer.
 * @param size Size of the buffer including the checksum word.
 * @return The checksum of the buffer passed.
 */
uint32_t calnstr_checksum(uint8_t *buffer, uint32_t size)
{
    uint32_t  checksum = 0u;
    uint32_t* pchecksumAddr = NULL;

    checksum = cal_checksum(buffer, (size - 4));

    // Copy checksum to last 4 buys of the buffer
    pchecksumAddr  = (uint32_t *) &buffer[size - 4];
    *pchecksumAddr = checksum;

    return checksum;
}

/**
 * This function calculates the checksum of the buffer compares it to the
 *  last word and returns a Bool variable if both matches or not.
 * @param buffer Pointer to the buffer.
 * @param size Size of the buffer including the checksum word.
 * @return 1 for checksum matched and 0 for checksum mismatch
 */
bool verify_checksum(uint8_t *buffer, uint32_t size)
{
    uint32_t  calchecksum = 0u;
    uint32_t* pchecksum = NULL;

    calchecksum = add_octets(buffer, (size - 4));
    pchecksum   = (uint32_t*)&buffer[size - 4];

    // Sum the checksum and check if its 0
    if (0 == (calchecksum + *pchecksum))
        return true;
    else
        return false;
}
