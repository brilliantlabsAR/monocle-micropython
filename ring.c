/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Raj Nakarja - Silicon Witchery AB
 * Authored by: Josuah Demangeon
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

#include "ring.h"

bool ring_full(ring_buf_t const *ring)
{
    uint16_t next = ring->tail + 1;

    if (next == sizeof(ring->buffer))
    {
        next = 0;
    }
    return next == ring->head;
}

bool ring_empty(ring_buf_t const *ring)
{
    return ring->head == ring->tail;
}

void ring_push(ring_buf_t *ring, uint8_t byte)
{
    ring->buffer[ring->tail++] = byte;

    if (ring->tail == sizeof(ring->buffer))
    {
        ring->tail = 0;
    }
}

uint8_t ring_pop(ring_buf_t *ring)
{
    uint8_t byte = ring->buffer[ring->head++];

    if (ring->head == sizeof(ring->buffer))
    {
        ring->head = 0;
    }
    return byte;
}
