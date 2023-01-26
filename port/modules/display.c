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

#include <stddef.h>

#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "nrfx_log.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#include "driver/bluetooth_data_protocol.h"
#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/spi.h"

#include "font.h"

#define FPGA_ADDR_ALIGN  128

#define LEN(x)      (sizeof(x) / sizeof*(x))
#define VAL(str)    #str
#define STR(str)    VAL(str)
#define ABS(x)      ((x) > 0 ? (x) : -(x))

#define RGB_TO_YUV444(r, g, b) { \
    128.0 + 0.29900 * (r) + 0.58700 * (g) + 0.11400 * (b) - 128.0, \
    128.0 - 0.16874 * (r) - 0.33126 * (g) + 0.50000 * (b), \
    128.0 + 0.50000 * (r) - 0.41869 * (g) - 0.08131 * (b) \
}

#define YUV422_BLACK    { 0x80, 0x00 }

#define LINE_THICKNESS  4

typedef struct
{
    uint8_t width, height;
    uint8_t const *bitmap;
} glyph_t;

enum
{
    OBJ_NULL,      // skip this object
    OBJ_RECTANGLE, // filled
    OBJ_LINE,      // diagonal line
    OBJ_ELLIPSIS,  // diagonal line
    OBJ_TEXT,      // a single line of text truncated at the end
};

typedef struct
{
    uint8_t *buf;
    size_t len;
    int16_t y;
} row_t;

typedef union
{
    void const *ptr;
    uint32_t u32;
} arg_t;

typedef struct
{
    int16_t x, y, width, height;
    uint8_t yuv444[3];
    uint8_t type;
    arg_t arg;
} obj_t;

static uint8_t const *font = font_50;
static int16_t glyph_gap_width = 2;

static inline void draw_pixel(row_t row, int16_t x, uint8_t yuv444[3])
{
    if (x * 2 < 0 || x * 2 + 1 > row.len)
    {
        LOG("row.y=%d x=%d", row.y, x);
    }
    if (x * 2 + 0 < row.len)
    {
        row.buf[x * 2 + 0] = yuv444[1 + x % 2];
    }
    if (x * 2 + 1 < row.len)
    {
        row.buf[x * 2 + 1] = yuv444[0];
    }
}

static inline void draw_segment(row_t row, int16_t x_beg, int16_t x_end, uint8_t yuv444[3])
{
    for (size_t len = row.len / 2, x = x_beg; x < x_end && x < len; x++)
    {
        draw_pixel(row, x, yuv444);
    }
}

static void render_rectangle(row_t row, obj_t *obj)
{
    draw_segment(row, obj->x, obj->x + obj->width, obj->yuv444);
}

static inline int16_t get_intersect(int16_t y,
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

static void render_line(row_t row, obj_t *obj)
{
    int16_t x0, x1;
    bool flip = obj->arg.u32;

    // Avoid divide by zero
    assert(obj->height > 0);

    // In order for the lines to have at least a bit of thickness, we compute
    // the position of two lines, away from each other by 1 pixel in x or y
    // direction depending if the line is mostly horizontal or vertical.
    int16_t line_x0 = obj->x - (obj->width < obj->height) * LINE_THICKNESS;
    int16_t line_y0 = obj->y - (obj->width > obj->height) * LINE_THICKNESS;
    int16_t line_x1 = obj->x + (obj->width < obj->height) * LINE_THICKNESS;
    int16_t line_y1 = obj->y + (obj->width > obj->height) * LINE_THICKNESS;

    // Then, we get the intersection of these lines with our horizontal axis.
    x0 = get_intersect(row.y, line_x0, line_y0, obj->width, obj->height, flip);
    x1 = get_intersect(row.y, line_x1, line_y1, obj->width, obj->height, flip);

    // We then fill the pixels between these two points.
    draw_segment(row, MIN(x0, x1), MAX(x0, x1), obj->yuv444);
}

static inline glyph_t get_glyph(uint8_t const *font, char c)
{
    glyph_t glyph;
    uint8_t const *f = font;

    // Only ASCII is supported for this early release
    // see how https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
    // encoded lookup tables for a strategy to support UTF-8.
    if (c < ' ' || c > '~')
    {
        return get_glyph(font, ' ');
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

static inline bool get_glyph_bit(glyph_t *glyph, int16_t x, int16_t y)
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
static inline void draw_glyph(row_t row, glyph_t *glyph, uint8_t yuv444[3])
{
    // for each vertical position
    for (int16_t x = 0; x < glyph->width && x < row.len / 2; x++)
    {
        // check if the bit is set
        if (get_glyph_bit(glyph, x, row.y) == true)
        {
            // and only if so, fill the buffer with it
            draw_pixel(row, x, yuv444);
        }
    }
}

/*
 * Accurately compute the given string's display width
 */
int16_t get_text_width(char const *s, size_t len)
{
    int16_t width = 0;

    // Scan the whole string and get the width of each glyph
    for (size_t i = 0; i < len; i++)
    {
        // Accumulate the width of this glyph to render
        width += (i == 0) ? 0 : glyph_gap_width;
        width += get_glyph(font, s[i]).width;
    }
    return width;
}

int16_t get_text_height(void)
{
    return font[0];
}

static void render_text(row_t row, obj_t *obj)
{
    char const *s = obj->arg.ptr;

    // Only a single row of text is supported.
    if (row.y > obj->y + font[0])
    {
        return;
    }

    for (int16_t x = obj->x; *s != '\0'; s++)
    {
        glyph_t glyph;

        // search the glyph within the font data
        glyph = get_glyph(font, *s);

        row_t local = {
            .buf = row.buf + x * 2,
            .len = row.len - x * 2,
            .y = row.y - obj->y
        };

        // render the glyph, reduce the buffer to only the section to draw into,
        // y coordinate is adjusted to be height within the glyph
        draw_glyph(local, &glyph, obj->yuv444);
        x += glyph.width + glyph_gap_width;
    }
}

static void render_ellipsis(row_t row, obj_t *obj)
{
    // TODO implement ellipsis: maybe that's college level trigonometry,
    // but it's kind of tough for me ^_^'
}

void fill_black(row_t row)
{
    uint8_t black[] = YUV422_BLACK;

    for (size_t i = 0; i + 1 < row.len; i += 2)
    {
        memcpy(row.buf + i, black, sizeof black);
    }
}

bool render_row(row_t row, obj_t *obj_list, size_t obj_num)
{
    bool drawn = false;

    for (size_t i = 0; i < obj_num; i++)
    {
        obj_t *obj = obj_list + i;

        // skip the object if it is not on the row we render.
        if (row.y < obj->y || row.y > obj->y + obj->height)
        {
            continue;
        }

        drawn = true;

        switch (obj->type)
        {
        case OBJ_NULL:
        {
            break;
        }
        case OBJ_RECTANGLE:
        {
            render_rectangle(row, obj);
            break;
        }
        case OBJ_LINE:
        {
            render_line(row, obj);
            break;
        }
        case OBJ_TEXT:
        {
            render_text(row, obj);
            break;
        }
        case OBJ_ELLIPSIS:
        {
            render_ellipsis(row, obj);
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

/* Python bindings */

STATIC mp_obj_t display___init__(void)
{
    // TODO: remove this once full startup procedure works.
    // Set the FPGA to show the graphic buffer.
    fpga_cmd(FPGA_GRAPHICS_CLEAR);
    nrfx_systick_delay_ms(30);
    fpga_cmd(FPGA_GRAPHICS_SWAP);
    nrfx_systick_delay_ms(30);
    fpga_cmd(FPGA_GRAPHICS_ON);
    nrfx_systick_delay_ms(30);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(display___init___obj, display___init__);

obj_t obj_list[50];
size_t obj_num;

STATIC void flush_blocks(row_t yuv422, size_t pos, size_t len)
{
    assert(pos + len <= yuv422.len);

    // Easier and more generic to place this optimization here than
    // checking every time from the caller.
    if (len == 0)
    {
        return;
    }

    // set the base address
    uint32_t u32 = yuv422.y * yuv422.len + pos;
    uint8_t base[sizeof u32] = { u32 >> 24, u32 >> 16, u32 >> 8, u32 >> 0 };
    assert(u32 < OV5640_WIDTH * OV5640_HEIGHT * 2);
    PRINTF(" %X", u32);
    fpga_cmd_write(FPGA_GRAPHICS_BASE, base, sizeof base);

    // Flush the content of the screen skipping empty bytes.
    fpga_cmd_write(FPGA_GRAPHICS_DATA, yuv422.buf + pos, len);
}

STATIC bool block_has_content(row_t yuv422, size_t pos)
{
    uint8_t black[] = YUV422_BLACK;

    assert(yuv422.len % sizeof black == 0);
    assert(pos % sizeof black == 0);
    assert(pos < yuv422.len);

    for (size_t i = pos; i < yuv422.len && i < pos + FPGA_ADDR_ALIGN; i += sizeof black)
    {
        if (memcmp(yuv422.buf + i, black, sizeof black) != 0)
        {
            return true;
        }
    }
    return false;
}

STATIC void flush_row(row_t yuv422)
{
    // Print all contiguous blocks that can be flushed altogether
    for (size_t i = 0;;)
    {
        size_t beg, end;

        // find the start position
        for (;; i+= FPGA_ADDR_ALIGN)
        {
            if (i == yuv422.len)
            {
                return;
            }
            if (block_has_content(yuv422, i))
            {
                break;
            }
        }
        beg = i;

        // find the end position
        for (; i < yuv422.len; i+= FPGA_ADDR_ALIGN)
        {
            if (!block_has_content(yuv422, i))
            {
                break;
            }
        }
        end = i;

        flush_blocks(yuv422, beg, end - beg);
    }
}

STATIC mp_obj_t display_show(void)
{
    uint8_t buf[ECX336CN_WIDTH * 2];
    uint8_t buf2[1 << 15]; memset(buf2, 0, sizeof buf2);
    row_t yuv422 = { .buf = buf, .len = sizeof buf, .y = 0 };

    // fill the display with YUV422 black pixels
    fpga_cmd(FPGA_GRAPHICS_CLEAR);
    nrfx_systick_delay_ms(30);

    // Walk through every line of the display, render it, send it to the FPGA.
    for (; yuv422.y < OV5640_HEIGHT; yuv422.y++)
    {
        // Clean the row before writing to it
        fill_black(yuv422);

        // Render a single row, and if anything was updated, also flush it
        if (render_row(yuv422, obj_list, obj_num))
        {
            flush_row(yuv422);
        }
    }

    // The framebuffer we wrote to is ready, now we can display it.
    fpga_cmd(FPGA_GRAPHICS_SWAP);

    // Empty the list of elements to draw.
    memset(obj_list, 0, sizeof obj_list);
    obj_num = 0;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(display_show_obj, &display_show);

STATIC mp_obj_t display_brightness(mp_obj_t brightness_in)
{
    mp_int_t brightness = mp_obj_get_int(brightness_in);
    ecx336cn_luminance_t tab[] = {
        ECX336CN_DIM,
        ECX336CN_LOW,
        ECX336CN_MEDIUM,
        ECX336CN_HIGH,
        ECX336CN_BRIGHT,
    };

    if (brightness >= MP_ARRAY_SIZE(tab))
    {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness must be between 0 and 4"));
    }

    ecx336cn_set_luminance(tab[brightness]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(display_brightness_obj, &display_brightness);

STATIC void new_obj(int type, mp_int_t x, mp_int_t y, mp_int_t width, mp_int_t height, mp_int_t rgb, arg_t arg)
{
    uint8_t yuv444[3] = RGB_TO_YUV444((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 0) & 0xFF);
    obj_t *gfx;

    assert(width >= 0);
    assert(height >= 0);

    if (rgb < 0 || rgb > 0xFFFFFF)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("color must be between 0x000000 and 0xFFFFFF"));
    }

    // Get the latest free slot
    if (obj_num >= LEN(obj_list))
    {
        mp_raise_OSError(MP_ENOMEM);
    }
    gfx = obj_list + obj_num;

    // This is the only place where we increment this number.
    obj_num++;

    // Fill the new slot.
    gfx->type = type;
    gfx->x = x;
    gfx->y = y;
    gfx->width = width;
    gfx->height = height;
    memcpy(gfx->yuv444, yuv444, sizeof yuv444);
    gfx->arg = arg;
}

STATIC mp_obj_t display_line(size_t argc, mp_obj_t const args[])
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t rgb = mp_obj_get_int(args[4]);
    arg_t arg = { .u32 = (x1 < x2) != (y1 < y2) };
    mp_int_t width = ABS(x1 - x2);
    mp_int_t height = ABS(y1 - y2);
    mp_int_t x = MIN(x1, x2);
    mp_int_t y = MIN(y1, y2);
    int type = OBJ_LINE;

    // Special case: avoid division by 0 later on.
    if (height == 0)
    {
        y -= LINE_THICKNESS / 2;
        height = LINE_THICKNESS;
        type = OBJ_RECTANGLE;
    }

    new_obj(type, x, y, width, height, rgb, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_line_obj, 5, 5, display_line);

STATIC mp_obj_t display_text(size_t argc, mp_obj_t const args[])
{
    arg_t arg = { .ptr = mp_obj_str_get_str(args[0]) };
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t rgb = mp_obj_get_int(args[3]);
    mp_int_t width = get_text_width(arg.ptr, strlen(arg.ptr));
    mp_int_t height = get_text_height();

    new_obj(OBJ_TEXT, x, y, width, height, rgb, arg);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_text_obj, 4, 4, display_text);

STATIC mp_obj_t display_fill(mp_obj_t rgb_in)
{
    mp_int_t rgb = mp_obj_get_int(rgb_in);
    arg_t none = {0};

    new_obj(OBJ_RECTANGLE, 0, 0, OV5640_WIDTH, OV5640_HEIGHT, rgb, none);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(display_fill_obj, display_fill);

STATIC mp_obj_t display_hline(size_t argc, mp_obj_t const args[])
{
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t width = mp_obj_get_int(args[2]);
    mp_int_t height = 1;
    mp_int_t rgb = mp_obj_get_int(args[3]);
    arg_t none = {0};

    new_obj(OBJ_RECTANGLE, x, y, width, height, rgb, none);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_hline_obj, 4, 4, display_hline);

STATIC mp_obj_t display_vline(size_t argc, mp_obj_t const args[])
{
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t height = mp_obj_get_int(args[2]);
    mp_int_t width = 1;
    mp_int_t rgb = mp_obj_get_int(args[3]);
    arg_t none = {0};

    new_obj(OBJ_RECTANGLE, x, y, width, height, rgb, none);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_vline_obj, 4, 4, display_vline);

STATIC const mp_rom_map_elem_t display_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_display) },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&display___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_fill),        MP_ROM_PTR(&display_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_line),        MP_ROM_PTR(&display_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_text),        MP_ROM_PTR(&display_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline),       MP_ROM_PTR(&display_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline),       MP_ROM_PTR(&display_vline_obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_show),        MP_ROM_PTR(&display_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_brightness),  MP_ROM_PTR(&display_brightness_obj) },
};
STATIC MP_DEFINE_CONST_DICT(display_module_globals, display_module_globals_table);

const mp_obj_module_t display_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&display_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_display, display_module);
