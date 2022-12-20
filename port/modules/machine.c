/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Glenn Ruben Bakke
 * Copyright (c) 2022 Brilliant Labs Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdio.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objstr.h"
#include "extmod/machine_mem.h"
#include "extmod/machine_pulse.h"
#include "genhdr/mpversion.h"

#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"

#include "driver_dfu.h"
#include "ble_gap.h"
#include "machine.h"

/**
 * Current version as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(machine_version_obj, BUILD_VERSION);

/**
 * Current git tag as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(machine_git_tag_obj, MICROPY_GIT_HASH);

/**
 * Board name as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(machine_board_name_obj, MICROPY_HW_BOARD_NAME);

/**
 * MCU name as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(machine_mcu_name_obj, MICROPY_HW_MCU_NAME);

STATIC mp_obj_t machine_update(const mp_obj_t reboot)
{
    if (mp_obj_is_true(reboot))
        MICROPY_BOARD_ENTER_BOOTLOADER(n_args, args);
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_update_obj, machine_update);

STATIC mp_obj_t machine_mac_address(void)
{
    static char m_mac_address[sizeof "XX:XX:XX:XX:XX:XX"];
    uint32_t err;
    ble_gap_addr_t addr = {0};
    char *str = m_mac_address;
    size_t sz = sizeof m_mac_address;
    int n;

    err = sd_ble_gap_addr_get(&addr);
    assert(err == 0);

    assert(sizeof m_mac_address / 3 > sizeof addr.addr);
    for (uint8_t i = 0; i < 6; i++) {
        n = snprintf(str, sz, i == 0 ? "%02X" : ":%02X", m_mac_address[i]);
        str += n;
        sz -= n;
    }
    return mp_obj_new_str(m_mac_address, strlen(m_mac_address));
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_mac_address_obj, machine_mac_address);


STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_umachine) },

    // classes for each hardware
    { MP_ROM_QSTR(MP_QSTR_Touch),               MP_ROM_PTR(&touch_type) },
    { MP_ROM_QSTR(MP_QSTR_Battery),             MP_ROM_PTR(&battery_type) },
    { MP_ROM_QSTR(MP_QSTR_FPGA),                MP_ROM_PTR(&fpga_type) },
    { MP_ROM_QSTR(MP_QSTR_Display),             MP_ROM_PTR(&display_type) },
    { MP_ROM_QSTR(MP_QSTR_Camera),              MP_ROM_PTR(&camera_type) },
    { MP_ROM_QSTR(MP_QSTR_Power),               MP_ROM_PTR(&power_type) },
    { MP_ROM_QSTR(MP_QSTR_RTCounter),           MP_ROM_PTR(&rtcounter_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer),               MP_ROM_PTR(&timer_type) },

    // local methods for the monocle
    { MP_ROM_QSTR(MP_QSTR_mac_address),         MP_ROM_PTR(&machine_mac_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_update),              MP_ROM_PTR(&machine_update_obj) },

    // Information about the version and device
    { MP_ROM_QSTR(MP_QSTR_board_name),          MP_ROM_PTR(&machine_board_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_git_tag),             MP_ROM_PTR(&machine_git_tag_obj) },
    { MP_ROM_QSTR(MP_QSTR_mcu_name),            MP_ROM_PTR(&machine_mcu_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_version),             MP_ROM_PTR(&machine_version_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_umachine, mp_module_machine);
