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

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define LEN(x) (sizeof(x) / sizeof *(x))

typedef struct
{
    uint8_t width, height;
    uint8_t const *bitmap;
} gfx_glyph_t;

static uint8_t const *gfx_font = font_50;
static int16_t gfx_glyph_gap_width = 2;

static inline void gfx_draw_pixel(gfx_row_t row, int16_t x, uint8_t yuv444[3])
{
    if (x * 2 + 0 < row.len)
    {
        row.buf[x * 2 + 0] = yuv444[1 + x % 2];
    }
    if (x * 2 + 1 < row.len)
    {
        row.buf[x * 2 + 1] = yuv444[0];
    }
}

static inline void gfx_draw_segment(gfx_row_t row, int16_t x_beg, int16_t x_end, uint8_t yuv444[3])
{
    for (size_t len = row.len / 2, x = x_beg; x < x_end && x < len; x++)
    {
        gfx_draw_pixel(row, x, yuv444);
    }
}

static void gfx_render_rectangle(gfx_row_t row, gfx_obj_t *obj)
{
    gfx_draw_segment(row, obj->x, obj->x + obj->width, obj->yuv444);
}

static inline int16_t gfx_get_intersect_line(int16_t y,
                                             int16_t obj_x, int16_t obj_y, int16_t obj_width, int16_t obj_height, bool flip)
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
    int16_t seg_height = y - obj_y;
    int16_t seg_width = obj_width * seg_height / obj_height;
    return obj_x + (flip ? obj_width - seg_width : seg_width);
}

static void gfx_render_line(gfx_row_t row, gfx_obj_t *obj)
{
    int16_t x0, x1;
    bool flip = obj->arg.u32;

    // Special case: purely horizontal line means divide by 0, fill as a rectangle instead
    if (obj->height == 0)
    {
        gfx_render_rectangle(row, obj);
        return;
    }

    // We need to know how many horizontal pixels to drawn to accomodate the line thickness
    // so we get two intersections: the one for the top, and the one for the bottom edge of
    // the line. This introduces an offset, which we correct with the +1
    x0 = gfx_get_intersect_line(row.y + 1, obj->x, obj->y, obj->width, obj->height + 1, flip);
    x1 = gfx_get_intersect_line(row.y, obj->x, obj->y, obj->width, obj->height + 1, flip);

    // We have the start and stop point of the segment, we can fill it
    gfx_draw_segment(row, MIN(x0, x1), MAX(x0, x1), obj->yuv444);
}

static inline gfx_glyph_t gfx_get_glyph(uint8_t const *font, char c)
{
    gfx_glyph_t glyph;
    uint8_t const *f = font;

    // Only ASCII is supported for this early release
    // see how https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
    // encoded lookup tables for a strategy to support UTF-8.
    if (c < ' ' || c > '~')
    {
        return gfx_get_glyph(font, ' ');
    }

    // Store the global font header properties.
    glyph.height = *f++;

    // Scan the font, seeking for the glyph to render.
    for (char i = ' '; i <= c; i++)
    {
        glyph.width = *f++;
        glyph.bitmap = f;
        f += (glyph.width * glyph.height + 7) / 8;
    }
    return glyph;
}

static inline bool gfx_get_glyph_bit(gfx_glyph_t *glyph, int16_t x, int16_t y)
{
    size_t i = y * glyph->width + x;

    // See the txt2cfont tool to understand this encoding.
    return glyph->bitmap[i / 8] & 1 << (i % 8);
}

/**
 * Render a single glyph onto the buffer.
 *
 * @param row The buffer onto which write, with parameters adjusted
 * to be local: as if the glyph had to be rendered on a screen of the same
 * dimension onto x=0 y=0.
 *
 * @param glpyh The glyph to render.
 */
static inline void gfx_draw_glyph(gfx_row_t row, gfx_glyph_t *glyph, uint8_t yuv444[3])
{
    // for each vertical position
    for (int16_t x = 0; x < glyph->width && x < row.len / 2; x++)
    {
        // check if the bit is set
        if (gfx_get_glyph_bit(glyph, x, row.y) == true)
        {
            // and only if so, fill the buffer with it
            gfx_draw_pixel(row, x, yuv444);
        }
    }
}

/*
 * Accurately compute the given string's display width
 */
int16_t gfx_get_text_width(char const *s, size_t len)
{
    int16_t width = 0;

    // Scan the whole string and get the width of each glyph
    for (size_t i = 0; i < len; i++)
    {
        // Accumulate the width of this glyph to render
        width += (i == 0) ? 0 : gfx_glyph_gap_width;
        width += gfx_get_glyph(gfx_font, s[i]).width;
    }
    return width;
}

int16_t gfx_get_text_height(void)
{
    return gfx_font[0];
}

static void gfx_render_text(gfx_row_t row, gfx_obj_t *obj)
{
    char const *s = obj->arg.ptr;

    // Only a single row of text is supported.
    if (row.y > obj->y + gfx_font[0])
    {
        return;
    }

    for (int16_t x = obj->x; *s != '\0'; s++)
    {
        gfx_glyph_t glyph;

        // search the glyph within the font data
        glyph = gfx_get_glyph(gfx_font, *s);

        gfx_row_t local = {
            .buf = row.buf + x * 2,
            .len = row.len - x * 2,
            .y = row.y - obj->y};

        // render the glyph, reduce the buffer to only the section to draw into,
        // y coordinate is adjusted to be height within the glyph
        gfx_draw_glyph(local, &glyph, obj->yuv444);
        x += glyph.width + gfx_glyph_gap_width;
    }
}

static void gfx_render_ellipsis(gfx_row_t row, gfx_obj_t *obj)
{
    // TODO implement ellipsis
}

void gfx_fill_black(gfx_row_t row)
{
    uint8_t black[] = GFX_YUV422_BLACK;

    for (size_t i = 0; i + 1 < row.len; i += 2)
    {
        memcpy(row.buf + i, black, sizeof black);
    }
}

bool gfx_render_row(gfx_row_t row, gfx_obj_t *obj_list, size_t obj_num)
{
    bool drawn = false;

    for (size_t i = 0; i < obj_num; i++)
    {
        gfx_obj_t *obj = obj_list + i;

        // skip the object if it is not on the row we render.
        if (row.y < obj->y || row.y > obj->y + obj->height)
        {
            continue;
        }

        drawn = true;

        switch (obj->type)
        {
        case GFX_TYPE_NULL:
        {
            break;
        }
        case GFX_TYPE_RECTANGLE:
        {
            gfx_render_rectangle(row, obj);
            break;
        }
        case GFX_TYPE_LINE:
        {
            gfx_render_line(row, obj);
            break;
        }
        case GFX_TYPE_TEXT:
        {
            gfx_render_text(row, obj);
            break;
        }
        case GFX_TYPE_ELLIPSIS:
        {
            gfx_render_ellipsis(row, obj);
            break;
        }
        default:
        {
            assert(!"unknown type");
        }
        }
    }
    return drawn;
}
