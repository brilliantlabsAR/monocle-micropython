/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon (name@email.com)
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

#include <stdint.h>
#include <stddef.h>

#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#include "nrfx_log.h"

#include "driver_iqs620.h"
#include "driver_battery.h"

#define LOG NRFX_LOG_ERROR

STATIC mp_obj_t machine_battery_level(void) {
    return MP_OBJ_NEW_SMALL_INT(battery_get_percent());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_battery_level_obj, machine_battery_level);

STATIC const mp_rom_map_elem_t machine_battery_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_level),     MP_ROM_PTR(&machine_battery_level_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_battery_locals_dict, machine_battery_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_battery_type,
    MP_QSTR_Battery,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_battery_locals_dict
);
