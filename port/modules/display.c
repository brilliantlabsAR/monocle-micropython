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

#include "nrfx_log.h"

#include "driver/bluetooth_data_protocol.h"
#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/spi.h"

STATIC mp_obj_t mod_display___init__(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_display___init___obj, mod_display___init__);

STATIC mp_obj_t display_show(mp_obj_t row_num_in)
{
    mp_int_t row_num = mp_obj_get_int(row_num_in);
    uint8_t row_buf[ECX336CN_WIDTH * 3];

    memset(row_buf, 0x55, sizeof row_buf);
    fpga_graphics_on();
    fpga_graphics_set_write_addr(row_num * sizeof row_buf);
    fpga_graphics_write_data(row_buf, sizeof row_buf);
    fpga_graphics_swap_buffer();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(display_show_obj, &display_show);

STATIC mp_obj_t display_brightness(mp_obj_t brightness_in)
{
    uint32_t brightness = mp_obj_get_int(brightness_in);
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

STATIC const mp_rom_map_elem_t display_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_display) },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&mod_display___init___obj) },

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
