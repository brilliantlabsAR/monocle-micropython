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

#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#include "nrfx_log.h"
#include "nrfx_twi.h"

#include "driver/i2c.h"
#include "driver/timer.h"
#include "driver/touch.h"

#define LEN(x) (sizeof(x) / sizeof*(x))

enum {
    TOUCH_ACTION_LONG,
    TOUCH_ACTION_PRESS,
    TOUCH_ACTION_SLIDE,
    TOUCH_ACTION_TAP,

    TOUCH_ACTION_NUM,
};

#define TOUCH_BUTTON_NUM 2

mp_obj_t callback_list[TOUCH_BUTTON_NUM][TOUCH_ACTION_NUM];

STATIC mp_obj_t mod_touch___init__(void)
{
    for (size_t i = 0; i < LEN(callback_list); i++)
        callback_list[0][i] = callback_list[1][i] = mp_const_none;

    // dependencies:
    touch_init();
    timer_init();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_touch___init___obj, mod_touch___init__);

static inline mp_obj_t touch_get_callback(touch_state_t trigger)
{
    switch (trigger) {
    case TOUCH_TRIGGER_0_TAP:       return callback_list[0][TOUCH_ACTION_TAP];
    case TOUCH_TRIGGER_1_TAP:       return callback_list[1][TOUCH_ACTION_TAP];
    case TOUCH_TRIGGER_0_PRESS:     return callback_list[0][TOUCH_ACTION_PRESS];
    case TOUCH_TRIGGER_1_PRESS:     return callback_list[1][TOUCH_ACTION_PRESS];
    case TOUCH_TRIGGER_0_LONG:      return callback_list[0][TOUCH_ACTION_LONG];
    case TOUCH_TRIGGER_1_LONG:      return callback_list[1][TOUCH_ACTION_LONG];
    case TOUCH_TRIGGER_0_1_SLIDE:   return callback_list[0][TOUCH_ACTION_SLIDE];
    case TOUCH_TRIGGER_1_0_SLIDE:   return callback_list[1][TOUCH_ACTION_SLIDE];
    default:                        return mp_const_none;
    }
}

/**
 * Overriding the default callback implemented in driver/iqs620.c
 * @param trigger The trigger that ran the callback.
 */
void touch_callback(touch_state_t trigger)
{
    mp_obj_t callback = touch_get_callback(trigger);

    if (callback == mp_const_none)
    {
        LOG("trigger=0x%02X no callback set", trigger);
    }
    else
    {
        LOG("trigger=0x%02X scheduling trigger", trigger);
        mp_sched_schedule(callback, MP_OBJ_NEW_SMALL_INT(trigger));
    }
}

/**
 * Setup a python function as a callback for a given action.
 * @param button_in Button (0 or 1) to use.
 * @param action_in Action to react to.
 * @param callback Python function to be called.
 */
STATIC mp_obj_t touch_bind(mp_obj_t button_in, mp_obj_t action_in, mp_obj_t callback)
{
    uint8_t button = mp_obj_get_int(button_in);
    uint8_t action = mp_obj_get_int(action_in);

    if (button >= TOUCH_BUTTON_NUM)
        mp_raise_ValueError(MP_ERROR_TEXT("button must be touch.A or touch.B"));
    if (action >= TOUCH_ACTION_NUM)
        mp_raise_ValueError(MP_ERROR_TEXT("action must be any of touch.TOUCH_*"));

    callback_list[button][action] = callback;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(touch_bind_obj, touch_bind);

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_touch) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&mod_touch___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_bind),                MP_ROM_PTR(&touch_bind_obj) },

    // constants
    { MP_ROM_QSTR(MP_QSTR_A),                   MP_OBJ_NEW_SMALL_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_B),                   MP_OBJ_NEW_SMALL_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_LONG),                MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_LONG) },
    { MP_ROM_QSTR(MP_QSTR_PRESS),               MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_SLIDE),               MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_SLIDE) },
    { MP_ROM_QSTR(MP_QSTR_TAP),                 MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_TAP) },
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
