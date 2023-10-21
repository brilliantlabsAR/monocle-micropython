/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Ltd.
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
#include "py/runtime.h"
#include "py/mperrno.h"

uint8_t font_data[] = {
#include "modules/font/ShareTechMonoBitmap-Regular-64.h"
};

STATIC mp_obj_t get_font_data(void)
{
    return mp_obj_new_bytes(font_data, sizeof(font_data));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_font_data_obj, &get_font_data);

STATIC mp_obj_t display_brightness(mp_obj_t brightness)
{
    int tab[] = {
        1, // DIM      750  cd/m2
        2, // LOW      1250 cd/m2
        0, // MEDIUM   2000 cd/m2, this is the default
        3, // HIGH     3000 cd/m2
        4, // BRIGHT   4000 cd/m2
    };

    if (mp_obj_get_int(brightness) >= MP_ARRAY_SIZE(tab))
    {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness must be between 0 and 4"));
    }

    uint8_t level = tab[mp_obj_get_int(brightness)];
    uint8_t command[2] = {0x05, 0xC8 | level};
    monocle_spi_write(DISPLAY, command, 2, false);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(display_brightness_obj, &display_brightness);

STATIC const mp_rom_map_elem_t display_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&display_brightness_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_font_data), MP_ROM_PTR(&get_font_data_obj)},
};
STATIC MP_DEFINE_CONST_DICT(display_module_globals, display_module_globals_table);

const mp_obj_module_t display_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&display_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR__display, display_module);
