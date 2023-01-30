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

#include "monocle.h"
#include "touch.h"
#include "py/runtime.h"

static struct
{
    bool enabled[_LEN_TOUCH_ACTION_T];
    mp_obj_t callback[_LEN_TOUCH_ACTION_T];
} touch_callback;

void touch_event_handler(touch_action_t action)
{
    if (touch_callback.enabled[action])
    {
        mp_sched_schedule(touch_callback.callback[action], mp_const_none);
    }
}

static touch_action_t decode_touch_action(uint8_t button, uint8_t action)
{
    switch (action)
    {
    case 0:
        if (button == 0)
        {
            return A_TOUCH;
        }
        return B_TOUCH;

    case 1:
        if (button == 0)
        {
            return A_DEEP_TOUCH;
        }
        return B_DEEP_TOUCH;

    default:
        if (button == 0)
        {
            return A_PROXIMITY;
        }
        return B_PROXIMITY;
    }
}

STATIC mp_obj_t touch_bind(mp_obj_t button, mp_obj_t action, mp_obj_t callback)
{
    if (mp_obj_get_int(button) >= 2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("must be touch.A or touch.B"));
    }

    if (mp_obj_get_int(action) >= 3)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be touch.TOUCH, touch.DEEP_TOUCH, or touch.PROXIMITY"));
    }

    touch_action_t touch_action = decode_touch_action(mp_obj_get_int(button),
                                                      mp_obj_get_int(action));

    touch_callback.enabled[(uint8_t)touch_action] = true;
    touch_callback.callback[(uint8_t)touch_action] = callback;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(touch_bind_obj, touch_bind);

STATIC mp_obj_t touch_unbind(mp_obj_t button, mp_obj_t action)
{
    if (mp_obj_get_int(button) >= 2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("must be touch.A or touch.B"));
    }

    if (mp_obj_get_int(action) >= 3)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be touch.TOUCH, touch.DEEP_TOUCH, or touch.PROXIMITY"));
    }

    touch_action_t touch_action = decode_touch_action(mp_obj_get_int(button),
                                                      mp_obj_get_int(action));

    touch_callback.enabled[(uint8_t)touch_action] = false;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(touch_unbind_obj, touch_unbind);

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_touch)},

    {MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&touch_bind_obj)},
    {MP_ROM_QSTR(MP_QSTR_unbind), MP_ROM_PTR(&touch_unbind_obj)},

    {MP_ROM_QSTR(MP_QSTR_A), MP_OBJ_NEW_SMALL_INT(0)},
    {MP_ROM_QSTR(MP_QSTR_B), MP_OBJ_NEW_SMALL_INT(1)},
    {MP_ROM_QSTR(MP_QSTR_TOUCH), MP_OBJ_NEW_SMALL_INT(0)},
    {MP_ROM_QSTR(MP_QSTR_DEEP_TOUCH), MP_OBJ_NEW_SMALL_INT(1)},
    {MP_ROM_QSTR(MP_QSTR_PROXIMITY), MP_OBJ_NEW_SMALL_INT(2)},
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
