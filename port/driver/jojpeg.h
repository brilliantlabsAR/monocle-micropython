/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Jon Olick <https://www.jonolick.com/contact.html>
 * Authored by: Josuah Demangeon <me@josuah.net>
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

typedef struct {
    // parameters
    uint16_t height, width;
    uint8_t components;
    bool subsample;

    // algorithm context
    uint8_t const *r, *g, *b;
    int dcy, dcu, dcv;
    int bit_buf, bit_cnt;

    // buffers
    uint8_t *rgb_buf;
    size_t rgb_len, rgb_pos;

    // callback
    void (*jpeg_write)(uint8_t*, size_t);

    // tables
    float fdtable_y[64], fdtable_uv[64];
} jojpeg_t;

extern void jojpeg_write(uint8_t const *buf, size_t len);

void jojpeg_start(jojpeg_t *ctx, size_t width, size_t height, uint8_t components, uint8_t quality);
bool jojpeg_append_16_rows(jojpeg_t *ctx, uint8_t *rgb_buf, size_t rgb_len);
