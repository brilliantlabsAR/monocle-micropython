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

#include <stdio.h>

#include "genhdr/mpversion.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "nrfx_reset_reason.h"
#include "monocle.h"
#include "driver/battery.h"
#include "ble_gap.h"

/** Current version as a string object. */
STATIC const MP_DEFINE_STR_OBJ(device_version_obj, BUILD_VERSION);

/** Current git tag as a string object. */
STATIC const MP_DEFINE_STR_OBJ(device_git_tag_obj, MICROPY_GIT_HASH);

/** Holding the reset cause string. */
STATIC mp_obj_t reset_cause_obj = mp_const_none;

STATIC mp_obj_t mod_device___init__(void)
{
    uint32_t state = NRF_POWER->RESETREAS;

    if (state & POWER_RESETREAS_RESETPIN_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_POWERED_ON);
    }
    else if (state & POWER_RESETREAS_DOG_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_CRASHED);
    }
    else if (state & POWER_RESETREAS_SREQ_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_SOFTWARE_RESET);
    }
    else if (state & POWER_RESETREAS_LOCKUP_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_CRASHED);
    }
    else if (state & POWER_RESETREAS_OFF_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_POWERED_ON);
    }
    else if (state & POWER_RESETREAS_DIF_Msk)
    {
        reset_cause_obj = MP_OBJ_NEW_QSTR(MP_QSTR_CRASHED);
    }
    assert(reset_cause_obj != NULL);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_device___init___obj, mod_device___init__);

STATIC mp_obj_t device_update(const mp_obj_t reboot)
{
    if (mp_obj_is_true(reboot))
        monocle_enter_bootloader();
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(device_update_obj, device_update);

STATIC mp_obj_t device_mac_address(void)
{
    static char m_mac_address[sizeof "XX:XX:XX:XX:XX:XX"];
    ble_gap_addr_t addr = {0};
    char *str = m_mac_address;
    size_t sz = sizeof m_mac_address;
    int n;

    app_err(sd_ble_gap_addr_get(&addr));

    assert(sizeof m_mac_address / 3 == sizeof addr.addr);
    for (uint8_t i = 0; i < 6; i++)
    {
        n = snprintf(str, sz, i == 0 ? "%02X" : ":%02X", m_mac_address[i]);
        str += n;
        sz -= n;
    }
    return mp_obj_new_str(m_mac_address, strlen(m_mac_address));
}
MP_DEFINE_CONST_FUN_OBJ_0(device_mac_address_obj, device_mac_address);

STATIC mp_obj_t device_battery_level(void)
{
    return MP_OBJ_NEW_SMALL_INT(battery_get_percent());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(device_battery_level_obj, device_battery_level);

NORETURN STATIC mp_obj_t device_reset(void)
{
    NVIC_SystemReset();
}
MP_DEFINE_CONST_FUN_OBJ_0(device_reset_obj, &device_reset);

STATIC mp_obj_t device_reset_cause(void)
{
    return reset_cause_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(device_reset_cause_obj, device_reset_cause);

STATIC const mp_rom_map_elem_t device_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_device)},
    {MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_device___init___obj)},

    // methods
    {MP_ROM_QSTR(MP_QSTR_mac_address), MP_ROM_PTR(&device_mac_address_obj)},
    {MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&device_update_obj)},
    {MP_ROM_QSTR(MP_QSTR_battery_level), MP_ROM_PTR(&device_battery_level_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&device_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset_cause), MP_ROM_PTR(&device_reset_cause_obj)},

    // constants
    {MP_ROM_QSTR(MP_QSTR_GIT_TAG), MP_ROM_PTR(&device_git_tag_obj)},
    {MP_ROM_QSTR(MP_QSTR_VERSION), MP_ROM_PTR(&device_version_obj)},
};
STATIC MP_DEFINE_CONST_DICT(device_module_globals, device_module_globals_table);

const mp_obj_module_t mp_module_device = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&device_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_device, mp_module_device);
