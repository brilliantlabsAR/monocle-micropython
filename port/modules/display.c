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

#include "libgfx.h"

#define LEN(x)      (sizeof(x) / sizeof*(x))
#define VAL(str)    #str
#define STR(str)    VAL(str)

#define FPGA_ADDR_ALIGN  128

STATIC mp_obj_t display___init__(void)
{
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

gfx_obj_t gfx_obj_list[10];
size_t gfx_obj_num;

STATIC size_t get_first_non_black(gfx_row_t yuv422)
{
    uint8_t black[] = GFX_YUV422_BLACK;

    assert(yuv422.len % sizeof black == 0);
    for (size_t i = 0; i < yuv422.len; i += sizeof black) {
        if (memcmp(yuv422.buf+i, black, sizeof black) != 0)
        {
            return i;
        }
    }
    return yuv422.len;
}

STATIC size_t get_last_non_black(gfx_row_t yuv422)
{
    uint8_t black[] = GFX_YUV422_BLACK;

    assert(yuv422.len % sizeof black == 0);
    for (size_t i = yuv422.len - sizeof black; i > 0; i -= sizeof black) {
        if (memcmp(yuv422.buf+i, black, sizeof black) != 0)
        {
            return i;
        }
    }
    return 0;
}

STATIC mp_obj_t display_show(void)
{
    uint8_t buf[ECX336CN_WIDTH * 2];
    gfx_row_t yuv422 = { .buf = buf, .len = sizeof buf, .y = 0 };

    // fill the display with YUV422 black pixels
    fpga_cmd(FPGA_GRAPHICS_CLEAR);
    nrfx_systick_delay_ms(30);

    // Walk through every line of the display, render it, send it to the FPGA.
    for (; yuv422.y < OV5640_HEIGHT; yuv422.y++)
    {
        // Clean the row before writing to it
        gfx_fill_black(yuv422);

        // Render a single row, and if anything was updated, also flush it
        if (gfx_render_row(yuv422, gfx_obj_list, gfx_obj_num))
        {
            // skip empty pixels
            size_t beg = get_first_non_black(yuv422);
            size_t end = get_last_non_black(yuv422) + FPGA_ADDR_ALIGN;

            // align the address as expected by the FPGA
            beg -= beg % FPGA_ADDR_ALIGN;
            end -= end % FPGA_ADDR_ALIGN;

            // skip empty lines
            if (beg == yuv422.len || end == 0)
            {
                continue;
            }

            // set the base address
            uint32_t u32 = yuv422.y * yuv422.len + beg;
            uint8_t base[sizeof u32] = { u32 >> 24, u32 >> 16, u32 >> 8, u32 >> 0 };
            fpga_cmd_write(FPGA_GRAPHICS_BASE, base, sizeof base);

            // Flush the content of the screen skipping empty bytes.
            fpga_cmd_write(FPGA_GRAPHICS_DATA, yuv422.buf + beg, end - beg);
        }
    }

    // The framebuffer we wrote to is ready, now we can display it.
    fpga_cmd(FPGA_GRAPHICS_SWAP);

    // Empty the list of elements to draw.
    memset(gfx_obj_list, 0, sizeof gfx_obj_list);
    gfx_obj_num = 0;

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
        mp_raise_ValueError(MP_ERROR_TEXT("brightness must be between 0 and 4"));

    ecx336cn_set_luminance(tab[brightness]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(display_brightness_obj, &display_brightness);

STATIC void new_gfx(gfx_type_t type, mp_int_t x, mp_int_t y, mp_int_t width, mp_int_t height, mp_int_t rgb, void const *ptr)
{
    uint8_t yuv444[3] = GFX_RGB_TO_YUV444((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 0) & 0xFF);
    gfx_obj_t *gfx;

    // validate parameters for convenience
    if (x < 0 || x >= OV5640_WIDTH)
        mp_raise_ValueError(MP_ERROR_TEXT("x must be between 0 and " STR(OV5640_WIDTH)));
    if (y < 0 || y >= OV5640_HEIGHT)
        mp_raise_ValueError(MP_ERROR_TEXT("y must be between 0 and " STR(OV5640_HEIGHT)));
    if (width <= 0)
        mp_raise_ValueError(MP_ERROR_TEXT("width must be greater than 0"));
    if (height <= 0)
        mp_raise_ValueError(MP_ERROR_TEXT("height must be greater than 0"));
    if (rgb < 0 || rgb > 0xFFFFFF)
        mp_raise_ValueError(MP_ERROR_TEXT("color must be between 0x000000 and 0xFFFFFF"));

    // Get the latest free slot
    if (gfx_obj_num >= LEN(gfx_obj_list))
        mp_raise_OSError(MP_ENOMEM);
    gfx = gfx_obj_list + gfx_obj_num;

    // This is the only place where we increment this number.
    gfx_obj_num++;

    // Fill the new slot.
    gfx->type = type;
    gfx->x = x;
    gfx->y = y;
    gfx->width = width;
    gfx->height = height;
    memcpy(gfx->yuv444, yuv444, sizeof yuv444);
    gfx->u.ptr = (void *)ptr;
}

STATIC mp_obj_t display_line(size_t argc, mp_obj_t const args[])
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t rgb = mp_obj_get_int(args[4]);
    mp_int_t tmp;
    bool flipped = ((x1 > x2) == (y1 > y2));

    if (x1 > x2)
        tmp = x1, x1 = x2, x2 = tmp;
    if (y1 > y2)
        tmp = y1, y1 = y2, y2 = tmp;

    new_gfx(GFX_TYPE_LINE, x1, y1, x2 - x1, y2 - y1, rgb, &flipped);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_line_obj, 5, 5, display_line);

STATIC mp_obj_t display_text(size_t argc, mp_obj_t const args[])
{
    const char *text = mp_obj_str_get_str(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t width = mp_obj_get_int(args[3]);
    mp_int_t rgb = mp_obj_get_int(args[4]);

    new_gfx(GFX_TYPE_TEXT, x, y, width, ECX336CN_HEIGHT, rgb, text);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_text_obj, 5, 5, display_text);

STATIC mp_obj_t display_txt(size_t argc, mp_obj_t const args[])
{
    new_gfx(GFX_TYPE_TEXT, 10, 10, 100, ECX336CN_HEIGHT, 0xFF0000, "oops");
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_txt_obj, 0, 5, display_txt);

STATIC const mp_rom_map_elem_t display_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_display) },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&display___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_text),        MP_ROM_PTR(&display_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_txt),         MP_ROM_PTR(&display_txt_obj) },
    { MP_ROM_QSTR(MP_QSTR_line),        MP_ROM_PTR(&display_line_obj) },

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
