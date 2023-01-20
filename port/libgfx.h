/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
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

typedef enum
{
    GFX_TYPE_NULL,      // skip this object
    GFX_TYPE_RECTANGLE, // filled
    GFX_TYPE_LINE,      // diagonal line
    GFX_TYPE_ELLIPSIS,  // diagonal line
    GFX_TYPE_TEXTBOX,   // box filled with text, clipped at the bottom
} gfx_type_t;

typedef struct
{
    uint16_t x, y, width, height;
    union
    {
        void *ptr;
        uint32_t u32;
    } u;
    uint8_t yuv444[3];
    uint8_t type;
} gfx_obj_t;

#define GFX_RGB_TO_YUV444(r, g, b) { \
    128.0 + 0.29900 * (r) + 0.58700 * (g) + 0.11400 * (b) - 128.0, \
    128.0 - 0.16874 * (r) - 0.33126 * (g) + 0.50000 * (b), \
    128.0 + 0.50000 * (r) - 0.41869 * (g) - 0.08131 * (b) \
}

#define GFX_YUV422_BLACK    { 0x80, 0x00 }

void gfx_set_color(gfx_obj_t *gfx, uint16_t line_num, gfx_obj_t *obj_list, size_t obj_num);
bool gfx_render_row(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj_list, size_t obj_num);
void gfx_fill_black(uint8_t *yuv422_buf, size_t yuv422_len);
