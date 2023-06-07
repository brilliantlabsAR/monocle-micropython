/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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

#include "monocle.h"
#include "py/runtime.h"

STATIC mp_obj_t delta_encode(mp_obj_t input_object)
{
    if (!mp_obj_is_type(input_object, &mp_type_list))
    {
        mp_raise_msg_varg(&mp_type_TypeError,
                          MP_ERROR_TEXT("data must be a list"));
    }

    size_t list_length;
    mp_obj_t *list_data;
    mp_obj_list_get(input_object, &list_length, &list_data);

    uint8_t bytes[list_length + 1];

    bytes[0] = (uint8_t)(mp_obj_get_int(list_data[0]) >> 8);
    bytes[1] = (uint8_t)mp_obj_get_int(list_data[0]);

    size_t i;
    for (i = 1; i < list_length; i++)
    {
        mp_int_t difference = mp_obj_get_int(list_data[i]) -
                              mp_obj_get_int(list_data[i - 1]);

        if (difference > 127 || difference < -127)
        {
            break;
        }

        bytes[i + 1] = difference;
    }

    return mp_obj_new_bytes(bytes, i + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(delta_encode_obj, delta_encode);

STATIC const mp_rom_map_elem_t compression_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_delta_encode), MP_ROM_PTR(&delta_encode_obj)},
};
STATIC MP_DEFINE_CONST_DICT(compression_module_globals, compression_module_globals_table);

const mp_obj_module_t compression_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&compression_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR__compression, compression_module);
