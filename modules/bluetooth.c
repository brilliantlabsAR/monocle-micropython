/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#include "mphalport.h"
#include "py/runtime.h"
#include "py/objarray.h"

static mp_obj_t receive_callback = mp_const_none;

void bluetooth_receive_callback_handler(const uint8_t *bytes, size_t len)
{
    if (receive_callback != mp_const_none)
    {
        mp_obj_t array = mp_obj_new_bytes(bytes, len);
        mp_sched_schedule(receive_callback, array);
    }
}

static mp_obj_t bluetooth_send(mp_obj_t buffer_in)
{
    if (!ble_are_tx_notifications_enabled(DATA_TX))
    {
        mp_raise_msg(&mp_type_OSError,
                     MP_ERROR_TEXT(
                         "notifications are not enabled on the data service"));
    }

    mp_buffer_info_t array;
    mp_get_buffer_raise(buffer_in, &array, MP_BUFFER_READ);

    if (array.len > ble_get_max_payload_size())
    {
        mp_raise_msg(&mp_type_ValueError,
                     MP_ERROR_TEXT("input buffer is too large"));
    }

    if (ble_send_raw_data(array.buf, array.len))
    {
        mp_raise_msg(&mp_type_OSError,
                     MP_ERROR_TEXT("raw data service is busy"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bluetooth_send_obj, bluetooth_send);

static mp_obj_t bluetooth_receive_callback(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
    {
        return receive_callback;
    }

    receive_callback = args[0];

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bluetooth_receive_callback_obj, 0, 1, bluetooth_receive_callback);

static mp_obj_t bluetooth_connected(void)
{
    return ble_are_tx_notifications_enabled(DATA_TX)
               ? mp_const_true
               : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluetooth_connected_obj, bluetooth_connected);

STATIC mp_obj_t bluetooth_max_length(void)
{
    return mp_obj_new_int(ble_get_max_payload_size());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluetooth_max_length_obj, bluetooth_max_length);

STATIC const mp_rom_map_elem_t bluetooth_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&bluetooth_send_obj)},
    {MP_ROM_QSTR(MP_QSTR_receive_callback), MP_ROM_PTR(&bluetooth_receive_callback_obj)},
    {MP_ROM_QSTR(MP_QSTR_connected), MP_ROM_PTR(&bluetooth_connected_obj)},
    {MP_ROM_QSTR(MP_QSTR_max_length), MP_ROM_PTR(&bluetooth_max_length_obj)},
};
STATIC MP_DEFINE_CONST_DICT(bluetooth_module_globals, bluetooth_module_globals_table);

const mp_obj_module_t bluetooth_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&bluetooth_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_bluetooth, bluetooth_module);
