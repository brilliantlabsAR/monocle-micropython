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
#include "shared/runtime/pyexec.h"
#include "genhdr/mpversion.h"

#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"

#include "driver_dfu.h"
#include "ble_gap.h"
#include "machine.h"

#define PYB_RESET_HARD      (0)
#define PYB_RESET_WDT       (1)
#define PYB_RESET_SOFT      (2)
#define PYB_RESET_LOCKUP    (3)
#define PYB_RESET_POWER_ON  (16)
#define PYB_RESET_LPCOMP    (17)
#define PYB_RESET_DIF       (18)
#define PYB_RESET_NFC       (19)

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

STATIC uint32_t reset_cause;

void machine_init(void)
{
    uint32_t state = NRF_POWER->RESETREAS;
    if (state & POWER_RESETREAS_RESETPIN_Msk) {
        reset_cause = PYB_RESET_HARD;
    } else if (state & POWER_RESETREAS_DOG_Msk) {
        reset_cause = PYB_RESET_WDT;
    } else if (state & POWER_RESETREAS_SREQ_Msk) {
        reset_cause = PYB_RESET_SOFT;
    } else if (state & POWER_RESETREAS_LOCKUP_Msk) {
        reset_cause = PYB_RESET_LOCKUP;
    } else if (state & POWER_RESETREAS_OFF_Msk) {
        reset_cause = PYB_RESET_POWER_ON;
    } else if (state & POWER_RESETREAS_DIF_Msk) {
        reset_cause = PYB_RESET_DIF;
    }

    // clear reset reason
    NRF_POWER->RESETREAS = (1 << reset_cause);
}

// Resets the board in a manner similar to pushing the external RESET button.
STATIC mp_obj_t machine_reset(void)
{
    NVIC_SystemReset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

STATIC mp_obj_t machine_soft_reset(void)
{
    pyexec_system_exit = PYEXEC_FORCED_EXIT;
    mp_raise_type(&mp_type_SystemExit);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_soft_reset_obj, machine_soft_reset);

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

STATIC mp_obj_t machine_lightsleep(void)
{
    __WFE();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_lightsleep_obj, machine_lightsleep);

STATIC mp_obj_t machine_deepsleep(void)
{
    __WFI();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_deepsleep_obj, machine_deepsleep);

STATIC mp_obj_t machine_reset_cause(void)
{
    return MP_OBJ_NEW_SMALL_INT(reset_cause);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);

STATIC mp_obj_t machine_enable_irq(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_enable_irq_obj, machine_enable_irq);

// Resets the board in a manner similar to pushing the external RESET button.
STATIC mp_obj_t machine_disable_irq(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_disable_irq_obj, machine_disable_irq);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_umachine) },

    // classes for each hardware
    { MP_ROM_QSTR(MP_QSTR_Touch),               MP_ROM_PTR(&machine_touch_type) },
    { MP_ROM_QSTR(MP_QSTR_Battery),             MP_ROM_PTR(&machine_battery_type) },
    { MP_ROM_QSTR(MP_QSTR_FPGA),                MP_ROM_PTR(&machine_fpga_type) },
    { MP_ROM_QSTR(MP_QSTR_Power),               MP_ROM_PTR(&machine_power_type) },
    { MP_ROM_QSTR(MP_QSTR_RTCounter),           MP_ROM_PTR(&machine_rtcounter_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer),               MP_ROM_PTR(&machine_timer_type) },

    // local methods for the monocle
    { MP_ROM_QSTR(MP_QSTR_mac_address),         MP_ROM_PTR(&machine_mac_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_update),              MP_ROM_PTR(&machine_update_obj) },

    // local methods from micropython
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset),          MP_ROM_PTR(&machine_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_cause),         MP_ROM_PTR(&machine_reset_cause_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_irq),          MP_ROM_PTR(&machine_enable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_irq),         MP_ROM_PTR(&machine_disable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_idle),                MP_ROM_PTR(&machine_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&machine_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_lightsleep),          MP_ROM_PTR(&machine_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep),           MP_ROM_PTR(&machine_deepsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem8),                MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16),               MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32),               MP_ROM_PTR(&machine_mem32_obj) },

    // reset causes
    { MP_ROM_QSTR(MP_QSTR_HARD_RESET),          MP_ROM_INT(PYB_RESET_HARD) },
    { MP_ROM_QSTR(MP_QSTR_WDT_RESET),           MP_ROM_INT(PYB_RESET_WDT) },
    { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),          MP_ROM_INT(PYB_RESET_SOFT) },
    { MP_ROM_QSTR(MP_QSTR_LOCKUP_RESET),        MP_ROM_INT(PYB_RESET_LOCKUP) },
    { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(PYB_RESET_POWER_ON) },
    { MP_ROM_QSTR(MP_QSTR_LPCOMP_RESET),        MP_ROM_INT(PYB_RESET_LPCOMP) },
    { MP_ROM_QSTR(MP_QSTR_DEBUG_IF_RESET),      MP_ROM_INT(PYB_RESET_DIF) },

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
