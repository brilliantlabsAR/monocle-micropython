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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/config.h"
#include "libgfx.h"
#include "font.h"

#include "nrfx_log.h"

#define LEN(x)  (sizeof(x) / sizeof*(x))

typedef struct
{
    uint8_t width, height;
    uint8_t const *bitmap;
} gfx_glyph_t;

uint8_t const *gfx_font = font_26;

static inline void gfx_draw_pixel(uint8_t *yuv422_buf, uint16_t x, uint8_t yuv444[3])
{
    yuv422_buf[0] = yuv444[1 + x % 2];
    yuv422_buf[1] = yuv444[0];
}

static inline void gfx_draw_segment(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t beg, uint16_t end, uint8_t yuv444[3])
{
    for (size_t len = yuv422_len / 3, x = beg; x < end && x < len; x++) {
        gfx_draw_pixel(yuv422_buf, x, yuv444);
    }
}

static void gfx_render_rectangle(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj)
{
    (void)y;
    gfx_draw_segment(yuv422_buf, yuv422_len, obj->x, obj->x + obj->width, obj->yuv444);
}

static uint16_t gfx_get_intersect_line(uint16_t y,
        uint16_t obj_x, uint16_t obj_y, uint16_t obj_width, uint16_t obj_height)
{
    // Thales theorem to find the intersection of the line with our line.
    // y0--------------------------+ [a1,b2] is the line we draw
    // a0                _,a1      | [b2,c2] is obj_width, the bounding box width
    // |             _,-'   |      | [a1,c2] is obj_height, the bounding box height
    // y1--------_b1'-------c1-----| [b1,c1] is seg_width, what we want to know
    // |     _.-'           |      | [a0,y1] is seg_height, which we know
    // |   b2. . . . . . . .c2     | [y0,y1] is y, the position of the line to render
    // +---------------------------+
    // seg_width / obj_width = seg_height / obj_height
    // seg_width = obj_width * seg_height / obj_height
    uint16_t seg_height = y - obj_y;
    uint16_t seg_width = obj_width * seg_height / obj_height;
    return obj_x + obj_width - seg_width;
}

static void gfx_render_line(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj)
{
    uint16_t x_beg, x_end;

    // Special case: purely horizontal line, fill as rectangle
    if (obj->height == 0) {
        gfx_render_rectangle(yuv422_buf, yuv422_len, y, obj);
        return;
    }

    // We need to know how many horizontal pixels to drawn to accomodate the line thickness
    // so we get two intersections: the one for the top, and the one for the bottom edge of
    // the line. This introduces an offset, which we correct with the +1
    x_beg = gfx_get_intersect_line(y + 1, obj->x, obj->y, obj->width, obj->height + 1);
    x_end = gfx_get_intersect_line(y, obj->x, obj->y, obj->width, obj->height + 1);

    // We have the start and stop point of the segment, we can fill it
    gfx_draw_segment(yuv422_buf, yuv422_len, x_beg, x_end, obj->yuv444);
}

static gfx_glyph_t gfx_get_glyph(uint8_t const *font, char c)
{
    gfx_glyph_t glyph;
    uint8_t const *f = font;

    // Only ASCII is supported for this early release
    // see how https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
    // encoded lookup tables for a strategy to support UTF-8.
    assert(c >= ' ' || c <= '~');

    // Store the global font header properties.
    glyph.height = *f++;

    // Scan the font, seeking for the glyph to render.
    for (char i = ' '; i <= c; i++) {
        glyph.width = *f++;
        glyph.bitmap = f;
        f += (glyph.width * glyph.height + 7) / 8;
    }
    return glyph;
}

static inline bool gfx_get_glyph_bit(gfx_glyph_t *glyph, uint16_t x, uint16_t y)
{
    size_t i = y * glyph->width + x;

    // See the txt2cfont tool to understand this encoding.
    return glyph->bitmap[i / 8] & 1 << (i % 8);
}

static size_t gfx_draw_glyph(uint8_t *yuv422_buf, size_t yuv422_len, gfx_glyph_t *glyph, uint16_t y, uint8_t yuv[3])
{
    // for each vertical position
    for (uint16_t x = 0; x < glyph->width && x * 3 < yuv422_len; x++) {

        // check if the bit is set
        if (gfx_get_glyph_bit(glyph, x, y) == true) {

            // and only if so, fill the buffer with it
            gfx_draw_pixel(yuv422_buf, x, yuv);
        }
    }
    return glyph->width;
}

static void gfx_render_textbox(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj)
{
    char const *txt = "welcome aboard, remember to look around";
    uint16_t x_beg = obj->x;
    uint16_t x_end = obj->x + obj->width;
    uint16_t space_width = 2;

    // Only support a single line of text for now
    if (y - obj->y >= gfx_font[0]) {
        return;
    }

    for (uint16_t n = 0, x = x_beg; *txt != '\0'; x += n) {
        gfx_glyph_t glyph;

        // search the glyph within the font data
        glyph = gfx_get_glyph(gfx_font, *txt++);

        // stop if we are about to overflow the textbox
        if (x >= x_end || x * 3 > yuv422_len)
            break;

        // render the glyph, reduce the buffer to only the section to
        // draw into, vertical is adjusted to be height within the glyph
        n = gfx_draw_glyph(yuv422_buf + x*3, yuv422_len, &glyph, y - obj->y, obj->yuv444);
        n += space_width;
    }
}

static void gfx_render_ellipsis(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj)
{
    // TODO implement ellipsis
}

void gfx_fill_black(uint8_t *yuv422_buf, size_t yuv422_len)
{
    for (size_t i = 0; i < yuv422_len; i++)
    {
        yuv422_buf[i] = (i % 2 == 0) ? 0x80 : 0x00;
    }
}

void gfx_render_row(uint8_t *yuv422_buf, size_t yuv422_len, uint16_t y, gfx_obj_t *obj_list, size_t obj_num)
{
    for (size_t i = 0; i < obj_num; i++) {
        gfx_obj_t *obj = obj_list + i;

        if (y < obj->y || y > obj->y + obj->height) {
            continue;
        }

        switch (obj->type) {
        case GFX_TYPE_RECTANGLE:
        LOG("GFX_TYPE_RECTANGLE len=%d y=%d obj={x=%d y=%d w=%d h=%d}", yuv422_len, y, obj->x, obj->y, obj->width, obj->height);
            gfx_render_rectangle(yuv422_buf, yuv422_len, y, obj);
            break;
        case GFX_TYPE_LINE:
        LOG("GFX_TYPE_LINE y=%d obj={x=%d y=%d w=%d h=%d}", yuv422_len, y, obj->x, obj->y, obj->width, obj->height);            gfx_render_line(yuv422_buf, yuv422_len, y, obj);
            break;
        case GFX_TYPE_TEXTBOX:
        LOG("GFX_TYPE_TEXTBOX y=%d obj={x=%d y=%d w=%d h=%d}", yuv422_len, y, obj->x, obj->y, obj->width, obj->height);            gfx_render_textbox(yuv422_buf, yuv422_len, y, obj);
            break;
        case GFX_TYPE_ELLIPSIS:
        LOG("GFX_TYPE_ELLIPSIS y=%d obj={x=%d y=%d w=%d h=%d}", yuv422_len, y, obj->x, obj->y, obj->width, obj->height);            gfx_render_ellipsis(yuv422_buf, yuv422_len, y, obj);
            break;
        default:
            assert(!"unknown type");
        }
    }
}
