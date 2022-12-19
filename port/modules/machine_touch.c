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

#include "driver_touch.h"

#include "machine.h"

#define LOG NRFX_LOG_ERROR

struct {
    mp_obj_t callback;
} machine_touch;

/**
 * Overriding the default callback implemented in driver_iqs620.c
 * @param trigger The trigger that ran the callback.
 */
void touch_callback(touch_state_t trigger)
{
    if (machine_touch.callback) {
        LOG("trigger=0x%02X scheduling trigger", trigger);
        mp_sched_schedule(machine_touch.callback, MP_OBJ_NEW_SMALL_INT(trigger));
    } else {
        LOG("trigger=0x%02X no callback set", trigger);
    }
}

void machine_touch_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "Touch(%s)", machine_touch.callback ? "on" : "off");
}

STATIC mp_obj_t machine_touch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    // Allowed arguments table.
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ},
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Extract the function callback from the arguments.
    machine_touch.callback = args[0].u_obj;

    // Return the newly created Touch object.
    return MP_OBJ_FROM_PTR(&machine_touch);
}

STATIC const mp_rom_map_elem_t machine_touch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_0_TAP),      MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_0_TAP) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_1_TAP),      MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_1_TAP) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_BOTH_TAP),   MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_BOTH_TAP) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_0_PRESS),    MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_0_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_1_PRESS),    MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_1_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_BOTH_PRESS), MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_BOTH_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_0_LONG),     MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_0_LONG) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_1_LONG),     MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_1_LONG) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_BOTH_LONG),  MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_BOTH_LONG) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_0_1_SLIDE),  MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_0_1_SLIDE) },
    { MP_ROM_QSTR(MP_QSTR_TOUCH_TRIGGER_1_0_SLIDE),  MP_OBJ_NEW_SMALL_INT(TOUCH_TRIGGER_1_0_SLIDE) },
};
STATIC MP_DEFINE_CONST_DICT(machine_touch_locals_dict, machine_touch_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_touch_type,
    MP_QSTR_Touch,
    MP_TYPE_FLAG_NONE,
    make_new, machine_touch_make_new,
    print, machine_touch_print,
    locals_dict, &machine_touch_locals_dict
);
