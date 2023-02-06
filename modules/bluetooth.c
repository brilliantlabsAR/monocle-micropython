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

#include "py/runtime.h"
#include "py/objarray.h"
#include "bluetooth.h"

static mp_obj_t bluetooth_send(mp_obj_t buffer_in)
{
    if (!mp_obj_is_type(buffer_in, &mp_type_bytes) && !mp_obj_is_type(buffer_in, &mp_type_bytearray))
    {
        mp_raise_TypeError(MP_ERROR_TEXT("buffer type must be bytes or bytearray"));
    }
    mp_obj_array_t *array = MP_OBJ_TO_PTR(buffer_in);

    ble_raw_tx(array->items, array->len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bluetooth_send_obj, bluetooth_send);

static mp_obj_t bluetooth_receive(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluetooth_receive_obj, bluetooth_receive);

static mp_obj_t bluetooth_max_length(void)
{
    return mp_obj_new_int(ble_negotiated_mtu);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluetooth_max_length_obj, bluetooth_max_length);

static mp_obj_t bluetooth_connected(void)
{
    bool connected = (ble_conn_handle != BLE_CONN_HANDLE_INVALID);
    return connected ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluetooth_connected_obj, bluetooth_connected);

STATIC const mp_rom_map_elem_t bluetooth_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__),     MP_ROM_QSTR(MP_QSTR_bluetooth)},
    {MP_ROM_QSTR(MP_QSTR_send),         MP_ROM_PTR(&bluetooth_send_obj)},
    {MP_ROM_QSTR(MP_QSTR_receive),      MP_ROM_PTR(&bluetooth_receive_obj)},
    {MP_ROM_QSTR(MP_QSTR_max_length),   MP_ROM_PTR(&bluetooth_max_length_obj)},
    {MP_ROM_QSTR(MP_QSTR_connected),    MP_ROM_PTR(&bluetooth_connected_obj)},
};
STATIC MP_DEFINE_CONST_DICT(bluetooth_module_globals, bluetooth_module_globals_table);

const mp_obj_module_t bluetooth_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&bluetooth_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_bluetooth, bluetooth_module);
