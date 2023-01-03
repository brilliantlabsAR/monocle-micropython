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

#include <stdint.h>

#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "shared/runtime/pyexec.h"

#include "nrfx_log.h"
#include "nrf_soc.h"

#include "driver/battery.h"
#include "driver/max77654.h"

#define PYB_RESET_HARD      (0)
#define PYB_RESET_WDT       (1)
#define PYB_RESET_SOFT      (2)
#define PYB_RESET_LOCKUP    (3)
#define PYB_RESET_POWER_ON  (16)
#define PYB_RESET_LPCOMP    (17)
#define PYB_RESET_DIF       (18)
#define PYB_RESET_NFC       (19)

STATIC mp_obj_t power_hibernate(void)
{
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_hibernate_obj, &power_hibernate);

STATIC mp_obj_t power_shutdown(mp_obj_t timeout)
{
    max77654_rail_vled(false);
    max77654_rail_10v(false);
    max77654_rail_2v7(false);
    max77654_rail_1v8(false);
    max77654_rail_1v2(false);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(power_shutdown_obj, &power_shutdown);

STATIC mp_obj_t power_soft_reset(void)
{
    pyexec_system_exit = PYEXEC_FORCED_EXIT;
    mp_raise_type(&mp_type_SystemExit);
}
MP_DEFINE_CONST_FUN_OBJ_0(power_soft_reset_obj, power_soft_reset);

STATIC mp_obj_t power_lightsleep(void)
{
    __WFE();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_lightsleep_obj, power_lightsleep);

STATIC mp_obj_t power_deepsleep(void)
{
    __WFI();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_deepsleep_obj, power_deepsleep);

STATIC const mp_rom_map_elem_t power_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_power) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&mod_power___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_hibernate),           MP_ROM_PTR(&power_hibernate_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutdown),            MP_ROM_PTR(&power_shutdown_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&power_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset),          MP_ROM_PTR(&power_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_idle),                MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_lightsleep),          MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep),           MP_ROM_PTR(&power_deepsleep_obj) },

    // constants
    { MP_ROM_QSTR(MP_QSTR_HARD_RESET),          MP_ROM_INT(PYB_RESET_HARD) },
    { MP_ROM_QSTR(MP_QSTR_WDT_RESET),           MP_ROM_INT(PYB_RESET_WDT) },
    { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),          MP_ROM_INT(PYB_RESET_SOFT) },
    { MP_ROM_QSTR(MP_QSTR_LOCKUP_RESET),        MP_ROM_INT(PYB_RESET_LOCKUP) },
    { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(PYB_RESET_POWER_ON) },
    { MP_ROM_QSTR(MP_QSTR_LPCOMP_RESET),        MP_ROM_INT(PYB_RESET_LPCOMP) },
    { MP_ROM_QSTR(MP_QSTR_DEBUG_IF_RESET),      MP_ROM_INT(PYB_RESET_DIF) },
};
STATIC MP_DEFINE_CONST_DICT(power_module_globals, power_module_globals_table);

const mp_obj_module_t power_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&power_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_power, power_module);
