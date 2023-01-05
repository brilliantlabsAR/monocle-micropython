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

mp_obj_t callback;

enum {
    TOUCH_LONG,
    TOUCH_PRESS,
    TOUCH_SLIDE,
    TOUCH_TAP,
};

STATIC mp_obj_t mod_touch___init__(void)
{
    // dependencies:
    touch_init();
    timer_init();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_touch___init___obj, mod_touch___init__);

/**
 * Overriding the default callback implemented in driver/iqs620.c
 * @param trigger The trigger that ran the callback.
 */
void touch_callback(touch_state_t trigger)
{
    if (callback)
    {
        LOG("trigger=0x%02X scheduling trigger", trigger);
        mp_sched_schedule(callback, MP_OBJ_NEW_SMALL_INT(trigger));
    }
    else
    {
        LOG("trigger=0x%02X no callback set", trigger);
    }
}

STATIC mp_obj_t touch_bind(mp_obj_t button, mp_obj_t action, mp_obj_t callback)
{
    (void)button;
    (void)action;
    (void)callback;
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
    { MP_ROM_QSTR(MP_QSTR_LONG),                MP_OBJ_NEW_SMALL_INT(TOUCH_LONG) },
    { MP_ROM_QSTR(MP_QSTR_PRESS),               MP_OBJ_NEW_SMALL_INT(TOUCH_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_SLIDE),               MP_OBJ_NEW_SMALL_INT(TOUCH_SLIDE) },
    { MP_ROM_QSTR(MP_QSTR_TAP),                 MP_OBJ_NEW_SMALL_INT(TOUCH_TAP) },
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
