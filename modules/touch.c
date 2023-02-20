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
#include "py/qstr.h"

static mp_obj_t callback[_LEN_TOUCH_ACTION_T] =
    {[0 ... _LEN_TOUCH_ACTION_T - 1] = mp_const_none};

void touch_event_handler(touch_action_t action)
{
    if (callback[action] != mp_const_none)
    {
        mp_obj_dict_t *dict = mp_obj_new_dict(0);

        switch (action)
        {
        case A_TOUCH:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_A));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_TOUCH));
            break;

        case B_TOUCH:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_B));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_TOUCH));
            break;

        case A_DEEP_TOUCH:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_A));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_DEEP_TOUCH));
            break;

        case B_DEEP_TOUCH:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_B));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_DEEP_TOUCH));
            break;

        case A_PROXIMITY:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_A));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_PROXIMITY));
            break;

        case B_PROXIMITY:
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_button),
                              MP_ROM_QSTR(MP_QSTR_B));
            mp_obj_dict_store(dict,
                              MP_ROM_QSTR(MP_QSTR_action),
                              MP_ROM_QSTR(MP_QSTR_PROXIMITY));
            break;

        default:
            break;
        }

        mp_sched_schedule(callback[action], MP_OBJ_FROM_PTR(dict));
    }
}

static touch_action_t decode_touch_action(qstr button, qstr action)
{
    if (button == MP_QSTR_A && action == MP_QSTR_TOUCH)
    {
        return A_TOUCH;
    }

    if (button == MP_QSTR_B && action == MP_QSTR_TOUCH)
    {
        return B_TOUCH;
    }

    if (button == MP_QSTR_A && action == MP_QSTR_DEEP_TOUCH)
    {
        return A_DEEP_TOUCH;
    }

    if (button == MP_QSTR_B && action == MP_QSTR_DEEP_TOUCH)
    {
        return B_DEEP_TOUCH;
    }

    if (button == MP_QSTR_A && action == MP_QSTR_PROXIMITY)
    {
        return A_PROXIMITY;
    }

    if (button == MP_QSTR_B && action == MP_QSTR_PROXIMITY)
    {
        return B_PROXIMITY;
    }

    return _LEN_TOUCH_ACTION_T;
}

STATIC mp_obj_t touch_callback(size_t n_args, const mp_obj_t *args)
{
    qstr button = mp_obj_str_get_qstr(args[0]);
    qstr action = mp_obj_str_get_qstr(args[1]);

    if ((button != MP_QSTR_A) &&
        (button != MP_QSTR_B))
    {
        mp_raise_ValueError(MP_ERROR_TEXT("must be touch.A or touch.B"));
    }

    if ((action != MP_QSTR_TOUCH) &&
        (action != MP_QSTR_DEEP_TOUCH) &&
        (action != MP_QSTR_PROXIMITY))
    {
        mp_raise_ValueError(MP_ERROR_TEXT(
            "must be touch.TOUCH, touch.DEEP_TOUCH, or touch.PROXIMITY"));
    }

    touch_action_t touch_action = decode_touch_action(button, action);

    if (n_args == 2)
    {
        return callback[touch_action];
    }

    if (!mp_obj_is_callable(args[2]) && (args[2] != mp_const_none))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("callback must be None or a callable object"));
    }

    callback[touch_action] = args[2];

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(touch_callback_obj, 2, 3, touch_callback);

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&touch_callback_obj)},

    {MP_ROM_QSTR(MP_QSTR_A), MP_ROM_QSTR(MP_QSTR_A)},
    {MP_ROM_QSTR(MP_QSTR_B), MP_ROM_QSTR(MP_QSTR_B)},
    {MP_ROM_QSTR(MP_QSTR_TOUCH), MP_ROM_QSTR(MP_QSTR_TOUCH)},
    {MP_ROM_QSTR(MP_QSTR_DEEP_TOUCH), MP_ROM_QSTR(MP_QSTR_DEEP_TOUCH)},
    {MP_ROM_QSTR(MP_QSTR_PROXIMITY), MP_ROM_QSTR(MP_QSTR_PROXIMITY)},
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
