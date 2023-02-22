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

static mp_obj_t touch_a_callback = mp_const_none;
static mp_obj_t touch_b_callback = mp_const_none;

void touch_event_handler(touch_action_t action)
{
    if (action == TOUCH_A && touch_a_callback != mp_const_none)
    {
        mp_sched_schedule(touch_a_callback, MP_ROM_QSTR(MP_QSTR_A));
    }

    if (action == TOUCH_B && touch_b_callback != mp_const_none)
    {
        mp_sched_schedule(touch_b_callback, MP_ROM_QSTR(MP_QSTR_B));
    }
}

STATIC mp_obj_t touch_state(size_t n_args, const mp_obj_t *args)
{
    touch_action_t action = touch_get_state();

    if (n_args == 0)
    {
        switch (action)
        {
        case TOUCH_A:
            return MP_OBJ_NEW_QSTR(MP_QSTR_A);
        case TOUCH_B:
            return MP_OBJ_NEW_QSTR(MP_QSTR_B);
        case TOUCH_BOTH:
            return MP_OBJ_NEW_QSTR(MP_QSTR_BOTH);
        default:
            return mp_const_none;
        }
    }

    qstr button = mp_obj_str_get_qstr(args[0]);

    if ((button != MP_QSTR_A) &&
        (button != MP_QSTR_B) &&
        (button != MP_QSTR_BOTH))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be touch.A, touch.B or touch.BOTH"));
    }

    if (button == MP_QSTR_A && action == TOUCH_A)
    {
        return mp_const_true;
    }

    if (button == MP_QSTR_B && action == TOUCH_B)
    {
        return mp_const_true;
    }

    if (button == MP_QSTR_BOTH && action == TOUCH_BOTH)
    {
        return mp_const_true;
    }

    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(touch_state_obj, 0, 1, touch_state);

STATIC mp_obj_t touch_callback(size_t n_args, const mp_obj_t *args)
{
    qstr button = mp_obj_str_get_qstr(args[0]);

    if ((button != MP_QSTR_A) &&
        (button != MP_QSTR_B) &&
        (button != MP_QSTR_BOTH))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be touch.A, touch.B or touch.BOTH"));
    }

    if (n_args == 1)
    {
        if (button == MP_QSTR_A)
        {
            return touch_a_callback;
        }
        if (button == MP_QSTR_B)
        {
            return touch_b_callback;
        }
        if (button == MP_QSTR_BOTH)
        {
            mp_obj_t tuple[] = {touch_a_callback, touch_b_callback};
            return mp_obj_new_tuple(2, tuple);
        }
    }

    mp_obj_t callback = args[1];

    if (!mp_obj_is_callable(callback) && (callback != mp_const_none))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("callback must be None or a callable object"));
    }

    if (button == MP_QSTR_A)
    {
        touch_a_callback = callback;
    }

    if (button == MP_QSTR_B)
    {
        touch_b_callback = callback;
    }

    if (button == MP_QSTR_BOTH)
    {
        touch_a_callback = callback;
        touch_b_callback = callback;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(touch_callback_obj, 1, 2, touch_callback);

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&touch_state_obj)},
    {MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&touch_callback_obj)},

    {MP_ROM_QSTR(MP_QSTR_A), MP_ROM_QSTR(MP_QSTR_A)},
    {MP_ROM_QSTR(MP_QSTR_B), MP_ROM_QSTR(MP_QSTR_B)},
    {MP_ROM_QSTR(MP_QSTR_BOTH), MP_ROM_QSTR(MP_QSTR_BOTH)},
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
